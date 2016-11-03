/*
 *  Author : WangBoJing , email : 1989wangbojing@gmail.com
 * 
 *  Copyright Statement:
 *  --------------------
 *  This software is protected by Copyright and the information contained
 *  herein is confidential. The software may not be copied and the information
 *  contained herein may not be used or disclosed except with the written
 *  permission of Author. (C) 2016
 * 
 *
 
****       *****
  ***        *
  ***        *                         *               *
  * **       *                         *               *
  * **       *                         *               *
  *  **      *                        **              **
  *  **      *                       ***             ***
  *   **     *       ******       ***********     ***********    *****    *****
  *   **     *     **     **          **              **           **      **
  *    **    *    **       **         **              **           **      *
  *    **    *    **       **         **              **            *      *
  *     **   *    **       **         **              **            **     *
  *     **   *            ***         **              **             *    *
  *      **  *       ***** **         **              **             **   *
  *      **  *     ***     **         **              **             **   *
  *       ** *    **       **         **              **              *  *
  *       ** *   **        **         **              **              ** *
  *        ***   **        **         **              **               * *
  *        ***   **        **         **     *        **     *         **
  *         **   **        **  *      **     *        **     *         **
  *         **    **     ****  *       **   *          **   *          *
*****        *     ******   ***         ****            ****           *
                                                                       *
                                                                      *
                                                                  *****
                                                                  ****


 *
 */

#include <pthread.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> 
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "NattyProtoClient.h"
#include "NattyTimer.h"
#include "NattyUtils.h"
#include "NattyNetwork.h"


static void ntySetupHeartBeatThread(void* self);
static void ntySetupRecvProcThread(void *self);
static void ntySendLogin(void *self);
static void ntySendLogout(void *self);
static void ntySendTimeout(int len);
static void* ntyRecvProc(void *arg);


#if 1 //

typedef void* (*RECV_CALLBACK)(void *arg);

typedef enum {
	STATUS_NETWORK_LOGIN,
	STATUS_NETWORK_PROXYDATA,
	STATUS_NETWORK_LOGOUT,
} StatusNetwork;


typedef struct _NATTYPROTOCOL {
	const void *_;
	C_DEVID devid; //App Or Device Id
	C_DEVID fromId;
	U8 level;
	U8 recvBuffer[RECV_BUFFER_SIZE];
	U16 recvLen;
	PROXY_CALLBACK onProxyCallback; //just for java
	RECV_CALLBACK onRecvCallback; //recv
	PROXY_CALLBACK onProxyFailed; //send data failed
	PROXY_CALLBACK onProxySuccess; //send data success
	PROXY_CALLBACK onProxyDisconnect;
	PROXY_CALLBACK onProxyReconnect;
	PROXY_CALLBACK onBindResult;
	PROXY_CALLBACK onUnBindResult;
	PROXY_CALLBACK onPacketRecv;
	PROXY_CALLBACK onPacketSuccess;
	U8 heartbeartRun;
	U8 p2pHeartbeatRun;
	U8 u8RecvExitFlag;
	U8 u8HeartbeatExistFlag;
	pthread_t heartbeatThread_id;
	pthread_t recvThread_id;
	struct sockaddr_in serveraddr;
} NattyProto;

typedef struct _NATTYPROTO_OPERA {
	size_t size;
	void* (*ctor)(void *_self, va_list *params);
	void* (*dtor)(void *_self);
	void* (*heartbeat)(void *_self); //start 
	void (*login)(void *_self); //argument is optional
	void (*logout)(void *_self); //argument is optional
	void (*proxyReq)(void *_self, C_DEVID toId, U8 *buf, int length);
	void (*proxyAck)(void *_self, C_DEVID friId, U32 ack);
	void (*bind)(void *_self, C_DEVID did);
	void (*unbind)(void *_self, C_DEVID did);
#if (NEY_PROTO_VERSION > 'B')
	int* (*p2pconnectReq)(void *_self, void* fTree, C_DEVID id); //for p2p
	int* (*p2pconnectAck)(void *_self, void* fTree, C_DEVID id); //for p2p
	void (*p2pdataReq)(void *_self, C_DEVID toId, U8 *buf, int length);
	void (*p2pdataAck)(void *_self, C_DEVID toId, U8 *buf, int length);
	void (*p2pheartbeatReq)(void* _self);
	void (*p2pheartbeatAck)(void *_self, C_DEVID toId, U8 *buf, int length);
#endif
} NattyProtoOpera;



#endif




void* ntyProtoClientCtor(void *_self, va_list *params) {
	NattyProto *proto = _self;
	struct hostent *server = NULL;

	proto->onRecvCallback = ntyRecvProc;
	proto->level = STATUS_NETWORK_LOGIN;
	proto->p2pHeartbeatRun = 0;
	proto->heartbeartRun = 0;
	proto->recvLen = 0;
	proto->devid = 0;

	proto->heartbeatThread_id = 0;
	proto->recvThread_id = 0;

	proto->u8HeartbeatExistFlag = 0;
	proto->u8RecvExitFlag = 0;

#if 1
	proto->onProxyCallback = NULL;
	proto->onProxyFailed = NULL;
	proto->onProxySuccess = NULL;
	proto->onProxyDisconnect = NULL;
	proto->onProxyReconnect = NULL;
	proto->onUnBindResult = NULL;
	proto->onBindResult = NULL;
	proto->onPacketRecv = NULL;
	proto->onPacketSuccess = NULL;
#endif

	init_timer(CURRENT_TIMER_NUM);

#if 1 //server addr init
#if 0 //android JNI don't support gethostbyname
	server = gethostbyname(SERVER_NAME);    
	if (server == NULL) {        
		ntylog("ERROR, no such host as %s\n", SERVER_NAME);  
		//exit(0);   
		bzero((char *) &proto->serveraddr, sizeof(proto->serveraddr)); 
		return proto;
	}
	bzero((char *) &proto->serveraddr, sizeof(proto->serveraddr));    
	proto->serveraddr.sin_family = AF_INET;    
	bcopy((char *)server->h_addr, (char *)&proto->serveraddr.sin_addr.s_addr, server->h_length);    
	proto->serveraddr.sin_port = htons(SERVER_PORT);
#else
	bzero((char *) &proto->serveraddr, sizeof(proto->serveraddr));    
	proto->serveraddr.sin_family = AF_INET;    
	proto->serveraddr.sin_addr.s_addr = inet_addr(SERVER_NAME);
	proto->serveraddr.sin_port = htons(SERVER_PORT);
#endif	
	

	ntyGenCrcTable();

#if 0 //set network callback
	void *pNetwork = ntyNetworkInstance();
	((Network*)pNetwork)->onDataLost = ntySendTimeout;
#endif

#endif

	return proto;
}

