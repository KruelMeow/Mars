
#include "TCPConnectionWrapper.h"
#include "webrtc/system_wrappers/interface/trace.h"
#include "webrtc/system_wrappers/interface/sleep.h"

#include "webrtc/base/network.h"
#include "webrtc/base/thread.h"

#if defined(WEBRTC_POSIX)
// linux/if.h can't be included at the same time as the posix sys/if.h, and
// it's transitively required by linux/route.h, so include that version on
// linux instead of the standard posix one.
#if defined(WEBRTC_LINUX)
#include <linux/if.h>
#include <linux/route.h>
#elif !defined(__native_client__)
#include <net/if.h>
#include<stdio.h>  
#include<stdlib.h>  
#include<string.h>  
#include<sys/ioctl.h>  
#include<sys/socket.h>  
#include<arpa/inet.h>  
#include<netinet/in.h>  
#include<net/if.h> 
#endif
#include <sys/socket.h>
#include <sys/utsname.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include<arpa/inet.h> 
#include <fcntl.h>
#include <errno.h>

#if defined(WEBRTC_ANDROID)
#include "webrtc/base/ifaddrs-android.h"
#elif !defined(__native_client__)
#include <ifaddrs.h>
#endif

#endif  // WEBRTC_POSIX


namespace cmcc_webrtc {
namespace test {

SocketPacket::SocketPacket(char* data,int len):len_(len){
    if(len <= MAXMSGLENGTH){
        memcpy(buf_,data,len);
        memset(buf_+len,0,MAXMSGLENGTH-len);
    }
}

TCPConnectionWrapper::TCPConnectionWrapper():connected_(false),
    reported_(false),
    csSocket_(CriticalSectionWrapper::CreateCriticalSection()),
    socketfd_(-1),
    csSendlist_(CriticalSectionWrapper::CreateCriticalSection()),
    cstcpMsgListener(CriticalSectionWrapper::CreateCriticalSection()),
    iRecvBufReadIndex_(0),
    iRecvBufWriteIndex_(0),
    sLocalIP_(""),
    ptrThreadSend_(NULL),
    ptrThreadRecv_(NULL),
    recvStopTcpCMD_(false),
    exit_(false),
    sServerIP_(""),
    bServerIsIPV4_(true),
    resetConnection_(false),
    pSipEventCallback_(NULL){
        pcharRecvBuf_ = new char[RECVBUFFLEN];
}
TCPConnectionWrapper::~TCPConnectionWrapper() {
    WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                 "TCPConnectionWrapper::~TCPConnectionWrapper");
    exit_ = true;
    Reset();
    listeners_TCPMsg_.clear();
    if(NULL != pcharRecvBuf_){
        delete pcharRecvBuf_;
        pcharRecvBuf_ = NULL;
    }
    if(NULL != csSendlist_){
        delete csSendlist_;
        csSendlist_ = NULL;
    }
    if(NULL != cstcpMsgListener){
        delete cstcpMsgListener;
        cstcpMsgListener = NULL;
    }
    if(NULL != csSocket_){
        delete csSocket_;
        csSocket_ = NULL;
    }
    WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                 "TCPConnectionWrapper::~TCPConnectionWrapper, end");
}
void TCPConnectionWrapper::Reset(){
    WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                 "TCPConnectionWrapper::ResetSocket");
    connected_ = false;
    //stop the thread
    if(NULL != ptrThreadSend_){
        ptrThreadSend_->Stop();
    }
    if(NULL != ptrThreadRecv_){
        ptrThreadRecv_->Stop();
    }
    //
    recvStopTcpCMD_ = false;
    ResetSocket();
}
void TCPConnectionWrapper::ResetSocket(){
    WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                 "TCPConnectionWrapper::ResetSocket");
    CriticalSectionScoped cs(csSocket_);
    if(socketfd_ > 0){
        shutdown(socketfd_,SHUT_RDWR);
        close(socketfd_);
        socketfd_ = -1;
    }
    sLocalIP_ = "";
    connected_ = false;
    sServerIP_ = "";
    //
    ClearSendPacketsList();
    //
    iRecvBufReadIndex_ = 0;
    iRecvBufWriteIndex_ = 0;
    if(NULL != pcharRecvBuf_){
        memset(pcharRecvBuf_,0,RECVBUFFLEN);
    }
    WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                 "TCPConnectionWrapper::ResetSocket end");
}
void TCPConnectionWrapper::Reconnet(){
    WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                 "TCPConnectionWrapper::Reconnet");
    recvStopTcpCMD_ = false;
    resetConnection_ = true;
    ResetSocket();
}
void TCPConnectionWrapper::ClearSendPacketsList(){
    CriticalSectionScoped cs(csSendlist_);
    for (socketPacketsSendList::iterator it = sendPackets_.begin();
         it != sendPackets_.end(); ++it) {
        delete *it;
    }
    sendPackets_.clear();
    //WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
    //             "TCPConnectionWrapper::ClearSendPacketsList");
}
bool TCPConnectionWrapper::IsStopCMDInPacketsList(){
    CriticalSectionScoped cs(csSendlist_);
    for (socketPacketsSendList::iterator it = sendPackets_.begin();
         it != sendPackets_.end(); ++it) {
        SocketPacket *packet = (SocketPacket *)(*it);
        if(NULL != strstr(packet->GetData(), STOPTCPCMD)){
            sServerAddr_ = "";
            return true;
        }
    }
    return false;
}
int TCPConnectionWrapper::SendData2Server(char* data, int len){
    if(NULL == data || len<= 0){
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                     "TCPConnectionWrapper::SendData2Server, param error and return");
        return -1;
    }
    SocketPacket* packet = new SocketPacket(data,len);
    CriticalSectionScoped cs(csSendlist_);
    sendPackets_.push_back(packet);
    //WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
    //             "TCPConnectionWrapper::SendData2Server, add cmd and size:%d", sendPackets_.size());
    return 0;
}
int TCPConnectionWrapper::SetServer(std::string strServer, int port){
    WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                 "TCPConnectionWrapper::SetServer,begin, server:%s,port:%d",strServer.c_str(), port);
    bool bIPV4 = bServerIsIPV4_;
#if defined(WEBRTC_ANDROID)
    bIPV4 = true;
#else
    if("" == sServerIP_){
        bIPV4 = IsServerTypeIPV4AndGetIP(strServer);
    }
