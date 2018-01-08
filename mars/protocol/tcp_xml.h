
#ifndef __TCP_XML_H_
#define __TCP_XML_H_

#if defined(WEBRTC_LINUX)
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#endif

#include <stddef.h>
#include "ezxml.h"

#define SUCCESS                         0
#define FAIL                            -1
#define ERROR							1
#define FLOW							2
              

#define	MAX_CODE_STR_LEN				8
#define	MAX_CALLID_STR_LEN				128
#define	MAX_IP_STR_LEN					32
#define	MAX_TEL_STR_LEN					128
#define	MAX_RTP_MAP_ATTR_STR_LEN		512
#define	MAX_RTP_MAP_NUM					16
#define	MAX_PASSWORD_LEN				128
#define	MAX_REASON_LEN					64
#define	MAX_RTP_MEDIA_CODE_LEN          64
#define MAX_MESSAGE_BUF_LEN             512

#define MAX_TCP_BUF_LEN 10240
#define D_SIP_MAX_IP_LEN 32

typedef struct __IP_ADDR_ST2{
    char m_strIp[D_SIP_MAX_IP_LEN];
	int m_iPort;
}ST_IP_ADDR2;

/* obtain a count of items in the hash */
#define HASH_COUNT(head) HASH_CNT(hh,head) 
#define HASH_CNT(hh,head) (head?(head->hh.tbl->num_items):0)

#define  ALOG(...)  __android_log_print(ANDROID_LOG_INFO,"xml:  ",__VA_ARGS__)

typedef struct UT_hash_bucket2 {
    struct UT_hash_handle *hh_head;   
    unsigned count;   
/* expand_mult is normally set to 0. In this situation, the max chain length    
* threshold is enforced at its default value, HASH_BKT_CAPACITY_THRESH. (If    
* the bucket's chain exceeds this length, bucket expansion is triggered).    
* However, setting expand_mult to a non-zero value delays bucket expansion    
* (that would be triggered by additions to this particular bucket)    
* until its chain length reaches a *multiple* of HASH_BKT_CAPACITY_THRESH.    
* (The multiplier is simply expand_mult+1). The whole idea of this    
* multiplier is to reduce bucket expansions, since they are expensive, in    
* situations where we know that a particular bucket tends to be overused.    
* It is better to let its chain length grow to a longer yet-still-bounded    
* value, than to do an O(n) bucket expansion too often.     */  
    unsigned expand_mult;
} UT_hash_bucket2;

typedef struct UT_hash_table2 {
	UT_hash_bucket2 *buckets;
	unsigned num_buckets, log2_num_buckets;   
	unsigned num_items;   
	struct UT_hash_handle *tail; 
	/* tail hh in app order, for fast append    */  
	ptrdiff_t hho; 
	/* hash handle offset (byte pos of hash handle in element */   
	/* in an ideal situation (all buckets used equally), no bucket would have    
	* more than ceil(#items/#buckets) items. that's the ideal chain length. */   
	unsigned ideal_chain_maxlen;   
	/* nonideal_items is the number of items in the hash whose chain position    
	* exceeds the ideal chain maxlen. these items pay the penalty for an uneven    
	* hash distribution; reaching them in a chain traversal takes >ideal steps */   
	unsigned nonideal_items;   
	/* ineffective expands occur when a bucket doubling was performed, but     
	* afterward, more than half the items in the hash had nonideal chain    
	* positions. If this happens on two consecutive expansions we inhibit any    
	* further expansion, as it's not helping; this happens when the hash    
	* function isn't a good fit for the key domain. When expansion is inhibited    
	* the hash will still work, albeit no longer in constant time. */   
	unsigned ineff_expands, noexpand;
} UT_hash_table2;