void* ntyProtoClientDtor(void *_self) {
	NattyProto *proto = _self;

	proto->onRecvCallback = NULL;
	proto->level = STATUS_NETWORK_LOGOUT;
	proto->p2pHeartbeatRun = 0;
	proto->heartbeartRun = 0;
	proto->recvLen = 0;

#if 1 //should send logout packet to server
#endif

	return proto;
}

/*
 * heartbeat Packet
 * VERSION					1			BYTE
 * MESSAGE TYPE				1			BYTE (req, ack)
 * TYPE					1			BYTE 
 * DEVID					8			BYTE
 * ACKNUM					4			BYTE (Network Module Set Value)
 * CRC 					4			BYTE (Network Module Set Value)
 * 
 * send to server addr
 */


int ntyHeartBeatCb(timer_id id, void *arg, int len) {
	NattyProto *proto = arg;
	U8 buf[NTY_LOGIN_ACK_LENGTH] = {0};	
	int ret = -1;

	void *pNetwork = ntyGetNetworkInstance();
	if (pNetwork == NULL) {	
		return -1;
	}
	bzero(buf, NTY_LOGIN_ACK_LENGTH);
	
	if (proto->devid == 0) return -2; //set devid

	buf[NEY_PROTO_VERSION_IDX] = NEY_PROTO_VERSION;	
	buf[NTY_PROTO_MESSAGE_TYPE] = (U8) MSG_REQ;	
	buf[NTY_PROTO_TYPE_IDX] = NTY_PROTO_HEARTBEAT_REQ;	
#if 0
	*(C_DEVID*)(&buf[NTY_PROTO_DEVID_IDX]) = proto->devid;
#else
	memcpy(&buf[NTY_PROTO_DEVID_IDX], &proto->devid, sizeof(C_DEVID));
#endif
	len = NTY_PROTO_LOGIN_REQ_CRC_IDX+sizeof(U32);
	
	ret = ntySendFrame(pNetwork, &proto->serveraddr, buf, len);

	return ret;
}


void* ntyProtoClientHeartBeat(void *_self) {
	NattyProto *proto = _self;

	int len, n;	
	U8 buf[NTY_LOGIN_ACK_LENGTH] = {0};	

	ntydbg(" heartbeatThread running\n");
	if (proto->heartbeartRun == 1) {		
		proto->heartbeartRun = 1;		
		return NULL;	
	}	
	proto->heartbeartRun = 1;
#if 0	
	while (1) {		
		Network *pNetwork = ntyGetNetworkInstance();
		if (pNetwork == NULL) {
			sleep(HEARTBEAT_TIMEOUT);	
			continue;
		}
		if (pNetwork->sockfd == -1) {
			sleep(HEARTBEAT_TIMEOUT);	
			continue;
		}
		bzero(buf, NTY_LOGIN_ACK_LENGTH);
		sleep(HEARTBEAT_TIMEOUT);	
		
		if (proto->devid == 0) continue; //set devid
		
		buf[NEY_PROTO_VERSION_IDX] = NEY_PROTO_VERSION;	
		buf[NTY_PROTO_MESSAGE_TYPE] = (U8) MSG_REQ;	
		buf[NTY_PROTO_TYPE_IDX] = NTY_PROTO_HEARTBEAT_REQ;	
#if 0
		*(C_DEVID*)(&buf[NTY_PROTO_DEVID_IDX]) = proto->devid;
#else
		memcpy(&buf[NTY_PROTO_DEVID_IDX], &proto->devid, sizeof(C_DEVID));
#endif
		len = NTY_PROTO_LOGIN_REQ_CRC_IDX+sizeof(U32);
		
		n = ntySendFrame(pNetwork, &proto->serveraddr, buf, len);
	}
#else
	add_timer(HEARTBEAT_TIMEOUT, ntyHeartBeatCb, proto, sizeof(NattyProto));
#endif
}

/*
 * Login Packet
 * VERSION					1			BYTE
 * MESSAGE TYPE				1			BYTE (req, ack)
 * TYPE					1			BYTE 
 * DEVID					8			BYTE
 * ACKNUM					4			BYTE (Network Module Set Value)
 * CRC 					4			BYTE (Network Module Set Value)
 * 
 * send to server addr
 */
void ntyProtoClientLogin(void *_self) {
	NattyProto *proto = _self;
	int len, n;	
	U8 buf[RECV_BUFFER_SIZE] = {0};	

	buf[NEY_PROTO_VERSION_IDX] = NEY_PROTO_VERSION;	
	buf[NTY_PROTO_MESSAGE_TYPE] = (U8) MSG_REQ;	
	buf[NTY_PROTO_TYPE_IDX] = NTY_PROTO_LOGIN_REQ;
#if 0
	*(C_DEVID*)(&buf[NTY_PROTO_LOGIN_REQ_DEVID_IDX]) = proto->devid;	
#else
	memcpy(&buf[NTY_PROTO_LOGIN_REQ_DEVID_IDX], &proto->devid, sizeof(C_DEVID));
#endif
	len = NTY_PROTO_LOGIN_REQ_CRC_IDX+sizeof(U32);				

	ntydbg(" ntyProtoClientLogin %d\n", __LINE__);
	void *pNetwork = ntyNetworkInstance();
	n = ntySendFrame(pNetwork, &proto->serveraddr, buf, len);
}

void ntyProtoClientBind(void *_self, C_DEVID did) {
	NattyProto *proto = _self;
	int len, n;	

	U8 buf[NORMAL_BUFFER_SIZE] = {0};	

	buf[NEY_PROTO_VERSION_IDX] = NEY_PROTO_VERSION;	
	buf[NTY_PROTO_MESSAGE_TYPE] = (U8) MSG_REQ;	
	buf[NTY_PROTO_TYPE_IDX] = NTY_PROTO_BIND_REQ;
#if 0
	*(C_DEVID*)(&buf[NTY_PROTO_BIND_APPID_IDX]) = proto->devid;
	*(C_DEVID*)(&buf[NTY_PROTO_BIND_DEVICEID_IDX]) = did;
#else
	memcpy(&buf[NTY_PROTO_BIND_APPID_IDX], &proto->devid, sizeof(C_DEVID));
	memcpy(&buf[NTY_PROTO_BIND_DEVICEID_IDX], &did, sizeof(C_DEVID));
#endif
	len = NTY_PROTO_BIND_CRC_IDX + sizeof(U32);

	ntydbg(" ntyProtoClientBind --> ");

	void *pNetwork = ntyNetworkInstance();
	n = ntySendFrame(pNetwork, &proto->serveraddr, buf, len);
}

