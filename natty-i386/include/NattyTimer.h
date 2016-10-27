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



#ifndef __NATTY_TIMER_H__
#define __NATTY_TIMER_H__

#include <signal.h>
#include <sys/time.h>
#include <string.h>

#include <netdb.h>
#include <time.h>
#include <sys/queue.h>



#include <pthread.h>
#include "NattyAbstractClass.h"

typedef void (*HANDLE_TIMER)(int sig);

typedef struct _NetworkTimer {
	const void *_;
	int sigNum;
	U32 timerProcess;
	HANDLE_TIMER timerFunc;
	pthread_mutex_t timer_mutex;
	pthread_cond_t timer_cond;
} NetworkTimer;

typedef struct _TIMEROPERA {
	size_t size;
	void* (*ctor)(void *_self, va_list *params);
	void* (*dtor)(void *_self);
	int (*start)(void *_self, HANDLE_TIMER fun);
	int (*stop)(void *_self);
} TimerOpera;




#define MAX_TIMER_NUM			1000
#define CURRENT_TIMER_NUM		20
#define TIMER_START				1
#define TIMER_TICK				1
#define INVALID_TIMER_ID		(-1)

typedef int timer_id;
typedef timer_id TimerId;
typedef int timer_expiry(timer_id id, void *user_data, int len);

/**
 * The type of the timer
 */
struct timer {
	LIST_ENTRY(timer) entries;	/**< list entry		*/	
	
	timer_id id;			/**< timer id		*/

	int interval;			/**< timer interval(second)*/
	int elapse; 			/**< 0 -> interval 	*/

	timer_expiry *cb;		/**< call if expiry 	*/
	void *user_data;		/**< callback arg	*/
	int len;			/**< user_data length	*/
};

/**
 * The timer list
 */
struct timer_list {
	LIST_HEAD(listheader, timer) header;	/**< list header 	*/
	int num;				/**< timer entry number */
	int max_num;				/**< max entry number	*/

	void (*old_sigfunc)(int);		/**< save previous signal handler */
	void (*new_sigfunc)(int);		/**< our signal handler	*/

	struct itimerval ovalue;		/**< old timer value */
	struct itimerval value;			/**< our internal timer value */
};






#define TIMER_TICK		200
#define MS(x)		(x*1000)

#define RECONNECT_TICK		60
#define S(x)		(MS(x)*1000)

void *ntyNetworkTimerInstance(void);
void *ntyReconnectTimerInstance(void);

int ntyStartTimer(void *self,  HANDLE_TIMER func);
int ntyStopTimer(void *self);



#endif








