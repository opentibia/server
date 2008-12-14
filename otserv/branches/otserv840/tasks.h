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
#include <boost/thread.hpp>

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

enum DispatcherState{
	STATE_RUNNING,
	STATE_CLOSING,
	STATE_TERMINATED,
};

class Dispatcher{
public:
	~Dispatcher() {}

	static Dispatcher& getDispatcher()
	{
		static Dispatcher dispatcher;
		return dispatcher;
	}

	void addTask(Task* task);
	void stop();
	void shutdown();

	static void dispatcherThread(void* p);

	enum DispatcherState{
		STATE_RUNNING,
		STATE_CLOSING,
		STATE_TERMINATED,
	};

protected:

	Dispatcher();
	void flush();

	boost::mutex m_taskLock;
	boost::condition_variable m_taskSignal;

	std::list<Task*> m_taskList;
	static DispatcherState m_threadState;
};


#endif