void ntyProtoClientUnBind(void *_self, C_DEVID did) {
	NattyProto *proto = _self;
	int len, n;	

	U8 buf[NORMAL_BUFFER_SIZE] = {0};	

	buf[NEY_PROTO_VERSION_IDX] = NEY_PROTO_VERSION;	
	buf[NTY_PROTO_MESSAGE_TYPE] = (U8) MSG_REQ;	
	buf[NTY_PROTO_TYPE_IDX] = NTY_PROTO_UNBIND_REQ;
#if 0
	*(C_DEVID*)(&buf[NTY_PROTO_UNBIND_APPID_IDX]) = proto->devid;
	*(C_DEVID*)(&buf[NTY_PROTO_UNBIND_DEVICEID_IDX]) = did;
#else
	memcpy(&buf[NTY_PROTO_BIND_APPID_IDX], &proto->devid, sizeof(C_DEVID));
	memcpy(&buf[NTY_PROTO_BIND_DEVICEID_IDX], &did, sizeof(C_DEVID));
#endif
	len = NTY_PROTO_UNBIND_CRC_IDX + sizeof(U32);

	void *pNetwork = ntyNetworkInstance();
	n = ntySendFrame(pNetwork, &proto->serveraddr, buf, len);
}



void ntyProtoClientLogout(void *_self) {
	NattyProto *proto = _self;
	int len, n;	
	U8 buf[RECV_BUFFER_SIZE] = {0};	

	buf[NEY_PROTO_VERSION_IDX] = NEY_PROTO_VERSION;	
	buf[NTY_PROTO_MESSAGE_TYPE] = (U8) MSG_REQ;	
	buf[NTY_PROTO_TYPE_IDX] = NTY_PROTO_LOGOUT_REQ;
#if 1
	//ntyU64ToU8Array(&buf[NTY_PROTO_DEVID_IDX], proto->devid);
	memcpy(&buf[NTY_PROTO_DEVID_IDX], &proto->devid, sizeof(C_DEVID));
#else
	*(C_DEVID*)(&buf[NTY_PROTO_DEVID_IDX]) = proto->devid;	
#endif
	len = NTY_PROTO_LOGIN_REQ_CRC_IDX+sizeof(U32);				

	void *pNetwork = ntyNetworkInstance();
	n = ntySendFrame(pNetwork, &proto->serveraddr, buf, len);
}

/*
 * Server Proxy Data Transport
 * VERSION					1			 BYTE
 * MESSAGE TYPE			 	1			 BYTE (req, ack)
 * TYPE				 	1			 BYTE 
 * DEVID					8			 BYTE (self devid)
 * ACKNUM					4			 BYTE (Network Module Set Value)
 * DEST DEVID				8			 BYTE (friend devid)
 * CONTENT COUNT				2			 BYTE 
 * CONTENT					*(CONTENT COUNT)	 BYTE 
 * CRC 				 	4			 BYTE (Network Module Set Value)
 * 
 * send to server addr, proxy to send one client
 * 
 */

void ntyProtoClientProxyReq(void *_self, C_DEVID toId, U8 *buf, int length) {
	int n = 0;
	NattyProto *proto = _self;

	buf[NEY_PROTO_VERSION_IDX] = NEY_PROTO_VERSION;
	buf[NTY_PROTO_MESSAGE_TYPE] = (U8) MSG_REQ;	
	buf[NTY_PROTO_TYPE_IDX] = NTY_PROTO_DATAPACKET_REQ;

//	LOG("ntyProtoClientProxyReq");
#if 1
	memcpy(&buf[NTY_PROTO_DATAPACKET_DEVID_IDX], &proto->devid, sizeof(C_DEVID));
	memcpy(&buf[NTY_PROTO_DATAPACKET_DEST_DEVID_IDX], &toId, sizeof(C_DEVID));
#else
	*(C_DEVID*)(&buf[NTY_PROTO_DATAPACKET_DEVID_IDX]) = (C_DEVID) proto->devid;
	*(C_DEVID*)(&buf[NTY_PROTO_DATAPACKET_DEST_DEVID_IDX]) = toId;
#endif
	
	*(U16*)(&buf[NTY_PROTO_DATAPACKET_CONTENT_COUNT_IDX]) = (U16)length;
	length += NTY_PROTO_DATAPACKET_CONTENT_IDX;
	length += sizeof(U32);

	//LOG("ntyProtoClientProxyReq, length:%d", length);
	void *pNetwork = ntyNetworkInstance();
	n = ntySendFrame(pNetwork, &proto->serveraddr, buf, length);
	
}

void ntyProtoClientProxyAck(void *_self, C_DEVID toId, U32 ack) {
	int len, n;	
	NattyProto *proto = _self;
	U8 buf[RECV_BUFFER_SIZE] = {0}; 
	
	buf[NEY_PROTO_VERSION_IDX] = NEY_PROTO_VERSION;
	buf[NTY_PROTO_TYPE_IDX] = NTY_PROTO_DATAPACKET_ACK;
	buf[NTY_PROTO_MESSAGE_TYPE] = (U8) MSG_ACK; 
#if 0
	*(C_DEVID*)(&buf[NTY_PROTO_DEVID_IDX]) = (C_DEVID) proto->devid;		
	*(U32*)(&buf[NTY_PROTO_ACKNUM_IDX]) = ack+1;
	*(C_DEVID*)(&buf[NTY_PROTO_DEST_DEVID_IDX]) = toId;
#else
	memcpy(&buf[NTY_PROTO_DEVID_IDX], &proto->devid, sizeof(C_DEVID));
	memcpy(&buf[NTY_PROTO_DEST_DEVID_IDX], &toId, sizeof(C_DEVID));
	ack = ack+1;
	memcpy(&buf[NTY_PROTO_ACKNUM_IDX], &ack, sizeof(int));
#endif
	len = NTY_PROTO_CRC_IDX+sizeof(U32);				

	void *pNetwork = ntyNetworkInstance();
	n = ntySendFrame(pNetwork, &proto->serveraddr, buf, len);
}


static const NattyProtoOpera ntyProtoOpera = {
	sizeof(NattyProto),
	ntyProtoClientCtor,
	ntyProtoClientDtor,
	ntyProtoClientHeartBeat,
	ntyProtoClientLogin,
	ntyProtoClientLogout,
	ntyProtoClientProxyReq,
	ntyProtoClientProxyAck,
	ntyProtoClientBind,
	ntyProtoClientUnBind,
#if (NEY_PROTO_VERSION > 'B')
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
#endif	
};

const void *pNattyProtoOpera = &ntyProtoOpera;

static void *pProtoOpera = NULL;

void *ntyProtoInstance(void) {
	if (pProtoOpera == NULL) {
		ntydbg("ntyProtoInstance\n");
		pProtoOpera = New(pNattyProtoOpera);
	}
	return pProtoOpera;
}

void ntyProtoRelease(void *self) {
	return Delete(self);
}

static void ntySetupHeartBeatThread(void* self) {
#if 1
	NattyProto *proto = self;
	NattyProtoOpera * const * protoOpera = self;
	int err;

	if (self && (*protoOpera) && (*protoOpera)->heartbeat) {
		if (proto->heartbeatThread_id != 0) {
			ntydbg(" heart beat thread is running \n");
			return ;
		}
		err = pthread_create(&proto->heartbeatThread_id, NULL, (*protoOpera)->heartbeat, self);				
		if (err != 0) { 				
			ntydbg(" can't create thread:%s\n", strerror(err)); 
			return ;			
		}
	}
#else
	NattyProto *proto = self;
	int err;
	if (self && proto && proto->heartbeat) {		
		err = pthread_create(&proto->heartbeatThread_id, NULL, proto->heartbeat, self);				
		if (err != 0) { 				
			ntydbg(" can't create thread:%s\n", strerror(err)); 
			exit(0);				
		}
	}
#endif
}

