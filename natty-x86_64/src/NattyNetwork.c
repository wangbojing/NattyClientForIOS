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



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> 
#include <pthread.h>
#include <poll.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <netinet/tcp.h>



#include "NattyNetwork.h"



static int count = 0;
static U32 msgAck = 0x0;
static int level = LEVEL_LOGIN;
static C_DEVID devid = 0;
static C_DEVID friendId = 0;
static C_DEVID tempId = 0;
static U32 ackNum = 0;
static int sockfd_local = 0;

static int ntyNetworkResendFrame(void *_self);
void sendP2PConnectAck(C_DEVID friId, U32 ack);
void sendProxyDataPacketAck(C_DEVID friId, U32 ack);
void sendP2PDataPacketAck(C_DEVID friId, U32 ack);
void sendP2PConnectNotifyAck(C_DEVID friId, U32 ack);
void sendP2PConnectNotify(C_DEVID fromId, C_DEVID toId);
U8 ntyGetReqType(void *self);
C_DEVID ntyGetDestDevId(void *self);





#define NTY_CRCTABLE_LENGTH			256
#define NTY_CRC_KEY		0x04c11db7ul
static U32 u32CrcTable[NTY_CRCTABLE_LENGTH] = {0};
void ntyGenCrcTable(void) {	
	U16 i,j;	
	U32 u32CrcNum = 0;	
	for (i = 0;i < NTY_CRCTABLE_LENGTH;i ++) {		
		U32 u32CrcNum = (i << 24);		
		for (j = 0;j < 8;j ++) {			
			if (u32CrcNum & 0x80000000L) {				
				u32CrcNum = (u32CrcNum << 1) ^ NTY_CRC_KEY;
			} else {				
				u32CrcNum = (u32CrcNum << 1);			
			}		
		}		
		u32CrcTable[i] = u32CrcNum;	
	}
}

U32 ntyGenCrcValue(U8 *buf, int length) {	
	U32 u32CRC = 0xFFFFFFFF;		
	while (length -- > 0) {		
		u32CRC = (u32CRC << 8) ^ u32CrcTable[((u32CRC >> 24) ^ *buf++) & 0xFF];	
	}	
	return u32CRC;
}


void ntyMessageOnDataLost(void) {
	////////////////////////////////////////////////////////////
	//ntyCancelTimer();
	////////////////////////////////////////////////////////////
	ntydbg("ntyMessageOnDataLost\n");

	void *pNetwork = ntyNetworkInstance();
	U8 u8ReqType = ntyGetReqType(pNetwork);
	if (u8ReqType == NTY_PROTO_P2P_HEARTBEAT_REQ) {
		C_DEVID destDevId = ntyGetDestDevId(pNetwork);
		void *pTree = ntyRBTreeInstance();
		FriendsInfo *pFriend = ntyRBTreeInterfaceSearch(pTree, destDevId);
		if(pFriend->counter > P2P_HEARTBEAT_TIMEOUT_COUNTR) {
			pFriend->isP2P = 0;
			//don't take reconnect
		}
	}
	
	void* pTimer = ntyNetworkTimerInstance();
	ntyStopTimer(pTimer);
#if 0
	if (LEVEL_P2PCONNECT == level) {
		level = LEVEL_P2PCONNECT;
	}
#endif
}

void ntyMessageOnAck(int signo) {
	//ntylog(" Get a Sigalarm, %d counts! \n", ++count);
	void *pNetwork = ntyNetworkInstance();
	if (++count > SENT_TIMEOUT) {
#if 0 // this action should post to java 
		ntyMessageOnDataLost();
#else
		if (pNetwork && ((Network *)pNetwork)->onDataLost) {
			((Network *)pNetwork)->onDataLost(count);
			count = 0;
		}
#endif
	} else {
		ntyNetworkResendFrame(pNetwork);
	}
}

void error(char *msg) {    
	perror(msg);    
	exit(0);
}

//(struct sockaddr *)&client->addr

static void* ntyNetworkCtor(void *_self, va_list *params) {
	Network *network = _self;
	network->onAck = ntyMessageOnAck;
	network->ackNum = 1;

#if 1 //Socket Init
	network->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (network->sockfd < 0) {
		error(" ERROR opening socket");
	}
	ntydbg(" sockfd %d, %s, %d\n",network->sockfd, __FILE__, __LINE__);
#endif
	
	return network;
}

static void* ntyNetworkDtor(void *_self) {
	return _self;
}

