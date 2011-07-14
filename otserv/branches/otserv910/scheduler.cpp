//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Scheduler-Objects for OpenTibia
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
#include "otpch.h"

#include <iostream>
#include "scheduler.h"
#include "exception.h"

Scheduler::Scheduler()
{
	m_lastEventId = 0;
	m_threadState = STATE_TERMINATED;
}

void Scheduler::start()
{
	m_threadState = STATE_RUNNING;
	boost::thread(boost::bind(&Scheduler::schedulerThread, (void*)this));
}

void Scheduler::schedulerThread(void* p)
{
	Scheduler* scheduler = (Scheduler*)p;

	ExceptionHandler schedulerExceptionHandler;
	schedulerExceptionHandler.InstallHandler();

	// NOTE: second argument defer_lock is to prevent from immediate locking
	boost::unique_lock<boost::mutex> eventLockUnique(scheduler->m_eventLock, boost::defer_lock);

	while(scheduler->m_threadState != STATE_TERMINATED){
		SchedulerTask* task = NULL;
		bool runTask = false;
		bool ret = true;

		// check if there are events waiting...
		eventLockUnique.lock();

		if(scheduler->m_eventList.empty()){
			#ifdef __DEBUG_SCHEDULER__
			std::cout << "Scheduler: No events" << std::endl;
			#endif
			scheduler->m_eventSignal.wait(eventLockUnique);
		}
		else{
			#ifdef __DEBUG_SCHEDULER__
			std::cout << "Scheduler: Waiting for event" << std::endl;
			#endif
			ret = scheduler->m_eventSignal.timed_wait(eventLockUnique, scheduler->m_eventList.top()->getCycle());
		}

		#ifdef __DEBUG_SCHEDULER__
		std::cout << "Scheduler: Signaled" << std::endl;
		#endif

		// the mutex is locked again now...
		if(!ret && (scheduler->m_threadState != STATE_TERMINATED)){
			// ok we had a timeout, so there has to be an event we have to execute...
			task = scheduler->m_eventList.top();
			scheduler->m_eventList.pop();

			// check if the event was stopped
			EventIdSet::iterator it = scheduler->m_eventIds.find(task->getEventId());
			if(it != scheduler->m_eventIds.end()){
				// was not stopped so we should run it
				runTask = true;
				scheduler->m_eventIds.erase(it);
			}
		}

		eventLockUnique.unlock();

		// add task to dispatcher
		if(task){
			// if it was not stopped
			if(runTask){
				// Expiration has another meaning for dispatcher tasks, reset it
				task->setDontExpire();
				#ifdef __DEBUG_SCHEDULER__
				std::cout << "Scheduler: Executing event " << task->getEventId() << std::endl;
				#endif
				g_dispatcher.addTask(task);
			}
			else{
				// was stopped, have to be deleted here
				delete task;
			}
		}
	}

	schedulerExceptionHandler.RemoveHandler();
}

uint32_t Scheduler::addEvent(SchedulerTask* task)
{
	bool do_signal = false;
	m_eventLock.lock();
	if(Scheduler::m_threadState == Scheduler::STATE_RUNNING){

		// check if the event has a valid id
		if(task->getEventId() == 0){
			// if not generate one
			if(m_lastEventId >= 0xFFFFFFFF){
				m_lastEventId = 0;
			}
			++m_lastEventId;
			task->setEventId(m_lastEventId);
		}
		// insert the eventid in the list of active events
		m_eventIds.insert(task->getEventId());

		// add the event to the queue
		m_eventList.push(task);

		// if the list was empty or this event is the top in the list
		// we have to signal it
		do_signal = (task == m_eventList.top());

#ifdef __DEBUG_SCHEDULER__
		std::cout << "Scheduler: Added event " << task->getEventId() << std::endl;
#endif
	}
#ifdef __DEBUG_SCHEDULER__
	else{
		std::cout << "Error: [Scheduler::addTask] Scheduler thread is terminated." << std::endl;
	}
#endif

	m_eventLock.unlock();

	if(do_signal){
		m_eventSignal.notify_one();
	}

	return task->getEventId();
}


bool Scheduler::stopEvent(uint32_t eventid)
{
	if(eventid == 0)
		return false;

	#ifdef __DEBUG_SCHEDULER__
	std::cout << "Scheduler: Stopping event " << eventid << std::endl;
	#endif

	m_eventLock.lock();

	// search the event id..
	EventIdSet::iterator it = m_eventIds.find(eventid);
	if(it != m_eventIds.end()) {
		// if it is found erase from the list
		m_eventIds.erase(it);
		m_eventLock.unlock();
		return true;
	}
	else{
		// this eventid is not valid
		m_eventLock.unlock();
		return false;
	}
}

void Scheduler::stop()
{
	#ifdef __DEBUG_SCHEDULER__
	std::cout << "Stopping Scheduler" << std::endl;
	#endif
	m_eventLock.lock();
	m_threadState = Scheduler::STATE_CLOSING;
	m_eventLock.unlock();
}

void Scheduler::shutdown()
{
	#ifdef __DEBUG_SCHEDULER__
	std::cout << "Shutdown Scheduler" << std::endl;
	#endif
	m_eventLock.lock();
	m_threadState = Scheduler::STATE_TERMINATED;

	//this list should already be empty
	while(!m_eventList.empty()){
		m_eventList.pop();
	}
	m_eventIds.clear();
	m_eventLock.unlock();
}