static void ntySetupRecvProcThread(void *self) {
	//NattyProtoOpera * const * protoOpera = self;
	NattyProto *proto = self;
	int err;
	pthread_t recvThread_id;

	if (self && proto && proto->onRecvCallback) {	
		if (proto->recvThread_id != 0) {
			ntydbg(" recv thread is running \n");
			return ;
		}
		
		err = pthread_create(&proto->recvThread_id, NULL, proto->onRecvCallback, self);				
		if (err != 0) { 				
			ntydbg(" can't create thread:%s\n", strerror(err)); 
			return ;	
		}
	}
}

static void ntySendLogin(void *self) {
	NattyProtoOpera * const * protoOpera = self;

	ntydbg(" ntySendLogin %d\n", __LINE__);
	if (self && (*protoOpera) && (*protoOpera)->login) {
		return (*protoOpera)->login(self);
	}
}

static void ntySendLogout(void *self) {
	NattyProtoOpera * const * protoOpera = self;

	if (self && (*protoOpera) && (*protoOpera)->logout) {
		return (*protoOpera)->logout(self);
	}
}

int ntySendDataPacket(C_DEVID toId, U8 *data, int length) {
	int n = 0;
	void *self = ntyProtoInstance();
	NattyProtoOpera * const *protoOpera = self;
	NattyProto *proto = self;
	
	U8 buf[NTY_PROXYDATA_PACKET_LENGTH] = {0};
	
#if 0
	buf[NEY_PROTO_VERSION_IDX] = NEY_PROTO_VERSION;
	buf[NTY_PROTO_MESSAGE_TYPE] = (U8) MSG_REQ;	
	buf[NTY_PROTO_TYPE_IDX] = NTY_PROTO_DATAPACKET_REQ;
	*(C_DEVID*)(&buf[NTY_PROTO_DATAPACKET_DEVID_IDX]) = (C_DEVID) proto->devid;
	*(C_DEVID*)(&buf[NTY_PROTO_DATAPACKET_DEST_DEVID_IDX]) = toId;
	
	*(U16*)(&buf[NTY_PROTO_DATAPACKET_CONTENT_COUNT_IDX]) = (U16)length;
	length += NTY_PROTO_DATAPACKET_CONTENT_IDX;
	length += sizeof(U32);
	void *pNetwork = ntyNetworkInstance();
	n = ntySendFrame(pNetwork, &proto->serveraddr, buf, length);
#else
	U8 *tempBuf = &buf[NTY_PROTO_DATAPACKET_CONTENT_IDX];
	memcpy(tempBuf, data, length);
	//LOG(" toId : %lld \n", toId);
	if (proto && (*protoOpera) && (*protoOpera)->proxyReq) {
		(*protoOpera)->proxyReq(proto, toId, buf, length);
		return 0;
	}
	return -1;
#endif
	
}

int ntySendMassDataPacket(U8 *data, int length) {	
	void *pTree = ntyRBTreeInstance();
	
	//LOG(" data:%s, length:%d", data, length);
	ntyFriendsTreeMass(pTree, ntySendDataPacket, data, length);

	return 0;
}

void ntySetSendSuccessCallback(PROXY_CALLBACK cb) {
	NattyProto* proto = ntyProtoInstance();
	proto->onProxySuccess = cb;
}

void ntySetSendFailedCallback(PROXY_CALLBACK cb) {
	NattyProto* proto = ntyProtoInstance();
	if (proto) {
		proto->onProxyFailed = cb;
	}
}

void ntySetProxyCallback(PROXY_CALLBACK cb) {
	NattyProto* proto = ntyProtoInstance();
	if (proto) {
		proto->onProxyCallback = cb;
	}
}

void ntySetProxyDisconnect(PROXY_CALLBACK cb) {
	NattyProto* proto = ntyProtoInstance();
	if (proto) {
		proto->onProxyDisconnect = cb;
	}
}

void ntySetProxyReconnect(PROXY_CALLBACK cb) {
	NattyProto* proto = ntyProtoInstance();
	if (proto) {
		proto->onProxyReconnect = cb;
	}
}

void ntySetBindResult(PROXY_CALLBACK cb) {
	NattyProto* proto = ntyProtoInstance();
	if (proto) {
		proto->onBindResult = cb;
	}
}

void ntySetUnBindResult(PROXY_CALLBACK cb) {
	NattyProto* proto = ntyProtoInstance();
	if (proto) {
		proto->onUnBindResult = cb;
	}
}

void ntySetPacketRecv(PROXY_CALLBACK cb) {
	NattyProto* proto = ntyProtoInstance();
	if (proto) {
		proto->onPacketRecv = cb;
	}
}

void ntySetPacketResult(PROXY_CALLBACK cb) {
	NattyProto* proto = ntyProtoInstance();
	if (proto) {
		proto->onPacketSuccess = cb;
	}
}



void ntySetDevId(C_DEVID id) {
	NattyProto* proto = ntyProtoInstance();
	if (proto) {
		proto->devid = id;
	}
}

int ntyGetNetworkStatus(void) {
	void *network = ntyNetworkInstance();
	return ntyGetSocket(network);
}

int ntyCheckProtoClientStatus(void) {
	NattyProto* proto = ntyProtoInstance();
	if (proto) {
		if (proto->onProxyCallback == NULL) return -2;
		if (proto->onProxyFailed == NULL) return -3;
		if (proto->onProxySuccess == NULL) return -4;
		if (proto->onProxyDisconnect == NULL) return -5;
		if (proto->onProxyReconnect == NULL) return -6;
		if (proto->onBindResult == NULL) return -7;
		if (proto->onUnBindResult == NULL) return -8;
		if (proto->onRecvCallback == NULL) return -9;
		if (proto->onPacketRecv == NULL) return -10;
		if (proto->onPacketSuccess == NULL) return -11;
	}
	return 0;
}


int ntyStartupClient(void) {
	int ret = ntyCheckProtoClientStatus();
	if (ret != 0) return ret;
	
	NattyProto* proto = ntyProtoInstance();
	if (proto) {
		ntySendLogin(proto);
		ntySetupHeartBeatThread(proto); //setup heart proc
		ntySetupRecvProcThread(proto); //setup recv proc
	}

	return ntyGetNetworkStatus();
}

void ntyShutdownClient(void) {
	NattyProto* proto = ntyProtoInstance();
	void *pNetwork = ntyNetworkInstance();
	ntyNetworkRelease(pNetwork);

	//proto->u8HeartbeatExistFlag = 1;
	//proto->u8RecvExitFlag = 1;
	
	proto->recvThread_id = 0;
	proto->heartbeatThread_id = 0;
	proto->heartbeartRun = 0;
}