static int ntyNetworkResendFrame(void *_self) {
	Network *network = _self;

	return sendto(network->sockfd, network->buffer, network->length, 0, 
			(struct sockaddr *)&network->addr, sizeof(struct sockaddr_in));
}


static int ntyNetworkSendFrame(void *_self, struct sockaddr_in *to, U8 *buf, int len) {
	//ntyStartTimer();
	Network *network = _self;	
	void* pTimer = ntyNetworkTimerInstance();
	if (buf[NTY_PROTO_MESSAGE_TYPE] != MSG_ACK) {
		ntyStartTimer(pTimer, network->onAck);	
		network->ackNum ++;
	}
	
	memcpy(&network->addr, to, sizeof(struct sockaddr_in));
	bzero(network->buffer, CACHE_BUFFER_SIZE);
	memcpy(network->buffer, buf, len);
	
	if (buf[NTY_PROTO_MESSAGE_TYPE] == MSG_REQ) {
		*(U32*)(&network->buffer[NTY_PROTO_ACKNUM_IDX]) = network->ackNum;
	}
	network->length = len;
	*(U32*)(&network->buffer[len-sizeof(U32)]) = ntyGenCrcValue(network->buffer, len-sizeof(U32));

	return sendto(network->sockfd, network->buffer, network->length, 0, 
		(struct sockaddr *)&network->addr, sizeof(struct sockaddr_in));
}

static int ntyNetworkRecvFrame(void *_self, U8 *buf, int len, struct sockaddr_in *from) {
	//ntyStartTimer();

	int n = 0;
	int clientLen = sizeof(struct sockaddr);
	struct sockaddr_in addr = {0};

	Network *network = _self;
	n = recvfrom(network->sockfd, buf, CACHE_BUFFER_SIZE, 0, (struct sockaddr*)&addr, /*(socklen_t*)*/&clientLen);
	U32 ackNum = *(U32*)(&buf[NTY_PROTO_ACKNUM_IDX]);

	memcpy(from, &addr, clientLen);
	if (buf[NTY_PROTO_MESSAGE_TYPE] == MSG_ACK) { //recv success		
		if (ackNum == network->ackNum + 1) {
			// CRC 
			
			// stop timer
			void* pTimer = ntyNetworkTimerInstance();
			ntyStopTimer(pTimer);

			return n;	
		} else {
			return -1;
		}
	} else if (buf[NTY_PROTO_MESSAGE_TYPE] == MSG_RET) {
		void* pTimer = ntyNetworkTimerInstance();
		ntyStopTimer(pTimer);

		//have send object
	} else if (buf[NTY_PROTO_MESSAGE_TYPE] == MSG_UPDATE) {
		void* pTimer = ntyNetworkTimerInstance();
		ntyStopTimer(pTimer);
	}

	return n;
	
}

static const NetworkOpera ntyNetworkOpera = {
	sizeof(Network),
	ntyNetworkCtor,
	ntyNetworkDtor,
	ntyNetworkSendFrame,
	ntyNetworkRecvFrame,
	ntyNetworkResendFrame,
	NULL,
};

const void *pNtyNetworkOpera = &ntyNetworkOpera;

static int ntySetupTcpClient(Network *network, const char *host, const char *service) {
	int res = -1;
	struct addrinfo *result, *rp;
	struct addrinfo hints;
	
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;          /* Any protocol */

	int ret = getaddrinfo(host, service, &hints, &result);
	if (ret != 0) {
		fprintf(stderr, "getaddrinfo: %d %s\n", ret,  gai_strerror(ret));
		return -1;
	}
#if 0
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		network->sockfd = socket(AF_INET, SOCK_STREAM, 0)
	}
#else
	rp = result;
	while (rp != NULL) {
		network->sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (network->sockfd < 0) continue;

		res = connect(network->sockfd, rp->ai_addr, rp->ai_addrlen);
		if (res == 0) {
			memcpy(&network->addr, rp->ai_addr, rp->ai_addrlen);
			break;
		}

		close(network->sockfd);
		network->sockfd = -1;
		rp = rp->ai_next;
	}

	freeaddrinfo(result);

	return network->sockfd;
#endif
}




