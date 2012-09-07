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

#include "tasks.h"
#include "otsystem.h"
#include <queue>
#include <set>

#define SCHEDULER_MINTICKS 20

class SchedulerTask : public Task
{
public:
	~SchedulerTask() {}

	void setEventId(uint32_t eventid) {m_eventid = eventid;}
	uint32_t getEventId() const {return m_eventid;}

	boost::system_time getCycle() const {return m_expiration;}

	bool operator<(const SchedulerTask& other) const
	{
		return getCycle() > other.getCycle();
	}

protected:

	SchedulerTask(uint32_t delay, const boost::function<void (void)>& f) : Task(delay, f) {
		m_eventid = 0;
	}

	uint32_t m_eventid;

	friend SchedulerTask* createSchedulerTask(uint32_t, const boost::function<void (void)>&);
};

inline SchedulerTask* createSchedulerTask(uint32_t delay, const boost::function<void (void)>& f)
{
	assert(delay != 0);
	if(delay < SCHEDULER_MINTICKS){
		delay = SCHEDULER_MINTICKS;
	}
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
	Scheduler();
	~Scheduler() {}

	uint32_t addEvent(SchedulerTask* task);
	bool stopEvent(uint32_t eventId);

	void start();
	void stop();
	void shutdown();
	void shutdownAndWait();

	enum SchedulerState{
		STATE_RUNNING,
		STATE_CLOSING,
		STATE_TERMINATED
	};

protected:
	static void schedulerThread(void* p);

	boost::thread m_thread;
	boost::mutex m_eventLock;
	boost::condition_variable m_eventSignal;

	uint32_t m_lastEventId;
	std::priority_queue<SchedulerTask*, std::vector<SchedulerTask*>, lessSchedTask > m_eventList;
	typedef std::set<uint32_t> EventIdSet;
	EventIdSet m_eventIds;
	SchedulerState m_threadState;
};

extern Scheduler g_scheduler;

#endif