typedef struct UT_hash_handle2 {
	struct UT_hash_table *tbl;   
	void *prev;                       
	/* prev element in app order      */   
	void *next;                       
	/* next element in app order      */   
	struct UT_hash_handle *hh_prev;   
	/* previous hh in bucket order    */   
	struct UT_hash_handle *hh_next;   
	/* next hh in bucket order        */   
	void *key;                        
	/* ptr to enclosing struct's key  */   
	unsigned keylen;                  
	/* enclosing struct's key len     */   
	unsigned hashv;                   
	/* result of hash-fcn(key)        */
} UT_hash_handle2;

typedef struct __TCP_RECV_FIFO_ST{
	char 	strBuf[MAX_TCP_BUF_LEN];
	char 	*pRead;
	char	*pWrite;
}ST_RECV_TCP_FIFO;

typedef enum __TCP_CONNECT_STS_E{
	E_TCP_INIT,
	E_TCP_LOGIN_SUCCESS,
}E_TCP_CONNECT_STS;

typedef struct __TCP_SOCKET_ST{
	int					iSocketId;
	long				m_iConectTime;
	int					m_iWaitLoginTime;
	E_TCP_CONNECT_STS	m_eConnectSts;
	struct ev_io 		*w_client;
	ST_IP_ADDR2 			m_stSipAddr;
	ST_RECV_TCP_FIFO	m_stTcpRecvBuf;
	UT_hash_handle2 		hh;
}ST_TCP_SOCKET;

typedef enum __RESULT_E{
	E_XML_FAIL = -1,
	E_XML_SUCCESS,
    E_XML_PROCESSING,
    E_XML_WAITINGTIMEOUT,
}E_XML_RESULT;

typedef enum __XML_CALL_STATUS_E{
	E_XML_CALL_UNKNOW,
	E_XML_CALL_INVITE,
	E_XML_CALL_RING,
	E_XML_CALL_TALKING,
	E_XML_CALL_HANGUP,
}E_XML_CALL_STATUS;

typedef enum __E_XML_MSG_TYPE{
	E_MSG_UNKNOW,
	E_MSG_REQUEST,
	E_MSG_RESPONSE,
}E_XML_MSG_TYPE;


typedef enum XML_CMD_TYPE {
	//����
	XML_CMD_UNKNOW,
	XML_CMD_LOGIN,
	XML_CMD_INVITE,
	XML_CMD_RING,
	XML_CMD_ANSWER,
	XML_CMD_HANGUP,
    XML_CMD_GET_CALL_SESSION,
	XML_CMD_REINVITE,
	XML_CMD_TCP_HEARTBEAT,
	XML_CMD_MESSAGE,
    XML_CMD_HANGUPACK,
    XML_CMD_LOGOUT,
}E_XML_CMD_TYPE;


static const struct  _decode_methods { 
	enum XML_CMD_TYPE id;
	const char *  text;
}decode_methods[] =
{
	{ XML_CMD_UNKNOW,"-UNKNOWN-" },
	{ XML_CMD_LOGIN,"login" },
	{ XML_CMD_INVITE,"invite" },
 	{ XML_CMD_RING,"ring" },
	{ XML_CMD_ANSWER, "answer" },
	{ XML_CMD_HANGUP,"hangup"},
	{ XML_CMD_GET_CALL_SESSION,"get_call_session"},
	{ XML_CMD_REINVITE,"reinvite"},
	{ XML_CMD_TCP_HEARTBEAT,"heartbeat"},	
	{ XML_CMD_MESSAGE,"message"},
    { XML_CMD_HANGUPACK,"hangupAck"},
	{ XML_CMD_LOGOUT,"logout"}
};

typedef struct __XML_RESULT_ST{
		E_XML_RESULT	m_eResultCode;
		char			m_strReason[MAX_REASON_LEN];
}ST_XML_RESULT;

typedef struct __XML_CALL_INFO_ST{
	char	m_strFromTel[MAX_TEL_STR_LEN];   //���к���
	char	m_strDisplayTel[MAX_TEL_STR_LEN]; //����������ʾ����

	char	m_strToTel[MAX_TEL_STR_LEN];

	char	m_strCallId[MAX_CALLID_STR_LEN];
	
	int     m_iIsfocus;
    int     m_iIsVolteCall;
}ST_XML_CALL_INFO;

