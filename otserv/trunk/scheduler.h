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

#include <functional>
#include "otsystem.h"

class Game;

class SchedulerTask : public std::unary_function<Game*, int> {
		  public:
					 inline __int64 getCycle() const {
								return _cycle;
					 }

					 // definition to make sure lower cycles end up front
					 // in the priority_queue used in the scheduler
					 inline bool operator<(const SchedulerTask& other) const {
								return getCycle() > other.getCycle();
					 }

					 virtual result_type operator()(const argument_type&) = 0;

					 virtual void setTicks(const __int64 ticks) {
								_cycle = OTSYS_TIME() + ticks;;
					 }
		  protected:
					 __int64 _cycle;
};

template<class Functor>
class TSchedulerTask : public SchedulerTask {
		  public:
					 TSchedulerTask(Functor f) : _f(f) { }

					 virtual result_type operator()(const argument_type& arg) {
								_f(arg);
                return 0;
					 }

		  protected:
					 Functor _f;
};

template<class Functor>
TSchedulerTask<Functor>* makeTask(Functor f) {
		  return new TSchedulerTask<Functor>(f);
}

template<class Functor>
TSchedulerTask<Functor>* makeTask(__int64 ticks, Functor f) {
		  TSchedulerTask<Functor> *t = new TSchedulerTask<Functor>(f);
		  t->setTicks(ticks);
		  return t;
}

class lessSchedTask : public std::binary_function<SchedulerTask*, SchedulerTask*, bool> {
		  public:
		  bool operator()(SchedulerTask*& t1, SchedulerTask*& t2) {
					 return *t1 < *t2;
		  }
};

template<class Functor, class Functor2,  class Arg>
class TCallList : public SchedulerTask {
		  public:
					 TCallList(Functor f, Functor2 f2, std::list<Arg>& call_list, __int64 interval) : _f(f), _f2(f2), _list(call_list), _interval(interval) {
					 }

					 result_type operator()(const argument_type& arg) {
                              if(!_f2(arg)){   
								result_type ret = _f(arg, _list.front());
								_list.pop_front();
								if (!_list.empty()) {
										  SchedulerTask* newtask = new TCallList<Functor, Functor2, Arg>(_f, _f2, _list, _interval);
										  newtask->setTicks(_interval);
										  arg->addEvent(newtask);
								}
								return ret;
                          }	
					 }
		  protected:
					 Functor _f;
					 Functor2 _f2;
					 std::list<Arg> _list;
					 __int64 _interval;
};

template<class Functor, class Functor2, class Arg>
SchedulerTask* makeTask(__int64 ticks, Functor f, std::list<Arg>& call_list, __int64 interval, Functor2 f2) {
		  TCallList<Functor, Functor2, Arg> *t = new TCallList<Functor, Functor2, Arg>(f, f2, call_list, interval);
		  t->setTicks(ticks);
		  return t;
}

#endif
