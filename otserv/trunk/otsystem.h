

#ifndef __OTTHREAD_H__
#define __OTTHREAD_H__


#if defined WIN32 || defined __WINDOWS__

#include <process.h>
#include <sys/timeb.h>

#define OTSYS_THREAD_RETURN  void

#define OTSYS_CREATE_THREAD(a, b) _beginthread(a, 0, b)


#define OTSYS_THREAD_LOCKVAR CRITICAL_SECTION

#define OTSYS_THREAD_LOCKVARINIT(a) InitializeCriticalSection(&a);
#define OTSYS_THREAD_LOCK(a)        EnterCriticalSection(&a);
#define OTSYS_THREAD_UNLOCK(a)      LeaveCriticalSection(&a);


#define OTSYS_SLEEP(a) Sleep(a);


inline __int64 OTSYS_TIME()
{
  _timeb t;
  _ftime(&t);
  return ((__int64)t.millitm) + ((__int64)t.time) * 1000;
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


#define OTSYS_THREAD_RETURN void*

inline void OTSYS_CREATE_THREAD(void *(*a)(void*), void *b)
{
  pthread_t id;
  pthread_create(&id, NULL, a, b);
}

typedef pthread_mutex_t OTSYS_THREAD_LOCKVAR;

#define OTSYS_THREAD_LOCKVARINIT(a) pthread_mutex_init(&a, NULL);
#define OTSYS_THREAD_LOCK(a)        pthread_mutex_lock(&a);
#define OTSYS_THREAD_UNLOCK(a)      pthread_mutex_unlock(&a);

inline void OTSYS_SLEEP(int t)
{
  timespec tv;
  tv.tv_sec  = t % 1000000;
  tv.tv_nsec = t / 1000;
  nanosleep(&tv, NULL);
}


typedef long long __int64;

inline __int64 OTSYS_TIME()
{
  timeb t;
  ftime(&t);
  return ((__int64)t.millitm) + ((__int64)t.time) * 1000;
}


#ifndef SOCKET
#define SOCKET int
#endif

#ifndef closesocket
#define closesocket close
#endif


#endif // #if defined WIN32 || defined __WINDOWS__




#endif // #ifndef __OTTHREAD_H__
