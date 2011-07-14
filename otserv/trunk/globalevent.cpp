////////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
////////////////////////////////////////////////////////////////////////
// Global events
////////////////////////////////////////////////////////////////////////
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
#include "otpch.h"

#include "globalevent.h"
#include "player.h"
#include "scheduler.h"

#ifdef __DEBUG_LUASCRIPTS__
#include <sstream>
#endif
#include <cmath>

GlobalEvents::GlobalEvents() :
	m_scriptInterface("GlobalEvent Interface")
{
	m_scriptInterface.initState();
	thinkEventId = 0;
	timerEventId = 0;
}

GlobalEvents::~GlobalEvents()
{
	clear();
}

void GlobalEvents::clearMap(GlobalEventMap& map)
{
	for(GlobalEventMap::iterator it = map.begin(); it != map.end(); ++it)
		delete it->second;

	map.clear();
}

void GlobalEvents::clear()
{
	g_scheduler.stopEvent(thinkEventId);
	thinkEventId = 0;
	g_scheduler.stopEvent(timerEventId);
	timerEventId = 0;

	clearMap(thinkMap);
	clearMap(serverMap);
	clearMap(timerMap);

	m_scriptInterface.reInitState();
}

Event* GlobalEvents::getEvent(const std::string& nodeName)
{
	if(asLowerCaseString(nodeName) == "globalevent")
		return new GlobalEvent(&m_scriptInterface);

	return NULL;
}

bool GlobalEvents::registerEvent(Event* event, xmlNodePtr)
{
	GlobalEvent* globalEvent = dynamic_cast<GlobalEvent*>(event);
	if(!globalEvent)
		return false;

	if(globalEvent->getEventType() == GLOBALEVENT_TIMER)
	{
		GlobalEventMap::iterator it = timerMap.find(globalEvent->getName());
		if(it == timerMap.end())
		{
			timerMap.insert(std::make_pair(globalEvent->getName(), globalEvent));
			if(timerEventId == 0)
			{
				timerEventId = g_scheduler.addEvent(createSchedulerTask(TIMER_INTERVAL,
					boost::bind(&GlobalEvents::timer, this)));
			}
			return true;
		}
	}
	else if(globalEvent->getEventType() != GLOBALEVENT_NONE)
	{
		GlobalEventMap::iterator it = serverMap.find(globalEvent->getName());
		if(it == serverMap.end())
		{
			serverMap.insert(std::make_pair(globalEvent->getName(), globalEvent));
			return true;
		}
	}
	else // think event
	{
		GlobalEventMap::iterator it = thinkMap.find(globalEvent->getName());
		if(it == thinkMap.end())
		{
			thinkMap.insert(std::make_pair(globalEvent->getName(), globalEvent));
			if(thinkEventId == 0)
			{
				thinkEventId = g_scheduler.addEvent(createSchedulerTask(SCHEDULER_MINTICKS,
					boost::bind(&GlobalEvents::think, this)));
			}
			return true;
		}
	}

	std::cout << "[Warning - GlobalEvents::configureEvent] Duplicate registered globalevent with name: " << globalEvent->getName() << std::endl;
	return false;
}

void GlobalEvents::startup()
{
	execute(GLOBALEVENT_STARTUP);
}

void GlobalEvents::timer()
{
	time_t now = time(NULL);
	for(GlobalEventMap::iterator it = timerMap.begin(); it != timerMap.end(); ++it)
	{
		if(it->second->getNextExecution() > now)
			continue;

		it->second->setNextExecution(it->second->getNextExecution() + 86400);
		if(!it->second->executeEvent())
			std::cout << "[Error - GlobalEvents::timer] Failed to execute event: " << it->second->getName() << std::endl;
	}

	g_scheduler.addEvent(createSchedulerTask(TIMER_INTERVAL,
		boost::bind(&GlobalEvents::timer, this)));
}

void GlobalEvents::think()
{
	int64_t now = OTSYS_TIME();
	for(GlobalEventMap::iterator it = thinkMap.begin(); it != thinkMap.end(); ++it)
	{
		if((it->second->getLastExecution() + it->second->getInterval()) > now)
			continue;

		it->second->setLastExecution(now);
		if(!it->second->executeEvent())
			std::cout << "[Error - GlobalEvents::think] Failed to execute event: " << it->second->getName() << std::endl;
	}

	g_scheduler.addEvent(createSchedulerTask(SCHEDULER_MINTICKS,
		boost::bind(&GlobalEvents::think, this)));
}

void GlobalEvents::execute(GlobalEvent_t type)
{
	for(GlobalEventMap::iterator it = serverMap.begin(); it != serverMap.end(); ++it)
	{
		if(it->second->getEventType() == type)
			it->second->executeEvent();
	}
}

