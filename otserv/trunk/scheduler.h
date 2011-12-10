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

#include "definitions.h"
#include "tasks.h"
#include "otsystem.h"
#include <boost/bind.hpp>
#include <vector>
#include <queue>
#include <set>

#define SCHEDULER_MINTICKS 50

class SchedulerTask : public Task
{
protected:
	SchedulerTask(const uint32_t& delay, const boost::function<void (void)>& f);

public:
	virtual ~SchedulerTask();

	void setEventId(const uint32_t& eventid);
	const uint32_t& getEventId() const;

	const boost::system_time& getCycle() const;
	bool operator<(const SchedulerTask& other) const;

protected:
	uint32_t m_eventid;

	friend SchedulerTask* createSchedulerTask(const uint32_t&, const boost::function<void (void)>&);
};

inline SchedulerTask* createSchedulerTask(const uint32_t& delay, const boost::function<void (void)>& f)
{
	assert(delay != 0);

	if (delay < SCHEDULER_MINTICKS)
	{
		return new SchedulerTask(SCHEDULER_MINTICKS, f);
	}

	return new SchedulerTask(delay, f);
}

class Scheduler
{
public:
	Scheduler();

	uint32_t addEvent(SchedulerTask* task);
	bool stopEvent(uint32_t eventId);

	void start();
	void stop();
	void shutdown();

	enum SchedulerState
	{
		STATE_RUNNING,
		STATE_CLOSING,
		STATE_TERMINATED
	};

protected:

	struct lessSchedTask : public std::binary_function<SchedulerTask*&, SchedulerTask*&, bool>
	{
		bool operator()(SchedulerTask*& t1, SchedulerTask*& t2) const
		{
			return (*t1) < (*t2);
		}
	};

	static void schedulerThread(void* p);

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