#endif
    std::string strIP = GetLocalSystemIP(bIPV4);
    if("" == strIP){
        WEBRTC_TRACE(cmcc_webrtc::kTraceError, cmcc_webrtc::KTraceSignalingConnection, -1,
                     "TCPConnectionWrapper::SetServer, failed to get local ip");
        //return -1;
    }
    if(0 == strIP.compare(sLocalIP_) && connected_ && "" != sServerAddr_){
        WEBRTC_TRACE(cmcc_webrtc::kTraceError, cmcc_webrtc::KTraceSignalingConnection, -1,
                     "TCPConnectionWrapper::SetServer, already linked to the server.,local:%s,new:%s",
                     sLocalIP_.c_str(),strIP.c_str());
        return 1;
    }
    exit_ = true;
    Reset();
    sServerAddr_ = strServer;
    iServerPort_ = port;
    connected_ = false;
    recvStopTcpCMD_ = false;
    exit_ = false;
    if(NULL == ptrThreadRecv_){
        ptrThreadRecv_ = ThreadWrapper::CreateThread(SocketThreadRecvFunc,
                                                     this,
                                                     kRealtimePriority,
                                                     "TCP recv thread");
    }
    if(NULL == ptrThreadRecv_){
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                     "TCPConnectionWrapper::SetServer failed to create recv thread!");
        return -2;
    }
    //create recv and send thread
    if(NULL == ptrThreadSend_){
        ptrThreadSend_ = ThreadWrapper::CreateThread(SocketThreadSendFunc,
                                                     this,
                                                     kRealtimePriority,
                                                     "TCP send thread");
    }
    if(NULL == ptrThreadSend_){
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                     "TCPConnectionWrapper::SetServer failed to create send thread!");
        return -3;
    }
    WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                 "TCPConnectionWrapper::SetServer, succeed to create recv and send thread");
    unsigned int threadRecv_id = 0;
    bool bstart = ptrThreadRecv_->Start(threadRecv_id);
    if(!bstart){
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                     "TCPConnectionWrapper::SetServer failed to start recv thread!");
        return -4;
    }
    unsigned int threadSend_id = 0;
    bstart = ptrThreadSend_->Start(threadSend_id);
    if(!bstart){
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                     "TCPConnectionWrapper::SetServer failed to start send thread!");
        return -5;
    }
    return 0;
}
int TCPConnectionWrapper::AddTCPMsgListener(TCPMsgListener* ob){
    if(NULL == ob){
        return -1;
    }
    CriticalSectionScoped cs(cstcpMsgListener);
    for (std::list<TCPMsgListener*>::iterator it = listeners_TCPMsg_.begin();
         it != listeners_TCPMsg_.end();
         ++ it ){
        if((*it) == ob){
            //      WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
            //								  "TCPConnectionWrapper::AddTCPMsgListener,already added");
            return 0;
        }
    }
    listeners_TCPMsg_.push_back(ob);
    //  WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
    //								  "TCPConnectionWrapper::AddTCPMsgListener,ob:%x,size:%d",ob,listeners_TCPMsg_.size());
    return 0;
}
int TCPConnectionWrapper::RemoveTCPMsgListener(TCPMsgListener* ob){
    if(NULL == ob){
        return -1;
    }
    //  WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
    //								  "TCPConnectionWrapper::RemoveTCPMsgListener,ob:%x,size:%d",ob,listeners_TCPMsg_.size());
    CriticalSectionScoped cs(cstcpMsgListener);
    for (std::list<TCPMsgListener*>::iterator it = listeners_TCPMsg_.begin();
         it != listeners_TCPMsg_.end();
         ++ it ){
        if((*it) == ob){
            //WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
            //							  "TCPConnectionWrapper::RemoveTCPMsgListener, ob:%x",ob);
            listeners_TCPMsg_.erase(it);
            //  WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
            //							  "TCPConnectionWrapper::RemoveTCPMsgListener, end ob:%x",ob);
            break;
        }
    }
    //  WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
    //								  "TCPConnectionWrapper::RemoveTCPMsgListener,end");
    return 0;
}
int TCPConnectionWrapper::SetSipEventDealCallBack(SipEventDealCallback* pCallback){
    CriticalSectionScoped cs(cstcpMsgListener);
    pSipEventCallback_ = pCallback;
    return 0;
}
void TCPConnectionWrapper::SendTCPConnectedMsg(){
    //  WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
    //							 "TCPConnectionWrapper::SendTCPConnectedMsg,size:%d",listeners_TCPMsg_.size() );
    CriticalSectionScoped cs(cstcpMsgListener);
    for (std::list<TCPMsgListener*>::iterator it = listeners_TCPMsg_.begin();
         it != listeners_TCPMsg_.end();
         ++ it ){
        (*it)->OnTCPConnected();
    }
}
void TCPConnectionWrapper::SendTCPDisconnectedMsg(ConnectionFailReason type){
    //  WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
    //							  "TCPConnectionWrapper::SendTCPDisconnectedMsg,type:%d,size:%d",type,listeners_TCPMsg_.size());
    CriticalSectionScoped cs(cstcpMsgListener);
    for (std::list<TCPMsgListener*>::iterator it = listeners_TCPMsg_.begin();
         it != listeners_TCPMsg_.end();
         ++ it ){
        (*it)->OnTCPDisconnected(type);
    }
}
void TCPConnectionWrapper::ProcessingInputData(char* data, int len){
    //WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
    //						  "TCPConnectionWrapper::ProcessingInputData, data:%s,len:%d",data,len);
    SendResponseToListener(data,len);
}
void TCPConnectionWrapper::SendResponseToListener(char*data, int len){
    CriticalSectionScoped cs(cstcpMsgListener);
    for (std::list<TCPMsgListener*>::iterator it = listeners_TCPMsg_.begin();
         it != listeners_TCPMsg_.end();
         ++ it ){
        (*it)->OnTCPRecvMsg(data,len);
    }
    ProcessingTCPMsg(data,len);
}
bool TCPConnectionWrapper::SocketThreadSendFunc(void* pThis){
    return (static_cast<TCPConnectionWrapper*>(pThis)->SocketThreadSendProcess());
}
bool TCPConnectionWrapper::SocketThreadRecvFunc(void* pThis){
    return (static_cast<TCPConnectionWrapper*>(pThis)->SocketThreadRecvProcess());
}
bool TCPConnectionWrapper::SocketThreadRecvProcess(){
    if(exit_){
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                     "TCPConnectionWrapper::SocketThreadRecvProcess, exit");
        return false;
    }
    //  WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
    //                   "TCPConnectionWrapper::SocketThreadRecvProcess, begin");
    /*  if(resetConnection_){
     WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
     "TCPConnectionWrapper::SocketThreadRecvProcess, reset connection");
     ResetSocket();
     resetConnection_ = false;
     }*/
    //get local ip
    bool bIPV4 = bServerIsIPV4_;
    if("" == sServerIP_){
        bIPV4 = IsServerTypeIPV4AndGetIP(sServerAddr_);
    }
    if("" == sServerIP_){
        WEBRTC_TRACE(cmcc_webrtc::kTraceError, cmcc_webrtc::KTraceSignalingConnection, -1,
                     "TCPConnectionWrapper::SocketThreadRecvProcess, failed to get server ip");
        cmcc_webrtc::SleepMs(500);
        return true;
    }
    std::string strIP = GetLocalSystemIP(bIPV4);
    if(!connected_ || strIP.compare(sLocalIP_) != 0){
        if("" == strIP){
            WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                         "TCPConnectionWrapper::SocketThreadRecvProcess, failed to get local ip");
            SendTCPDisconnectedMsg(ConnectionFail_GetLocalIP);
            cmcc_webrtc::SleepMs(1000);//sleep 1 second and try again
            return true;
        }
        sLocalIP_ = strIP;
        
        csSocket_->Enter();
        if(socketfd_ > 0){
            shutdown(socketfd_,SHUT_RDWR);
            close(socketfd_);
            socketfd_ = -1;
            WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                         "TCPConnectionWrapper::SocketThreadRecvProcess, close old socket");
            SendTCPDisconnectedMsg(ConnectionFail_Connect);
        }
        csSocket_->Leave();
        
        if(socketfd_ <= 0){
            if(bIPV4){
                socketfd_ = socket(AF_INET,SOCK_STREAM,0);
            }else{
                socketfd_ = socket(AF_INET6,SOCK_STREAM, 0);
            }
        }
        if(-1 == socketfd_){
            SendTCPDisconnectedMsg(ConnectionFail_NewSocket);
            WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                         "TCPConnectionWrapper::SocketThreadRecvProcess, failed to new socket:%d",errno);
            SleepMs(1000);//sleep 1 second and try again
            return true;
        }else{
            WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                         "TCPConnectionWrapper::SocketThreadRecvProcess, new socket()");
        }
        //int nNetTimeout = 1000;//超时时长
        //setsockopt(socketfd_,SOL_SOCKET,SO_RCVTIMEO,(char *)&nNetTimeout,sizeof(int));
        struct sockaddr_in6 server_sockaddrin6;
        memset(&addr_ser_,0,sizeof(addr_ser_));
        if(bIPV4){
            addr_ser_.sin_family= AF_INET;
            addr_ser_.sin_addr.s_addr = inet_addr(sServerIP_.c_str());
            addr_ser_.sin_port=htons(iServerPort_);
        }else{
            memset(&server_sockaddrin6, 0, sizeof(struct sockaddr_in6));
            server_sockaddrin6.sin6_family = AF_INET6;
            server_sockaddrin6.sin6_port = htons(iServerPort_);
            inet_pton(AF_INET6, sServerIP_.c_str(), &server_sockaddrin6.sin6_addr);
        }
        if(recvStopTcpCMD_){
            ResetSocket();
            SendTCPDisconnectedMsg(ConnectionFail_ClientClose);
            recvStopTcpCMD_ = false;
            WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                         "TCPConnectionWrapper::SocketThreadRecvProcess, stop cmd and return false");
            return false;
        }
        int error= 0;
        if(bIPV4){
            //error = connect(socketfd_,(struct sockaddr *)&addr_ser_,sizeof(struct sockaddr));
            error = Connect_Nonb(socketfd_,(struct sockaddr *)&addr_ser_,sizeof(struct sockaddr));
        }
        else{
            //error = connect(socketfd_,(struct sockaddr *)&server_sockaddrin6,sizeof(struct sockaddr_in6));
            error = Connect_Nonb(socketfd_,(struct sockaddr *)&server_sockaddrin6,sizeof(struct sockaddr_in6));
        }
        if(-1 == error){
            SendTCPDisconnectedMsg(ConnectionFail_Connect);
            WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                         "TCPConnectionWrapper::SocketThreadRecvProcess, failed to connect server,%s,ip:%s,port:%d",sServerAddr_.c_str(),sServerIP_.c_str(),iServerPort_);
            recvStopTcpCMD_ = false;
            ResetSocket();
            SleepMs(1000);//sleep 1 second and try again
            return true;
        }else{
            WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                         "TCPConnectionWrapper::SocketThreadRecvProcess, succeed to connect server,%s:%d,socket:%d",
                         sServerAddr_.c_str(),iServerPort_,socketfd_);
            if(IsStopCMDInPacketsList()){
                recvStopTcpCMD_ = true;
                ClearSendPacketsList();
                WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                             "TCPConnectionWrapper::SocketThreadRecvProcess, stop cmd in list and return false");
                return false;
            }else{
                ClearSendPacketsList();
                SendTCPConnectedMsg();
                connected_ = true;
            }
        }
    }
    if(NULL == pcharRecvBuf_){
        pcharRecvBuf_ = new char[RECVBUFFLEN];
    }
    if(NULL == pcharRecvBuf_){
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                     "TCPConnectionWrapper::SocketThreadRecvProcess ,return false, failed to create new bufer");
        SleepMs(1000);//sleep 1 second and try again
        return true;
    }
    if(socketfd_ <= 0){
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                     "TCPConnectionWrapper::SocketThreadRecvProcess ,return false, socket:%d",socketfd_);
        SleepMs(1000);//sleep 1 second and try again
        return true;
    }
    if(!connected_){
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                     "TCPConnectionWrapper::SocketThreadRecvProcess ,return false, server not connected");
        SleepMs(1000);//sleep 1 second and try again
        return true;
    }
    int nBufLen = RECVBUFFLEN - iRecvBufWriteIndex_;
    if(nBufLen <= 1024){// the write index
        if(iRecvBufReadIndex_ < iRecvBufWriteIndex_){//move the left to the head
            memmove(pcharRecvBuf_,pcharRecvBuf_+iRecvBufReadIndex_,iRecvBufWriteIndex_-iRecvBufReadIndex_);
            memset(pcharRecvBuf_+iRecvBufWriteIndex_-iRecvBufReadIndex_,0,RECVBUFFLEN-(iRecvBufWriteIndex_-iRecvBufReadIndex_));
            iRecvBufWriteIndex_ = iRecvBufWriteIndex_-iRecvBufReadIndex_;
            nBufLen = RECVBUFFLEN - iRecvBufWriteIndex_;
        }else{
            memset(pcharRecvBuf_,0,RECVBUFFLEN);
            iRecvBufReadIndex_ = 0;
            iRecvBufWriteIndex_ = 0;
            nBufLen = RECVBUFFLEN;
        }
        iRecvBufReadIndex_ = 0;
    }
    //  WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
    //						  "TCPConnectionWrapper::SocketThreadRecvProcess, try to recv");
    int len=recv(socketfd_,pcharRecvBuf_+iRecvBufWriteIndex_,nBufLen,0); //MSG_DONTWAIT
    if(SOCKET_ERROR == len){
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                     "TCPConnectionWrapper::SocketThreadRecvProcess, recv ret is SOCKET_ERROR,stop cmd:%d",recvStopTcpCMD_);
        if(recvStopTcpCMD_){
            ResetSocket();
            SendTCPDisconnectedMsg(ConnectionFail_ClientClose);
            recvStopTcpCMD_ = false;
            return false;
        }
        ResetSocket();
        SendTCPDisconnectedMsg(ConnectionFail_RecvError);
        //    SleepMs(1000);//sleep 1 second and try again
        return true;
    }else if(0 == len){//the server close the link
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                     "TCPConnectionWrapper::SocketThreadRecvProcess, recv ret is 0,stop cmd:%d",recvStopTcpCMD_);
        if(recvStopTcpCMD_){
            ResetSocket();
            SendTCPDisconnectedMsg(ConnectionFail_ClientClose);
            return false;
        }
        ResetSocket();
        SendTCPDisconnectedMsg(ConnectionFail_RecvLenZero);
        SleepMs(1000);//sleep 1 second and try again
        return true;
    }else if(len >0){
        iRecvBufWriteIndex_ += len;
        while(true){
            //find the complete signaling msg
            const char* header = "xxxbbb";
            const char* tailer = "xxxeee";
            char* begin = strstr(pcharRecvBuf_+iRecvBufReadIndex_,header);
            char* end = strstr(pcharRecvBuf_+iRecvBufReadIndex_,tailer);
            if(begin != NULL && end != NULL){
                *end = '\0';
                int nLength = std::string(pcharRecvBuf_+iRecvBufReadIndex_).size();
                //if(NULL == strstr(pcharRecvBuf_+iRecvBufReadIndex_,"heartbeat")){
                //		   WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                //	       					 "TCPConnectionWrapper::SocketThreadRecvProcess, recv string:%s,len:%d",
                //	       					 pcharRecvBuf_+iRecvBufReadIndex_,
                //	                             nLength);
                //}
                if(nLength > 0){
                    ProcessingInputData(pcharRecvBuf_+iRecvBufReadIndex_, nLength);
                }
                iRecvBufReadIndex_ += nLength+std::string(tailer).size();
            }else{
                break;
            }
        }
    }else{
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                     "TCPConnectionWrapper::SocketThreadRecvProcess, error:recv len is:%d",len);
    }
    return true;
}
bool TCPConnectionWrapper::SocketThreadSendProcess(){
    if(exit_){
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                     "TCPConnectionWrapper::SocketThreadSendProcess, exit");
        return false;
    }
    if(socketfd_ <= 0 || !connected_){
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                     "TCPConnectionWrapper::SocketThreadSendProcess, not connected");
        SleepMs(300);
        return true;
    }
    SocketPacket* packet = NULL;
    {
        csSendlist_->Enter();
        if(sendPackets_.empty()){
            csSendlist_->Leave();
            //WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
            //         "TCPConnectionWrapper::SocketThreadSendProcess, no packets and sleep, size:%d",sendPackets_.size());
            cmcc_webrtc::SleepMs(100);
            return true;
        }
        packet = sendPackets_.front();
        sendPackets_.pop_front();
        csSendlist_->Leave();
    }
    if(NULL != strstr(packet->GetData(), STOPTCPCMD)){
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                     "TCPConnectionWrapper::SocketThreadSendProcess, rece stop tcp cmd");
        sServerAddr_ = "";
        recvStopTcpCMD_ = true;
        ResetSocket();
        delete packet;
        return false;
    }
    if(recvStopTcpCMD_){
        ResetSocket();
        SendTCPDisconnectedMsg(ConnectionFail_ClientClose);
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                     "TCPConnectionWrapper::SocketThreadSendProcess, recv stop cmd and return false");
        delete packet;
        return false;
    }
    int ret = send(socketfd_,packet->GetData(),packet->GetLength(),MSG_DONTWAIT);
    
    if(SOCKET_ERROR == ret){
        SendTCPDisconnectedMsg(ConnectionFail_SendError);
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                     "TCPConnectionWrapper::SocketThreadSendProcess, send and ret is SOCKET_ERROR");
        delete packet;
        SleepMs(1000);//sleep 1 sencod and try again
        return true;
    }else{
        if(NULL == strstr(packet->GetData(),"heartbeat")){
            WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                         "TCPConnectionWrapper::SocketThreadSendProcess,%s,len:%d",
                         packet->GetData(),packet->GetLength());
        }
    }
    delete packet;
    return true;
}
std::string TCPConnectionWrapper::GetLocalIP(){
    if("" == sLocalIP_){
        sLocalIP_= GetLocalSystemIP(bServerIsIPV4_);
        if("" == sLocalIP_){
            WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                         "TCPConnectionWrapper::GetLocalIP, failed to get local ip");
        }
    }
    return sLocalIP_;
}
std::string TCPConnectionWrapper::GetServerIP(){
    return sServerIP_;
}
bool TCPConnectionWrapper::IsServerTypeIPV4AndGetIP(std::string serverName){
#if defined(WEBRTC_IOS)
    if(serverName != sServerAddr_ || "" == sServerIP_){
        const struct addrinfo hints{};
        struct addrinfo * res;
        int err = getaddrinfo(serverName.c_str(), NULL, &hints, &res);
        // Post-process the results of `getaddrinfo` to work around
        bool IPV4 = true;
        if (err == 0) {
            for (const struct addrinfo * addr = res; addr != NULL; addr = addr->ai_next) {
                char temp[50]={'0'};
                switch (addr->ai_family) {
                    case AF_INET: {
                        const struct sockaddr_in *address = (const struct sockaddr_in*) addr->ai_addr;
                        const char* pszTemp = inet_ntop(AF_INET, &address->sin_addr, temp, sizeof(temp));
                        if(sServerIP_.compare(temp) != 0){
                            WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                                         "TCPConnectionWrapper::IsServerTypeIPV4AndGetIP,ipv4 wlan name:%s,ip:%s",serverName.c_str(),temp);
                            sServerIP_ = pszTemp;
                        }
                        IPV4 = true;
                        bServerIsIPV4_ = true;
                        sServerAddr_ = serverName;
                        freeaddrinfo(res);
                        return IPV4;
                    } break;
                    case AF_INET6: {
                        const struct sockaddr_in6* address6 = (struct sockaddr_in6*)addr->ai_addr;
                        const char* pszTemp = inet_ntop(AF_INET6, &address6->sin6_addr, temp, sizeof(temp));
                        if(sServerIP_.compare(pszTemp) != 0){
                            WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                                         "TCPConnectionWrapper::IsServerTypeIPV4AndGetIP,ipv6,name:%s,ip:%s",serverName.c_str(),temp);
                            sServerIP_ = std::string(pszTemp);
                        }
                        bServerIsIPV4_ = false;
                        IPV4 = false;
                        sServerAddr_ = serverName;
                        freeaddrinfo(res);
                        return IPV4;
                    } break;
                    default: {
                    } break;
                }
            }
            freeaddrinfo(res);
        }
        return IPV4;
    }
