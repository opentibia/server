////////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
////////////////////////////////////////////////////////////////////////
// Global events.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
////////////////////////////////////////////////////////////////////////

#ifndef __OTSERV_GLOBALEVENT_H__
#define __OTSERV_GLOBALEVENT_H__

#include "baseevents.h"
#include "const.h"

#define TIMER_INTERVAL 1000

enum GlobalEvent_t
{
	GLOBALEVENT_NONE,
	GLOBALEVENT_TIMER,
	GLOBALEVENT_STARTUP,
	GLOBALEVENT_SHUTDOWN
};

class GlobalEvent;
typedef std::map<std::string, GlobalEvent*> GlobalEventMap;

class GlobalEvents : public BaseEvents
{
public:
	GlobalEvents();
	virtual ~GlobalEvents();

	void startup();
	void timer();
	void think();
	void execute(const GlobalEvent_t& type);

	GlobalEventMap getEventMap(const GlobalEvent_t& type);
	void clearMap(GlobalEventMap& map);

protected:
	virtual const std::string& getScriptBaseName() const;
	virtual void clear();

	virtual Event* getEvent(const std::string& nodeName);
	virtual bool registerEvent(Event* event, xmlNodePtr p);

	virtual LuaScriptInterface& getScriptInterface();
	LuaScriptInterface m_scriptInterface;

	GlobalEventMap thinkMap;
	GlobalEventMap serverMap;
	GlobalEventMap timerMap;
	int32_t thinkEventId;
	int32_t timerEventId;
};

class GlobalEvent : public Event
{
public:
	GlobalEvent(LuaScriptInterface* _interface);
	virtual ~GlobalEvent();

	virtual bool configureEvent(xmlNodePtr p);

	uint32_t executeEvent();

	const GlobalEvent_t& getEventType() const;
	const std::string& getName() const;

	const uint32_t& getInterval() const;

	const int64_t& getLastExecution() const;
	void setLastExecution(const int64_t& time);

	const time_t& getNextExecution() const;
	void setNextExecution(const time_t& time);

protected:
	virtual const std::string& getScriptEventName() const;

	std::string m_name;
	time_t m_nextExecution;
	int64_t m_lastExecution;
	uint32_t m_interval;
	GlobalEvent_t m_eventType;
};

#endif // __OTSERV_GLOBALEVENT_H__
