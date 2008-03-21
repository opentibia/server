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

#ifndef __OTSERV_SCHEDULER_H__
#define __OTSERV_SCHEDULER_H__

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <vector>
#include <queue>
#include <set>

#include "otsystem.h"
#include "tasks.h"

class SchedulerTask : public Task
{
public:
	~SchedulerTask() {}
		
	void setEventId(uint32_t eventid) {m_eventid = eventid;}
	uint32_t getEventId() const {return m_eventid;}
	
	uint64_t getCycle() const {return m_cycle;}
	
	bool operator<(const SchedulerTask& other) const
	{
		return getCycle() > other.getCycle();
	}
	
protected:
	
	SchedulerTask(uint32_t delay, boost::function<void (void)> f) : Task(f) {
		m_cycle = OTSYS_TIME() + delay;
		m_eventid = 0;
	}
	
	uint64_t m_cycle;
	uint32_t m_eventid;
	
	friend SchedulerTask* createSchedulerTask(uint32_t, boost::function<void (void)>);
};

inline SchedulerTask* createSchedulerTask(uint32_t delay, boost::function<void (void)> f)
{
	assert(delay != 0);
	return new SchedulerTask(delay, f);
}

class lessSchedTask : public std::binary_function<SchedulerTask*&, SchedulerTask*&, bool>
{
public:
	bool operator()(SchedulerTask*& t1, SchedulerTask*& t2)
	{
		return (*t1) < (*t2);
	}
};

class Scheduler
{
public:
	~Scheduler() {}
	
	static Scheduler& getScheduler()
	{
		static Scheduler scheduler;
		return scheduler;
	}
	
	uint32_t addEvent(SchedulerTask* task);
	bool stopEvent(uint32_t eventId);
	void stop();
	void shutdown();
	
	static OTSYS_THREAD_RETURN schedulerThread(void* p);
	
protected:
	Scheduler();

	OTSYS_THREAD_LOCKVAR m_eventLock;
	OTSYS_THREAD_SIGNALVAR m_eventSignal;

	uint32_t m_lastEventId;
	std::priority_queue<SchedulerTask*, std::vector<SchedulerTask*>, lessSchedTask > m_eventList;
	typedef std::set<uint32_t> EventIdSet;
	EventIdSet m_eventIds;
	static bool m_shutdown;
};


#endif
