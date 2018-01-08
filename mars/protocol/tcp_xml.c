#include "tcp_xml.h"
#if defined(WEBRTC_POSIX)
#include<stdio.h>  
#include<stdlib.h>  
#include<string.h>
#endif  // WEBRTC_POSIX


static int GetBufferStringLength(char* buffer,int buflen){
  int bufpos = 0;
  while (buffer[bufpos] && (bufpos < buflen)) {
    ++bufpos;
  }
  return bufpos;
}
static int XmlEn_AddPakcetHeader(char* buffer,int buflen){
  int headerLen = 7;
  if(buflen < headerLen){
  	return 0;
  }
  buffer[0] = 'x';
  buffer[1] = 'x';
  buffer[2] = 'x';
  buffer[3] = 'b';
  buffer[4] = 'b';
  buffer[5] = 'b';
  buffer[6] = '\n';
  return headerLen;
}
static int XmlEn_AddPakcetTailer(char* buffer,int buflen){
  int stringLen = GetBufferStringLength(buffer,buflen);
  int tailerLen = 7;
  if(stringLen+tailerLen>= buflen){
  	return 0;
  }
  buffer[stringLen] = '\n';
  buffer[stringLen+1] = 'x';
  buffer[stringLen+2] = 'x';
  buffer[stringLen+3] = 'x';
  buffer[stringLen+4] = 'e';
  buffer[stringLen+5] = 'e';
  buffer[stringLen+6] = 'e';
  return stringLen+tailerLen;
}
static int XmlEn_BuildStringVa(char **buffer, int *space, const char *fmt, va_list ap)
{
  int result;
  if (!buffer || !*buffer || !space || !*space)
    return -1;
  result = vsnprintf(*buffer, *space, fmt, ap);
  if (result < 0)
    return -1;
  else if (result > *space)
    result = *space;
  *buffer += result;
  *space -= result;
  return 0;
}

int XmlEn_BuildString(char **buffer, int *space, const char *fmt, ...){
  va_list ap;
  int result;
  va_start(ap, fmt);
  result = XmlEn_BuildStringVa(buffer, space, fmt, ap);
  va_end(ap);
  return result;
}

void XmlEn_int2str(int i, char *buf){
  sprintf(buf,"%d",i);
}