GlobalEventMap GlobalEvents::getEventMap(GlobalEvent_t type)
{
	switch(type)
	{
		case GLOBALEVENT_NONE:
			return thinkMap;

		case GLOBALEVENT_TIMER:
			return timerMap;

		case GLOBALEVENT_STARTUP:
		case GLOBALEVENT_SHUTDOWN:

		default:
			break;
	}

	return GlobalEventMap();
}

GlobalEvent::GlobalEvent(LuaScriptInterface* _interface):
	Event(_interface)
{
	m_lastExecution = OTSYS_TIME();
	m_nextExecution = 0;
	m_interval = 0;
}

bool GlobalEvent::configureEvent(xmlNodePtr p)
{
	std::string strValue;
	if(!readXMLString(p, "name", strValue))
	{
		std::cout << "[Error - GlobalEvent::configureEvent] No name for a globalevent." << std::endl;
		return false;
	}

	m_name = strValue;
	m_eventType = GLOBALEVENT_NONE;
	if(readXMLString(p, "type", strValue))
	{
		std::string tmpStrValue = asLowerCaseString(strValue);
		if(tmpStrValue == "startup" || tmpStrValue == "start" || tmpStrValue == "load")
			m_eventType = GLOBALEVENT_STARTUP;
		else if(tmpStrValue == "shutdown" || tmpStrValue == "quit" || tmpStrValue == "exit")
			m_eventType = GLOBALEVENT_SHUTDOWN;
		else
		{
			std::cout << "[Error - GlobalEvent::configureEvent] No valid type \"" << strValue << "\" for globalevent with name " << m_name << std::endl;
			return false;
		}

		return true;
	}
	else if(readXMLString(p, "time", strValue) || readXMLString(p, "at", strValue))
	{
		std::vector<std::string> params = explodeString(strValue, ":");
		int32_t atoiparams0 = atoi(params[0].c_str());
		if(atoiparams0 < 0 || atoiparams0 > 23)
		{
			std::cout << "[Error - GlobalEvent::configureEvent] No valid hour \"" << strValue << "\" for globalevent with name " << m_name << std::endl;
			return false;
		}

		m_interval |= atoiparams0 << 16;
		int32_t hour = atoiparams0;
		int32_t min = 0;
		int32_t sec = 0;
		if(params.size() > 1)
		{
			int32_t atoiparams1 = atoi(params[1].c_str());
			if(atoiparams1 < 0 || atoiparams1 > 59)
			{
				std::cout << "[Error - GlobalEvent::configureEvent] No valid minute \"" << strValue << "\" for globalevent with name " << m_name << std::endl;
				return false;
			}

			min = atoiparams1;
			if(params.size() > 2)
			{
				int32_t atoiparams2 = atoi(params[2].c_str());
				if(atoiparams2 < 0 || atoiparams2 > 59)
				{
					std::cout << "[Error - GlobalEvent::configureEvent] No valid second \"" << strValue << "\" for globalevent with name " << m_name << std::endl;
					return false;
				}

				sec = atoiparams2;
			}
		}

		time_t current_time = time(NULL);
		tm* timeinfo = localtime(&current_time);
		timeinfo->tm_hour = hour;
		timeinfo->tm_min = min;
		timeinfo->tm_sec = sec;
		time_t difference = (time_t)difftime(mktime(timeinfo), current_time);
		if(difference < 0)
			difference += 86400;

		m_nextExecution = current_time + difference;
		m_eventType = GLOBALEVENT_TIMER;
		return true;
	}

	int32_t intValue;
	if(readXMLInteger(p, "interval", intValue))
	{
		m_interval = std::max((int32_t)SCHEDULER_MINTICKS, intValue);
		return true;
	}

	std::cout << "[Error - GlobalEvent::configureEvent] No interval for globalevent with name " << m_name << std::endl;
	return false;
}

std::string GlobalEvent::getScriptEventName()
{
	switch(m_eventType)
	{
		case GLOBALEVENT_STARTUP:
			return "onStartup";
		case GLOBALEVENT_SHUTDOWN:
			return "onShutdown";
		case GLOBALEVENT_TIMER:
			return "onTime";
		default:
			break;
	}

	return "onThink";
}

uint32_t GlobalEvent::executeEvent()
{
	if(m_scriptInterface->reserveScriptEnv())
	{
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();

		env->setScriptId(m_scriptId, m_scriptInterface);
		lua_State* L = m_scriptInterface->getLuaState();
		m_scriptInterface->pushFunction(m_scriptId);

		int32_t params = 0;
		if(m_eventType == GLOBALEVENT_NONE || m_eventType == GLOBALEVENT_TIMER)
		{
			lua_pushnumber(L, m_interval);
			params = 1;
		}

		bool result = m_scriptInterface->callFunction(params);
		m_scriptInterface->releaseScriptEnv();
		return result;
	}
	else
	{
		std::cout << "[Error - GlobalEvent::executeEvent] Call stack overflow." << std::endl;
		return 0;
	}
}