typedef struct __XML_RTP_MAP_ST{
	int		m_iUse;
	char	m_strName[MAX_RTP_MAP_ATTR_STR_LEN];
	char	m_strAttr[MAX_RTP_MAP_ATTR_STR_LEN];
    int     m_ifmtpUse;
    char    m_strfmtpName[MAX_RTP_MAP_ATTR_STR_LEN];
    char    m_strfmtpAttr[MAX_RTP_MAP_ATTR_STR_LEN];
	
}ST_XML_RTP_MAP;

typedef struct __XML_MEDIA_INFO_ST{
	int				m_iUse;
	char			m_strIp[MAX_IP_STR_LEN];
	int				m_iPort;

	char			m_MediaCode[MAX_RTP_MEDIA_CODE_LEN];
	ST_XML_RTP_MAP	m_stRtpMap[MAX_RTP_MAP_NUM];

}ST_XML_MEDIA_INFO;

typedef struct __XML_MEDIA_SDP_ST{
	int						m_iUse;		// 0��ʾ��sdpý����Ϣ
	ST_XML_MEDIA_INFO		m_stAudio;
	ST_XML_MEDIA_INFO		m_stVideo;

}ST_XML_MEDIA_SDP;

//��¼����
typedef struct __XML_LOGIN_REQ_ST{
	char		m_strUser[MAX_TEL_STR_LEN];
	char		m_strPassword[MAX_PASSWORD_LEN];
}ST_XML_LOGIN_REQ;

//��¼��Ӧ
typedef struct __XML_LOGIN_RSP_ST{
	char				m_strUser[MAX_TEL_STR_LEN];
	char				m_strPassword[MAX_PASSWORD_LEN];
    int                 m_iLoginRspBySip;
	ST_XML_RESULT		m_stResult;
}ST_XML_LOGIN_RSP;

// INVITE 
typedef struct __XML_INVITE_ST{
	ST_XML_CALL_INFO		m_stCall;

	ST_XML_MEDIA_SDP		m_stMediaSdp;
	
}ST_XML_INVITE;

//RING
typedef struct __XML_RING_ST{
	ST_XML_CALL_INFO		m_stCall;

	ST_XML_MEDIA_SDP		m_stMediaSdp;  // sdp use Ϊ1������183����
}ST_XML_RING;

//ANSWER
typedef struct __XML_ANSWER_ST{
	ST_XML_CALL_INFO		m_stCall;

	ST_XML_MEDIA_SDP		m_stMediaSdp;
}ST_XML_ANSWER;

//reinvite �л�
typedef struct __XML_REINVITE_ST{
	ST_XML_CALL_INFO		m_stCall;

	ST_XML_MEDIA_SDP		m_stMediaSdp;
}ST_XML_REINVITE;

typedef struct __XML_UPDATE_ST{
    ST_XML_CALL_INFO		m_stCall;
    
    ST_XML_MEDIA_SDP		m_stMediaSdp;
}ST_XML_UPDATE;

//��ѯ�Ự��Ϣ����
typedef struct __XML_GET_CALL_SESSION_REQ_ST{
	ST_XML_CALL_INFO		m_stCall;

	ST_XML_MEDIA_SDP		m_stMediaSdp;

	E_XML_CALL_STATUS		m_eCallStatus;
	
}ST_XML_GET_CALL_SESSION_REQ;


//��ѯ�Ự��Ϣ��Ӧ
typedef struct __XML_GET_CALL_SESSION_RSP_ST{
	ST_XML_CALL_INFO		m_stCall;

	ST_XML_MEDIA_SDP		m_stMediaSdp;

	
	E_XML_CALL_STATUS		m_eCallStatus;
	ST_XML_RESULT		m_stResult;
}ST_XML_GET_CALL_SESSION_RSP;