static void* ntyTcpNetworkCtor(void *self, va_list *params) {
	int res = -1;
	Network *network = self;
#if 0
	if ((network->sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		LOG("Socket Error:%s\n", strerror(errno));
		return network;
	}

	memset(&network->addr, 0, sizeof(network->addr));
	network->addr.sin_family = AF_INET;
	network->addr.sin_port = htons(SERVER_PORT);
	network->addr.sin_addr.s_addr = inet_addr(SERVER_NAME);

	res = connect(network->sockfd, (struct sockaddr*)(&network->addr), sizeof(struct sockaddr));
	LOG("connect failed :%d --> %s\n", res, strerror(errno));
	if (-1 == res) {
		close(network->sockfd);
		network->sockfd = -1;
		return network;
	}
#else

	ntySetupTcpClient(network, SERVER_HOSTNAME, "echo");

#endif

	return network;
}

static void* ntyTcpNetworkDtor(void *self) {
	Network *network = self;
	close(network->sockfd);
	network->sockfd = -1;
	return self;
}

static int ntyTcpNetworkSendFrame(void *self, struct sockaddr_in *to, U8 *buf, int len) {
	Network *network = self;

	if (network->sockfd == -1) return -1;
	
	memcpy(&network->addr, to, sizeof(struct sockaddr_in));
	bzero(network->buffer, CACHE_BUFFER_SIZE);
	memcpy(network->buffer, buf, len);
	
	if (buf[NTY_PROTO_MESSAGE_TYPE] == MSG_REQ) {
		*(U32*)(&network->buffer[NTY_PROTO_ACKNUM_IDX]) = network->ackNum;
	}
	network->length = len;
	*(U32*)(&network->buffer[len-sizeof(U32)]) = ntyGenCrcValue(network->buffer, len-sizeof(U32));
	
	return send(network->sockfd, network->buffer, len, 0);
}

static int ntyTcpNetworkRecvFrame(void *self, U8 *buf, int len, struct sockaddr_in *from) {
	Network *network = self;
	int nSize = sizeof(struct sockaddr_in);

	if (network->sockfd == -1) return -1;

	getpeername(network->sockfd,(struct sockaddr*)from, &nSize);
	
	return recv(network->sockfd, buf, len, 0);
}

static int ntyTcpNetworkReconnect(void *self) {
	Network *network = self;
	if (0 != connect(network->sockfd, (struct sockaddr*)(&network->addr), sizeof(struct sockaddr))) {
		//close(network->sockfd);
		//network->sockfd = -1;
		return -1;
	}
	return 0;
}


static const NetworkOpera ntyTcpNetworkOpera = {
	sizeof(Network),
	ntyTcpNetworkCtor,
	ntyTcpNetworkDtor,
	ntyTcpNetworkSendFrame,
	ntyTcpNetworkRecvFrame,
	NULL,
	ntyTcpNetworkReconnect,
};
const void *pNtyTcpNetworkOpera = &ntyTcpNetworkOpera;

static void *pNetworkOpera = NULL;

void *ntyNetworkInstance(void) {
	if (pNetworkOpera == NULL) {
		pNetworkOpera = New(pNtyTcpNetworkOpera);
	}
	return pNetworkOpera;
}

void *ntyGetNetworkInstance(void) {
	return pNetworkOpera;
}

void* ntyNetworkRelease(void *self) {	
	Delete(self);
	pNetworkOpera = NULL;
	return self;
}

int ntySendFrame(void *self, struct sockaddr_in *to, U8 *buf, int len) {
	const NetworkOpera *const * pNetworkOpera = self;

	if (self && (*pNetworkOpera) && (*pNetworkOpera)->send) {
		return (*pNetworkOpera)->send(self, to, buf, len);
	}
	return -1;
}

int ntyRecvFrame(void *self, U8 *buf, int len, struct sockaddr_in *from) {
	const NetworkOpera *const * pNetworkOpera = self;

	if (self && (*pNetworkOpera) && (*pNetworkOpera)->recv) {
		return (*pNetworkOpera)->recv(self, buf, len, from);
	}
	return -2;
}

int ntyReconnect(void *self) {
	ntydbg("ntyReconnect to server\n");
	return ntyTcpNetworkReconnect(self);
}

int ntyGetSocket(void *self) {
	Network *network = self;
	if (network == NULL) return -1;
	return network->sockfd;
}

U8 ntyGetReqType(void *self) {
	Network *network = self;
	return network->buffer[NTY_PROTO_TYPE_IDX];
}

C_DEVID ntyGetDestDevId(void *self) {
	Network *network = self;
	return *(C_DEVID*)(&network->buffer[NTY_PROTO_DEST_DEVID_IDX]);
}

