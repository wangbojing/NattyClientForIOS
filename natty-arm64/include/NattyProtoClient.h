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


#ifndef __NATTY_PROTO_CLIENT_H__
#define __NATTY_PROTO_CLIENT_H__




typedef enum {
	STATUS_TIMEOUT = 0x0,
	STATUS_NOEXIST,
} StatusSendResult;


typedef unsigned long long DEVID;
typedef unsigned char U8;

typedef void (*PROXYCALLBACK)(int len);
typedef void (*PROXYHANDLECB)(DEVID id, int len);


int ntySendDataPacket(DEVID toId, U8 *data, int length);
int ntySendMassDataPacket(U8 *data, int length);
void ntySetSendSuccessCallback(PROXYCALLBACK cb);
void ntySetSendFailedCallback(PROXYCALLBACK cb);
void ntySetProxyCallback(PROXYHANDLECB cb);
void ntySetProxyReconnect(PROXYCALLBACK cb);
void ntySetProxyDisconnect(PROXYCALLBACK cb);
U8* ntyGetRecvBuffer(void);
void ntySetDevId(DEVID id);
int ntyGetRecvBufferSize(void);
int ntyStartupClient(void);
void ntyReleaseNetwork(void);
int ntyGetNetwortkStatus(void);
void ntyShutdownClient(void);


#endif