//�һ�
typedef struct __XML_HANGUP_ST{
	ST_XML_CALL_INFO		m_stCall;	
	ST_XML_RESULT		m_stResult;
}ST_XML_HANGUP_RSP;


//��Ϣ
typedef struct __XML_MESSAGE_ST{
	ST_XML_CALL_INFO		m_stCall;	
	char					m_strContent[MAX_MESSAGE_BUF_LEN];

}ST_XML_MESSAGE;

#ifdef __cplusplus
extern "C" {
#endif
int XmlEncodeRequst_Login(ST_XML_LOGIN_REQ* pstXmlLoginReq, char* pcharResultBuf, int iSize, int bFirstTime,char* deviceType);
int XmlEncodeRequst_Login2(ST_XML_LOGIN_REQ* pstXmlLoginReq, char* pcharResultBuf, int iSize, int bFirstTime, int iTimeSec,char* deviceType);
int XmlEncodeRsponse_Login(ST_XML_LOGIN_RSP* pstXmlLoginRsp, char* pcharResultBuf, int iSize);

int XmlEncodeRequest_Invite(ST_XML_INVITE* pstXmlInvite, char* pcharResultBuf, int iSize);
int XmlEncodeRequest_ReInvite(ST_XML_INVITE* pstXmlInvite,char* pcharResultBuf, int iSize);

int XmlEncodeResponse_Ring(ST_XML_RING* pstXmlRing, char* pcharResultBuf, int iSize);

int XmlEncodeResponse_Answer(ST_XML_ANSWER* pstXmlAnswer, char* pcharResultBuf, int iSize);
int XmlEncodeRequest_Hangup(ST_XML_HANGUP_RSP* pstXmlHangup, char* pcharResultBuf, int iSize);

int XmlEncodeRequest_GetCallSession(ST_XML_GET_CALL_SESSION_REQ* pstXmlCallSession, char* pcharResultBuf, int iSize);
int XmlEncodeRsponse_GetCallSession(int iMsgId,ST_XML_GET_CALL_SESSION_RSP* pstXmlCallSession, char* pcharResultBuf, int iSize);

int XmlEncodeRequest_Heartbeat(char* pcharResultBuf, int iSize);
int XmlEncodeRequest_Message(ST_XML_MESSAGE *pstXmlMessage, char* pResultBuf, int iSize);

int XmlDecodeParseType(const char *msg);

//int XmlDecodeRequest_Login(ezxml_t XmlBody,ST_XML_LOGIN_REQ* pstXmlLoginReq);
int XmlDecodeResponse_Login(ezxml_t XmlBody,ST_XML_LOGIN_RSP *pstXmlLoginRsp);
int XmlDecodeResponse_Logout(ezxml_t XmlBody,ST_XML_LOGIN_RSP *pstXmlLoginRsp);

int XmlDecodeRequest_Invite(ezxml_t XmlBody,ST_XML_INVITE* pstXmlInvite);
int XmlDecodeRequest_Reinvite(ezxml_t XmlBody,ST_XML_REINVITE*   pstXmlReInvite);
int XmlDecodeRsponse_Ring(ezxml_t XmlBody,ST_XML_RING *pstXmlRing);
int XmlDecodeRsponse_Answer(ezxml_t XmlBody,ST_XML_ANSWER   *pstXmlAnswer);
int XmlDecodeRequest_GetCallSession(ezxml_t XmlBody,ST_XML_GET_CALL_SESSION_REQ   *pstXmlGetCallSession);
int XmlDecodeRequest_Hangup(ezxml_t XmlBody,ST_XML_HANGUP_RSP   *pstXmlHangup);
int XmlDecodeRequest_Message(ezxml_t XmlBody, ST_XML_MESSAGE *pstXmlMessage);


int XmlEncodeRequst_noop( char* pcharResultBuf, int iSize );



#ifdef __cplusplus
}
#endif

#endif 