#if 1
void ntyBindClient(C_DEVID did) {
	NattyProto* proto = ntyProtoInstance();

	if (proto) {
		ntyProtoClientBind(proto, did);
	}
}

void ntyUnBindClient(C_DEVID did) {
	NattyProto* proto = ntyProtoInstance();

	if (proto) {
		ntyProtoClientUnBind(proto, did);
	}
}
#endif

U8* ntyGetRecvBuffer(void) {
	NattyProto* proto = ntyProtoInstance();
	if (proto) {
		proto->recvBuffer[NTY_PROTO_DATAPACKET_CONTENT_IDX+proto->recvLen] = 0x0;
		return proto->recvBuffer+NTY_PROTO_DATAPACKET_CONTENT_IDX;
	}
	return NULL;
}
#if 1
C_DEVID ntyGetFromDevID(void) {
	NattyProto* proto = ntyProtoInstance();
	if (proto) {
		return proto->fromId;
	}
	return 0x0;
}
#endif

int ntyGetRecvBufferSize(void) {
	NattyProto* proto = ntyProtoInstance();
	if (proto) {
		return proto->recvLen;
	}
	return -1;
}

static void ntySendTimeout(int len) {
	NattyProto* proto = ntyProtoInstance();
	if (proto && proto->onProxyFailed) {
		proto->onProxyFailed(STATUS_TIMEOUT);
	}

	void* pTimer = ntyNetworkTimerInstance();
	ntyStopTimer(pTimer);
}

static void ntyReconnectProc(int len) {
	void *pNetwork = ntyNetworkInstance();
	ntydbg("ntyReconnectProc : %d\n", ntyGetSocket(pNetwork));
	if (-1 == ntyGetSocket(pNetwork)) { //Reconnect failed
		pNetwork = ntyNetworkRelease(pNetwork);
		pNetwork = NULL;
		
	} else { //Reconnect success
		ntyStartupClient();
		//
		void *pConnTimer = ntyReconnectTimerInstance();	
		ntyStopTimer(pConnTimer);

		NattyProto *proto = ntyProtoInstance();
		if (proto->onProxyReconnect) {
			proto->onProxyReconnect(0);
		}
	}
	return ;
}

static int ntyReconnectTimerCb(timer_id id, void *user_data, int len) {
	void *pNetwork = ntyNetworkInstance();
	ntydbg("ntyReconnectProc : %d\n", ntyGetSocket(pNetwork));
	if (-1 == ntyGetSocket(pNetwork)) { //Reconnect failed
		pNetwork = ntyNetworkRelease(pNetwork);
		pNetwork = NULL;
		
	} else { //Reconnect success
		ntyStartupClient();
		int ret = del_timer(id);

		NattyProto *proto = ntyProtoInstance();
		if (proto->onProxyReconnect) {
			proto->onProxyReconnect(0);
		}
	}
}

void ntyReleaseNetwork(void) {
#if 1
	void *network = ntyGetNetworkInstance();
#endif
	if (network != NULL) {
		network = ntyNetworkRelease(network);
		network = NULL;
	}
#if 0
	void *pConnTimer = ntyReconnectTimerInstance();
	ntyStartTimer(pConnTimer, ntyReconnectProc);
#else
	add_timer(60, ntyReconnectTimerCb, NULL, 0);
#endif
}

void ntyDestoryNetwork(void *network) {
	network = ntyNetworkRelease(network);
	network = NULL;
}

C_DEVID* ntyGetFriendsList(int *Count) {
	void *pTree = ntyRBTreeInstance();
	
	C_DEVID *list = ntyFriendsTreeGetAllNodeList(pTree);
	*Count = ntyFriendsTreeGetNodeCount(pTree);

	return list;
}

void ntyReleaseFriendsList(C_DEVID **list) {
	C_DEVID *pList = *list;
	free(pList);
	pList = NULL;
}

#if 1



int ntySendVoicePacket(U8 *buffer, int length, C_DEVID toId) {
	U16 Count = length / NTY_VOICEREQ_PACKET_LENGTH + 1 ;
	U32 pktLength = NTY_VOICEREQ_DATA_LENGTH, i;
	U8 *pkt = buffer;
	void *pNetwork = ntyNetworkInstance();
	NattyProto* proto = ntyProtoInstance();
	int ret = -1;

	LOG(" destId:%d, pktIndex:%d, pktTotal:%d", NTY_PROTO_VOICEREQ_DESTID_IDX,
		NTY_PROTO_VOICEREQ_PKTINDEX_IDX, NTY_PROTO_VOICEREQ_PKTTOTLE_IDX);
	
	for (i = 0;i < Count;i ++) {
		pkt = buffer+(i*NTY_VOICEREQ_PACKET_LENGTH);

		pkt[NEY_PROTO_VERSION_IDX] = NEY_PROTO_VERSION;
		pkt[NTY_PROTO_MESSAGE_TYPE] = (U8) MSG_REQ;	
		pkt[NTY_PROTO_VOICEREQ_TYPE_IDX] = NTY_PROTO_VOICE_REQ;

		memcpy(pkt+NTY_PROTO_VOICEREQ_SELFID_IDX, &proto->devid, sizeof(C_DEVID));
		memcpy(pkt+NTY_PROTO_VOICEREQ_DESTID_IDX, &toId, sizeof(C_DEVID));

		memcpy(pkt+NTY_PROTO_VOICEREQ_PKTINDEX_IDX, &i, sizeof(U16));
		memcpy(pkt+NTY_PROTO_VOICEREQ_PKTTOTLE_IDX, &Count , sizeof(U16));

		if (i == Count-1) { //last packet
			pktLength = (length % NTY_VOICEREQ_PACKET_LENGTH) - NTY_VOICEREQ_EXTEND_LENGTH;
		}

		memcpy(pkt+NTY_PROTO_VOICEREQ_PKTLENGTH_IDX, &pktLength, sizeof(U32));
		
		ret = ntySendFrame(pNetwork, &proto->serveraddr, pkt, pktLength+NTY_VOICEREQ_EXTEND_LENGTH);

		LOG(" index : %d", i );
		LOG(" pktLength:%d, Count:%d, ret:%d, selfIdx:%d\n",
			pktLength+NTY_VOICEREQ_EXTEND_LENGTH, Count, ret, NTY_PROTO_VOICEREQ_SELFID_IDX);

		usleep(200 * 1000); //Window Send
	}

	return 0;
}

