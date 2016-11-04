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



#include <signal.h>
#include <sys/time.h>
#include <string.h>

#include "NattyTimer.h"


static void* ntyTimerCtor(void *_self, va_list *params) {
	NetworkTimer *timer = _self;
	pthread_mutex_t blank_mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t blank_cond = PTHREAD_COND_INITIALIZER;
	
	timer->sigNum = SIGALRM;
	timer->timerProcess = 0;
	memcpy(&timer->timer_mutex, &blank_mutex, sizeof(timer->timer_mutex));
	memcpy(&timer->timer_cond, &blank_cond, sizeof(timer->timer_cond));

	return timer;
}

static void* ntyTimerDtor(void *_self) {
	return _self;
}

static int ntyStartTimerOpera(void *_self, HANDLE_TIMER fun) {
	NetworkTimer *timer = _self;
	struct itimerval tick;
	timer->timerFunc = fun;

	signal(timer->sigNum, timer->timerFunc);
	memset(&tick, 0, sizeof(tick));

	tick.it_value.tv_sec = 0;
	tick.it_value.tv_usec = MS(TIMER_TICK);

	tick.it_interval.tv_sec = 0;
	tick.it_interval.tv_usec = MS(TIMER_TICK);

	
	pthread_mutex_lock(&timer->timer_mutex);
	while (timer->timerProcess) {
		pthread_cond_wait(&timer->timer_cond, &timer->timer_mutex);
	}
	timer->timerProcess = 1;
	if (setitimer(ITIMER_REAL, &tick, NULL) < 0) {
		printf("Set timer failed!\n");
	}
	pthread_mutex_unlock(&timer->timer_mutex);
	
	return 0;
}


static int ntyStopTimerOpera(void *_self) {
	NetworkTimer *timer = _self;

	struct itimerval tick;

	signal(timer->sigNum, timer->timerFunc);
	memset(&tick, 0, sizeof(tick));

	tick.it_value.tv_sec = 0;
	tick.it_value.tv_usec = 0;//MS(TIMER_TICK);

	tick.it_interval.tv_sec = 0;
	tick.it_interval.tv_usec = 0;//MS(TIMER_TICK);

	pthread_mutex_lock(&timer->timer_mutex);
	timer->timerProcess = 0;
#if 0 //mac os don't support
	pthread_cond_broadcast(&timer->timer_cond);
#else
	pthread_cond_signal(&timer->timer_cond);
#endif
	if (setitimer(ITIMER_REAL, &tick, NULL) < 0) {
		printf("Set timer failed!\n");
	}
	pthread_mutex_unlock(&timer->timer_mutex);
	
	return 0;
}




static const TimerOpera ntyTimerOpera = {
	sizeof(NetworkTimer),
	ntyTimerCtor,
	ntyTimerDtor,
	ntyStartTimerOpera,
	ntyStopTimerOpera,
};

const void *pNtyTimerOpera = &ntyTimerOpera;

static void* pNetworkTimer = NULL;
void *ntyNetworkTimerInstance(void) {
	if (pNetworkTimer == NULL) {
		pNetworkTimer = New(pNtyTimerOpera);
	}
	return pNetworkTimer;
}

int ntyStartTimer(void *self,  HANDLE_TIMER func) {
	const TimerOpera* const *handle = self;
	if (self && (*handle) && (*handle)->start) {
		return (*handle)->start(self, func);
	}
	return -2;
}

int ntyStopTimer(void *self) {
	const TimerOpera* const *handle = self;
	if (self && (*handle) && (*handle)->stop) {
		return (*handle)->stop(self);
	}
	return -2;
}

void ntyNetworkTimerRelease(void *self) {
	
	return Delete(self);
}


static void* ntyReconnTimerCtor(void *_self, va_list *params) {
	NetworkTimer *timer = _self;
	pthread_mutex_t blank_mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t blank_cond = PTHREAD_COND_INITIALIZER;
	
	timer->sigNum = SIGALRM;
	timer->timerProcess = 0;
	memcpy(&timer->timer_mutex, &blank_mutex, sizeof(timer->timer_mutex));
	memcpy(&timer->timer_cond, &blank_cond, sizeof(timer->timer_cond));

	return timer;
}

