

#ifndef __OTTHREAD_H__
#define __OTTHREAD_H__


#if defined WIN32 || defined __WINDOWS__

#include <windows.h>
#include <process.h>
#include <sys/timeb.h>

#define OTSYS_THREAD_RETURN  void

#define OTSYS_CREATE_THREAD(a, b) _beginthread(a, 0, b)


#define OTSYS_THREAD_LOCKVAR CRITICAL_SECTION

#define OTSYS_THREAD_LOCKVARINIT(a) InitializeCriticalSection(&a);
#define OTSYS_THREAD_LOCK(a)        EnterCriticalSection(&a);
#define OTSYS_THREAD_UNLOCK(a)      LeaveCriticalSection(&a);

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


inline int OTSYS_THREAD_WAITSIGNAL_TIMED(OTSYS_THREAD_SIGNALVAR& signal, OTSYS_THREAD_LOCKVAR& lock, __int64 cycle)
{
  DWORD tout = (DWORD)(cycle - OTSYS_TIME());
  if (tout < 0)
    tout = 0;

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
#include <netdb.h>
#include <stdint.h>


#define OTSYS_THREAD_RETURN void*

inline void OTSYS_CREATE_THREAD(void *(*a)(void*), void *b)
{
  pthread_t id;
  pthread_create(&id, NULL, a, b);
}

typedef pthread_mutex_t OTSYS_THREAD_LOCKVAR;

#define OTSYS_THREAD_LOCKVARINIT(a)   pthread_mutex_init(&a, NULL);
#define OTSYS_THREAD_LOCK(a)          pthread_mutex_lock(&a);
#define OTSYS_THREAD_UNLOCK(a)        pthread_mutex_unlock(&a);
#define OTSYS_THREAD_TIMEOUT			  ETIMEDOUT
#define OTSYS_THREAD_SIGNALVARINIT(a) pthread_cond_init(&a, NULL);
#define OTSYS_THREAD_SIGNAL_SEND(a)   pthread_cond_signal(&a);

typedef pthread_cond_t OTSYS_THREAD_SIGNALVAR;

inline void OTSYS_SLEEP(int t)
{
  timespec tv;
  tv.tv_sec  = t / 1000;
  tv.tv_nsec = t % 1000;
  nanosleep(&tv, NULL);
}

typedef uint64_t __int64;

inline __int64 OTSYS_TIME()
{
  timeb t;
  ftime(&t);
  return ((__int64)t.millitm) + ((__int64)t.time) * 1000;
}

#define OTSYS_THREAD_WAITSIGNAL(a,b) pthread_cond_wait(&a, &b)

#include <iostream>
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




#endif // #ifndef __OTTHREAD_H__
