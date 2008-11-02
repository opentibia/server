//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
//
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

#include <sstream>
#include "creatureevent.h"
#include "tools.h"
#include "player.h"

CreatureEvents::CreatureEvents() :
m_scriptInterface("CreatureScript Interface")
{
	m_scriptInterface.initState();
	m_logInEvent = NULL;
	m_logOutEvent = NULL;
}

CreatureEvents::~CreatureEvents()
{
	delete m_logInEvent;
	delete m_logOutEvent;

	CreatureEventList::iterator it;
	for(it = m_creatureEvents.begin(); it != m_creatureEvents.end(); ++it){
		delete it->second;
	}
}

void CreatureEvents::clear()
{
	//clear global events
	if(m_logInEvent){
		m_logInEvent->clearEvent();
		delete m_logInEvent;
		m_logInEvent = NULL;
	}
	if(m_logOutEvent){
		m_logOutEvent->clearEvent();
		delete m_logOutEvent;
		m_logOutEvent = NULL;
	}
	//clear creature events
	CreatureEventList::iterator it;
	for(it = m_creatureEvents.begin(); it != m_creatureEvents.end(); ++it){
		it->second->clearEvent();
	}
	//clear lua state
	m_scriptInterface.reInitState();
}

LuaScriptInterface& CreatureEvents::getScriptInterface()
{
	return m_scriptInterface;
}

std::string CreatureEvents::getScriptBaseName()
{
	return "creaturescripts";
}

Event* CreatureEvents::getEvent(const std::string& nodeName)
{
	if(asLowerCaseString(nodeName) == "event"){
		return new CreatureEvent(&m_scriptInterface);
	}
	return NULL;
}

bool CreatureEvents::registerEvent(Event* event, xmlNodePtr p)
{
	CreatureEvent* creatureEvent = dynamic_cast<CreatureEvent*>(event);
	if(!creatureEvent){
		return false;
	}

	switch(creatureEvent->getEventType()){
	case CREATURE_EVENT_NONE:
		std::cout << "Error: [CreatureEvents::registerEvent] Trying to register event without type!." << std::endl;
		return false;
		break;
	// global events are stored in
	// member variables and their name is ignored!
	case CREATURE_EVENT_LOGIN:
		delete m_logInEvent;
		m_logInEvent = creatureEvent;
		return true;
	case CREATURE_EVENT_LOGOUT:
		delete m_logOutEvent;
		m_logOutEvent = creatureEvent;
		return true;
	// other events are stored in a std::map
	default:
		{
			CreatureEvent* oldEvent = getEventByName(creatureEvent->getName(), false);
			if(oldEvent){
				// if there was an event with the same that is not loaded
				// (happens when realoading), it is reused
				if(oldEvent->isLoaded() == false &&
					oldEvent->getEventType() == creatureEvent->getEventType()){
					oldEvent->copyEvent(creatureEvent);
				}
				return false;
			}
			else{
				//if not, register it normally
				m_creatureEvents[creatureEvent->getName()] = creatureEvent;
				return true;
			}
		}
	}
}

CreatureEvent* CreatureEvents::getEventByName(const std::string& name, bool forceLoaded /*= true*/)
{
	CreatureEventList::iterator it = m_creatureEvents.find(name);
	if(it != m_creatureEvents.end()){
		// After reloading, a creature can have script that was not
		// loaded again, if is the case, return NULL
		if(!forceLoaded || it->second->isLoaded()){
			return it->second;
		}
	}
	return NULL;
}

uint32_t CreatureEvents::playerLogIn(Player* player)
{
	// fire global event if is registered
	if(m_logInEvent){
		return m_logInEvent->executeOnLogin(player);
	}
	return 1;
}

uint32_t CreatureEvents::playerLogOut(Player* player)
{
	// fire global event if is registered
	if(m_logOutEvent){
		return m_logOutEvent->executeOnLogout(player);
	}
	return 1;
}

/////////////////////////////////////

CreatureEvent::CreatureEvent(LuaScriptInterface* _interface) :
Event(_interface)
{
	m_type = CREATURE_EVENT_NONE;
	m_isLoaded = false;
}

CreatureEvent::~CreatureEvent()
{
	//
}

bool CreatureEvent::configureEvent(xmlNodePtr p)
{
	std::string str;
	//Name that will be used in monster xml files and
	// lua function to register events to reference this event
	if(readXMLString(p, "name", str)){
		m_eventName = str;
	}
	else{
		std::cout << "Error: [CreatureEvent::configureEvent] No name for creature event." << std::endl;
		return false;
	}

	if(readXMLString(p, "type", str)){
		if(asLowerCaseString(str) == "login"){
			m_type = CREATURE_EVENT_LOGIN;
		}
		else if(asLowerCaseString(str) == "logout"){
			m_type = CREATURE_EVENT_LOGOUT;
		}
		else if(asLowerCaseString(str) == "die"){
			m_type = CREATURE_EVENT_DIE;
		}
		else if(asLowerCaseString(str) == "kill"){
			m_type = CREATURE_EVENT_KILL;
		}
		else{
			std::cout << "Error: [CreatureEvent::configureEvent] No valid type for creature event." << str << std::endl;
			return false;
		}
	}
	else{
		std::cout << "Error: [CreatureEvent::configureEvent] No type for creature event."  << std::endl;
		return false;
	}
	m_isLoaded = true;
	return true;
}