static void* ntyReconnTimerDtor(void *_self) {
	return _self;
}

static int ntyStartReconnTimerHandle(void *_self, HANDLE_TIMER fun) {
	NetworkTimer *timer = _self;
	struct itimerval tick;
	timer->timerFunc = fun;

	signal(timer->sigNum, timer->timerFunc);
	memset(&tick, 0, sizeof(tick));

	tick.it_value.tv_sec = RECONNECT_TICK;
	tick.it_value.tv_usec = 0;

	tick.it_interval.tv_sec = RECONNECT_TICK;
	tick.it_interval.tv_usec = 0;

#if 0	
	pthread_mutex_lock(&timer->timer_mutex);
	while (timer->timerProcess) {
		pthread_cond_wait(&timer->timer_cond, &timer->timer_mutex);
	}
	timer->timerProcess = 1;
#endif
	if (setitimer(ITIMER_REAL, &tick, NULL) < 0) {
		printf("Set timer failed!\n");
	}
#if 0
	pthread_mutex_unlock(&timer->timer_mutex);
#endif	
	return 0;
}


static int ntyStopReconnTimerHandle(void *_self) {
	NetworkTimer *timer = _self;

	struct itimerval tick;

	signal(timer->sigNum, timer->timerFunc);
	memset(&tick, 0, sizeof(tick));

	tick.it_value.tv_sec = 0;
	tick.it_value.tv_usec = 0;//MS(TIMER_TICK);

	tick.it_interval.tv_sec = 0;
	tick.it_interval.tv_usec = 0;//MS(TIMER_TICK);
#if 0
	pthread_mutex_lock(&timer->timer_mutex);
	timer->timerProcess = 0;
#if 0 //mac os don't support
	pthread_cond_broadcast(&timer->timer_cond);
#else
	pthread_cond_signal(&timer->timer_cond);
#endif
#endif
	if (setitimer(ITIMER_REAL, &tick, NULL) < 0) {
		printf("Set timer failed!\n");
	}
#if 0
	pthread_mutex_unlock(&timer->timer_mutex);
#endif
	return 0;
}



static const TimerOpera ntyReconnectTimerHandle = {
	sizeof(NetworkTimer),
	ntyReconnTimerCtor,
	ntyReconnTimerDtor,
	ntyStartReconnTimerHandle,
	ntyStopReconnTimerHandle,
};

const void *pNtyReconnectTimerHandle = &ntyReconnectTimerHandle;
static void* pReconnectTimer = NULL;

void *ntyReconnectTimerInstance(void) {
	if (pReconnectTimer == NULL) {
		pReconnectTimer = New(pNtyReconnectTimerHandle);
	}
	return pReconnectTimer;
}



#if 1 //WHEEL Alg

static struct timer_list timer_list;
static pthread_mutex_t timer_mutex[MAX_TIMER_NUM];

static void sig_func(int signo);


int init_timer(int count)
{
	int ret = 0;
	
	if(count <=0 || count > MAX_TIMER_NUM) {
		printf("the timer max number MUST less than %d.\n", MAX_TIMER_NUM);
		return -1;
	}
	
	memset(&timer_list, 0, sizeof(struct timer_list));
	LIST_INIT(&timer_list.header);
	timer_list.max_num = count;	

	/* Register our internal signal handler and store old signal handler */
	if ((timer_list.old_sigfunc = signal(SIGALRM, sig_func)) == SIG_ERR) {
		return -1;
	}
	timer_list.new_sigfunc = sig_func;

	/* Setting our interval timer for driver our mutil-timer and store old timer value */
	timer_list.value.it_value.tv_sec = TIMER_START;
	timer_list.value.it_value.tv_usec = 0;
	timer_list.value.it_interval.tv_sec = TIMER_TICK;
	timer_list.value.it_interval.tv_usec = 0;
	ret = setitimer(ITIMER_REAL, &timer_list.value, &timer_list.ovalue);

	return ret;
}