void XmlEn_BuildXmlStr(char *buf, int len, char *s){
  XmlEn_BuildString(&buf, &len, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  XmlEn_BuildString(&buf, &len, s);
}

int XmlEncode_MakeReqId(){
	static int  iWebReqId = 1;
	iWebReqId ++;
	if(iWebReqId > 655535){
		iWebReqId = 1;
	}
	return iWebReqId;
}
int XmlEncode_Result(ezxml_t Xml,ST_XML_RESULT *pstResult){
	if(NULL == Xml || NULL == pstResult){
		return FAIL;
	}
	ezxml_t pXmlReturn = ezxml_add_child(Xml,"return",0);	

	if(E_XML_SUCCESS ==pstResult->m_eResultCode)
	{
		ezxml_set_attr_d(pXmlReturn, "result", "success");

	}
	else
	{
		ezxml_set_attr_d(pXmlReturn, "result", "fail");
		ezxml_set_attr_d(pXmlReturn, "reason", pstResult->m_strReason);
	}
	return SUCCESS;
}
int XmlEncode_Call(ezxml_t pXml,ST_XML_CALL_INFO	*pstCall)
{
	if(NULL == pXml || NULL == pstCall)
	{
		return FAIL;
	}
	ezxml_t pXmlCall = ezxml_add_child(pXml,"call",0);	
	ezxml_set_attr_d(pXmlCall, "from", pstCall->m_strFromTel);
	ezxml_set_attr_d(pXmlCall, "display", pstCall->m_strDisplayTel);
	ezxml_set_attr_d(pXmlCall, "to", pstCall->m_strToTel);
	ezxml_set_attr_d(pXmlCall, "callid", pstCall->m_strCallId);
	if(1 == pstCall->m_iIsfocus){
		ezxml_set_attr_d(pXmlCall, "isfocus", "1");
	}
	
	return SUCCESS;
}
static int XmlDecode_Call(ezxml_t pXml,ST_XML_CALL_INFO	*pstCall)
{
	ezxml_t  pXmlCall;
	if(NULL == pXml || NULL == pstCall)
	{
//		logprintf(ERROR,"[XmlDecode_Call] input tree error\n");
		return FAIL;
	}
	memset(pstCall,0,sizeof(ST_XML_CALL_INFO));
	pXmlCall = ezxml_child(pXml, "call") ;
	if(NULL != pXmlCall)
	{
		strncpy(pstCall->m_strFromTel, (char *)ezxml_attr(pXmlCall,"from"), sizeof(pstCall->m_strFromTel));
		strncpy(pstCall->m_strDisplayTel, (char *)ezxml_attr(pXmlCall,"display"), sizeof(pstCall->m_strDisplayTel));
		strncpy(pstCall->m_strToTel, (char *)ezxml_attr(pXmlCall,"to"), sizeof(pstCall->m_strToTel));
		strncpy(pstCall->m_strCallId, (char *)ezxml_attr(pXmlCall,"callid"), sizeof(pstCall->m_strCallId));
		char strIsFoucus[32]="";
		char* attrisfocus = (char *)ezxml_attr(pXmlCall,"isfocus");
		if(NULL != attrisfocus){
		    strncpy(strIsFoucus, attrisfocus, sizeof(strIsFoucus));
			pstCall->m_iIsfocus = atoi(strIsFoucus);
		}else{
		    pstCall->m_iIsfocus = 0;
		}
        pstCall->m_iIsVolteCall = 0;
	}
	
	return SUCCESS;
}
static int XmlDecode_Media(ezxml_t pXml,ST_XML_MEDIA_SDP	 *pstMediaSdp)
{
	ezxml_t  pXmlCall;
	if(NULL == pXml || NULL == pstMediaSdp)
	{
//		logprintf(ERROR,"[XmlDecode_Media] input tree error\n");
		return FAIL;
	}
	memset(pstMediaSdp,0,sizeof(ST_XML_MEDIA_SDP));
	pXmlCall = ezxml_child(pXml, "media") ;
	if(NULL != pXmlCall)
	{
		pstMediaSdp->m_iUse = 1;
		ezxml_t pXmlAudio = ezxml_child(pXmlCall, "audio") ;
		if(NULL != pXmlAudio)
		{
			pstMediaSdp->m_stAudio.m_iUse = 1;
			strncpy(pstMediaSdp->m_stAudio.m_strIp, (char *)ezxml_attr(pXmlAudio,"ip"), sizeof(pstMediaSdp->m_stAudio.m_strIp));
			char strPort[8] = "";
			strncpy(strPort, (char *)ezxml_attr(pXmlAudio,"port"), sizeof(strPort));
			pstMediaSdp->m_stAudio.m_iPort = atoi(strPort);
			strncpy(pstMediaSdp->m_stAudio.m_MediaCode, (char *)ezxml_attr(pXmlAudio,"code"), sizeof(pstMediaSdp->m_stAudio.m_MediaCode));
			ezxml_t pXmlRtpMap =NULL;
			int id =0;
            int first = 1;
			for (pXmlRtpMap = ezxml_child(pXmlAudio, "rtpmap") ; pXmlRtpMap ; pXmlRtpMap=pXmlRtpMap->next)
			{
				if(id >=MAX_RTP_MAP_NUM-1 )
					break;
                if(first != 1 && strncmp((char*)ezxml_attr(pXmlRtpMap,"name"),"fmtp",4) != 0){
                    id++;
                }
				pstMediaSdp->m_stAudio.m_stRtpMap[id].m_iUse=1;
                if(strncmp((char*)ezxml_attr(pXmlRtpMap,"name"),"fmtp",4) != 0){
				    strncpy(pstMediaSdp->m_stAudio.m_stRtpMap[id].m_strName, (char *)ezxml_attr(pXmlRtpMap,"name")
					  , sizeof(pstMediaSdp->m_stAudio.m_stRtpMap[id].m_strName));
				    strncpy(pstMediaSdp->m_stAudio.m_stRtpMap[id].m_strAttr, (char *)ezxml_attr(pXmlRtpMap,"attr")
					  , sizeof(pstMediaSdp->m_stAudio.m_stRtpMap[id].m_strAttr));
                }else{
                    pstMediaSdp->m_stAudio.m_stRtpMap[id].m_ifmtpUse = 1;
                    strncpy(pstMediaSdp->m_stAudio.m_stRtpMap[id].m_strfmtpName, (char *)ezxml_attr(pXmlRtpMap,"name")
                            , sizeof(pstMediaSdp->m_stAudio.m_stRtpMap[id].m_strName));
                    strncpy(pstMediaSdp->m_stAudio.m_stRtpMap[id].m_strfmtpAttr, (char *)ezxml_attr(pXmlRtpMap,"attr")
                            , sizeof(pstMediaSdp->m_stAudio.m_stRtpMap[id].m_strAttr));
                }
                first = 0;
				
			}
		}

		ezxml_t pXmlVideo = ezxml_child(pXmlCall, "video") ;
		if(NULL != pXmlVideo)
		{
			pstMediaSdp->m_stVideo.m_iUse = 1;
			strncpy(pstMediaSdp->m_stVideo.m_strIp, (char *)ezxml_attr(pXmlVideo,"ip"), sizeof(pstMediaSdp->m_stVideo.m_strIp));
			char strPort[8] = "";
			strncpy(strPort, (char *)ezxml_attr(pXmlVideo,"port"), sizeof(strPort));
			pstMediaSdp->m_stVideo.m_iPort = atoi(strPort);
		
			strncpy(pstMediaSdp->m_stVideo.m_MediaCode, (char *)ezxml_attr(pXmlVideo,"code"), sizeof(pstMediaSdp->m_stVideo.m_MediaCode));
		
			ezxml_t pXmlRtpMap =NULL;
			int id =0;
			for (pXmlRtpMap = ezxml_child(pXmlVideo, "rtpmap") ; pXmlRtpMap ; pXmlRtpMap=pXmlRtpMap->next)
			{
				if(id >=MAX_RTP_MAP_NUM )
					break;
		
				
				pstMediaSdp->m_stVideo.m_stRtpMap[id].m_iUse=1;
				strncpy(pstMediaSdp->m_stVideo.m_stRtpMap[id].m_strName, (char *)ezxml_attr(pXmlRtpMap,"name")
					, sizeof(pstMediaSdp->m_stVideo.m_stRtpMap[id].m_strName));
				
				strncpy(pstMediaSdp->m_stVideo.m_stRtpMap[id].m_strAttr, (char *)ezxml_attr(pXmlRtpMap,"attr")
					, sizeof(pstMediaSdp->m_stVideo.m_stRtpMap[id].m_strAttr));
		
				id++;
			}
		
		
		}

		
	}

	return SUCCESS;

}



static int XmlEncode_Media(ezxml_t pXml,ST_XML_MEDIA_SDP	 *pstMediaSdp)
{

	if(NULL == pXml || NULL == pstMediaSdp)
	{
		return FAIL;
	}
	
//	logprintf(FLOW,"[XmlEncode_Media]  SDP[%d]  audio[%d]  video[%d]\n",pstMediaSdp->m_iUse
//		,pstMediaSdp->m_stAudio.m_iUse,pstMediaSdp->m_stVideo.m_iUse);

	if(1 !=pstMediaSdp->m_iUse || (1 != pstMediaSdp->m_stAudio.m_iUse && 1 != pstMediaSdp->m_stVideo.m_iUse))
	{
		return FAIL;
	}

	
	ezxml_t pXmlMedia = ezxml_add_child(pXml,"media",0);	

		
	if(1 == pstMediaSdp->m_stAudio.m_iUse)
	{
		ezxml_t pXmlAudio = ezxml_add_child(pXmlMedia,"audio",0);
		ezxml_set_attr_d(pXmlAudio, "ip", pstMediaSdp->m_stAudio.m_strIp);
		char strPort[8] = "";
		sprintf(strPort,"%d",pstMediaSdp->m_stAudio.m_iPort);
		ezxml_set_attr_d(pXmlAudio, "port", strPort);

		ezxml_set_attr_d(pXmlAudio, "code", pstMediaSdp->m_stAudio.m_MediaCode);

		
		int id = 0;
		for(id = 0;id<MAX_RTP_MAP_NUM;id ++)
		{
			if(1 == pstMediaSdp->m_stAudio.m_stRtpMap[id].m_iUse)
			{
				ezxml_t pXmlRtpMap = ezxml_add_child(pXmlAudio,"rtpmap",0);

				ezxml_set_attr_d(pXmlRtpMap, "name", pstMediaSdp->m_stAudio.m_stRtpMap[id].m_strName);
				ezxml_set_attr_d(pXmlRtpMap, "attr", pstMediaSdp->m_stAudio.m_stRtpMap[id].m_strAttr);
                //fmtp
                if(1 == pstMediaSdp->m_stAudio.m_stRtpMap[id].m_ifmtpUse){
                    ezxml_t pXmlFtmpMap = ezxml_add_child(pXmlAudio,"rtpmap",0);
                    ezxml_set_attr_d(pXmlFtmpMap, "name", pstMediaSdp->m_stAudio.m_stRtpMap[id].m_strfmtpName);
                    ezxml_set_attr_d(pXmlFtmpMap, "attr", pstMediaSdp->m_stAudio.m_stRtpMap[id].m_strfmtpAttr);
                }
			}

		}

		
	}


	if(1 == pstMediaSdp->m_stVideo.m_iUse)
	{
		ezxml_t pXmlVideo = ezxml_add_child(pXmlMedia,"video",0);
		ezxml_set_attr_d(pXmlVideo, "ip", pstMediaSdp->m_stVideo.m_strIp);
		char strPort[8] = "";
		sprintf(strPort,"%d",pstMediaSdp->m_stVideo.m_iPort);
		ezxml_set_attr_d(pXmlVideo, "port", strPort);


		ezxml_set_attr_d(pXmlVideo, "code", pstMediaSdp->m_stVideo.m_MediaCode);
		
		int id = 0;
		for(id = 0;id<MAX_RTP_MAP_NUM;id ++)
		{
			if(1 == pstMediaSdp->m_stVideo.m_stRtpMap[id].m_iUse)
			{
				ezxml_t pXmlRtpMap = ezxml_add_child(pXmlVideo,"rtpmap",0);

				ezxml_set_attr_d(pXmlRtpMap, "name", pstMediaSdp->m_stVideo.m_stRtpMap[id].m_strName);
				ezxml_set_attr_d(pXmlRtpMap, "attr", pstMediaSdp->m_stVideo.m_stRtpMap[id].m_strAttr);

			}

		}
	}



	return SUCCESS;
}

int XmlEncodeRequst_Login2(ST_XML_LOGIN_REQ   *pstXmlLoginReq, char* pResultBuf, int iSize , int bFirstTime , int iTimeSec, char* deviceType){
	char  *s=NULL;
	ezxml_t XmlRoot;
	ezxml_t  pXmlUserinfo;
	char strMsgId[8]="";
	char strTime[16] = "";
	memset(pResultBuf,0,iSize);
	int len = XmlEn_AddPakcetHeader(pResultBuf,iSize);
	XmlRoot = ezxml_new_d("request");
	ezxml_set_attr_d(XmlRoot, "type", "login");

	sprintf(strMsgId,"%d",XmlEncode_MakeReqId());
	ezxml_set_attr_d(XmlRoot, "msgid",strMsgId );
    
	
	pXmlUserinfo = ezxml_add_child(XmlRoot,"userinfo",0);	
	ezxml_set_attr_d(pXmlUserinfo, "user", pstXmlLoginReq->m_strUser);
	ezxml_set_attr_d(pXmlUserinfo, "passwd", pstXmlLoginReq->m_strPassword);
    if(1 == bFirstTime){
        ezxml_set_attr_d(pXmlUserinfo, "first", "yes");
    }
	ezxml_set_attr_d(pXmlUserinfo, "devicetype", deviceType);
	
	sprintf(strTime,"%d",iTimeSec);
	ezxml_set_attr_d(pXmlUserinfo, "nonce", strTime);
    
	ezxml_pretty(XmlRoot);
	s = ezxml_toxml(XmlRoot);
	XmlEn_BuildXmlStr(pResultBuf+len, iSize-len, s);
	ezxml_free(XmlRoot);
	XmlEn_AddPakcetTailer(pResultBuf,iSize);
	if(s != NULL){
		free(s);
	}
	return SUCCESS;
}
int XmlEncodeRequst_Login(ST_XML_LOGIN_REQ   *pstXmlLoginReq, char* pResultBuf, int iSize , int bFirstTime,char* deviceType){
	char  *s=NULL;
	ezxml_t XmlRoot;
	ezxml_t  pXmlUserinfo;
	char strMsgId[8]="";
	memset(pResultBuf,0,iSize);
	int len = XmlEn_AddPakcetHeader(pResultBuf,iSize);
	XmlRoot = ezxml_new_d("request");
	ezxml_set_attr_d(XmlRoot, "type", "login");

	sprintf(strMsgId,"%d",XmlEncode_MakeReqId());
	ezxml_set_attr_d(XmlRoot, "msgid",strMsgId );
    
	
	pXmlUserinfo = ezxml_add_child(XmlRoot,"userinfo",0);	
	ezxml_set_attr_d(pXmlUserinfo, "user", pstXmlLoginReq->m_strUser);
	ezxml_set_attr_d(pXmlUserinfo, "passwd", pstXmlLoginReq->m_strPassword);
    if(1 == bFirstTime){
        ezxml_set_attr_d(pXmlUserinfo, "first", "yes");
    }
    ezxml_set_attr_d(pXmlUserinfo, "devicetype", deviceType);
	ezxml_pretty(XmlRoot);
	s = ezxml_toxml(XmlRoot);
	XmlEn_BuildXmlStr(pResultBuf+len, iSize-len, s);
	ezxml_free(XmlRoot);
	XmlEn_AddPakcetTailer(pResultBuf,iSize);
	if(s != NULL){
		free(s);
	}
	return SUCCESS;
}
int XmlEncodeRequst_noop( char* pResultBuf, int iSize )
{
	char  *s=NULL;
	ezxml_t XmlRoot;
	char strMsgId[8]="";
	memset(pResultBuf,0,iSize);
	int len = XmlEn_AddPakcetHeader(pResultBuf,iSize);
	XmlRoot = ezxml_new_d("request");
	ezxml_set_attr_d(XmlRoot, "type", "heartbeat");

	sprintf(strMsgId,"%d",XmlEncode_MakeReqId());
	ezxml_set_attr_d(XmlRoot, "msgid","8950" );
	ezxml_pretty(XmlRoot);
	s = ezxml_toxml(XmlRoot);
	XmlEn_BuildXmlStr(pResultBuf+len, iSize-len, s);
	ezxml_free(XmlRoot);
	XmlEn_AddPakcetTailer(pResultBuf,iSize);
	if(s != NULL){
		free(s);
	}
	return SUCCESS;
}

int XmlEncodeRsponse_Login(ST_XML_LOGIN_RSP   *pstXmlLoginRsp, char* pResultBuf, int iSize){

	char  *s=NULL;
	ezxml_t XmlRoot;
	ezxml_t  pXmlUserinfo;
	char strMsgId[8]="";

	memset(pResultBuf,0,iSize);
	int len = XmlEn_AddPakcetHeader(pResultBuf,iSize);

	XmlRoot = ezxml_new_d("response");
	ezxml_set_attr_d(XmlRoot, "type", "login");

	sprintf(strMsgId,"%d",XmlEncode_MakeReqId());
	ezxml_set_attr_d(XmlRoot, "msgid",strMsgId );
	
	pXmlUserinfo = ezxml_add_child(XmlRoot,"userinfo",0);	
	ezxml_set_attr_d(pXmlUserinfo, "user", pstXmlLoginRsp->m_strUser);
	ezxml_set_attr_d(pXmlUserinfo, "passwd", pstXmlLoginRsp->m_strPassword);

	XmlEncode_Result(XmlRoot,&pstXmlLoginRsp->m_stResult);
	
	ezxml_pretty(XmlRoot);
	s = ezxml_toxml(XmlRoot);
	XmlEn_BuildXmlStr(pResultBuf+len, iSize-len, s);
	ezxml_free(XmlRoot);
	XmlEn_AddPakcetTailer(pResultBuf,iSize);
	if(s != NULL){
		free(s);
	}
	return SUCCESS;
}

int XmlDecodeRequest_Login(ezxml_t XmlBody,ST_XML_LOGIN_REQ* pstXmlLoginReq){

	ezxml_t  pXmlUserInfo;
	if(NULL == XmlBody || NULL == pstXmlLoginReq){
//		logprintf(ERROR,"[XmlDecodeRequest_Login] input tree error\n");
		return FAIL;
	}
	memset(pstXmlLoginReq,0,sizeof(pstXmlLoginReq));
	pXmlUserInfo = ezxml_child(XmlBody, "userinfo");
	if(NULL != pXmlUserInfo){
		strncpy(pstXmlLoginReq->m_strUser, (char *)ezxml_attr(pXmlUserInfo,"user"), sizeof(pstXmlLoginReq->m_strUser));
		strncpy(pstXmlLoginReq->m_strPassword, (char *)ezxml_attr(pXmlUserInfo,"passwd"), sizeof(pstXmlLoginReq->m_strPassword));
	}
	pXmlUserInfo = ezxml_child(XmlBody, "return");
	if(NULL != pXmlUserInfo){
      strncpy(pstXmlLoginReq->m_strUser, (char *)ezxml_attr(pXmlUserInfo,"result"), sizeof(pstXmlLoginReq->m_strUser));
    }

	return SUCCESS;
}

int XmlDecodeResponse_Login(ezxml_t XmlBody,ST_XML_LOGIN_RSP *pstXmlLoginRsp)
{
	ezxml_t  pXmlUserInfo;
	if(NULL == XmlBody || NULL == pstXmlLoginRsp)
	{
//		logprintf(ERROR,"[XmlDecodeResponse_Login] input tree error\n");
		return FAIL;
	}
	memset(pstXmlLoginRsp,0,sizeof(ST_XML_LOGIN_RSP));
    pstXmlLoginRsp->m_iLoginRspBySip = 0;
	pXmlUserInfo = ezxml_child(XmlBody, "userinfo") ;
	
	if(NULL != pXmlUserInfo)
	{
		strncpy(pstXmlLoginRsp->m_strUser, (char *)ezxml_attr(pXmlUserInfo,"user"), sizeof(pstXmlLoginRsp->m_strUser));
		strncpy(pstXmlLoginRsp->m_strPassword, (char *)ezxml_attr(pXmlUserInfo,"passwd"), sizeof(pstXmlLoginRsp->m_strPassword));
	}
	ezxml_t ResultXml = ezxml_child(XmlBody, "return") ;
	if(NULL != ResultXml)
	{
		char strResult[32]="";
		strncpy(strResult, (char *)ezxml_attr(ResultXml,"result"), sizeof(strResult));
		if(0 ==strcmp(strResult,"success")){
			pstXmlLoginRsp->m_stResult.m_eResultCode = E_XML_SUCCESS;
		}
		else{
			pstXmlLoginRsp->m_stResult.m_eResultCode = E_XML_FAIL;
			strncpy(pstXmlLoginRsp->m_stResult.m_strReason, (char *)ezxml_attr(ResultXml,"reason")
				, sizeof(pstXmlLoginRsp->m_stResult.m_strReason));
		}
	}
	return SUCCESS;
}

int XmlDecodeResponse_Logout(ezxml_t XmlBody,ST_XML_LOGIN_RSP *pstXmlLoginRsp)
{
	ezxml_t  pXmlUserInfo;
	if(NULL == XmlBody || NULL == pstXmlLoginRsp)
	{
//		logprintf(ERROR,"[XmlDecodeResponse_Login] input tree error\n");
		return FAIL;
	}
	memset(pstXmlLoginRsp,0,sizeof(ST_XML_LOGIN_RSP));
	pXmlUserInfo = ezxml_child(XmlBody, "userinfo") ;
	
	if(NULL != pXmlUserInfo)
	{
		strncpy(pstXmlLoginRsp->m_strUser, (char *)ezxml_attr(pXmlUserInfo,"user"), sizeof(pstXmlLoginRsp->m_strUser));
	}	
	return SUCCESS;
}


int XmlDecodeRequest_Invite(ezxml_t XmlBody,ST_XML_INVITE* pstXmlInvite)
{
	if(NULL == XmlBody || NULL == pstXmlInvite){
//		logprintf(ERROR,"[XmlDecodeRequest_Invite] input tree error\n");
		return FAIL;
	}
	memset(pstXmlInvite,0,sizeof(ST_XML_INVITE));

	XmlDecode_Call(XmlBody,&pstXmlInvite->m_stCall);
	XmlDecode_Media(XmlBody,&pstXmlInvite->m_stMediaSdp);

	return SUCCESS;
}

int XmlEncodeRequest_Invite(ST_XML_INVITE   *pstXmlInvite, char* pResultBuf,int iSize)
{
	char  *s=NULL;
	ezxml_t XmlRoot;
	char strMsgId[8]="";

	memset(pResultBuf,0,iSize);
	int len = XmlEn_AddPakcetHeader(pResultBuf,iSize);
	XmlRoot = ezxml_new_d("request");
	ezxml_set_attr_d(XmlRoot, "type", "invite");

	sprintf(strMsgId,"%d",XmlEncode_MakeReqId());
	ezxml_set_attr_d(XmlRoot, "msgid",strMsgId );

	XmlEncode_Call(XmlRoot,&pstXmlInvite->m_stCall);
	XmlEncode_Media(XmlRoot,&pstXmlInvite->m_stMediaSdp);
	
	ezxml_pretty(XmlRoot);
	s = ezxml_toxml(XmlRoot);
	XmlEn_BuildXmlStr(pResultBuf+len, iSize-len, s);
	ezxml_free(XmlRoot);
	XmlEn_AddPakcetTailer(pResultBuf,iSize);
	if(s != NULL)
	{
		free(s);
	}
	return SUCCESS;
}


int XmlDecodeRequest_Reinvite(ezxml_t XmlBody,ST_XML_REINVITE*   pstXmlReInvite){
	if(NULL == XmlBody || NULL == pstXmlReInvite)
	{
//		logprintf(ERROR,"[XmlDecodeRequest_Reinvite] input tree error\n");
		return FAIL;
	}
	memset(pstXmlReInvite,0,sizeof(ST_XML_REINVITE));

	XmlDecode_Call(XmlBody,&pstXmlReInvite->m_stCall);
	XmlDecode_Media(XmlBody,&pstXmlReInvite->m_stMediaSdp);
	
	return SUCCESS;
}

int XmlEncodeRequest_ReInvite(ST_XML_INVITE   *pstXmlInvite, char* pResultBuf, int iSize)
{

	char  *s=NULL;
	ezxml_t XmlRoot;
	char strMsgId[8]="";
	memset(pResultBuf,0,iSize);
	int len = XmlEn_AddPakcetHeader(pResultBuf,iSize);

	XmlRoot = ezxml_new_d("request");
	ezxml_set_attr_d(XmlRoot, "type", "reinvite");

	sprintf(strMsgId,"%d",XmlEncode_MakeReqId());
	ezxml_set_attr_d(XmlRoot, "msgid",strMsgId );

	XmlEncode_Call(XmlRoot,&pstXmlInvite->m_stCall);
	XmlEncode_Media(XmlRoot,&pstXmlInvite->m_stMediaSdp);
	
	ezxml_pretty(XmlRoot);
	s = ezxml_toxml(XmlRoot);
	XmlEn_BuildXmlStr(pResultBuf+len, iSize-len, s);
	ezxml_free(XmlRoot);
	XmlEn_AddPakcetTailer(pResultBuf,iSize);
	if(s != NULL)
	{
		free(s);
	}
	return SUCCESS;
}

int XmlDecodeRsponse_Ring(ezxml_t XmlBody,ST_XML_RING *pstXmlRing){
	if(NULL == XmlBody || NULL == pstXmlRing){
//		logprintf(ERROR,"[XmlDecodeRsponse_Ring] input tree error\n");
		return FAIL;
	}
	memset(pstXmlRing,0,sizeof(ST_XML_RING));
	
	XmlDecode_Call(XmlBody,&pstXmlRing->m_stCall);
	XmlDecode_Media(XmlBody,&pstXmlRing->m_stMediaSdp);

	return SUCCESS;
}

int XmlEncodeResponse_Ring(ST_XML_RING   *pstXmlRing, char* pResultBuf, int iSize)
{
	char  *s=NULL;
	ezxml_t XmlRoot;
	char strMsgId[8]="";
	memset(pResultBuf,0,iSize);
	int len = XmlEn_AddPakcetHeader(pResultBuf,iSize);
	XmlRoot = ezxml_new_d("response");
	ezxml_set_attr_d(XmlRoot, "type", "ring");
	sprintf(strMsgId,"%d",XmlEncode_MakeReqId());
	ezxml_set_attr_d(XmlRoot, "msgid",strMsgId );

	XmlEncode_Call(XmlRoot,&pstXmlRing->m_stCall);
	XmlEncode_Media(XmlRoot,&pstXmlRing->m_stMediaSdp);
	
	ezxml_pretty(XmlRoot);
	s = ezxml_toxml(XmlRoot);
	XmlEn_BuildXmlStr(pResultBuf+len, iSize-len, s);
	ezxml_free(XmlRoot);
	XmlEn_AddPakcetTailer(pResultBuf,iSize);
	if(s != NULL)
	{
		free(s);
	}
	return SUCCESS;
}

int XmlDecodeRsponse_Answer(ezxml_t XmlBody,ST_XML_ANSWER   *pstXmlAnswer){

	if(NULL == XmlBody || NULL == pstXmlAnswer)
	{
//		logprintf(ERROR,"[XmlDecodeRsponse_Answer] input tree error\n");
		return FAIL;
	}
	memset(pstXmlAnswer,0,sizeof(ST_XML_ANSWER));
	XmlDecode_Call(XmlBody,&pstXmlAnswer->m_stCall);
	XmlDecode_Media(XmlBody,&pstXmlAnswer->m_stMediaSdp);
    pstXmlAnswer->m_stCall.m_iIsVolteCall = 0;

	return SUCCESS;
}
int XmlEncodeResponse_Answer(ST_XML_ANSWER   *pstXmlAnswer, char* pResultBuf, int iSize){
	char  *s=NULL;
	ezxml_t XmlRoot;
	char strMsgId[8]="";
	memset(pResultBuf,0,iSize);
	int len = XmlEn_AddPakcetHeader(pResultBuf,iSize);

	XmlRoot = ezxml_new_d("response");
	ezxml_set_attr_d(XmlRoot, "type", "answer");

	sprintf(strMsgId,"%d",XmlEncode_MakeReqId());
	ezxml_set_attr_d(XmlRoot, "msgid",strMsgId );

	XmlEncode_Call(XmlRoot,&pstXmlAnswer->m_stCall);
	XmlEncode_Media(XmlRoot,&pstXmlAnswer->m_stMediaSdp);
	
	ezxml_pretty(XmlRoot);
	s = ezxml_toxml(XmlRoot);
	XmlEn_BuildXmlStr(pResultBuf+len, iSize-len, s);
	ezxml_free(XmlRoot);
	XmlEn_AddPakcetTailer(pResultBuf,iSize);
	if(s != NULL){
		free(s);
	}
	return SUCCESS;
}
int XmlEncodeRequest_Heartbeat(char* pResultBuf, int iSize)
{
	char  *s=NULL;
	ezxml_t XmlRoot;
	char strMsgId[8]="";
	memset(pResultBuf,0,iSize);
	int len = XmlEn_AddPakcetHeader(pResultBuf,iSize);

	XmlRoot = ezxml_new_d("request");
	ezxml_set_attr_d(XmlRoot, "type", "heartbeat");
	sprintf(strMsgId,"%d",XmlEncode_MakeReqId());
	ezxml_set_attr_d(XmlRoot, "msgid",strMsgId );
	
	ezxml_pretty(XmlRoot);
	s = ezxml_toxml(XmlRoot);
	XmlEn_BuildXmlStr(pResultBuf+len, iSize, s);
	ezxml_free(XmlRoot);
	XmlEn_AddPakcetTailer(pResultBuf,iSize);
	if(s != NULL){
		free(s);
	}
	return SUCCESS;
}
int XmlDecodeRequest_GetCallSession(ezxml_t XmlBody,ST_XML_GET_CALL_SESSION_REQ   *pstXmlGetCallSession){
	ezxml_t  pXml;
	if(NULL == XmlBody || NULL == pstXmlGetCallSession)
	{
//		logprintf(ERROR,"[XmlDecodeRequest_GetCallSession] input tree error\n");
		return FAIL;
	}
	memset(pstXmlGetCallSession,0,sizeof(ST_XML_GET_CALL_SESSION_REQ));

	XmlDecode_Call(XmlBody,&pstXmlGetCallSession->m_stCall);
	XmlDecode_Media(XmlBody,&pstXmlGetCallSession->m_stMediaSdp);

	pXml = ezxml_child(XmlBody, "status") ;
	if(NULL != pXml)
	{
		char strSts[32]="";
		
		strncpy(strSts, (char *)ezxml_attr(pXml,"sts"), sizeof(strSts));	
		if(0 ==strcmp(strSts,"invite"))
		{
			pstXmlGetCallSession->m_eCallStatus = E_XML_CALL_INVITE;
		}
		else if(0 ==strcmp(strSts,"ring"))
		{
			pstXmlGetCallSession->m_eCallStatus = E_XML_CALL_RING;
		}
		else if(0 ==strcmp(strSts,"talking"))
		{
			pstXmlGetCallSession->m_eCallStatus = E_XML_CALL_TALKING;
		}
		else if(0 ==strcmp(strSts,"hangup"))
		{
			pstXmlGetCallSession->m_eCallStatus = E_XML_CALL_HANGUP;
		}
		
	}
	
	return SUCCESS;
}



int XmlEncodeRequest_GetCallSession(ST_XML_GET_CALL_SESSION_REQ   *pstXmlCallSession, char* pResultBuf, int iSize)
{

	char  *s=NULL;
	ezxml_t XmlRoot;
	char strMsgId[8]="";

	memset(pResultBuf,0, iSize);
	int len = XmlEn_AddPakcetHeader(pResultBuf,iSize);
	XmlRoot = ezxml_new_d("request");
	ezxml_set_attr_d(XmlRoot, "type", "get_call_session");

	sprintf(strMsgId,"%d",XmlEncode_MakeReqId());
	ezxml_set_attr_d(XmlRoot, "msgid",strMsgId );

	XmlEncode_Call(XmlRoot,&pstXmlCallSession->m_stCall);

	XmlEncode_Media(XmlRoot,&pstXmlCallSession->m_stMediaSdp);

	ezxml_t pXmlStatus = ezxml_add_child(XmlRoot,"status",0);
	switch (pstXmlCallSession->m_eCallStatus)
	{
		case	E_XML_CALL_UNKNOW:
			break;
		case	E_XML_CALL_INVITE:
			ezxml_set_attr_d(pXmlStatus, "sts", "invite");
			break;
		case	E_XML_CALL_RING:
			ezxml_set_attr_d(pXmlStatus, "sts", "ring");
			break;
		case	E_XML_CALL_TALKING:
			ezxml_set_attr_d(pXmlStatus, "sts", "talking");
			break;
		case	E_XML_CALL_HANGUP:
			ezxml_set_attr_d(pXmlStatus, "sts", "hangup");
			break;

	}
	
	ezxml_pretty(XmlRoot);
	s = ezxml_toxml(XmlRoot);
	XmlEn_BuildXmlStr(pResultBuf+len, iSize-len, s);
	ezxml_free(XmlRoot);
	XmlEn_AddPakcetTailer(pResultBuf,iSize);
	if(s != NULL)
	{
		free(s);
	}
	return SUCCESS;
}
int XmlDecodeRsponse_GetCallSession(ezxml_t XmlBody,ST_XML_GET_CALL_SESSION_RSP   *pstXmlGetCallSessionRsp){
	ezxml_t  pXml;
	if(NULL == XmlBody || pstXmlGetCallSessionRsp)
	{
//		logprintf(ERROR,"[XmlDecodeRsponse_GetCallSession] input tree error\n");
		return FAIL;
	}
	memset(pstXmlGetCallSessionRsp,0,sizeof(ST_XML_GET_CALL_SESSION_RSP));
	XmlDecode_Call(XmlBody,&pstXmlGetCallSessionRsp->m_stCall);
	XmlDecode_Media(XmlBody,&pstXmlGetCallSessionRsp->m_stMediaSdp);

	pXml = ezxml_child(XmlBody, "status") ;
	if(NULL != pXml)
	{
		char strSts[32]="";
		strncpy(strSts, (char *)ezxml_attr(pXml,"sts"), sizeof(strSts));	
		if(0 ==strcmp(strSts,"invite"))
		{
			pstXmlGetCallSessionRsp->m_eCallStatus = E_XML_CALL_INVITE;
		}
		else if(0 ==strcmp(strSts,"ring"))
		{
			pstXmlGetCallSessionRsp->m_eCallStatus = E_XML_CALL_RING;
		}
		else if(0 ==strcmp(strSts,"talking"))
		{
			pstXmlGetCallSessionRsp->m_eCallStatus = E_XML_CALL_TALKING;
		}
		else if(0 ==strcmp(strSts,"hangup"))
		{
			pstXmlGetCallSessionRsp->m_eCallStatus = E_XML_CALL_HANGUP;
		}
		
	}
	
	ezxml_t ResultXml = ezxml_child(XmlBody, "return") ;
	if(NULL != ResultXml)
	{
		char strResult[32]="";
		
		strncpy(strResult, (char *)ezxml_attr(ResultXml,"result"), sizeof(strResult));
		if(0 ==strcmp(strResult,"success"))
		{
			pstXmlGetCallSessionRsp->m_stResult.m_eResultCode = E_XML_SUCCESS;
		}
		else
		{
			pstXmlGetCallSessionRsp->m_stResult.m_eResultCode = E_XML_FAIL;
		}
		
		strncpy(pstXmlGetCallSessionRsp->m_stResult.m_strReason, (char *)ezxml_attr(ResultXml,"reason")
			, sizeof(pstXmlGetCallSessionRsp->m_stResult.m_strReason));
	
	}	
	return SUCCESS;
}


int XmlEncodeRsponse_GetCallSession(int iMsgId,ST_XML_GET_CALL_SESSION_RSP   *pstXmlCallSession, char* pResultBuf, int iSize){
	char  *s=NULL;
	ezxml_t XmlRoot;
	char strMsgId[8]="";

	memset(pResultBuf,0,iSize);
	int len = XmlEn_AddPakcetHeader(pResultBuf,iSize);

	XmlRoot = ezxml_new_d("response");
	ezxml_set_attr_d(XmlRoot, "type", "get_call_session");

	sprintf(strMsgId,"%d",iMsgId);
	ezxml_set_attr_d(XmlRoot, "msgid",strMsgId );

	XmlEncode_Call(XmlRoot,&pstXmlCallSession->m_stCall);
	XmlEncode_Media(XmlRoot,&pstXmlCallSession->m_stMediaSdp);

	ezxml_t pXmlStatus = ezxml_add_child(XmlRoot,"status",0);
	switch (pstXmlCallSession->m_eCallStatus)
	{
		case	E_XML_CALL_UNKNOW:
			break;
		case	E_XML_CALL_INVITE:
			ezxml_set_attr_d(pXmlStatus, "sts", "invite");
			break;
		case	E_XML_CALL_RING:
			ezxml_set_attr_d(pXmlStatus, "sts", "ring");
			break;
		case	E_XML_CALL_TALKING:
			ezxml_set_attr_d(pXmlStatus, "sts", "talking");
			break;
		case	E_XML_CALL_HANGUP:
			ezxml_set_attr_d(pXmlStatus, "sts", "hangup");
			break;
	}

	XmlEncode_Result(XmlRoot,&pstXmlCallSession->m_stResult);
	
	ezxml_pretty(XmlRoot);
	s = ezxml_toxml(XmlRoot);
	XmlEn_BuildXmlStr(pResultBuf+len, iSize-len, s);
	ezxml_free(XmlRoot);
	XmlEn_AddPakcetTailer(pResultBuf,iSize);
	if(s != NULL){
		free(s);
	}
	return SUCCESS;
}

int XmlDecodeRequest_Hangup(ezxml_t XmlBody,ST_XML_HANGUP_RSP   *pstXmlHangup){
	if(NULL == XmlBody || NULL == pstXmlHangup){
//		logprintf(ERROR,"[XmlDecodeRequest_Hangup] input tree error\n");
		return FAIL;
	}
	memset(pstXmlHangup,0,sizeof(ST_XML_HANGUP_RSP));

	XmlDecode_Call(XmlBody,&pstXmlHangup->m_stCall);

	ezxml_t ResultXml = ezxml_child(XmlBody, "return");
	if(NULL != ResultXml){
		char strResult[32]="";
		strncpy(strResult, (char *)ezxml_attr(ResultXml,"result"), sizeof(strResult));
		if(0 ==strcmp(strResult,"success")){
			pstXmlHangup->m_stResult.m_eResultCode = E_XML_SUCCESS;
		}else{
			pstXmlHangup->m_stResult.m_eResultCode = E_XML_FAIL;
		}
		char* reason = (char *)ezxml_attr(ResultXml,"reason");
		if(NULL != reason){
		  strncpy(pstXmlHangup->m_stResult.m_strReason,reason 
			    , sizeof(pstXmlHangup->m_stResult.m_strReason));
		}
	}	
	return SUCCESS;
}

int XmlEncodeRequest_Hangup(ST_XML_HANGUP_RSP   *pstXmlHangup, char* pResultBuf, int iSize){
	if(NULL == pResultBuf || iSize <= 0){
		return -1;
	}
	char  *s=NULL;
	ezxml_t XmlRoot;
	char strMsgId[8]="";
	memset(pResultBuf,0,iSize);
	int len = XmlEn_AddPakcetHeader(pResultBuf,iSize);

	XmlRoot = ezxml_new_d("request");
	ezxml_set_attr_d(XmlRoot, "type", "hangup");

	sprintf(strMsgId,"%d",XmlEncode_MakeReqId());
	ezxml_set_attr_d(XmlRoot, "msgid",strMsgId );

	XmlEncode_Call(XmlRoot,&pstXmlHangup->m_stCall);

	XmlEncode_Result(XmlRoot,&pstXmlHangup->m_stResult);

	ezxml_pretty(XmlRoot);
	s = ezxml_toxml(XmlRoot);
	XmlEn_BuildXmlStr(pResultBuf+len, iSize-len, s);
	ezxml_free(XmlRoot);
	XmlEn_AddPakcetTailer(pResultBuf,iSize);
	if(s != NULL){
		free(s);
	}
	return SUCCESS;
}

int XmlEncodeRequest_Message(ST_XML_MESSAGE   *pstXmlMessage, char* pResultBuf, int iSize){
	if(NULL == pResultBuf || iSize <= 0){
        return -1;
    }
	char  *s=NULL;
	ezxml_t XmlRoot;
	char strMsgId[8]="";
	memset(pResultBuf,0,iSize);

	XmlRoot = ezxml_new_d("request");
	ezxml_set_attr_d(XmlRoot, "type", "message");

	sprintf(strMsgId,"%d",XmlEncode_MakeReqId());
	ezxml_set_attr_d(XmlRoot, "msgid",strMsgId );

	XmlEncode_Call(XmlRoot,&pstXmlMessage->m_stCall);
	ezxml_t pXmlReturn = ezxml_add_child(XmlRoot,"content",0);	

	ezxml_set_attr_d(pXmlReturn, "msg", pstXmlMessage->m_strContent);
	ezxml_pretty(XmlRoot);
	s = ezxml_toxml(XmlRoot);
	XmlEn_BuildXmlStr(pResultBuf, iSize, s);
	ezxml_free(XmlRoot);
	if(s != NULL){
		free(s);
	}
	return SUCCESS;
}
int XmlDecodeRequest_Message(ezxml_t XmlBody, ST_XML_MESSAGE *pstXmlMessage){
    if(NULL == XmlBody){
        //		logprintf(ERROR,"[XmlDecodeRequest_Message] input tree error\n");
        return FAIL;
    }
    if(NULL == pstXmlMessage){
        return FAIL;
    }
    memset(pstXmlMessage,0,sizeof(ST_XML_MESSAGE));
    XmlDecode_Call(XmlBody,&pstXmlMessage->m_stCall);
    ezxml_t ResultXml = ezxml_child(XmlBody, "content") ;
    if(NULL != ResultXml){
        memcpy(pstXmlMessage->m_strContent, (char *)ezxml_attr(ResultXml,"msg"), MAX_MESSAGE_BUF_LEN);
    }
    return SUCCESS;
}
int Xml_DecodeParseCmdType(int id, const char *name){
	int len = strlen(decode_methods[id].text);
	int l_name = name ? strlen(name) : 0;
	
	return (l_name >= len &&
		strstr(name,decode_methods[id].text));
}
int XmlDecodeParseType(const char *msg){
	int i, res = 0;
	if (strlen(msg)<= 0){
		return -1;
	}
	for (i = (int)(sizeof(decode_methods) / sizeof(decode_methods[0]))-1;i >= 0 && !res; i--){
		if (Xml_DecodeParseCmdType(i, msg)){		
			res = decode_methods[i].id;
		}
	}
	return res;
}

