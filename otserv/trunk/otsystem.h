//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////


#ifndef __OTTHREAD_H__
#define __OTTHREAD_H__


#include "definitions.h"

#if defined WIN32 || defined __WINDOWS__

#include <windows.h>
#include <process.h>    /* _beginthread, _endthread */
#include <stddef.h>
#include <stdlib.h>
#include <conio.h>
#include <sys/timeb.h>
#include <winsock.h>

#define OTSYS_CREATE_THREAD(a, b) _beginthread(a, 0, b)


#define OTSYS_THREAD_LOCKVAR CRITICAL_SECTION

#define OTSYS_THREAD_LOCKVARINIT(a) InitializeCriticalSection(&a);
#define OTSYS_THREAD_LOCKVARRELEASE(a) DeleteCriticalSection(&a);
#define OTSYS_THREAD_LOCK(a)        EnterCriticalSection(&a);
#define OTSYS_THREAD_UNLOCK(a)      LeaveCriticalSection(&a);
#define OTSYS_THREAD_UNLOCK_PTR(a)  LeaveCriticalSection(a);

#define OTSYS_THREAD_TIMEOUT			  WAIT_TIMEOUT
#define OTSYS_THREAD_SIGNALVARINIT(a) a = CreateEvent(NULL, FALSE, FALSE, NULL)
#define OTSYS_THREAD_SIGNAL_SEND(a)   SetEvent(a);

typedef HANDLE OTSYS_THREAD_SIGNALVAR;

inline __int64 OTSYS_TIME()
{
  _timeb t;
  _ftime(&t);
  return ((__int64)t.millitm) + ((__int64)t.time) * 1000;
}

inline int OTSYS_THREAD_WAITSIGNAL(OTSYS_THREAD_SIGNALVAR& signal, OTSYS_THREAD_LOCKVAR& lock)
{
  LeaveCriticalSection(&lock);
  WaitForSingleObject(signal, INFINITE);
  EnterCriticalSection(&lock);

  return -0x4711;
}

inline void OTSYS_SLEEP(uint32_t t){
	Sleep(t);
}


inline int OTSYS_THREAD_WAITSIGNAL_TIMED(OTSYS_THREAD_SIGNALVAR& signal, OTSYS_THREAD_LOCKVAR& lock, __int64 cycle)
{
  __int64 tout64 = (cycle - OTSYS_TIME());
  
  DWORD tout = 0;
  if (tout64 > 0)
    tout = (DWORD)(tout64);

  LeaveCriticalSection(&lock);
  int ret = WaitForSingleObject(signal, tout);
  EnterCriticalSection(&lock);

  return ret;
}

typedef int socklen_t;

#else  // #if defined WIN32 || defined __WINDOWS__

#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdint.h>


inline void OTSYS_CREATE_THREAD(void *(*a)(void*), void *b)
{
  pthread_attr_t attr;
  pthread_t id;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
  pthread_create(&id, &attr, a, b);
}

typedef pthread_mutex_t OTSYS_THREAD_LOCKVAR;

inline void OTSYS_THREAD_LOCKVARINIT(OTSYS_THREAD_LOCKVAR& l) {
		  pthread_mutexattr_t attr;
		  pthread_mutexattr_init(&attr);
		  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
		  pthread_mutex_init(&l, &attr);
}

#define OTSYS_THREAD_LOCKVARRELEASE(a)  //todo: working macro

#define OTSYS_THREAD_LOCK(a)          pthread_mutex_lock(&a);
#define OTSYS_THREAD_UNLOCK(a)        pthread_mutex_unlock(&a);
#define OTSYS_THREAD_UNLOCK_PTR(a)    pthread_mutex_unlock(a);
#define OTSYS_THREAD_TIMEOUT			  ETIMEDOUT
#define OTSYS_THREAD_SIGNALVARINIT(a) pthread_cond_init(&a, NULL);
#define OTSYS_THREAD_SIGNAL_SEND(a)   pthread_cond_signal(&a);

typedef pthread_cond_t OTSYS_THREAD_SIGNALVAR;

inline void OTSYS_SLEEP(int t)
{
  timespec tv;
  tv.tv_sec  = t / 1000;
  tv.tv_nsec = (t % 1000)*1000000;
  nanosleep(&tv, NULL);
}

inline __int64 OTSYS_TIME()
{
  timeb t;
  ftime(&t);
  return ((__int64)t.millitm) + ((__int64)t.time) * 1000;
}

#define OTSYS_THREAD_WAITSIGNAL(a,b) pthread_cond_wait(&a, &b)

inline int OTSYS_THREAD_WAITSIGNAL_TIMED(OTSYS_THREAD_SIGNALVAR& signal, OTSYS_THREAD_LOCKVAR& lock, __int64 cycle) {
		  timespec tv;
		  tv.tv_sec = (__int64)(cycle / 1000);
		  // tv_nsec is in nanoseconds while we only store microseconds...
		  tv.tv_nsec = (__int64)(cycle % 1000) * 1000000;
		  return pthread_cond_timedwait(&signal, &lock, &tv);
}


#ifndef SOCKET
#define SOCKET int
#endif

#ifndef closesocket
#define closesocket close
#endif


#endif // #if defined WIN32 || defined __WINDOWS__


class OTSYS_THREAD_LOCK_CLASS{
public:
	inline OTSYS_THREAD_LOCK_CLASS(OTSYS_THREAD_LOCKVAR &a){
		mutex = &a;
		OTSYS_THREAD_LOCK(a)
	};
	inline ~OTSYS_THREAD_LOCK_CLASS(){
		OTSYS_THREAD_UNLOCK_PTR(mutex)
	};
	OTSYS_THREAD_LOCKVAR *mutex;
};



#endif // #ifndef __OTTHREAD_H__