int ntyAudioPacketEncode(U8 *pBuffer, int length) {
	int i = 0, j = 0, k = 0, idx = 0;
	int pktCount = length / NTY_VOICEREQ_DATA_LENGTH;
	int pktLength = pktCount * NTY_VOICEREQ_PACKET_LENGTH + (length % NTY_VOICEREQ_DATA_LENGTH) + NTY_VOICEREQ_EXTEND_LENGTH;
	//U8 *pktIndex = pBuffer + pktCount * NTY_VOICEREQ_PACKET_LENGTH;

	LOG("pktLength :%d, pktCount:%d\n", pktLength, pktCount);
	if (pktCount >= (NTY_VOICEREQ_COUNT_LENGTH-1)) return -1;

	j = pktLength - NTY_CRCNUM_LENGTH;
	k = length;

	LOG("j :%d, k :%d, pktCount:%d, last:%d\n", j, k, pktCount, (length % NTY_VOICEREQ_DATA_LENGTH));
	for (idx = 0;idx < (length % NTY_VOICEREQ_DATA_LENGTH);idx ++) {
		pBuffer[--j] = pBuffer[--k];
	}	
			

	for (i = pktCount;i > 0;i --) {
		j = i * NTY_VOICEREQ_PACKET_LENGTH - NTY_CRCNUM_LENGTH;
		k = i * NTY_VOICEREQ_DATA_LENGTH;
		LOG("j :%d, k :%d\n", j, k);
		for (idx = 0;idx < NTY_VOICEREQ_DATA_LENGTH;idx ++) {
			pBuffer[--j] = pBuffer[--k];
		}
	}

	return pktLength;
}



int u32DataLength = 0;
static U8 u8RecvBigBuffer[NTY_BIGBUFFER_SIZE] = {0};
static U8 u8SendBigBuffer[NTY_BIGBUFFER_SIZE] = {0};
TimerId tBigTimer = -1;

int ntyGetRecvBigLength(void) {
	return u32DataLength;
}

U8 *ntyGetRecvBigBuffer(void) {
	return u8RecvBigBuffer;
}

U8 *ntyGetSendBigBuffer(void) {
	return u8SendBigBuffer;
}

static int ntySendBigBufferCb(timer_id id, void *user_data, int len) {
	NattyProto* proto = ntyProtoInstance();
	if (proto && proto->onPacketSuccess) {
		proto->onPacketSuccess(1); //Failed
		if (tBigTimer != -1) {
			del_timer(tBigTimer);
			tBigTimer = -1;
		}
	}

	return 0;
}



int ntySendBigBuffer(U8 *u8Buffer, int length, C_DEVID toId) {
	int i = 0;

	tBigTimer = add_timer(10, ntySendBigBufferCb, NULL, 0);

	int ret = ntyAudioPacketEncode(u8Buffer, length);
	LOG(" ntySendBigBuffer --> Ret %d, %x", ret, u8Buffer[0]);

	ntySendVoicePacket(u8Buffer, length, toId);
	
	C_DEVID tToId = 0;
	memcpy(&tToId, u8Buffer+NTY_PROTO_VOICEREQ_DESTID_IDX, sizeof(C_DEVID));
	LOG(" ntySendBigBuffer --> toId : %lld, %d", tToId, NTY_PROTO_VOICEREQ_DESTID_IDX);



	return 0;
}

int ntyAudioRecodeDepacket(U8 *buffer, int length) {
	int i = 0;
	U8 *pBuffer = ntyGetRecvBigBuffer();	
	U16 index = ntyU8ArrayToU16(buffer+NTY_PROTO_VOICEREQ_PKTINDEX_IDX);
	U16 Count = ntyU8ArrayToU16(&buffer[NTY_PROTO_VOICEREQ_PKTTOTLE_IDX]);
	U32 pktLength = ntyU8ArrayToU32(&buffer[NTY_PROTO_VOICEREQ_PKTLENGTH_IDX]);

	//nty_printf(" Count:%d, index:%d, pktLength:%d, length:%d, pktLength%d\n", 
	//				Count, index, pktLength, length, NTY_PROTO_VOICEREQ_PKTLENGTH_IDX);

	if (length != pktLength+NTY_VOICEREQ_EXTEND_LENGTH) return 2;

	
	for (i = 0;i < pktLength;i ++) {
		pBuffer[index * NTY_VOICEREQ_DATA_LENGTH + i] = buffer[NTY_VOICEREQ_HEADER_LENGTH + i];
	}

	if (index == Count-1) {
		u32DataLength = NTY_VOICEREQ_DATA_LENGTH*(Count-1) + pktLength;
		return 1;
	}

	return 0;
}



