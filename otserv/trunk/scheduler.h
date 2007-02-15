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
#include <functional>
#include "otsystem.h"

class Game;

class SchedulerTask {
public:
	SchedulerTask() {
		_eventid = 0;
		_cycle = 0;
  }

	virtual ~SchedulerTask()
	{
		//
	}

	// definition to make sure lower cycles end up front
	// in the priority_queue used in the scheduler
	inline bool operator<(const SchedulerTask& other) const
	{
		return getCycle() > other.getCycle();
	}

	virtual void operator()(Game* arg) = 0;

	virtual void setEventId(uint32_t id)
	{
		_eventid = id;
	}

	inline uint32_t getEventId() const
	{
		return _eventid;
	}

	virtual void setTicks(const int64_t ticks)
	{
		_cycle = OTSYS_TIME() + ticks;
	}

	inline int64_t getCycle() const
	{
		return _cycle;
	}

protected:
	uint32_t _eventid;
	int64_t _cycle;
};

class TSchedulerTask : public SchedulerTask{
public:
	TSchedulerTask(boost::function1<void, Game*> f) :
		_f(f)
	{
		//
	}

	virtual ~TSchedulerTask()
	{
		//
	}

	virtual void operator()(Game* arg)
	{
		_f(arg);
	}

protected:
	boost::function1<void, Game*> _f;
};

inline SchedulerTask* makeTask(boost::function1<void, Game*> f) {return new TSchedulerTask(f);}
inline SchedulerTask* makeTask(int64_t ticks, boost::function1<void, Game*> f){
	SchedulerTask* ret = new TSchedulerTask(f);
	//ret->setEventId(0);
	ret->setTicks(ticks);
	return ret;
}

class lessSchedTask : public std::binary_function<SchedulerTask*, SchedulerTask*, bool>{
public:
	bool operator()(SchedulerTask*& t1, SchedulerTask*& t2)
	{
		return *t1 < *t2;
	}
};

#endif