#elif defined(WEBRTC_ANDROID) || defined(WEBRTC_LINUX)
    bServerIsIPV4_ = true;
    if("" == sServerIP_){
        struct hostent* serverhostent = gethostbyname( sServerAddr_.c_str());
        if(NULL == serverhostent){
            WEBRTC_TRACE(cmcc_webrtc::kTraceError, cmcc_webrtc::KTraceSignalingConnection, -1,
                         "TCPConnectionWrapper::IsServerTypeIPV4AndGetIP, failed to get hostent");
            return false;
        }
        struct in_addr server = *(struct in_addr * ) serverhostent->h_addr;
        sServerIP_ = inet_ntoa(server);
        if("" == sServerIP_){
            WEBRTC_TRACE(cmcc_webrtc::kTraceError, cmcc_webrtc::KTraceSignalingConnection, -1,
                         "TCPConnectionWrapper::IsServerTypeIPV4AndGetIP, failed to get server ip");
            return false;
        }
    }
#endif
    return bServerIsIPV4_;
}
std::string TCPConnectionWrapper::GetLocalSystemIP(const bool bIPV4)
{
    std::string  strIP = "";
#if defined(_WIN32)
    hostent* localHost;
    localHost = gethostbyname( "" );
    if(localHost)
    {
        if(localHost->h_addrtype != AF_INET)
        {
            WEBRTC_TRACE(
                         kTraceError,
                         kTraceTransport,
                         -1,
                         "LocalHostAddress can only get local IP for IP Version 4");
            bIPV4 = false;
            return strIP;
        }
        strcpy(localIP,
               inet_ntoa((*(struct in_addr *)localHost->h_addr_list[0]).S_un.S_addr));
        return strIP;
    }
    else
    {
        int32_t error = WSAGetLastError();
        WEBRTC_TRACE(kTraceWarning, kTraceTransport, -1,
                     "gethostbyname failed, error:%d", error);
        return strIP;
    }
#elif (defined(WEBRTC_IOS))
    struct ifaddrs *interfaces;
    if( getifaddrs(&interfaces) == 0 ) {
        struct ifaddrs *interface;
        for( interface=interfaces; interface; interface=interface->ifa_next ) {
            if( (interface->ifa_flags & IFF_UP)
               && ! (interface->ifa_flags & IFF_LOOPBACK) ) {
                char temp[50]={'0'};
                if( interface->ifa_addr && interface->ifa_addr->sa_family==AF_INET &&
                   (strncmp(interface->ifa_name, "en0",3) == 0 && bIPV4) ) {
                    const struct sockaddr_in *addr = (const struct sockaddr_in*) interface->ifa_addr;
                    const unsigned char* addrBytes = (const unsigned char*)&addr->sin_addr.s_addr;
                    sprintf(temp,"%u.%u.%u.%u",(unsigned)addrBytes[0],(unsigned)addrBytes[1],(unsigned)addrBytes[2],(unsigned)addrBytes[3]);
                    if(sLocalIP_.compare(temp) != 0){
                        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                                     "TCPConnectionWrapper::GetLocalSystemIP,ipv4 wlan name:%s,ip:%s",interface->ifa_name,temp);
                    }
                    freeifaddrs(interfaces);
                    return std::string(temp);
                }else if(interface->ifa_addr && interface->ifa_addr->sa_family == AF_INET6 &&
                         (strncmp(interface->ifa_name, "en0",3) == 0) &&!bIPV4){
                    const struct sockaddr_in6* addr6 = (struct sockaddr_in6*)interface->ifa_addr;
                    const char* pszTemp = inet_ntop(AF_INET6, &addr6->sin6_addr, temp, sizeof(temp));
                    if(sLocalIP_.compare(pszTemp) != 0){
                        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                                     "TCPConnectionWrapper::GetLocalSystemIP,ipv6 wlan name:%s,ip:%s",interface->ifa_name,temp);
                    }
                    freeifaddrs(interfaces);
                    return std::string(pszTemp);
                }
            }
        }
        for( interface=interfaces; interface; interface=interface->ifa_next ) {
            if( (interface->ifa_flags & IFF_UP)
               && ! (interface->ifa_flags & IFF_LOOPBACK) ){
                char temp[50]={'0'};
                if( interface->ifa_addr && interface->ifa_addr->sa_family== AF_INET &&bIPV4) {
                    const struct sockaddr_in *addr = (const struct sockaddr_in*) interface->ifa_addr;
                    const unsigned char* addrBytes = (const unsigned char*)&addr->sin_addr.s_addr;
                    sprintf(temp,"%u.%u.%u.%u",(unsigned)addrBytes[0],(unsigned)addrBytes[1],(unsigned)addrBytes[2],(unsigned)addrBytes[3]);
                    if(sLocalIP_.compare(temp) != 0){
                        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                                     "TCPConnectionWrapper::GetLocalSystemIP,ipv4 name:%s,ip:%s",interface->ifa_name,temp);
                    }
                    freeifaddrs(interfaces);
                    return std::string(temp);
                }else if(interface->ifa_addr && interface->ifa_addr->sa_family == AF_INET6 &&!bIPV4 && !bIPV4){
                    const struct sockaddr_in6* addr6 = (struct sockaddr_in6*)interface->ifa_addr;
                    const char* pszTemp = inet_ntop(AF_INET6, &addr6->sin6_addr, temp, sizeof(temp));
                    if(sLocalIP_.compare(pszTemp) != 0){
                        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                                     "TCPConnectionWrapper::GetLocalSystemIP,ipv6 name:%s,ip:%s",interface->ifa_name,temp);
                    }
                    freeifaddrs(interfaces);
                    return std::string(pszTemp);
                }
            }
        }
        freeifaddrs(interfaces);
    }
    return strIP;