void ntyPacketClassifier(void *arg, U8 *buf, int length) {
	NattyProto *proto = arg;
	NattyProtoOpera * const *protoOpera = arg;
	Network *pNetwork = ntyNetworkInstance();
	
	if (buf[NTY_PROTO_TYPE_IDX] == NTY_PROTO_LOGIN_ACK) {
		int i = 0;
		
		int count = ntyU8ArrayToU16(&buf[NTY_PROTO_LOGIN_ACK_FRIENDS_COUNT_IDX]);
		void *pTree = ntyRBTreeInstance();

		LOG("count : %d", count);
		for (i = 0;i < count;i ++) {
			//C_DEVID friendId = *(C_DEVID*)(&buf[NTY_PROTO_LOGIN_ACK_FRIENDSLIST_DEVID_IDX(i)]);
			C_DEVID friendId = 0;
			ntyU8ArrayToU64(&buf[NTY_PROTO_LOGIN_ACK_FRIENDSLIST_DEVID_IDX(i)], &friendId);
			LOG(" friendId i:%d --> %lld\n", i+1, friendId);

			FriendsInfo *friendInfo = ntyRBTreeInterfaceSearch(pTree, friendId);
			if (NULL == friendInfo) {
				FriendsInfo *pFriend = (FriendsInfo*)malloc(sizeof(FriendsInfo));
				assert(pFriend);
				pFriend->sockfd = ntyGetSocket(pNetwork);
				pFriend->isP2P = 0;
				pFriend->counter = 0;
				pFriend->addr = ntyU8ArrayToU32(&buf[NTY_PROTO_LOGIN_ACK_FRIENDSLIST_ADDR_IDX(i)]);
				pFriend->port = ntyU8ArrayToU16(&buf[NTY_PROTO_LOGIN_ACK_FRIENDSLIST_PORT_IDX(i)]);
				ntyRBTreeInterfaceInsert(pTree, friendId, pFriend);
			} else {
				friendInfo->sockfd = ntyGetSocket(pNetwork);;
				friendInfo->isP2P = 0;
				friendInfo->counter = 0;
				friendInfo->addr = ntyU8ArrayToU32(&buf[NTY_PROTO_LOGIN_ACK_FRIENDSLIST_ADDR_IDX(i)]);
				friendInfo->port = ntyU8ArrayToU16(&buf[NTY_PROTO_LOGIN_ACK_FRIENDSLIST_PORT_IDX(i)]);
			}					
		}

		proto->level = LEVEL_DATAPACKET;
		//ntylog("NTY_PROTO_LOGIN_ACK\n");
		
	} else if (buf[NTY_PROTO_TYPE_IDX] == NTY_PROTO_DATAPACKET_REQ) {
		//U16 cliCount = *(U16*)(&buf[NTY_PROTO_DATAPACKET_NOTIFY_CONTENT_COUNT_IDX]);
		U8 data[RECV_BUFFER_SIZE] = {0};//NTY_PROTO_DATAPACKET_NOTIFY_CONTENT_IDX
		U16 recByteCount = ntyU8ArrayToU16(&buf[NTY_PROTO_DATAPACKET_NOTIFY_CONTENT_COUNT_IDX]);
		C_DEVID friId = 0;
		ntyU8ArrayToU64(&buf[NTY_PROTO_DEVID_IDX], &friId);
		U32 ack = ntyU8ArrayToU32(&buf[NTY_PROTO_ACKNUM_IDX]);

		memcpy(data, buf+NTY_PROTO_DATAPACKET_CONTENT_IDX, recByteCount);
		LOG(" recv:%s end\n", data);

		memcpy(&proto->fromId, &friId, sizeof(C_DEVID));
		if (buf[NTY_PROTO_MESSAGE_TYPE] == MSG_RET) {
			if (proto->onProxyFailed)
				proto->onProxyFailed(STATUS_NOEXIST);
			
			//continue;
		}
		LOG("proxyAck start");
		if (arg && (*protoOpera) && (*protoOpera)->proxyAck) {
			(*protoOpera)->proxyAck(proto, friId, ack);
		}

		LOG("proxyAck end");
		if (proto->onProxyCallback) {
			proto->recvLen -= (NTY_PROTO_DATAPACKET_CONTENT_IDX+sizeof(U32));
			proto->onProxyCallback(proto->recvLen);
		}
		LOG("onProxyCallback end");
	} else if (buf[NTY_PROTO_TYPE_IDX] == NTY_PROTO_DATAPACKET_ACK) {
		LOG(" send success\n");
		if (proto->onProxySuccess) {
			proto->onProxySuccess(0);
		}
	} else if (buf[NTY_PROTO_TYPE_IDX] == NTY_PROTO_BIND_ACK) {
#if 0
		int result = ntyU8ArrayToU32(&buf[NTY_PROTO_BIND_ACK_RESULT_IDX]);
#else
		int result = 0;
		memcpy(&result, &buf[NTY_PROTO_BIND_ACK_RESULT_IDX], sizeof(int));
#endif
		if (proto->onBindResult) {
			proto->onBindResult(result);
		}
		ntydbg(" NTY_PROTO_BIND_ACK\n");
	} else if (buf[NTY_PROTO_TYPE_IDX] == NTY_PROTO_UNBIND_ACK) {
#if 0
		int result = ntyU8ArrayToU32(&buf[NTY_PROTO_BIND_ACK_RESULT_IDX]);
#else
		int result = 0;
		memcpy(&result, &buf[NTY_PROTO_BIND_ACK_RESULT_IDX], sizeof(int));
#endif
		if (proto->onUnBindResult) {
			proto->onUnBindResult(result);
		}
		ntydbg(" NTY_PROTO_UNBIND_ACK\n");
	} else if (buf[NTY_PROTO_TYPE_IDX] == NTY_PROTO_VOICE_REQ) {
		int ret = ntyAudioRecodeDepacket(buf, length);
		if (ret == 1) { //the end
			C_DEVID friId = 0;
			ntyU8ArrayToU64(&buf[NTY_PROTO_DEVID_IDX], &friId);
			memcpy(&proto->fromId, &friId, sizeof(C_DEVID));

			if (proto->onPacketRecv) {
				proto->onPacketRecv(u32DataLength);
			}
		}		
	} else if (buf[NTY_PROTO_TYPE_IDX] == NTY_PROTO_VOICE_ACK) {
		if (tBigTimer != -1) {
			del_timer(tBigTimer);
			tBigTimer = -1;
		}
		LOG(" onPacketSuccess --> ");
		if (proto->onPacketSuccess) {
			proto->onPacketSuccess(0); //Success
		}
	}
}

static U8 rBuffer[NTY_VOICEREQ_PACKET_LENGTH] = {0};
static U16 rLength = 0;
extern U32 ntyGenCrcValue(U8 *buf, int length);

int ntyPacketValidator(void *self, U8 *buffer, int length) {
	int bCopy = 0, bIndex = 0, ret = -1;
	U32 uCrc = 0, uClientCrc = 0;
	int bLength = length;

	LOG(" rLength :%d, length:%d\n", rLength, length);
	uCrc = ntyGenCrcValue(buffer, length-4);
	uClientCrc = ntyU8ArrayToU32(buffer+length-4);
	if (uCrc != uClientCrc) {
		do {
			bCopy = (bLength > NTY_VOICEREQ_PACKET_LENGTH ? NTY_VOICEREQ_PACKET_LENGTH : bLength);
			bCopy = (((rLength + bCopy) > NTY_VOICEREQ_PACKET_LENGTH) ? (NTY_VOICEREQ_PACKET_LENGTH - rLength) : bCopy);
			
			memcpy(rBuffer+rLength, buffer+bIndex, bCopy);
			rLength += bCopy;
			
			uCrc = ntyGenCrcValue(rBuffer, rLength-4);
			uClientCrc = ntyU8ArrayToU32(rBuffer+rLength-4);

			LOG("uCrc:%x  uClientCrc:%x", uCrc, uClientCrc);
			if (uCrc == uClientCrc)	 {
				LOG(" CMD:%x, Version:[%d]\n", rBuffer[NTY_PROTO_TYPE_IDX], rBuffer[NEY_PROTO_VERSION_IDX]);
				
				ntyPacketClassifier(self, rBuffer, rLength);

				rLength = 0;
				ret = 0;
			} 
			
			bLength -= bCopy;
			bIndex += bCopy;
			rLength %= NTY_VOICEREQ_PACKET_LENGTH;
			
		} while (bLength);
	} else {
		ntyPacketClassifier(self, buffer, length);
		rLength = 0;
		ret = 0;
	}
	return ret;
}

#endif

