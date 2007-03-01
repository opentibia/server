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


#ifndef __OTSYSTEM_H__
#define __OTSYSTEM_H__

#include "definitions.h"
#include "logger.h"

#include <list>
#include <algorithm>

#if defined WIN32 || defined __WINDOWS__
#ifdef __WIN_LOW_FRAG_HEAP__
#define _WIN32_WINNT 0x0501
#endif
#include <windows.h>
#include <process.h>    /* _beginthread, _endthread */
#include <stddef.h>
#include <stdlib.h>
#include <conio.h>
#include <sys/timeb.h>
#include <winsock.h>

#define OTSYS_CREATE_THREAD(a, b) _beginthread(a, 0, b)

#ifndef __DEBUG_CRITICALSECTION__
	#define OTSYS_THREAD_LOCKVAR CRITICAL_SECTION

	#define OTSYS_THREAD_LOCKVARINIT(a) InitializeCriticalSection(&a);
	#define OTSYS_THREAD_LOCKVARRELEASE(a) DeleteCriticalSection(&a);
	#define OTSYS_THREAD_LOCK(a, b)        EnterCriticalSection(&a);
	#define OTSYS_THREAD_UNLOCK(a, b)      LeaveCriticalSection(&a);
	#define OTSYS_THREAD_UNLOCK_PTR(a, b)  LeaveCriticalSection(a);
#else
	#define OTSYS_THREAD_LOCKVAR HANDLE
	
	static void addLockLog(OTSYS_THREAD_LOCKVAR* a, const char* s, bool lock);

	struct logBlock {
		bool lock;
		unsigned long mutexaddr;
		std::string str;
		uint64_t time;
		int threadid;
	};

	#define OTSYS_THREAD_LOCKVARINIT(a)    a = CreateMutex(NULL, FALSE, NULL);
	#define OTSYS_THREAD_LOCKVARRELEASE(a) CloseHandle(a);
	#define OTSYS_THREAD_LOCK(a, b) { WaitForSingleObject(a,INFINITE); addLockLog(&a, b, true);}
	inline int OTSYS_THREAD_LOCKEX(HANDLE a, int b)   {return WaitForSingleObject(a, b);}
	#define OTSYS_THREAD_UNLOCK(a, b)         {addLockLog(&a, b, false); ReleaseMutex(a);}
	#define OTSYS_THREAD_UNLOCK_PTR(a, b)     {addLockLog(a, b, false); ReleaseMutex(*a);}
#endif

	#define OTSYS_THREAD_TIMEOUT			  WAIT_TIMEOUT
	#define OTSYS_THREAD_SIGNALVARINIT(a) a = CreateEvent(NULL, FALSE, FALSE, NULL)
	#define OTSYS_THREAD_SIGNAL_SEND(a)   SetEvent(a);

typedef HANDLE OTSYS_THREAD_SIGNALVAR;

inline int64_t OTSYS_TIME()
{
	_timeb t;
	_ftime(&t);
	return ((int64_t)t.millitm) + ((int64_t)t.time) * 1000;
}

inline int OTSYS_THREAD_WAITSIGNAL(OTSYS_THREAD_SIGNALVAR& signal, OTSYS_THREAD_LOCKVAR& lock)
{
	//LeaveCriticalSection(&lock);
	OTSYS_THREAD_UNLOCK(lock, "OTSYS_THREAD_WAITSIGNAL");
	WaitForSingleObject(signal, INFINITE);
	//EnterCriticalSection(&lock);
	OTSYS_THREAD_LOCK(lock, "OTSYS_THREAD_WAITSIGNAL");

	return -0x4711;
}

inline void OTSYS_SLEEP(uint32_t t){
	Sleep(t);
}


inline int OTSYS_THREAD_WAITSIGNAL_TIMED(OTSYS_THREAD_SIGNALVAR& signal, OTSYS_THREAD_LOCKVAR& lock, int64_t cycle)
{
	int64_t tout64 = (cycle - OTSYS_TIME());
  
	DWORD tout = 0;
	if(tout64 > 0)
		tout = (DWORD)(tout64);

	//LeaveCriticalSection(&lock);
	OTSYS_THREAD_UNLOCK(lock, "OTSYS_THREAD_WAITSIGNAL_TIMED");
	int ret = WaitForSingleObject(signal, tout);
	//EnterCriticalSection(&lock);
	OTSYS_THREAD_LOCK(lock, "OTSYS_THREAD_WAITSIGNAL_TIMED");

	return ret;
}

typedef int socklen_t;

inline void PERROR(const char*a)
{
	LPVOID lpMsg;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS, NULL, 
		GetLastError(), 
		MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), 
		(LPTSTR) &lpMsg, 0, NULL);  
		fprintf(stderr,"%s:(%d)%s\n",a,GetLastError(),lpMsg); 
		LocalFree(lpMsg); 
};