#elif (defined(WEBRTC_MAC))
    char localname[255];
    if (gethostname(localname, 255) != -1)
    {
        hostent* localHost;
        localHost = gethostbyname(localname);
        if(localHost)
        {
            if(localHost->h_addrtype != AF_INET)
            {
                WEBRTC_TRACE(
                             kTraceError,
                             kTraceTransport,
                             -1,
                             "LocalHostAddress can only get local IP for IP Version 4");
                return -1;
            }
            strIP = std::string(inet_ntoa(*(struct in_addr*)*localHost->h_addr_list));
            WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                         "TCPConnectionWrapper::GetLocalSystemIP,ip:%s",strIP.c_str());
            return strIP;
        }
    }
    WEBRTC_TRACE(kTraceWarning, kTraceTransport, -1, "gethostname failed");
    return -1;
#else // WEBRTC_LINUX
    int sockfd, size  = 1;
    struct ifreq* ifr;
    struct ifconf ifc;
    
    if (0 > (sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP))){
        return strIP;
    }
    ifc.ifc_len = size*sizeof (struct ifreq);
    ifc.ifc_req = NULL;
    do
    {
        ++size;
        // Buffer size needed is unknown. Try increasing it until no overflow
        // occurs.
        if (NULL == (ifc.ifc_req = (ifreq*)realloc(ifc.ifc_req, size*sizeof (struct ifreq)))) {
            fprintf(stderr, "Out of memory.\n");
            exit(EXIT_FAILURE);
        }
        ifc.ifc_len = size*sizeof (struct ifreq);
        if (ioctl(sockfd, SIOCGIFCONF, &ifc))
        {
            free(ifc.ifc_req);
            close(sockfd);
            return strIP;
        }
    } while  (size*sizeof (struct ifreq) <= ifc.ifc_len);
    
    ifr = ifc.ifc_req;
    for (;(char *) ifr < (char *) ifc.ifc_req + ifc.ifc_len; ++ifr)
    {
        if (ifr->ifr_addr.sa_data == (ifr+1)->ifr_addr.sa_data)
        {
            continue;  // duplicate, skip it
        }
        if (ioctl(sockfd, SIOCGIFFLAGS, ifr))
        {
            continue;  // failed to get flags, skip it
        }
        if(strncmp(ifr->ifr_name, "eth0",4) == 0 || strncmp(ifr->ifr_name, "wlan",4) == 0 )
        {
            char addr[INET_ADDRSTRLEN]={'0'};
            inet_ntop(AF_INET,&(((struct sockaddr_in*)(&ifr->ifr_addr))->sin_addr),addr,INET_ADDRSTRLEN);
            if(sLocalIP_.compare(addr) != 0){
                WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                             "TCPConnectionWrapper::GetLocalSystemIP,ip:%s,name:%s,len:%d",addr,ifr->ifr_name,ifc.ifc_len);
            }
            close(sockfd);
            free(ifc.ifc_req);
            return std::string(addr);
        }
    }
    //second time to get the ip
    ifr = ifc.ifc_req;
    for (;(char *) ifr < (char *) ifc.ifc_req + ifc.ifc_len; ++ifr)
    {
        if (ifr->ifr_addr.sa_data == (ifr+1)->ifr_addr.sa_data)
        {
            continue;  // duplicate, skip it
        }
        if (ioctl(sockfd, SIOCGIFFLAGS, ifr))
        {
            continue;  // failed to get flags, skip it
        }
        if(strncmp(ifr->ifr_name, "lo",3) == 0)
        {
            continue;
        }else {
            char addr[INET_ADDRSTRLEN]={'0'};
            inet_ntop(AF_INET,&(((struct sockaddr_in*)(&ifr->ifr_addr))->sin_addr),addr,INET_ADDRSTRLEN);
            if(sLocalIP_.compare(addr) != 0){
                WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                             "TCPConnectionWrapper::GetLocalSystemIP,ip:%s,name:%s,len:%d",addr,ifr->ifr_name,ifc.ifc_len);
            }
            close(sockfd);
            free(ifc.ifc_req);
            return std::string(addr);
        }
    }
    free(ifc.ifc_req);
    close(sockfd);
    return strIP;