/**
 * Destroy the timer list.
 *
 * @return          0 means ok, the other means fail.
 */
int destroy_timer(void)
{
	struct timer *node = NULL;
	
	if ((signal(SIGALRM, timer_list.old_sigfunc)) == SIG_ERR) {
		return -1;
	}

	if((setitimer(ITIMER_REAL, &timer_list.ovalue, &timer_list.value)) < 0) {
		return -1;
	}
	
	while (!LIST_EMPTY(&timer_list.header)) {/* Delete. */
		node = LIST_FIRST(&timer_list.header);
		LIST_REMOVE(node, entries);
		/* Free node */
		printf("Remove id %d\n", node->id);
		free(node->user_data);
		free(node);
	}
	
	memset(&timer_list, 0, sizeof(struct timer_list));

	return 0;
}

/**
 * Add a timer to timer list.
 *
 * @param interval  The timer interval(second).  
 * @param cb  	    When cb!= NULL and timer expiry, call it.  
 * @param user_data Callback's param.  
 * @param len  	    The length of the user_data.  
 *
 * @return          The timer ID, if == INVALID_TIMER_ID, add timer fail.
 */
timer_id add_timer(int interval, timer_expiry *cb, void *user_data, int len)
{
	struct timer *node = NULL;	
	pthread_mutex_t blank_mutex = PTHREAD_MUTEX_INITIALIZER;

	if (cb == NULL || interval <= 0) {
		return INVALID_TIMER_ID;
	}

	if(timer_list.num < timer_list.max_num) {
		timer_list.num++;
	} else {
		return INVALID_TIMER_ID;
	} 
	
	if((node = malloc(sizeof(struct timer))) == NULL) {
		return INVALID_TIMER_ID;
	}
	if(user_data != NULL || len != 0) {
		node->user_data = malloc(len);
		memcpy(node->user_data, user_data, len);
		node->len = len;
	} else {
		node->user_data = NULL;
		node->len = 0;
	}

	node->cb = cb;
	node->interval = interval;
	node->elapse = 0;
	node->id = timer_list.num;
	memcpy(&timer_mutex[node->id], &blank_mutex, sizeof(timer_mutex[node->id]));
	
	LIST_INSERT_HEAD(&timer_list.header, node, entries);
	
	return node->id;
}

/**
 * Delete a timer from timer list.
 *
 * @param id  	    The timer ID.  
 *
 * @return          0 means ok, the other fail.
 */
int del_timer(timer_id id)
{
	if (id <0 || id > timer_list.max_num) {
		return -1;
	}
			
	struct timer *node = timer_list.header.lh_first;
	for ( ; node != NULL; node = node->entries.le_next) {
		printf("Total timer num %d/timer id %d.\n", timer_list.num, id);
		if (id == node->id) {
			LIST_REMOVE(node, entries);
			timer_list.num--;
			
			pthread_mutex_lock(&timer_mutex[id]);
			if (node->user_data != NULL)
				free(node->user_data);

			free(node);
			pthread_mutex_unlock(&timer_mutex[id]);
			return 0;
		}
	}
	
	/* Can't find the timer */
	return -1;
}

/* Tick Bookkeeping */
static void sig_func(int signo)
{
	struct timer *node = timer_list.header.lh_first;
	for ( ; node != NULL; node = node->entries.le_next) {
		node->elapse++;
		if(node->elapse >= node->interval) {
			node->elapse = 0;
			pthread_mutex_lock(&timer_mutex[node->id]);
			node->cb(node->id, node->user_data, node->len);
			pthread_mutex_unlock(&timer_mutex[node->id]);
		}
	}
}

static char *fmt_time(char *tstr)
{
	time_t t;

	t = time(NULL);
	strcpy(tstr, ctime(&t));
	tstr[strlen(tstr)-1] = '\0';

	return tstr;
}

#endif