inline void SOCKET_PERROR(const char* a)
{ 
	LPVOID lpMsg; 
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS, NULL, 
		WSAGetLastError(), 
		MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), 
		(LPTSTR) &lpMsg, 0, NULL);  
		fprintf(stderr,"%s:(%d)%s\n",a,WSAGetLastError(),lpMsg); 
		LocalFree(lpMsg); 
};

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
#include <errno.h>

#define PTHREAD_MUTEX_RECURSIVE_NP PTHREAD_MUTEX_RECURSIVE


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

#define OTSYS_THREAD_LOCK(a, b)          pthread_mutex_lock(&a);
#define OTSYS_THREAD_UNLOCK(a, b)        pthread_mutex_unlock(&a);
#define OTSYS_THREAD_UNLOCK_PTR(a, b)    pthread_mutex_unlock(a);
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

inline int64_t OTSYS_TIME()
{
  timeb t;
  ftime(&t);
  return ((int64_t)t.millitm) + ((int64_t)t.time) * 1000;
}

inline int OTSYS_THREAD_WAITSIGNAL(OTSYS_THREAD_SIGNALVAR& signal, OTSYS_THREAD_LOCKVAR& lock){
	return pthread_cond_wait(signal, lock);
}

inline int OTSYS_THREAD_WAITSIGNAL_TIMED(OTSYS_THREAD_SIGNALVAR& signal, OTSYS_THREAD_LOCKVAR& lock, int64_t cycle){
	timespec tv;
	tv.tv_sec = (int64_t)(cycle / 1000);
	// tv_nsec is in nanoseconds while we only store microseconds...
	tv.tv_nsec = (int64_t)(cycle % 1000) * 1000000;
	return pthread_cond_timedwait(&signal, &lock, &tv);
}


#ifndef SOCKET
#define SOCKET int
#endif

#ifndef closesocket
#define closesocket close
#endif

#define PERROR(a) perror(a)
#define SOCKET_PERROR(a) perror(a)

#endif // #if defined WIN32 || defined __WINDOWS__


#ifdef __DEBUG_CRITICALSECTION__

class OTSYS_THREAD_LOCK_CLASS{
public:
	inline OTSYS_THREAD_LOCK_CLASS(OTSYS_THREAD_LOCKVAR &a){
		logmsg = NULL;
		mutex = &a;
		OTSYS_THREAD_LOCK(a, NULL)
	};

	inline OTSYS_THREAD_LOCK_CLASS(OTSYS_THREAD_LOCKVAR &a, const char* s){
		mutex = &a;
		OTSYS_THREAD_LOCK(a, NULL)

		logmsg = s;

		OTSYS_THREAD_LOCK_CLASS::addLog(mutex, s, true);
	}

	static void addLog(OTSYS_THREAD_LOCKVAR* a, const char *s, bool lock) {
		if(s == NULL)
			return;

    logBlock lb;
		lb.mutexaddr = (unsigned long)(a);
		lb.lock = lock;
		lb.str = s;
		lb.time = OTSYS_TIME();
		lb.threadid = GetCurrentThreadId();

		OTSYS_THREAD_LOCK_CLASS::loglist.push_back(lb);

		if(OTSYS_THREAD_LOCK_CLASS::loglist.size() > 1000) {
			OTSYS_THREAD_LOCK_CLASS::loglist.pop_front();
		}
	}

	inline ~OTSYS_THREAD_LOCK_CLASS(){
		OTSYS_THREAD_LOCK_CLASS::addLog(mutex, logmsg, false);
		OTSYS_THREAD_UNLOCK_PTR(mutex, NULL)
	};
		
	OTSYS_THREAD_LOCKVAR *mutex;
	const char* logmsg;
	typedef std::list< logBlock > LogList;
	static LogList loglist;
};

static void addLockLog(OTSYS_THREAD_LOCKVAR* a, const char* s, bool lock)
{
	OTSYS_THREAD_LOCK_CLASS::addLog(a, s, lock);
}

#else

class OTSYS_THREAD_LOCK_CLASS{
public:
	inline OTSYS_THREAD_LOCK_CLASS(OTSYS_THREAD_LOCKVAR &a){
		mutex = &a;
		OTSYS_THREAD_LOCK(a, NULL)
	};

	inline OTSYS_THREAD_LOCK_CLASS(OTSYS_THREAD_LOCKVAR &a, const char* s){
		mutex = &a;
		OTSYS_THREAD_LOCK(a, NULL)
	}

	inline ~OTSYS_THREAD_LOCK_CLASS(){
		OTSYS_THREAD_UNLOCK_PTR(mutex, NULL)
	};
		
	OTSYS_THREAD_LOCKVAR *mutex;
};

#endif //__DEBUG_CRITICALSECTION__

#endif // #ifndef __OTSYSTEM_H__