#endif
}
int TCPConnectionWrapper::DecodeTcpMsgLoginResponse(ezxml_t ezxmlbody){
    WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceEngineWrapper, -1,
                 "TCPConnectionWrapper::DecodeTcpMsgLoginResponse");
    if(NULL == ezxmlbody){
        return -1;
    }
    ST_XML_LOGIN_RSP stXmlLoginReq;
    stXmlLoginReq.m_strUser[0] = 'a';
    int ret = XmlDecodeResponse_Login(ezxmlbody,&stXmlLoginReq);
    if(SUCCESS == ret){
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceEngineWrapper, -1,
                     "TCPConnectionWrapper::DecodeTcpMsgLoginResponse, response is:%s,%s,result:%d,%s",
                     stXmlLoginReq.m_strUser,
                     stXmlLoginReq.m_strPassword,
                     stXmlLoginReq.m_stResult.m_eResultCode,
                     stXmlLoginReq.m_stResult.m_strReason);
    }else{
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceEngineWrapper, -1,
                     "TCPConnectionWrapper::DecodeTcpMsgLoginResponse, failed to decode response");
        return -2;
    }
    CriticalSectionScoped cs(cstcpMsgListener);
    if(NULL != pSipEventCallback_){
        ret = pSipEventCallback_->DealSipEventRegisterResponse(&stXmlLoginReq);
    }else{
        ret = -100;
    }
    return ret;
}
int TCPConnectionWrapper::DecodeTcpLogoutMessage(ezxml_t ezxmlbody){
    WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceEngineWrapper, -1,
                 "TCPConnectionWrapper::DecodeTcpLogoutMessage");
    if(NULL == ezxmlbody){
        return -1;
    }
    ST_XML_LOGIN_RSP stXmlLoginOut;
    stXmlLoginOut.m_strUser[0] = 'a';
    int ret = XmlDecodeResponse_Logout(ezxmlbody,&stXmlLoginOut);
    if(SUCCESS == ret){
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceEngineWrapper, -1,
                     "TCPConnectionWrapper::DecodeTcpLogoutMessage, user:%s",
                     stXmlLoginOut.m_strUser);
    }else{
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceEngineWrapper, -1,
                     "TCPConnectionWrapper::DecodeTcpLogoutMessage, failed to decode response");
        return -2;
    }
    CriticalSectionScoped cs(cstcpMsgListener);
    if(NULL != pSipEventCallback_){
        ret = pSipEventCallback_->DealSipEventUnregisterResopnse(&stXmlLoginOut);
    }else{
        ret = -100;
    }
    return ret;
}