std::string CreatureEvent::getScriptEventName()
{
	//Depending on the type script event name is different
	switch(m_type){
	case CREATURE_EVENT_LOGIN:
		return "onLogin";
		break;
	case CREATURE_EVENT_LOGOUT:
		return "onLogout";
		break;
	case CREATURE_EVENT_DIE:
		return "onDie";
		break;
	case CREATURE_EVENT_KILL:
		return "onKill";
		break;

	case CREATURE_EVENT_NONE:
	default:
		return "";
		break;
	}
}

void CreatureEvent::copyEvent(CreatureEvent* creatureEvent)
{
	m_scriptId = creatureEvent->m_scriptId;
	m_scriptInterface = creatureEvent->m_scriptInterface;
	m_scripted = creatureEvent->m_scripted;
	m_isLoaded = creatureEvent->m_isLoaded;
}

void CreatureEvent::clearEvent()
{
	m_scriptId = 0;
	m_scriptInterface = NULL;
	m_scripted = false;
	m_isLoaded = false;
}

uint32_t CreatureEvent::executeOnLogin(Player* player)
{
	//onLogin(cid)
	if(m_scriptInterface->reserveScriptEnv()){
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();

		#ifdef __DEBUG_LUASCRIPTS__
		std::stringstream desc;
		desc << player->getName();
		env->setEventDesc(desc.str());
		#endif

		env->setScriptId(m_scriptId, m_scriptInterface);
		env->setRealPos(player->getPosition());

		uint32_t cid = env->addThing(player);

		lua_State* L = m_scriptInterface->getLuaState();

		m_scriptInterface->pushFunction(m_scriptId);
		lua_pushnumber(L, cid);

		int32_t result = m_scriptInterface->callFunction(1);
		m_scriptInterface->releaseScriptEnv();

		return (result == LUA_TRUE);
	}
	else{
		std::cout << "[Error] Call stack overflow. CreatureEvent::executeOnLogin" << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeOnLogout(Player* player)
{
	//onLogout(cid)
	if(m_scriptInterface->reserveScriptEnv()){
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();

		#ifdef __DEBUG_LUASCRIPTS__
		std::stringstream desc;
		desc << player->getName();
		env->setEventDesc(desc.str());
		#endif

		env->setScriptId(m_scriptId, m_scriptInterface);
		env->setRealPos(player->getPosition());

		uint32_t cid = env->addThing(player);

		lua_State* L = m_scriptInterface->getLuaState();

		m_scriptInterface->pushFunction(m_scriptId);
		lua_pushnumber(L, cid);

		int32_t result = m_scriptInterface->callFunction(1);
		m_scriptInterface->releaseScriptEnv();

		return (result != LUA_FALSE);
	}
	else{
		std::cout << "[Error] Call stack overflow. CreatureEvent::executeOnLogout" << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeOnDie(Creature* creature, Item* corpse)
{
	//onDie(cid, corpse)
	if(m_scriptInterface->reserveScriptEnv()){
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();

		#ifdef __DEBUG_LUASCRIPTS__
		std::stringstream desc;
		desc << creature->getName();
		env->setEventDesc(desc.str());
		#endif

		env->setScriptId(m_scriptId, m_scriptInterface);
		env->setRealPos(creature->getPosition());

		uint32_t cid = env->addThing(creature);
		uint32_t corpseid = env->addThing(corpse);

		lua_State* L = m_scriptInterface->getLuaState();

		m_scriptInterface->pushFunction(m_scriptId);
		lua_pushnumber(L, cid);
		lua_pushnumber(L, corpseid);

		int32_t result = m_scriptInterface->callFunction(2);
		m_scriptInterface->releaseScriptEnv();

		return (result != LUA_FALSE);
	}
	else{
		std::cout << "[Error] Call stack overflow. CreatureEvent::executeOnDie" << std::endl;
		return 0;
	}
}

uint32_t CreatureEvent::executeOnKill(Creature* creature, Creature* target)
{
	//onKill(cid, target)
	if(m_scriptInterface->reserveScriptEnv()){
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();

		#ifdef __DEBUG_LUASCRIPTS__
		std::stringstream desc;
		desc << creature->getName();
		env->setEventDesc(desc.str());
		#endif

		env->setScriptId(m_scriptId, m_scriptInterface);
		env->setRealPos(creature->getPosition());

		uint32_t cid = env->addThing(creature);
		uint32_t targetId = env->addThing(target);

		lua_State* L = m_scriptInterface->getLuaState();

		m_scriptInterface->pushFunction(m_scriptId);
		lua_pushnumber(L, cid);
		lua_pushnumber(L, targetId);

		int32_t result = m_scriptInterface->callFunction(2);
		m_scriptInterface->releaseScriptEnv();

		return (result != LUA_FALSE);
	}
	else{
		std::cout << "[Error] Call stack overflow. CreatureEvent::executeOnKill" << std::endl;
		return 0;
	}
}
