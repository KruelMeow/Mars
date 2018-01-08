
#ifndef WEBRTC_TEST_TCP_CONNETION_WRAPPER_H_
#define WEBRTC_TEST_TCP_CONNETION_WRAPPER_H_

#include <list>
#include "webrtc/base/basictypes.h"
#include "webrtc/base/ipaddress.h"
#include "webrtc/base/asynctcpsocket.h"
#include "webrtc/system_wrappers/interface/scoped_refptr.h"
#include "webrtc/system_wrappers/interface/thread_wrapper.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "sipEventDealCallback.h"
namespace cmcc_webrtc {
namespace test {
#define MAXMSGLENGTH 2048
#define RECVBUFFLEN 10240

#define STOPTCPCMD "stoptcpconnection"
    
#define CONNECT_TIMEOUT  10

enum ConnectionFailReason{
    ConnectionFail_Unknown = 0,
    ConnectionFail_GetLocalIP,        //failed to get local system ip
    ConnectionFail_RecvError,         //recv return error
    ConnectionFail_RecvLenZero,       //server close the link
    ConnectionFail_NewSocket,         //local error
    ConnectionFail_Connect,           //server unreachable
    ConnectionFail_SendError,         //server close the link
    ConnectionFail_ClientClose,       //client close the socket
    ConnectionFail_Reconnect,         //client close the socket
};

class SocketPacket{
public:
    SocketPacket(char* data,int len);
    inline char* GetData(){return buf_;}
    inline int GetLength(){return len_;}
private:
    char buf_[MAXMSGLENGTH];
    int len_;
};
typedef std::list<SocketPacket*> socketPacketsSendList;
class TCPMsgListener{
public:
    virtual ~TCPMsgListener(){};
    virtual void OnTCPConnected()=0;
    virtual void OnTCPDisconnected(ConnectionFailReason type)=0;
    virtual void OnTCPRecvMsg(char* pData, int len)=0;
};

class TCPConnectionWrapper{
public:
    TCPConnectionWrapper();
    ~TCPConnectionWrapper();
    int SetServer(std::string strServer, int port);
    int SendData2Server(char* data, int len);
    int AddTCPMsgListener(TCPMsgListener* ob);
    int RemoveTCPMsgListener(TCPMsgListener* ob);
    int SetSipEventDealCallBack(SipEventDealCallback* pCallback);
    void Reset();
    std::string GetLocalIP();
    std::string GetServerIP();
    void Reconnet();
private:
    std::string GetLocalSystemIP(const bool bIPV4);
    static bool SocketThreadSendFunc(void*);
    static bool SocketThreadRecvFunc(void*);
    bool SocketThreadSendProcess();
    bool SocketThreadRecvProcess();
    void ProcessingInputData(char* data, int len);
    void SendResponseToListener(char*data, int len);
    void SendTCPConnectedMsg();
    void SendTCPDisconnectedMsg(ConnectionFailReason type);
    void ClearSendPacketsList();
    bool IsStopCMDInPacketsList();
    void ResetSocket();
    bool IsServerTypeIPV4AndGetIP(std::string serverName);
    int  Connect_Nonb(int sockfd, const struct sockaddr *saprt, socklen_t salen);
    //
    int DecodeTcpMsgLoginResponse(ezxml_t ezxmlbody);
    int DecodeTcpMsgInvite(ezxml_t ezxmlbody);
    int DecodeTcpMsgAnswer(ezxml_t ezxmlbody);
    int DecodeTcpMsgRing(ezxml_t ezxmlbody);
    int DecodeTcpMsgGetCallsession(ezxml_t ezxmlbody);
    int DecodeTcpMsgReinvite(ezxml_t ezxmlbody);
    int DecodeTcpMsgHangup(ezxml_t ezxmlbody);
    int DecodeTcpMsgHangupACK(ezxml_t ezxmlbody);
    int DecodeTcpMsgTcpHeartbeat(ezxml_t ezxmlbody);
    int DecodeTcpMsgMessage(ezxml_t ezxmlbody);
    int DecodeTcpLogoutMessage(ezxml_t ezxmlbody);
    int ProcessingTCPMsg(char* pData, int len);

private:
    //new socket
    CriticalSectionWrapper* csSocket_;
    int socketfd_;
    struct sockaddr_in addr_local_;
    struct sockaddr_in addr_ser_;
    //Thread recv and send socket data
    ThreadWrapper* ptrThreadSend_;
    ThreadWrapper* ptrThreadRecv_;
    //list of
    CriticalSectionWrapper* csSendlist_;
    socketPacketsSendList sendPackets_;
    //
    std::string sServerAddr_;
    std::string sServerIP_;
    bool bServerIsIPV4_;
    int iServerPort_;
    std::string sLocalIP_;
    //
    int error_;
    bool connected_;
    bool reported_;
    bool recvStopTcpCMD_;
    bool resetConnection_;
    bool exit_;
    //
    char* pcharRecvBuf_;
    int iRecvBufReadIndex_;
    int iRecvBufWriteIndex_;
    //
    CriticalSectionWrapper* cstcpMsgListener;
    std::list<TCPMsgListener*> listeners_TCPMsg_;
    //
    SipEventDealCallback *pSipEventCallback_;
};

}  // namespace test
}  // namespace cmcc_webrtc

#endif  // WEBRTC_TEST_TCP_CONNETION_WRAPPER_H_