int TCPConnectionWrapper::DecodeTcpMsgGetCallsession(ezxml_t ezxmlbody){
    if(NULL == ezxmlbody){
        return -1;
    }
    ST_XML_GET_CALL_SESSION_REQ   stXmlGetCallSession;
    int ret = XmlDecodeRequest_GetCallSession(ezxmlbody,&stXmlGetCallSession);
    if(SUCCESS != ret){
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceEngineWrapper, -1,
                     "TCPConnectionWrapper::DecodeTcpMsgGetCallsession, failed to decode the request");
        return false;
    }
    CriticalSectionScoped cs(cstcpMsgListener);
    if(NULL != pSipEventCallback_){
        ret = pSipEventCallback_->DealSipEventGetCallsession(&stXmlGetCallSession);
    }else{
        ret = -100;
    }
    return ret;
}
int TCPConnectionWrapper::DecodeTcpMsgInvite(ezxml_t ezxmlbody){
    if(NULL == ezxmlbody){
        return -1;
    }
    ST_XML_INVITE stXmlInvite;
    int ret = XmlDecodeRequest_Invite(ezxmlbody,&stXmlInvite);
    if(SUCCESS != ret){
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceEngineWrapper, -1,
                     "TCPConnectionWrapper::DecodeTcpMsgInvite, failed to decode response");
        return -2;
    }
    CriticalSectionScoped cs(cstcpMsgListener);
    if(NULL != pSipEventCallback_){
        ret = pSipEventCallback_->DealSipEventInvite(&stXmlInvite);
    }else{
        ret = -100;
    }
    return ret;
}
int TCPConnectionWrapper::DecodeTcpMsgAnswer(ezxml_t ezxmlbody){
    WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceEngineWrapper, -1,
                 "TCPConnectionWrapper::DecodeTcpMsgAnswer()");
    if(NULL == ezxmlbody){
        return -1;
    }
    ST_XML_ANSWER	stXmlAnswer;
    int ret = XmlDecodeRsponse_Answer(ezxmlbody,&stXmlAnswer);
    //
    if(SUCCESS != ret){
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceEngineWrapper, -1,
                     "TCPConnectionWrapper::DecodeTcpMsgAnswer, failed to decode answer response");
        return -2;
    }
    CriticalSectionScoped cs(cstcpMsgListener);
    if(NULL != pSipEventCallback_){
        ret = pSipEventCallback_->DealSipEventAnswer(&stXmlAnswer);
    }else{
        ret = -100;
    }
    return ret;
}
int TCPConnectionWrapper::DecodeTcpMsgRing(ezxml_t ezxmlbody){
    WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceEngineWrapper, -1,
                 "TCPConnectionWrapper::DecodeTcpMsgRing()");
    if(NULL == ezxmlbody){
        return -1;
    }
    ST_XML_RING stXmlRing;
    int ret = XmlDecodeRsponse_Ring(ezxmlbody,&stXmlRing);
    if(SUCCESS != ret){
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceEngineWrapper, -1,
                     "TCPConnectionWrapper::DecodeTcpMsgRing, failed to decode the response");
        return -2;
    }
    CriticalSectionScoped cs(cstcpMsgListener);
    if(NULL != pSipEventCallback_){
        ret = pSipEventCallback_->DealSipEventRing(&stXmlRing);
    }else{
        ret = -100;
    }
    return ret;
}
int TCPConnectionWrapper::DecodeTcpMsgReinvite(ezxml_t ezxmlbody){
    if(NULL == ezxmlbody){
        return -1;
    }
    ST_XML_REINVITE stXmlReInvite;
    int ret = XmlDecodeRequest_Reinvite(ezxmlbody,&stXmlReInvite);
    if(SUCCESS != ret){
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceEngineWrapper, -1,
                     "TCPConnectionWrapper::DecodeTcpMsgReinvite, failed to decode the request");
        return -2;
    }
    CriticalSectionScoped cs(cstcpMsgListener);
    if(NULL != pSipEventCallback_){
        ret = pSipEventCallback_->DealSipEventReinvite(&stXmlReInvite);
    }else{
        ret = -100;
    }
    return ret;
}
int TCPConnectionWrapper::DecodeTcpMsgHangup(ezxml_t ezxmlbody){
    WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceEngineWrapper, -1,
                 "TCPConnectionWrapper::DecodeTcpMsgHangup, begin");
    if(NULL == ezxmlbody){
        return -1;
    }
    ST_XML_HANGUP_RSP stXmlHangup;
    int ret = XmlDecodeRequest_Hangup(ezxmlbody,&stXmlHangup);
    if(0 != ret){
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceEngineWrapper, -1,
                     "TCPConnectionWrapper::DecodeTcpMsgHangup, decode request error:%d",ret);
        return -2;
    }
    CriticalSectionScoped cs(cstcpMsgListener);
    if(NULL != pSipEventCallback_){
        ret = pSipEventCallback_->DealSipEventHangup(&stXmlHangup);
    }else{
        ret = -100;
    }
    return ret;
}
int TCPConnectionWrapper::DecodeTcpMsgHangupACK(ezxml_t ezxmlbody){
    WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceEngineWrapper, -1,
                 "TCPConnectionWrapper::DecodeTcpMsgHangupACK, begin");
    if(NULL == ezxmlbody){
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceEngineWrapper, -1,
                     "TCPConnectionWrapper::DecodeTcpMsgHangupACK, parameter is NULL");
        return -1;
    }
    ST_XML_HANGUP_RSP stXmlHangup;
    int ret = XmlDecodeRequest_Hangup(ezxmlbody,&stXmlHangup);
    if(0 != ret){
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceEngineWrapper, -1,
                     "TCPConnectionWrapper::DecodeTcpMsgHangupACK, decode request error:%d",ret);
        return -2;
    }
    CriticalSectionScoped cs(cstcpMsgListener);
    if(NULL != pSipEventCallback_){
        ret = pSipEventCallback_->DealSipEventHangupACK(&stXmlHangup);
    }else{
        ret = -100;
    }
    return ret;
}

