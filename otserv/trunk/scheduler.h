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


#ifndef __OTSERV_SCHEDULER_H
#define __OTSERV_SCHEDULER_H

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

	virtual ~SchedulerTask() { };

	// definition to make sure lower cycles end up front
	// in the priority_queue used in the scheduler
	inline bool operator<(const SchedulerTask& other) const {
		return getCycle() > other.getCycle();
	}

	virtual void operator()(Game* arg) = 0;

	virtual void setEventId(unsigned long id) {
		_eventid = id;
	}

	inline unsigned long getEventId() const {
		return _eventid;
	}

	virtual void setTicks(const __int64 ticks) {
		_cycle = OTSYS_TIME() + ticks;
	}

	inline __int64 getCycle() const {
		return _cycle;
	}

protected:
	unsigned long _eventid;
	__int64 _cycle;
};

class TSchedulerTask : public SchedulerTask {
public:
	TSchedulerTask(boost::function1<void, Game*> f) : _f(f) {
	}

	virtual void operator()(Game* arg) {
		_f(arg);
	}

	virtual ~TSchedulerTask() { }

protected:
	boost::function1<void, Game*> _f;
};

SchedulerTask* makeTask(boost::function1<void, Game*> f);
SchedulerTask* makeTask(__int64 ticks, boost::function1<void, Game*> f);

class lessSchedTask : public std::binary_function<SchedulerTask*, SchedulerTask*, bool> {
public:
	bool operator()(SchedulerTask*& t1, SchedulerTask*& t2) {
		return *t1 < *t2;
	}
};


template<class ArgType>
class TCallList : public SchedulerTask {
public:
	TCallList(boost::function<int(Game*, ArgType)> f1, boost::function<bool(Game*)> f2, std::list<ArgType>& call_list, __int64 interval) :
	_f1(f1), _f2(f2), _list(call_list), _interval(interval) {
	}
	
	void operator()(Game* arg) {
		if(_eventid != 0 && !_f2(arg)) {
			int ret = _f1(arg, _list.front());
			_list.pop_front();
			if (ret && !_list.empty()) {
				SchedulerTask* newTask = new TCallList(_f1, _f2, _list, _interval);
				newTask->setTicks(_interval);
				newTask->setEventId(this->getEventId());
				arg->addEvent(newTask);
			}
		}

		return;
	}

private:
	boost::function<int(Game*, ArgType)> _f1;
	boost::function<bool(Game*)>_f2;
	std::list<ArgType> _list;
	__int64 _interval;
};

template<class ArgType>
SchedulerTask* makeTask(__int64 ticks, boost::function<int(Game*, ArgType)> f1, std::list<ArgType>& call_list, __int64 interval, boost::function<bool(Game*)> f2) {
	TCallList<ArgType> *t = new TCallList<ArgType>(f1, f2, call_list, interval);
	t->setTicks(ticks);
	return t;
}

#endif