static void* ntyRecvProc(void *arg) {
	struct sockaddr_in addr;
	int clientLen = sizeof(struct sockaddr_in);
	NattyProto *proto = arg;
	NattyProtoOpera * const *protoOpera = arg;
	U8 *buf = proto->recvBuffer;

	int ret;

//	ntydbg(" ntyRecvProc %d\n", fds.fd);
	while (1) {
		
		void *pNetwork = ntyGetNetworkInstance();
		struct pollfd fds;
		fds.fd = ntyGetSocket(pNetwork);
		fds.events = POLLIN;
		if (fds.fd == -1) { //disconnect 
			sleep(1);
			continue;
		}
		ret = poll(&fds, 1, 5);
		if (ret) {
			bzero(buf, RECV_BUFFER_SIZE);
			proto->recvLen = ntyRecvFrame(pNetwork, buf, RECV_BUFFER_SIZE, &addr);
			if (proto->recvLen == 0 || proto->recvLen > RECV_BUFFER_SIZE) { //disconnect
				//ntyReconnect(pNetwork);
				//Release Network
				
				LOG("Prepare to Reconnect to server, proto->recvLen:%d\n", proto->recvLen);
				if (ntyGetSocket(pNetwork) != -1){
					if (proto->onProxyDisconnect) {
						proto->onProxyDisconnect(0);
					}
#if 0
					ntyDestoryNetwork(pNetwork);
#else
					ntyReleaseNetwork();
#endif
				}

				sleep(1);
				continue;
			}
			LOG("\n%d.%d.%d.%d:%d, length:%d --> %x\n", *(unsigned char*)(&addr.sin_addr.s_addr), *((unsigned char*)(&addr.sin_addr.s_addr)+1),
				*((unsigned char*)(&addr.sin_addr.s_addr)+2), *((unsigned char*)(&addr.sin_addr.s_addr)+3),													
				addr.sin_port, proto->recvLen, buf[NTY_PROTO_TYPE_IDX]);
			
#if 1
			ntyPacketValidator(arg, buf, proto->recvLen);
#else
			if (buf[NTY_PROTO_TYPE_IDX] == NTY_PROTO_LOGIN_ACK) {
				int i = 0;
				
				int count = ntyU8ArrayToU16(&buf[NTY_PROTO_LOGIN_ACK_FRIENDS_COUNT_IDX]);
				void *pTree = ntyRBTreeInstance();

				//LOG("count : %d", count);
				for (i = 0;i < count;i ++) {
					//C_DEVID friendId = *(C_DEVID*)(&buf[NTY_PROTO_LOGIN_ACK_FRIENDSLIST_DEVID_IDX(i)]);
					C_DEVID friendId = 0;
					ntyU8ArrayToU64(&buf[NTY_PROTO_LOGIN_ACK_FRIENDSLIST_DEVID_IDX(i)], &friendId);
					ntydbg(" friendId i:%d --> %lld\n", i+1, friendId);

					FriendsInfo *friendInfo = ntyRBTreeInterfaceSearch(pTree, friendId);
					if (NULL == friendInfo) {
						FriendsInfo *pFriend = (FriendsInfo*)malloc(sizeof(FriendsInfo));
						assert(pFriend);
						pFriend->sockfd = ntyGetSocket(pNetwork);
						pFriend->isP2P = 0;
						pFriend->counter = 0;
						pFriend->addr = ntyU8ArrayToU32(&buf[NTY_PROTO_LOGIN_ACK_FRIENDSLIST_ADDR_IDX(i)]);
						pFriend->port = ntyU8ArrayToU16(&buf[NTY_PROTO_LOGIN_ACK_FRIENDSLIST_PORT_IDX(i)]);
						ntyRBTreeInterfaceInsert(pTree, friendId, pFriend);
					} else {
						friendInfo->sockfd = ntyGetSocket(pNetwork);;
						friendInfo->isP2P = 0;
						friendInfo->counter = 0;
						friendInfo->addr = ntyU8ArrayToU32(&buf[NTY_PROTO_LOGIN_ACK_FRIENDSLIST_ADDR_IDX(i)]);
						friendInfo->port = ntyU8ArrayToU16(&buf[NTY_PROTO_LOGIN_ACK_FRIENDSLIST_PORT_IDX(i)]);
					}					
				}
		
				proto->level = LEVEL_DATAPACKET;
				//ntylog("NTY_PROTO_LOGIN_ACK\n");
				
			} else if (buf[NTY_PROTO_TYPE_IDX] == NTY_PROTO_DATAPACKET_REQ) {
				//U16 cliCount = *(U16*)(&buf[NTY_PROTO_DATAPACKET_NOTIFY_CONTENT_COUNT_IDX]);
				U8 data[RECV_BUFFER_SIZE] = {0};//NTY_PROTO_DATAPACKET_NOTIFY_CONTENT_IDX
				U16 recByteCount = ntyU8ArrayToU16(&buf[NTY_PROTO_DATAPACKET_NOTIFY_CONTENT_COUNT_IDX]);
				C_DEVID friId = 0;
				ntyU8ArrayToU64(&buf[NTY_PROTO_DEVID_IDX], &friId);
				U32 ack = ntyU8ArrayToU32(&buf[NTY_PROTO_ACKNUM_IDX]);

				memcpy(data, buf+NTY_PROTO_DATAPACKET_CONTENT_IDX, recByteCount);
				LOG(" recv:%s end\n", data);

				//memset(proto->fromId, 0, sizeof(10));
				memcpy(&proto->fromId, &friId, sizeof(C_DEVID));
				//sendProxyDataPacketAck(friId, ack);
				if (buf[NTY_PROTO_MESSAGE_TYPE] == MSG_RET) {
					if (proto->onProxyFailed)
						proto->onProxyFailed(STATUS_NOEXIST);
					
					continue;
				}
				//LOG("proxyAck start");
				if (arg && (*protoOpera) && (*protoOpera)->proxyAck) {
					(*protoOpera)->proxyAck(proto, friId, ack);
				}

				//LOG("proxyAck end");
				if (proto->onProxyCallback) {
					proto->recvLen -= (NTY_PROTO_DATAPACKET_CONTENT_IDX+sizeof(U32));
					proto->onProxyCallback(proto->recvLen);
				}
				//LOG("onProxyCallback end");
			} else if (buf[NTY_PROTO_TYPE_IDX] == NTY_PROTO_DATAPACKET_ACK) {
				//LOG(" send success\n");
				if (proto->onProxySuccess) {
					proto->onProxySuccess(0);
				}
			} else if (buf[NTY_PROTO_TYPE_IDX] == NTY_PROTO_BIND_ACK) {
			#if 0
				int result = ntyU8ArrayToU32(&buf[NTY_PROTO_BIND_ACK_RESULT_IDX]);
			#else
				int result = 0;
				memcpy(&result, &buf[NTY_PROTO_BIND_ACK_RESULT_IDX], sizeof(int));
			#endif
				if (proto->onBindResult) {
					proto->onBindResult(result);
				}
				ntydbg(" NTY_PROTO_BIND_ACK\n");
			} else if (buf[NTY_PROTO_TYPE_IDX] == NTY_PROTO_UNBIND_ACK) {
			#if 0
				int result = ntyU8ArrayToU32(&buf[NTY_PROTO_BIND_ACK_RESULT_IDX]);
			#else
				int result = 0;
				memcpy(&result, &buf[NTY_PROTO_BIND_ACK_RESULT_IDX], sizeof(int));
			#endif
				if (proto->onUnBindResult) {
					proto->onUnBindResult(result);
				}
				ntydbg(" NTY_PROTO_UNBIND_ACK\n");
			} else if (buf[NTY_PROTO_TYPE_IDX] == NTY_PROTO_VOICE_REQ) {
				
			} 
			
#endif
		}
	}
}