int TCPConnectionWrapper::DecodeTcpMsgTcpHeartbeat(ezxml_t ezxmlbody){
    if(NULL == ezxmlbody){
        return -1;
    }
    CriticalSectionScoped cs(cstcpMsgListener);
    int ret = -100;
    if(NULL != pSipEventCallback_){
        ret = pSipEventCallback_->DealSipEventTCPHeartbeat();
    }
    return ret;
}
int TCPConnectionWrapper::DecodeTcpMsgMessage(ezxml_t ezxmlbody){
    if(NULL == ezxmlbody){
        return -1;
    }
    ST_XML_MESSAGE stXmlMessage;
    int ret = XmlDecodeRequest_Message(ezxmlbody,&stXmlMessage);
    if(0 != ret){
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceEngineWrapper, -1,
                     "TCPConnectionWrapper::DecodeTcpMsgMessage, decode message error:%d",ret);
        return -2;
    }
    CriticalSectionScoped cs(cstcpMsgListener);
    if(NULL != pSipEventCallback_){
        ret = pSipEventCallback_->DealSipEventMessage(&stXmlMessage);
    }else{
        ret = -100;
    }
    return ret;
}
int TCPConnectionWrapper::ProcessingTCPMsg(char* pData, int len){
    if(NULL == strstr(pData,"heartbeat")){
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceEngineWrapper, -1,
                     "TCPConnectionWrapper::ProcessingTCPMsg, msg:%s,len:%d",pData,len);
    }
    int cmdType = XmlDecodeParseType(pData);
    ezxml_t ezxmlbody = ezxml_parse_str(pData,len);
    if(XML_CMD_TCP_HEARTBEAT != cmdType){
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceEngineWrapper, -1,
                     "TCPConnectionWrapper::ProcessingTCPMsg,type:%d,name:%s",cmdType,ezxmlbody->name);
    }
    switch(cmdType){
        case XML_CMD_UNKNOW:
            break;
        case XML_CMD_LOGIN:
            DecodeTcpMsgLoginResponse(ezxmlbody);
            break;
        case XML_CMD_INVITE:
            DecodeTcpMsgInvite(ezxmlbody);
            break;
        case XML_CMD_RING:
            DecodeTcpMsgRing(ezxmlbody);
            break;
        case XML_CMD_ANSWER:
            DecodeTcpMsgAnswer(ezxmlbody);
            break;
        case XML_CMD_GET_CALL_SESSION:
            DecodeTcpMsgGetCallsession(ezxmlbody);
            break;
        case XML_CMD_REINVITE:
            DecodeTcpMsgReinvite(ezxmlbody);
            break;
        case XML_CMD_HANGUP:
            DecodeTcpMsgHangup(ezxmlbody);
            break;
        case XML_CMD_HANGUPACK:
            DecodeTcpMsgHangupACK(ezxmlbody);
            break;
        case XML_CMD_TCP_HEARTBEAT:
            DecodeTcpMsgTcpHeartbeat(ezxmlbody);
            break;
        case XML_CMD_MESSAGE:
            DecodeTcpMsgMessage(ezxmlbody);
            break;
        case XML_CMD_LOGOUT:
            DecodeTcpLogoutMessage(ezxmlbody);
        default:
            break;
    }
    ezxml_free(ezxmlbody);
    return 0;
}

