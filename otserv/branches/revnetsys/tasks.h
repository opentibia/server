//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Special Tasks which require more arguments than possible
// with STL functions...
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

#ifndef __OTSERV_TASKS_H__
#define __OTSERV_TASKS_H__

#include <boost/function.hpp>
#include "otsystem.h"

class Task{
public:
	~Task() {}
	
	void operator()(){
		m_f();
	}
		
protected:
	
	Task(boost::function<void (void)> f){
		m_f = f;
	}
	
	boost::function<void (void)> m_f;
	
	friend Task* createTask(boost::function<void (void)>);
};

inline Task* createTask(boost::function<void (void)> f){
	return new Task(f);
}

class Dispatcher{
public:
	~Dispatcher() {}
	
	static Dispatcher& getDispatcher()
	{
		static Dispatcher dispatcher;
		return dispatcher;
	}
	
	void addTask(Task* task);
	
	static OTSYS_THREAD_RETURN dispatcherThread(void *p);
	
protected:
	Dispatcher();
	
	OTSYS_THREAD_LOCKVAR m_taskLock;
	OTSYS_THREAD_SIGNALVAR m_taskSignal;
	
	std::list<Task*> m_taskList;
};


#endif