int TCPConnectionWrapper::Connect_Nonb(int sockfd, const struct sockaddr *saprt, socklen_t salen){
    
    fd_set fdr, fdw;
    struct timeval timeout;
    int err = 0;
    int errlen = sizeof(err);
    
    /*设置套接字为非阻塞*/
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags < 0) {
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                     "TCPConnectionWrapper::Connect_Nonb, Get flags error:%s",strerror(errno));
        return -1;
    }
    flags |= O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, flags) < 0) {
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                     "TCPConnectionWrapper::Connect_Nonb, Set flags error:%s",strerror(errno));
        return -1;
    }
    
    /*阻塞情况下linux系统默认超时时间为75s*/
    int rc = connect(sockfd, saprt, salen);
    if (rc != 0) {
        if (errno == EINPROGRESS) {
            WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                         "TCPConnectionWrapper::Connect_Nonb, Doing connecting...");
            /*正在处理连接*/
            FD_ZERO(&fdr);
            FD_ZERO(&fdw);
            FD_SET(sockfd, &fdr);
            FD_SET(sockfd, &fdw);
            timeout.tv_sec = CONNECT_TIMEOUT;
            timeout.tv_usec = 0;
            rc = select(sockfd + 1, &fdr, &fdw, NULL, &timeout);
            WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                         "TCPConnectionWrapper::Connect_Nonb, select, rc is:%d",rc);
            /*select调用失败*/
            if (rc < 0) {
                WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                             "TCPConnectionWrapper::Connect_Nonb, failed to select, error:%s",strerror(errno));
                return -1;
            }
            /*连接超时*/
            if (rc == 0) {
                WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                             "TCPConnectionWrapper::Connect_Nonb, select, Connect timeout");
                return -1;
            }
            /*[1] 当连接成功建立时，描述符变成可写,rc=1*/
            if (1 == rc && FD_ISSET(sockfd, &fdw)) {
                /*设置为阻塞*/
                flags = fcntl(sockfd, F_GETFL, 0);
                if (flags < 0) {
                    WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                                 "TCPConnectionWrapper::Connect_Nonb, Connect success, but get flags error:%s", strerror(errno));
                    return -1;
                }
                flags &=~O_NONBLOCK;
                if (fcntl(sockfd, F_SETFL, flags) < 0) {
                    WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                                 "TCPConnectionWrapper::Connect_Nonb, Connect success, but set flags error:%s", strerror(errno));
                    return -1;
                }
                
                WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                             "TCPConnectionWrapper::Connect_Nonb, Connect success");
                return 0;
            }
            /*[2] 当连接建立遇到错误时，描述符变为即可读，也可写，rc=2 遇到这种情况，可调用getsockopt函数*/
            if (2 == rc) {
                if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, (socklen_t *)&errlen) == -1) {
                    WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                                 "TCPConnectionWrapper::Connect_Nonb, getsockopt(SO_ERROR):%s", strerror(errno));
                    return -1;
                }
                if (0 == err) {
                    /*设置为阻塞*/
                    flags = fcntl(sockfd, F_GETFL, 0);
                    if (flags < 0) {
                        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                                     "TCPConnectionWrapper::Connect_Nonb, Connect success 1, but get flags error:%s", strerror(errno));
                        return -1;
                    }
                    flags &=~O_NONBLOCK;
                    if (fcntl(sockfd, F_SETFL, flags) < 0) {
                        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                                     "TCPConnectionWrapper::Connect_Nonb, Connect success 1, but set flags error:%s", strerror(errno));
                        return -1;
                    }
                    
                    WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                                 "TCPConnectionWrapper::Connect_Nonb, Connect success 1");
                    return 0;
                }else{
                    errno = err;
                    WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                                 "TCPConnectionWrapper::Connect_Nonb, connect error:%s", strerror(errno));
                    return -1;
                }
            }
            
        }
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                     "TCPConnectionWrapper::Connect_Nonb, connect failed, error:%s", strerror(errno));
        return -1;
    }
    
    /*设置为阻塞*/
    flags = fcntl(sockfd, F_GETFL, 0);
    if (flags < 0) {
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                     "TCPConnectionWrapper::Connect_Nonb, Connect success 2, but get flags error:%s", strerror(errno));
        return -1;
    }
    flags &=~O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, flags) < 0) {
        WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                     "TCPConnectionWrapper::Connect_Nonb, Connect success 2, but set flags error:%s", strerror(errno));
        return -1;
    }
    
    WEBRTC_TRACE(cmcc_webrtc::kTraceStateInfo, cmcc_webrtc::KTraceSignalingConnection, -1,
                 "TCPConnectionWrapper::Connect_Nonb, Connect success 2");
    return 0;
}

}  // namespace test
}  // namespace cmcc_webrtc


