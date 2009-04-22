//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Lua script interface
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

#include <string>
#include <iostream>
#include <iomanip>

#include "luascript.h"
#include "player.h"
#include "item.h"
#include "game.h"
#include "house.h"
#include "housetile.h"
#include "status.h"
#include "combat.h"
#include "spells.h"
#include "condition.h"
#include "monsters.h"
#include "baseevents.h"
#include "iologindata.h"
#include "configmanager.h"
#include "town.h"
#include "vocation.h"
#include "teleport.h"
#include "ioban.h"
#include "raids.h"

extern Game g_game;
extern Monsters g_monsters;
extern ConfigManager g_config;
extern Spells* g_spells;

enum
{
	EVENT_ID_LOADING = 1,
	EVENT_ID_USER = 1000,
};

ScriptEnviroment::AreaMap ScriptEnviroment::m_areaMap;
uint32_t ScriptEnviroment::m_lastAreaId = 0;
ScriptEnviroment::CombatMap ScriptEnviroment::m_combatMap;
uint32_t ScriptEnviroment::m_lastCombatId = 0;
ScriptEnviroment::ConditionMap ScriptEnviroment::m_conditionMap;
uint32_t ScriptEnviroment::m_lastConditionId = 0;

ScriptEnviroment::ThingMap ScriptEnviroment::m_globalMap;
ScriptEnviroment::StorageMap ScriptEnviroment::m_globalStorageMap;

ScriptEnviroment::ScriptEnviroment()
{
	m_lastUID = 70000;
	m_loaded = true;
	resetEnv();
}

ScriptEnviroment::~ScriptEnviroment()
{
	for(CombatMap::iterator it = m_combatMap.begin(); it != m_combatMap.end(); ++it)
		delete it->second;

	m_combatMap.clear();
	for(AreaMap::iterator it = m_areaMap.begin(); it != m_areaMap.end(); ++it)
		delete it->second;

	m_areaMap.clear();
	resetEnv();
}

void ScriptEnviroment::resetEnv()
{
	m_scriptId = m_callbackId = 0;
	m_timerEvent = false;
	m_realPos = Position();
	m_interface = NULL;

	for(std::list<Item*>::iterator it = m_tempItems.begin(); it != m_tempItems.end(); ++it)
	{
		if((*it)->getParent() == VirtualCylinder::virtualCylinder)
			delete *it;
	}

	m_tempItems.clear();
	for(DBResMap::iterator it = m_tempResults.begin(); it != m_tempResults.end(); ++it)
		it->second->free();

	m_tempResults.clear();
	m_localMap.clear();
}

bool ScriptEnviroment::saveGameState()
{
	if(!g_config.getBool(ConfigManager::SAVE_GLOBAL_STORAGE))
		return true;

	Database* db = Database::getInstance();

	DBQuery query;
	query << "DELETE FROM `global_storage` WHERE `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID);
	if(!db->executeQuery(query.str()))
		return false;

	DBInsert query_insert(db);
	query_insert.setQuery("INSERT INTO `global_storage` (`key`, `world_id`, `value`) VALUES ");
	for(StorageMap::const_iterator it = m_globalStorageMap.begin(); it != m_globalStorageMap.end(); ++it)
	{
		char buffer[25 + it->second.length()];
		sprintf(buffer, "%u, %u, %s", it->first, g_config.getNumber(ConfigManager::WORLD_ID), db->escapeString(it->second).c_str());
		if(!query_insert.addRow(buffer))
			return false;
	}

	if(!query_insert.execute())
		return false;

	return true;
}

bool ScriptEnviroment::loadGameState()
{
	Database* db = Database::getInstance();
	DBResult* result;

	DBQuery query;
	query << "SELECT `key`, `value` FROM `global_storage` WHERE `world_id` = " << g_config.getNumber(ConfigManager::WORLD_ID);
	if((result = db->storeQuery(query.str())))
	{
		do
			m_globalStorageMap[result->getDataInt("key")] = result->getDataString("value");
		while(result->next());
		result->free();
	}

	query.str("");
	return true;
}

bool ScriptEnviroment::setCallbackId(int32_t callbackId, LuaScriptInterface* scriptInterface)
{
	if(m_callbackId == 0)
	{
		m_callbackId = callbackId;
		m_interface = scriptInterface;
		return true;
	}

	//nested callbacks are not allowed
	if(m_interface)
		m_interface->reportError(__FUNCTION__, "Nested callbacks!");

	return false;
}

void ScriptEnviroment::getEventInfo(int32_t& scriptId, std::string& desc, LuaScriptInterface*& scriptInterface, int32_t& callbackId, bool& timerEvent)
{
	scriptId = m_scriptId;
	desc = m_eventdesc;
	scriptInterface = m_interface;
	callbackId = m_callbackId;
	timerEvent = m_timerEvent;
}

void ScriptEnviroment::addUniqueThing(Thing* thing)
{
	Item* item = thing->getItem();
	if(item && item->getUniqueId() != 0)
	{
		int32_t uid = item->getUniqueId();
		Thing* tmp = m_globalMap[uid];
		if(tmp)
		{
			if(item->getActionId() != 2000)
				std::cout << "Duplicate uniqueId " << uid << std::endl;
		}
		else
			m_globalMap[uid] = thing;
	}
}

void ScriptEnviroment::removeUniqueThing(Thing* thing)
{
	Item* item = thing->getItem();
	if(item && item->getUniqueId() != 0)
	{
		int32_t uid = item->getUniqueId();
		ThingMap::iterator it = m_globalMap.find(uid);
		if(it != m_globalMap.end())
			m_globalMap.erase(it);
	}
}

uint32_t ScriptEnviroment::addThing(Thing* thing)
{
	if(!thing)
		return 0;

	for(ThingMap::iterator it = m_localMap.begin(); it != m_localMap.end(); ++it)
	{
		if(it->second == thing)
			return it->first;
	}

	uint32_t newUid;
	if(Creature* creature = thing->getCreature())
		newUid = creature->getID();
	else
	{
		if(Item* item = thing->getItem())
		{
			uint32_t uid = item->getUniqueId();
			if(uid && item->getTile() == item->getParent())
			{
				m_localMap[uid] = thing;
				return uid;
			}
		}

		++m_lastUID;
		if(m_lastUID < 70000)
			m_lastUID = 70000;

		while(m_localMap[m_lastUID])
			++m_lastUID;

		newUid = m_lastUID;
	}

	m_localMap[newUid] = thing;
	return newUid;
}

void ScriptEnviroment::insertThing(uint32_t uid, Thing* thing)
{
	if(!m_localMap[uid])
		m_localMap[uid] = thing;
	else
		std::cout << std::endl << "Lua Script Error: Thing uid already taken.";
}

Thing* ScriptEnviroment::getThingByUID(uint32_t uid)
{
	Thing* tmp = m_localMap[uid];
	if(tmp && !tmp->isRemoved())
		return tmp;

	tmp = m_globalMap[uid];
	if(tmp && !tmp->isRemoved())
		return tmp;

	if(uid >= 0x10000000)
	{
		tmp = g_game.getCreatureByID(uid);
		if(tmp && !tmp->isRemoved())
		{
			m_localMap[uid] = tmp;
			return tmp;
		}
	}

	return NULL;
}

Item* ScriptEnviroment::getItemByUID(uint32_t uid)
{
	if(Thing* tmp = getThingByUID(uid))
	{
		if(Item* item = tmp->getItem())
			return item;
	}

	return NULL;
}

Container* ScriptEnviroment::getContainerByUID(uint32_t uid)
{
	if(Item* tmp = getItemByUID(uid))
	{
		if(Container* container = tmp->getContainer())
			return container;
	}

	return NULL;
}

Creature* ScriptEnviroment::getCreatureByUID(uint32_t uid)
{
	if(Thing* tmp = getThingByUID(uid))
	{
		if(Creature* creature = tmp->getCreature())
			return creature;
	}

	return NULL;
}

Player* ScriptEnviroment::getPlayerByUID(uint32_t uid)
{
	if(Thing* tmp = getThingByUID(uid))
	{
		if(Creature* creature = tmp->getCreature())
		{
			if(Player* player = creature->getPlayer())
				return player;
		}
	}

	return NULL;
}

void ScriptEnviroment::removeItemByUID(uint32_t uid)
{
	ThingMap::iterator it;
	it = m_localMap.find(uid);
	if(it != m_localMap.end())
		m_localMap.erase(it);

	it = m_globalMap.find(uid);
	if(it != m_globalMap.end())
		m_globalMap.erase(it);
}

uint32_t ScriptEnviroment::addCombatArea(AreaCombat* area)
{
	uint32_t newAreaId = m_lastAreaId + 1;
	m_areaMap[newAreaId] = area;

	m_lastAreaId++;
	return newAreaId;
}

AreaCombat* ScriptEnviroment::getCombatArea(uint32_t areaId)
{
	AreaMap::const_iterator it = m_areaMap.find(areaId);
	if(it != m_areaMap.end())
		return it->second;

	return NULL;
}

uint32_t ScriptEnviroment::addCombatObject(Combat* combat)
{
	uint32_t newCombatId = m_lastCombatId + 1;
	m_combatMap[newCombatId] = combat;

	m_lastCombatId++;
	return newCombatId;
}

Combat* ScriptEnviroment::getCombatObject(uint32_t combatId)
{
	CombatMap::iterator it = m_combatMap.find(combatId);
	if(it != m_combatMap.end())
		return it->second;

	return NULL;
}

uint32_t ScriptEnviroment::addConditionObject(Condition* condition)
{
	uint32_t newConditionId = m_lastConditionId + 1;
	m_conditionMap[newConditionId] = condition;

	m_lastConditionId++;
	return m_lastConditionId;
}

Condition* ScriptEnviroment::getConditionObject(uint32_t conditionId)
{
	ConditionMap::iterator it = m_conditionMap.find(conditionId);
	if(it != m_conditionMap.end())
		return it->second;

	return NULL;
}

void ScriptEnviroment::addTempItem(Item* item)
{
	m_tempItems.push_back(item);
}

void ScriptEnviroment::removeTempItem(Item* item)
{
	ItemList::iterator it = std::find(m_tempItems.begin(), m_tempItems.end(), item);
	if(it != m_tempItems.end())
		m_tempItems.erase(it);
}

uint32_t ScriptEnviroment::addResult(DBResult* res)
{
	uint32_t lastId = 0;
	while(m_tempResults.find(lastId) != m_tempResults.end())
		lastId++;

	m_tempResults[lastId] = res;
	return lastId;
}

bool ScriptEnviroment::removeResult(uint32_t rid)
{
	DBResMap::iterator it = m_tempResults.find(rid);
	if(it == m_tempResults.end())
		return false;

	if(it->second)
		it->second->free();

	m_tempResults.erase(it);
	return true;
}

DBResult* ScriptEnviroment::getResult(uint32_t rid)
{
	DBResMap::iterator it = m_tempResults.find(rid);
	if(it != m_tempResults.end())
		return it->second;

	return NULL;
}

bool ScriptEnviroment::getGlobalStorageValue(const uint32_t key, std::string& value) const
{
	StorageMap::const_iterator it = m_globalStorageMap.find(key);
	if(it != m_globalStorageMap.end())
	{
		value = it->second;
		return true;
	}

	value = "-1";
	return false;
}

bool ScriptEnviroment::addGlobalStorageValue(const uint32_t key, const std::string& value)
{
	m_globalStorageMap[key] = value;
	return true;
}

bool ScriptEnviroment::eraseGlobalStorageValue(const uint32_t key)
{
	return m_globalStorageMap.erase(key);
}

void ScriptEnviroment::streamVariant(std::stringstream& stream, const std::string& local, const LuaVariant& var)
{
	stream << local << " = {" << std::endl;
	stream << "type = " << var.type;
	switch(var.type)
	{
		case VARIANT_NUMBER:
			stream << "," << std::endl << "number = " << var.number;
			break;
		case VARIANT_STRING:
			stream << "," << std::endl << "string = \"" << var.text << "\"";
			break;
		case VARIANT_TARGETPOSITION:
		case VARIANT_POSITION:
		{
			stream << "," << std::endl;
			streamPosition(stream, "pos", var.pos);
			break;
		}
		case VARIANT_NONE:
		default:
			break;
	}

	stream << std::endl << "}" << std::endl;
}

void ScriptEnviroment::streamThing(std::stringstream& stream, const std::string& local, Thing* thing, uint32_t thingId)
{
	stream << local << " = {" << std::endl;
	if(thing && thing->getItem())
	{
		const Item* item = thing->getItem();
		stream << "uid = " << thingId << "," << std::endl;
		stream << "itemid = " << item->getID() << "," << std::endl;

		if(item->hasSubType())
			stream << "type = " << item->getSubType() << "," << std::endl;
		else
			stream << "type = 0," << std::endl;

		stream << "actionid = " << item->getActionId() << std::endl;
	}
	else if(thing && thing->getCreature())
	{
		const Creature* creature = thing->getCreature();
		stream << "uid = " << thingId << "," << std::endl;
		stream << "itemid = 1," << std::endl;

		char type;
		if(creature->getPlayer())
			type = 1;
		else if(creature->getMonster())
			type = 2;
		else
			type = 3; //npc

		stream << "type = " << type << "," << std::endl;
		stream << "actionid = 0" << std::endl;
	}
	else
	{
		stream << "uid = 0," << std::endl;
		stream << "itemid = 0," << std::endl;
		stream << "type = 0," << std::endl;
		stream << "actionid = 0" << std::endl;
	}

	stream << "}" << std::endl;
}

void ScriptEnviroment::streamPosition(std::stringstream& stream, const std::string& local, const PositionEx& position)
{
	stream << local << " = {" << std::endl;
	stream << "x = " << position.x << "," << std::endl;
	stream << "y = " << position.y << "," << std::endl;
	stream << "z = " << position.z << "," << std::endl;
	stream << "stackpos = " << position.stackpos << std::endl;
	stream << "}" << std::endl;
}

void ScriptEnviroment::streamPosition(std::stringstream& stream, const std::string& local, const Position& position, uint32_t stackpos)
{
	stream << local << " = {" << std::endl;
	stream << "x = " << position.x << "," << std::endl;
	stream << "y = " << position.y << "," << std::endl;
	stream << "z = " << position.z << "," << std::endl;
	stream << "stackpos = " << stackpos << std::endl;
	stream << "}" << std::endl;
}

std::string LuaScriptInterface::getErrorDesc(ErrorCode_t code)
{
	switch(code)
	{
		case LUA_ERROR_PLAYER_NOT_FOUND:
			return "Player not found";
		case LUA_ERROR_MONSTER_NOT_FOUND:
			return "Monster not found";
		case LUA_ERROR_NPC_NOT_FOUND:
			return "NPC not found";
		case LUA_ERROR_CREATURE_NOT_FOUND:
			return "Creature not found";
		case LUA_ERROR_ITEM_NOT_FOUND:
			return "Item not found";
		case LUA_ERROR_THING_NOT_FOUND:
			return "Thing not found";
		case LUA_ERROR_TILE_NOT_FOUND:
			return "Tile not found";
		case LUA_ERROR_HOUSE_NOT_FOUND:
			return "House not found";
		case LUA_ERROR_COMBAT_NOT_FOUND:
			return "Combat not found";
		case LUA_ERROR_CONDITION_NOT_FOUND:
			return "Condition not found";
		case LUA_ERROR_AREA_NOT_FOUND:
			return "Area not found";
		case LUA_ERROR_CONTAINER_NOT_FOUND:
			return "Container not found";
		case LUA_ERROR_VARIANT_NOT_FOUND:
			return "Variant not found";
		case LUA_ERROR_VARIANT_UNKNOWN:
			return "Unknown variant type";
		case LUA_ERROR_SPELL_NOT_FOUND:
			return "Spell not found";
		default:
			break;
	}

	return "Wrong error code!";
}

ScriptEnviroment LuaScriptInterface::m_scriptEnv[21];
int32_t LuaScriptInterface::m_scriptEnvIndex = -1;

LuaScriptInterface::LuaScriptInterface(std::string interfaceName)
{
	m_luaState = NULL;
	m_interfaceName = interfaceName;
	m_lastEventTimerId = 1000;
}

LuaScriptInterface::~LuaScriptInterface()
{
	closeState();
}

bool LuaScriptInterface::reInitState()
{
	closeState();
	return initState();
}

void LuaScriptInterface::dumpLuaStack()
{
	int32_t a = lua_gettop(m_luaState);
	std::cout << "stack size: " << a << std::endl;
	for(int32_t i = 1; i <= a ; ++i)
		std::cout << lua_typename(m_luaState, lua_type(m_luaState,-i)) << " " << lua_topointer(m_luaState, -i) << std::endl;
}

int32_t LuaScriptInterface::loadFile(const std::string& file, Npc* npc /* = NULL*/)
{
	//loads file as a chunk at stack top
	int32_t ret = luaL_loadfile(m_luaState, file.c_str());
	if(ret != 0)
	{
		m_lastLuaError = popString(m_luaState);
		//reportError(NULL, m_lastLuaError);
		return -1;
	}

	//check that it is loaded as a function
	if(lua_isfunction(m_luaState, -1) == 0)
		return -1;

	m_loadingFile = file;
	this->reserveScriptEnv();
	ScriptEnviroment* env = this->getScriptEnv();
	env->setScriptId(EVENT_ID_LOADING, this);
	env->setNpc(npc);

	//execute it
	ret = lua_pcall(m_luaState, 0, 0, 0);
	if(ret != 0)
	{
		reportError(NULL, std::string(popString(m_luaState)));
		this->releaseScriptEnv();
		return -1;
	}

	this->releaseScriptEnv();
	return 0;
}

int32_t LuaScriptInterface::loadBuffer(const std::string& text, Npc* npc /* = NULL*/)
{
	//loads buffer as a chunk at stack top
	const char* buffer = text.c_str();
	int32_t ret = luaL_loadbuffer(m_luaState, buffer, strlen(buffer), "loadBuffer");
	if(ret != 0)
	{
		m_lastLuaError = popString(m_luaState);
		reportError(NULL, m_lastLuaError);
		return -1;
	}

	//check that it is loaded as a function
	if(lua_isfunction(m_luaState, -1) == 0)
		return -1;

	m_loadingFile = "loadBuffer";
	this->reserveScriptEnv();
	ScriptEnviroment* env = this->getScriptEnv();
	env->setScriptId(EVENT_ID_LOADING, this);
	env->setNpc(npc);

	//execute it
	ret = lua_pcall(m_luaState, 0, 0, 0);
	if(ret != 0)
	{
		reportError(NULL, std::string(popString(m_luaState)));
		this->releaseScriptEnv();
		return -1;
	}

	this->releaseScriptEnv();
	return 0;
}

int32_t LuaScriptInterface::getEvent(const std::string& eventName)
{
	//get our events table
	lua_getfield(m_luaState, LUA_REGISTRYINDEX, "EVENTS");
	if(lua_istable(m_luaState, -1) == 0)
	{
		lua_pop(m_luaState, 1);
		return -1;
	}

	//get current event function pointer
	lua_getglobal(m_luaState, eventName.c_str());
	if(lua_isfunction(m_luaState, -1) == 0)
	{
		lua_pop(m_luaState, 1);
		return -1;
	}

	//save in our events table
	lua_pushnumber(m_luaState, m_runningEventId);
	lua_pushvalue(m_luaState, -2);
	lua_rawset(m_luaState, -4);
	lua_pop(m_luaState, 2);

	//reset global value of this event
	lua_pushnil(m_luaState);
	lua_setglobal(m_luaState, eventName.c_str());

	m_cacheFiles[m_runningEventId] = m_loadingFile + ":" + eventName;
	++m_runningEventId;
	return m_runningEventId - 1;
}

const std::string& LuaScriptInterface::getFileById(int32_t scriptId)
{
	const static std::string unk = "(Unknown scriptfile)";
	if(scriptId != EVENT_ID_LOADING)
	{
		ScriptsCache::iterator it = m_cacheFiles.find(scriptId);
		if(it != m_cacheFiles.end())
			return it->second;

		return unk;
	}

	return m_loadingFile;
}

void LuaScriptInterface::reportError(const char* function, const std::string& errorDesc)
{
	ScriptEnviroment* env = getScriptEnv();
	int32_t scriptId;
	int32_t callbackId;
	bool timerEvent;
	std::string eventDesc;
	LuaScriptInterface* scriptInterface;
	env->getEventInfo(scriptId, eventDesc, scriptInterface, callbackId, timerEvent);

	std::cout << std::endl << "Lua Script Error: ";
	if(scriptInterface)
	{
		std::cout << "[" << scriptInterface->getInterfaceName() << "] " << std::endl;
		if(timerEvent)
			std::cout << "in a timer event called from: " << std::endl;
		if(callbackId)
			std::cout << "in callback: " << scriptInterface->getFileById(callbackId) << std::endl;

		std::cout << scriptInterface->getFileById(scriptId) << std::endl;
	}

	std::cout << eventDesc << std::endl;
	if(function)
		std::cout << function << "(). ";

	std::cout << errorDesc << std::endl;
}

bool LuaScriptInterface::pushFunction(int32_t functionId)
{
	lua_getfield(m_luaState, LUA_REGISTRYINDEX, "EVENTS");
	if(lua_istable(m_luaState, -1) != 0)
	{
		lua_pushnumber(m_luaState, functionId);
		lua_rawget(m_luaState, -2);
		lua_remove(m_luaState, -2);
		if(lua_isfunction(m_luaState, -1) != 0)
			return true;
	}

	return false;
}

bool LuaScriptInterface::initState()
{
	m_luaState = luaL_newstate();
	if(!m_luaState)
		return false;

	luaL_openlibs(m_luaState);
	registerFunctions();
	if(loadFile(getFilePath(FILE_TYPE_OTHER, "lib/data.lua")) == -1)
		std::cout << "Warning: [LuaScriptInterface::initState] Cannot load " << getFilePath(FILE_TYPE_OTHER, "lib/data.lua") << "." << std::endl;

	lua_newtable(m_luaState);
	lua_setfield(m_luaState, LUA_REGISTRYINDEX, "EVENTS");

	m_runningEventId = EVENT_ID_USER;
	return true;
}

bool LuaScriptInterface::closeState()
{
	if(m_luaState)
	{
		m_cacheFiles.clear();
		for(LuaTimerEvents::iterator it = m_timerEvents.begin(); it != m_timerEvents.end(); ++it)
		{
			for(std::list<int32_t>::iterator lt = it->second.parameters.begin(); lt != it->second.parameters.end(); ++lt)
				luaL_unref(m_luaState, LUA_REGISTRYINDEX, *lt);

			it->second.parameters.clear();
			luaL_unref(m_luaState, LUA_REGISTRYINDEX, it->second.function);
		}

		m_timerEvents.clear();
		lua_close(m_luaState);
	}

	return true;
}

void LuaScriptInterface::executeTimerEvent(uint32_t eventIndex)
{
	LuaTimerEvents::iterator it = m_timerEvents.find(eventIndex);
	if(it != m_timerEvents.end())
	{
		//push function
		lua_rawgeti(m_luaState, LUA_REGISTRYINDEX, it->second.function);

		//push parameters
		for(std::list<int32_t>::reverse_iterator rt = it->second.parameters.rbegin(); rt != it->second.parameters.rend(); ++rt)
			lua_rawgeti(m_luaState, LUA_REGISTRYINDEX, *rt);

		//call the function
		if(reserveScriptEnv())
		{
			ScriptEnviroment* env = getScriptEnv();
			env->setTimerEvent();
			env->setScriptId(it->second.scriptId, this);
			callFunction(it->second.parameters.size());
			releaseScriptEnv();
		}
		else
			std::cout << "[Error] Call stack overflow. LuaScriptInterface::executeTimerEvent" << std::endl;

		//free resources
		for(std::list<int32_t>::iterator lt = it->second.parameters.begin(); lt != it->second.parameters.end(); ++lt)
			luaL_unref(m_luaState, LUA_REGISTRYINDEX, *lt);

		it->second.parameters.clear();
		luaL_unref(m_luaState, LUA_REGISTRYINDEX, it->second.function);
		m_timerEvents.erase(it);
	}
}

int32_t LuaScriptInterface::luaErrorHandler(lua_State* L)
{
	lua_getfield(L, LUA_GLOBALSINDEX, "debug");
	if (!lua_istable(L, -1))
	{
		lua_pop(L, 1);
		return 1;
	}

	lua_getfield(L, -1, "traceback");
	if (!lua_isfunction(L, -1))
	{
		lua_pop(L, 2);
		return 1;
	}

	lua_pushvalue(L, 1);
	lua_pushinteger(L, 2);
	lua_call(L, 2, 1);
	return 1;
}

int32_t LuaScriptInterface::callFunction(uint32_t nParams)
{
	int32_t result = LUA_ERROR, size = lua_gettop(m_luaState), errorIndex = lua_gettop(m_luaState) - nParams;
	lua_pushcfunction(m_luaState, luaErrorHandler);
	lua_insert(m_luaState, errorIndex);

	if(lua_pcall(m_luaState, nParams, 1, errorIndex) != 0)
		LuaScriptInterface::reportError(NULL, std::string(LuaScriptInterface::popString(m_luaState)));
	else
		result = (int32_t)LuaScriptInterface::popBoolean(m_luaState);

	lua_remove(m_luaState, errorIndex);
	if((lua_gettop(m_luaState) + (int32_t)nParams  + 1) != size)
		LuaScriptInterface::reportError(NULL, "Stack size changed!");

	return result;
}

void LuaScriptInterface::pushVariant(lua_State* L, const LuaVariant& var)
{
	lua_newtable(L);
	setField(L, "type", var.type);

	switch(var.type)
	{
		case VARIANT_NUMBER:
			setField(L, "number", var.number);
			break;
		case VARIANT_STRING:
			setField(L, "string", var.text);
			break;
		case VARIANT_TARGETPOSITION:
		case VARIANT_POSITION:
		{
			lua_pushstring(L, "pos");
			pushPosition(L, var.pos);
			lua_settable(L, -3);
			break;
		}
		case VARIANT_NONE:
			break;
	}
}

void LuaScriptInterface::pushThing(lua_State* L, Thing* thing, uint32_t thingid)
{
	lua_newtable(L);
	if(thing && thing->getItem())
	{
		const Item* item = thing->getItem();
		setField(L, "uid", thingid);
		setField(L, "itemid", item->getID());

		if(item->hasSubType())
			setField(L, "type", item->getSubType());
		else
			setField(L, "type", 0);

		setField(L, "actionid", item->getActionId());
	}
	else if(thing && thing->getCreature())
	{
		const Creature* creature = thing->getCreature();
		setField(L, "uid", thingid);
		setField(L, "itemid", 1);
		char type;
		if(creature->getPlayer())
			type = 1;
		else if(creature->getMonster())
			type = 2;
		else
			type = 3; //npc

		setField(L, "type", type);
		setField(L, "actionid", 0);
	}
	else
	{
		setField(L, "uid", 0);
		setField(L, "itemid", 0);
		setField(L, "type", 0);
		setField(L, "actionid", 0);
	}
}

void LuaScriptInterface::pushPosition(lua_State* L, const PositionEx& position)
{
	pushPosition(L, position, position.stackpos);
}

void LuaScriptInterface::pushPosition(lua_State* L, const Position& position, uint32_t stackpos)
{
	lua_newtable(L);
	setField(L, "z", position.z);
	setField(L, "y", position.y);
	setField(L, "x", position.x);
	setField(L, "stackpos", stackpos);
}

void LuaScriptInterface::pushCallback(lua_State* L, int32_t callback)
{
	lua_rawgeti(L, LUA_REGISTRYINDEX, callback);
}

LuaVariant LuaScriptInterface::popVariant(lua_State* L)
{
	uint32_t type = getField(L, "type");

	LuaVariant var;
	var.type = (LuaVariantType_t)type;

	switch(type)
	{
		case VARIANT_NUMBER:
			var.number = getFieldU32(L, "number");
			break;
		case VARIANT_STRING:
			var.text = getField(L, "string");
			break;
		case VARIANT_POSITION:
		case VARIANT_TARGETPOSITION:
		{
			lua_pushstring(L, "pos");
			lua_gettable(L, -2);
			popPosition(L, var.pos);
			break;
		}
		default:
			var.type = VARIANT_NONE;
			break;
	}

	lua_pop(L, 1); //table

	return var;
}

void LuaScriptInterface::popPosition(lua_State* L, PositionEx& position)
{
	position.z = getField(L, "z");
	position.y = getField(L, "y");
	position.x = getField(L, "x");
	position.stackpos = getField(L, "stackpos");

	lua_pop(L, 1); //table
}

void LuaScriptInterface::popPosition(lua_State* L, Position& position, uint32_t& stackpos)
{
	position.z = getField(L, "z");
	position.y = getField(L, "y");
	position.x = getField(L, "x");
	stackpos = getField(L, "stackpos");
	lua_pop(L, 1); //table
}

bool LuaScriptInterface::popBoolean(lua_State* L)
{
	lua_pop(L, 1);
	if(lua_isnumber(L, 0) == 1)
		return (bool)lua_tonumber(L, 0);

	return (bool)lua_toboolean(L, 0);
}

int64_t LuaScriptInterface::popNumber(lua_State* L)
{
	lua_pop(L, 1);
	if(lua_isboolean(L, 0) == 1)
		return (int64_t)lua_toboolean(L, 0);

	return (int64_t)lua_tonumber(L, 0);
}

double LuaScriptInterface::popFloatNumber(lua_State* L)
{
	lua_pop(L, 1);
	if(lua_isboolean(L, 0) == 1)
		return (int64_t)lua_toboolean(L, 0);

	return (double)lua_tonumber(L, 0);
}

const char* LuaScriptInterface::popString(lua_State* L)
{
	lua_pop(L, 1);
	const char* str = lua_tostring(L, 0);
	if(!str || strlen(str) == 0)
		return "";

	return str;
}

int32_t LuaScriptInterface::popCallback(lua_State* L)
{
	return luaL_ref(L, LUA_REGISTRYINDEX);
}

void LuaScriptInterface::setField(lua_State* L, const char* index, int32_t val)
{
	lua_pushstring(L, index);
	lua_pushnumber(L, (double)val);
	lua_settable(L, -3);
}

void LuaScriptInterface::setField(lua_State* L, const char* index, const std::string& val)
{
	lua_pushstring(L, index);
	lua_pushstring(L, val.c_str());
	lua_settable(L, -3);
}

void LuaScriptInterface::setFieldBool(lua_State* L, const char* index, bool val)
{
	lua_pushstring(L, index);
	lua_pushboolean(L, val);
	lua_settable(L, -3);
}

void LuaScriptInterface::setFieldDouble(lua_State* L, const char* index, double val)
{
	lua_pushstring(L, index);
	lua_pushnumber(L, val);
	lua_settable(L, -3);
}

int32_t LuaScriptInterface::getField(lua_State* L, const char* key)
{
	int32_t result;
	lua_pushstring(L, key);
	lua_gettable(L, -2); // get table[key]
	result = (int32_t)lua_tonumber(L, -1);
	lua_pop(L, 1); // remove number and key
	return result;
}

uint32_t LuaScriptInterface::getFieldU32(lua_State* L, const char* key)
{
	uint32_t result;
	lua_pushstring(L, key);
	lua_gettable(L, -2); // get table[key]
	result = (uint32_t)lua_tonumber(L, -1);
	lua_pop(L, 1); // remove number and key
	return result;
}

bool LuaScriptInterface::getFieldBool(lua_State* L, const char* key)
{
	bool result;
	lua_pushstring(L, key);
	lua_gettable(L, -2); // get table[key]
	result = (lua_toboolean(L, -1) == 1);
	lua_pop(L, 1); // remove number and key
	return result;
}

std::string LuaScriptInterface::getFieldString(lua_State* L, const char* key)
{
	std::string result;
	lua_pushstring(L, key);
	lua_gettable(L, -2); // get table[key]
	result = lua_tostring(L, -1);
	lua_pop(L, 1); // remove number and key
	return result;
}

std::string LuaScriptInterface::getGlobalString(lua_State* L, const std::string& _identifier, const std::string& _default /*= ""*/)
{
	lua_getglobal(L, _identifier.c_str());
	if(!lua_isstring(L, -1))
	{
		lua_pop(L, 1);
		return _default;
	}

	int32_t len = (int32_t)lua_strlen(L, -1);
	std::string ret(lua_tostring(L, -1), len);

	lua_pop(L, 1);
	return ret;
}

bool LuaScriptInterface::getGlobalBool(lua_State* L, const std::string& _identifier, const std::string& _default /*= "no"*/)
{
	return booleanString(LuaScriptInterface::getGlobalString(L, _identifier, _default));
}

int32_t LuaScriptInterface::getGlobalNumber(lua_State* L, const std::string& _identifier, const int32_t _default /*= 0*/)
{
	return (int32_t)LuaScriptInterface::getGlobalDouble(L, _identifier, _default);
}

double LuaScriptInterface::getGlobalDouble(lua_State* L, const std::string& _identifier, const double _default /*= 0*/)
{
	lua_getglobal(L, _identifier.c_str());
	if(!lua_isnumber(L, -1))
	{
		lua_pop(L, 1);
		return _default;
	}

	double val = (double)lua_tonumber(L, -1);
	lua_pop(L, 1);
	return val;
}

void LuaScriptInterface::getValue(const std::string& key, lua_State* L, lua_State* _L)
{
	lua_getglobal(L, key.c_str());
	moveValue(L, _L);
}

void LuaScriptInterface::moveValue(lua_State* from, lua_State* to)
{
	switch(lua_type(from, -1))
	{
		case LUA_TNIL:
			lua_pushnil(to);
			break;
		case LUA_TBOOLEAN:
			lua_pushboolean(to, lua_toboolean(from, -1));
			break;
		case LUA_TNUMBER:
			lua_pushnumber(to, lua_tonumber(from, -1));
			break;
		case LUA_TSTRING:
		{
			size_t len;
			const char* str = lua_tolstring(from, -1, &len);

			lua_pushlstring(to, str, len);
			break;
		}
		case LUA_TTABLE:
		{
			lua_newtable(to);
			lua_pushnil(from); // First key
			while(lua_next(from, -2))
			{
				// Move value to the other state
				moveValue(from, to); // Value is popped, key is left
				// Move key to the other state
				lua_pushvalue(from, -1); // Make a copy of the key to use for the next iteration
				moveValue(from, to); // Key is in other state.
				// We still have the key in the 'from' state ontop of the stack

				lua_insert(to, -2); // Move key above value
				lua_settable(to, -3); // Set the key
			}

			break;
		}
		default:
			break;
	}

	lua_pop(from, 1); // Pop the value we just read
}

void LuaScriptInterface::registerFunctions()
{
	//TODO: Change all player lua functions that a creature has use of too, to support creatures aswell,

	//example(...)
	//lua_register(L, "name", C_function);

	//getPlayerFood(cid)
	lua_register(m_luaState, "getPlayerFood", LuaScriptInterface::luaGetPlayerFood);

	//getCreatureHealth(cid)
	lua_register(m_luaState, "getCreatureHealth", LuaScriptInterface::luaGetCreatureHealth);

	//getCreatureMaxHealth(cid)
	lua_register(m_luaState, "getCreatureMaxHealth", LuaScriptInterface::luaGetCreatureMaxHealth);

	//getCreatureMana(cid)
	lua_register(m_luaState, "getCreatureMana", LuaScriptInterface::luaGetCreatureMana);

	//getCreatureMaxMana(cid)
	lua_register(m_luaState, "getCreatureMaxMana", LuaScriptInterface::luaGetCreatureMaxMana);

	//getCreatureLookDirection(cid)
	lua_register(m_luaState, "getCreatureLookDirection", LuaScriptInterface::luaGetCreatureLookDirection);

	//getPlayerLevel(cid)
	lua_register(m_luaState, "getPlayerLevel", LuaScriptInterface::luaGetPlayerLevel);

	//getPlayerExperience(cid)
	lua_register(m_luaState, "getPlayerExperience", LuaScriptInterface::luaGetPlayerExperience);

	//getPlayerMagLevel(cid[, ignoreBuffs])
	lua_register(m_luaState, "getPlayerMagLevel", LuaScriptInterface::luaGetPlayerMagLevel);

	//getPlayerSpentMana(cid)
	lua_register(m_luaState, "getPlayerSpentMana", LuaScriptInterface::luaGetPlayerSpentMana);

	//getPlayerAccess(cid)
	lua_register(m_luaState, "getPlayerAccess", LuaScriptInterface::luaGetPlayerAccess);

	//getPlayerSkillLevel(cid, skillid)
	lua_register(m_luaState, "getPlayerSkillLevel", LuaScriptInterface::luaGetPlayerSkillLevel);

	//getPlayerSkillTries(cid, skillid)
	lua_register(m_luaState, "getPlayerSkillTries", LuaScriptInterface::luaGetPlayerSkillTries);

	//getPlayerTown(cid)
	lua_register(m_luaState, "getPlayerTown", LuaScriptInterface::luaGetPlayerTown);

	//getPlayerVocation(cid)
	lua_register(m_luaState, "getPlayerVocation", LuaScriptInterface::luaGetPlayerVocation);

	//getPlayerIp(cid)
	lua_register(m_luaState, "getPlayerIp", LuaScriptInterface::luaGetPlayerIp);

	//getPlayerRequiredMana(cid, magicLevel)
	lua_register(m_luaState, "getPlayerRequiredMana", LuaScriptInterface::luaGetPlayerRequiredMana);

	//getPlayerRequiredSkillTries(cid, skillId, skillLevel)
	lua_register(m_luaState, "getPlayerRequiredSkillTries", LuaScriptInterface::luaGetPlayerRequiredSkillTries);

	//getPlayerItemCount(cid, itemid)
	lua_register(m_luaState, "getPlayerItemCount", LuaScriptInterface::luaGetPlayerItemCount);

	//getPlayerMoney(cid)
	lua_register(m_luaState, "getPlayerMoney", LuaScriptInterface::luaGetPlayerMoney);

	//getPlayerSoul(cid)
	lua_register(m_luaState, "getPlayerSoul", LuaScriptInterface::luaGetPlayerSoul);

	//getPlayerFreeCap(cid)
	lua_register(m_luaState, "getPlayerFreeCap", LuaScriptInterface::luaGetPlayerFreeCap);

	//getPlayerLight(cid)
	lua_register(m_luaState, "getPlayerLight", LuaScriptInterface::luaGetPlayerLight);

	//getPlayerSlotItem(cid, slot)
	lua_register(m_luaState, "getPlayerSlotItem", LuaScriptInterface::luaGetPlayerSlotItem);

	//getPlayerWeapon(cid[, ignoreAmmo])
	lua_register(m_luaState, "getPlayerWeapon", LuaScriptInterface::luaGetPlayerWeapon);

	//getPlayerItemById(cid, deepSearch, itemId[, subType])
	lua_register(m_luaState, "getPlayerItemById", LuaScriptInterface::luaGetPlayerItemById);

	//getPlayerDepotItems(cid, depotid)
	lua_register(m_luaState, "getPlayerDepotItems", LuaScriptInterface::luaGetPlayerDepotItems);

	//getPlayerGuildId(cid)
	lua_register(m_luaState, "getPlayerGuildId", LuaScriptInterface::luaGetPlayerGuildId);

	//getPlayerGuildName(cid)
	lua_register(m_luaState, "getPlayerGuildName", LuaScriptInterface::luaGetPlayerGuildName);

	//getPlayerGuildRankId(cid)
	lua_register(m_luaState, "getPlayerGuildRankId", LuaScriptInterface::luaGetPlayerGuildRankId);

	//getPlayerGuildRank(cid)
	lua_register(m_luaState, "getPlayerGuildRank", LuaScriptInterface::luaGetPlayerGuildRank);

	//getPlayerGuildNick(cid)
	lua_register(m_luaState, "getPlayerGuildNick", LuaScriptInterface::luaGetPlayerGuildNick);

	//getPlayerGuildLevel(cid)
	lua_register(m_luaState, "getPlayerGuildLevel", LuaScriptInterface::luaGetPlayerGuildLevel);

	//getPlayerSex(cid)
	lua_register(m_luaState, "getPlayerSex", LuaScriptInterface::luaGetPlayerSex);

	//getPlayerGUID(cid)
	lua_register(m_luaState, "getPlayerGUID", LuaScriptInterface::luaGetPlayerGUID);

	//getPlayerNameDescription(cid)
	lua_register(m_luaState, "getPlayerNameDescription", LuaScriptInterface::luaGetPlayerNameDescription);

	//doPlayerSetNameDescription(cid, description)
	lua_register(m_luaState, "doPlayerSetNameDescription", LuaScriptInterface::luaDoPlayerSetNameDescription);

	//getPlayerAccountId(cid)
	lua_register(m_luaState, "getPlayerAccountId", LuaScriptInterface::luaGetPlayerAccountId);

	//getPlayerAccount(cid)
	lua_register(m_luaState, "getPlayerAccount", LuaScriptInterface::luaGetPlayerAccount);

	//getPlayerFlagValue(cid, flag)
	lua_register(m_luaState, "getPlayerFlagValue", LuaScriptInterface::luaGetPlayerFlagValue);

	//getPlayerCustomFlagValue(cid, flag)
	lua_register(m_luaState, "getPlayerCustomFlagValue", LuaScriptInterface::luaGetPlayerCustomFlagValue);

	//getPlayerPromotionLevel(cid)
	lua_register(m_luaState, "getPlayerPromotionLevel", LuaScriptInterface::luaGetPlayerPromotionLevel);

	//setPlayerPromotionLevel(cid, level)
	lua_register(m_luaState, "setPlayerPromotionLevel", LuaScriptInterface::luaSetPlayerPromotionLevel);

	//getPlayerGroupId(cid)
	lua_register(m_luaState, "getPlayerGroupId", LuaScriptInterface::luaGetPlayerGroupId);

	//setPlayerGroupId(cid, newGroupId)
	lua_register(m_luaState, "setPlayerGroupId", LuaScriptInterface::luaSetPlayerGroupId);

	//doPlayerSendOutfitWindow(cid)
	lua_register(m_luaState, "doPlayerSendOutfitWindow", LuaScriptInterface::luaDoPlayerSendOutfitWindow);

	//doPlayerLearnInstantSpell(cid, name)
	lua_register(m_luaState, "doPlayerLearnInstantSpell", LuaScriptInterface::luaDoPlayerLearnInstantSpell);

	//doPlayerUnlearnInstantSpell(cid, name)
	lua_register(m_luaState, "doPlayerUnlearnInstantSpell", LuaScriptInterface::luaDoPlayerUnlearnInstantSpell);

	//getPlayerLearnedInstantSpell(cid, name)
	lua_register(m_luaState, "getPlayerLearnedInstantSpell", LuaScriptInterface::luaGetPlayerLearnedInstantSpell);

	//getPlayerInstantSpellCount(cid)
	lua_register(m_luaState, "getPlayerInstantSpellCount", LuaScriptInterface::luaGetPlayerInstantSpellCount);

	//getPlayerInstantSpellInfo(cid, index)
	lua_register(m_luaState, "getPlayerInstantSpellInfo", LuaScriptInterface::luaGetPlayerInstantSpellInfo);

	//getInstantSpellInfo(cid, name)
	lua_register(m_luaState, "getInstantSpellInfo", LuaScriptInterface::luaGetInstantSpellInfo);

	//getPlayerStorageValue(uid, key)
	lua_register(m_luaState, "getPlayerStorageValue", LuaScriptInterface::luaGetPlayerStorageValue);

	//setPlayerStorageValue(uid, key, value)
	lua_register(m_luaState, "setPlayerStorageValue", LuaScriptInterface::luaSetPlayerStorageValue);

	//getGlobalStorageValue(key)
	lua_register(m_luaState, "getGlobalStorageValue", LuaScriptInterface::luaGetGlobalStorageValue);

	//setGlobalStorageValue(key, value)
	lua_register(m_luaState, "setGlobalStorageValue", LuaScriptInterface::luaSetGlobalStorageValue);

	//getPlayersOnline()
	lua_register(m_luaState, "getPlayersOnline", LuaScriptInterface::luaGetPlayersOnline);

	//getTileInfo(pos)
	lua_register(m_luaState, "getTileInfo", LuaScriptInterface::luaGetTileInfo);

	//getItemWeaponType(uid)
	lua_register(m_luaState, "getItemWeaponType", LuaScriptInterface::luaGetItemWeaponType);

	//getItemRWInfo(uid)
	lua_register(m_luaState, "getItemRWInfo", LuaScriptInterface::luaGetItemRWInfo);

	//getThingFromPos(pos[, displayError])
	lua_register(m_luaState, "getThingFromPos", LuaScriptInterface::luaGetThingFromPos);

	//getThing(uid)
	lua_register(m_luaState, "getThing", LuaScriptInterface::luaGetThing);

	//doTileQueryAdd(uid, pos[, flags])
	lua_register(m_luaState, "doTileQueryAdd", LuaScriptInterface::luaDoTileQueryAdd);

	//getThingPos(uid)
	lua_register(m_luaState, "getThingPos", LuaScriptInterface::luaGetThingPos);

	//getTileItemById(pos, itemId[, subType])
	lua_register(m_luaState, "getTileItemById", LuaScriptInterface::luaGetTileItemById);

	//getTileItemByType(pos, type)
	lua_register(m_luaState, "getTileItemByType", LuaScriptInterface::luaGetTileItemByType);

	//getTileThingByPos(pos)
	lua_register(m_luaState, "getTileThingByPos", LuaScriptInterface::luaGetTileThingByPos);

	//getTopCreature(pos)
	lua_register(m_luaState, "getTopCreature", LuaScriptInterface::luaGetTopCreature);

	//doRemoveItem(uid[, count])
	lua_register(m_luaState, "doRemoveItem", LuaScriptInterface::luaDoRemoveItem);

	//doPlayerFeed(cid, food)
	lua_register(m_luaState, "doPlayerFeed", LuaScriptInterface::luaDoFeedPlayer);

	//doPlayerSendCancel(cid, text)
	lua_register(m_luaState, "doPlayerSendCancel", LuaScriptInterface::luaDoPlayerSendCancel);

	//doPlayerSendDefaultCancel(cid, ReturnValue)
	lua_register(m_luaState, "doPlayerSendDefaultCancel", LuaScriptInterface::luaDoSendDefaultCancel);

	//getSearchString(fromPosition, toPosition[, fromIsCreature[, toIsCreature]])
	lua_register(m_luaState, "getSearchString", LuaScriptInterface::luaGetSearchString);

	//getClosestFreeTile(cid, targetpos[, extended[, ignoreHouse]])
	lua_register(m_luaState, "getClosestFreeTile", LuaScriptInterface::luaGetClosestFreeTile);

	//doTeleportThing(cid, newpos[, pushmove])
	lua_register(m_luaState, "doTeleportThing", LuaScriptInterface::luaDoTeleportThing);

	//doTransformItem(uid, newId[, count/subType])
	lua_register(m_luaState, "doTransformItem", LuaScriptInterface::luaDoTransformItem);

	//doCreatureSay(cid, text, type[, pos])
	lua_register(m_luaState, "doCreatureSay", LuaScriptInterface::luaDoCreatureSay);

	//doSendMagicEffect(pos, type[, player])
	lua_register(m_luaState, "doSendMagicEffect", LuaScriptInterface::luaDoSendMagicEffect);

	//doSendDistanceShoot(frompos, topos, type)
	lua_register(m_luaState, "doSendDistanceShoot", LuaScriptInterface::luaDoSendDistanceShoot);

	//doChangeTypeItem(uid, newtype)
	lua_register(m_luaState, "doChangeTypeItem", LuaScriptInterface::luaDoChangeTypeItem);

	//doSetItemActionId(uid, actionid)
	lua_register(m_luaState, "doSetItemActionId", LuaScriptInterface::luaDoSetItemActionId);

	//doSetItemText(uid, text[, writer[, date]])
	lua_register(m_luaState, "doSetItemText", LuaScriptInterface::luaDoSetItemText);

	//doSetItemSpecialDescription(uid, desc)
	lua_register(m_luaState, "doSetItemSpecialDescription", LuaScriptInterface::luaDoSetItemSpecialDescription);

	//doSendAnimatedText(pos, text, color[, player])
	lua_register(m_luaState, "doSendAnimatedText", LuaScriptInterface::luaDoSendAnimatedText);

	//doPlayerAddSkillTry(cid, skillid, n)
	lua_register(m_luaState, "doPlayerAddSkillTry", LuaScriptInterface::luaDoPlayerAddSkillTry);

	//doCreatureAddHealth(cid, health[, force])
	lua_register(m_luaState, "doCreatureAddHealth", LuaScriptInterface::luaDoCreatureAddHealth);

	//doCreatureAddMana(cid, mana)
	lua_register(m_luaState, "doCreatureAddMana", LuaScriptInterface::luaDoCreatureAddMana);

	//setCreatureMaxHealth(cid, health)
	lua_register(m_luaState, "setCreatureMaxHealth", LuaScriptInterface::luaSetCreatureMaxHealth);

	//setCreatureMaxMana(cid, mana)
	lua_register(m_luaState, "setCreatureMaxMana", LuaScriptInterface::luaSetCreatureMaxMana);

	//doPlayerAddSpentMana(cid, amount)
	lua_register(m_luaState, "doPlayerAddSpentMana", LuaScriptInterface::luaDoPlayerAddSpentMana);

	//doPlayerAddSoul(cid, soul)
	lua_register(m_luaState, "doPlayerAddSoul", LuaScriptInterface::luaDoPlayerAddSoul);

	//doPlayerAddItem(uid, itemid[, count/subype[, canDropOnMap = TRUE]])
	//Returns uid of the created item
	lua_register(m_luaState, "doPlayerAddItem", LuaScriptInterface::luaDoPlayerAddItem);

	//doPlayerAddItemEx(cid, uid[, canDropOnMap = FALSE])
	lua_register(m_luaState, "doPlayerAddItemEx", LuaScriptInterface::luaDoPlayerAddItemEx);

	//doPlayerSendTextMessage(cid, MessageClasses, message)
	lua_register(m_luaState, "doPlayerSendTextMessage", LuaScriptInterface::luaDoPlayerSendTextMessage);

	//doPlayerSendChannelMessage(cid, author, message, SpeakClasses, channel)
	lua_register(m_luaState, "doPlayerSendChannelMessage", LuaScriptInterface::luaDoPlayerSendChannelMessage);

	//doPlayerSendToChannel(cid, targetId, SpeakClasses, message, channel[, time])
	lua_register(m_luaState, "doPlayerSendToChannel", LuaScriptInterface::luaDoPlayerSendToChannel);

	//doPlayerAddMoney(cid, money)
	lua_register(m_luaState, "doPlayerAddMoney", LuaScriptInterface::luaDoPlayerAddMoney);

	//doPlayerRemoveMoney(cid, money)
	lua_register(m_luaState, "doPlayerRemoveMoney", LuaScriptInterface::luaDoPlayerRemoveMoney);

	//doPlayerWithdrawMoney(cid, money)
	lua_register(m_luaState, "doPlayerWithdrawMoney", LuaScriptInterface::luaDoPlayerWithdrawMoney);

	//doPlayerDepositMoney(cid, money)
	lua_register(m_luaState, "doPlayerDepositMoney", LuaScriptInterface::luaDoPlayerDepositMoney);

	//doPlayerTransferMoneyTo(cid, target, money)
	lua_register(m_luaState, "doPlayerTransferMoneyTo", LuaScriptInterface::luaDoPlayerTransferMoneyTo);

	//doShowTextDialog(cid, itemid, text)
	lua_register(m_luaState, "doShowTextDialog", LuaScriptInterface::luaDoShowTextDialog);

	//doDecayItem(uid)
	lua_register(m_luaState, "doDecayItem", LuaScriptInterface::luaDoDecayItem);

	//doCreateItem(itemid, type/count, pos)
	//Returns uid of the created item, only works on tiles.
	lua_register(m_luaState, "doCreateItem", LuaScriptInterface::luaDoCreateItem);

	//doCreateItemEx(itemid[, count/subType = -1])
	lua_register(m_luaState, "doCreateItemEx", LuaScriptInterface::luaDoCreateItemEx);

	//doTileAddItemEx(pos, uid)
	lua_register(m_luaState, "doTileAddItemEx", LuaScriptInterface::luaDoTileAddItemEx);

	//doAddContainerItemEx(uid, virtuid)
	lua_register(m_luaState, "doAddContainerItemEx", LuaScriptInterface::luaDoAddContainerItemEx);

	//doRelocate(pos, posTo)
	//Moves all moveable objects from pos to posTo
	lua_register(m_luaState, "doRelocate", LuaScriptInterface::luaDoRelocate);

	//doCreateTeleport(itemid, topos, createpos)
	lua_register(m_luaState, "doCreateTeleport", LuaScriptInterface::luaDoCreateTeleport);

	//doCreateMonster(name, pos[, displayError])
	lua_register(m_luaState, "doCreateMonster", LuaScriptInterface::luaDoCreateMonster);

	//doCreateNpc(name, pos[, displayError])
	lua_register(m_luaState, "doCreateNpc", LuaScriptInterface::luaDoCreateNpc);

	//doSummonMonster(cid, name)
	lua_register(m_luaState, "doSummonMonster", LuaScriptInterface::luaDoSummonMonster);

	//doConvinceCreature(cid, target)
	lua_register(m_luaState, "doConvinceCreature", LuaScriptInterface::luaDoConvinceCreature);

	//getMonsterTargetList(cid)
	lua_register(m_luaState, "getMonsterTargetList", LuaScriptInterface::luaGetMonsterTargetList);

	//getMonsterFriendList(cid)
	lua_register(m_luaState, "getMonsterFriendList", LuaScriptInterface::luaGetMonsterFriendList);

	//doMonsterSetTarget(cid, target)
	lua_register(m_luaState, "doMonsterTarget", LuaScriptInterface::luaDoMonsterSetTarget);

	//doMonsterChangeTarget(cid)
	lua_register(m_luaState, "doMonsterChangeTarget", LuaScriptInterface::luaDoMonsterChangeTarget);

	//getMonsterInfo(name)
	lua_register(m_luaState, "getMonsterInfo", LuaScriptInterface::luaGetMonsterInfo);

	//getMonsterHealingSpells(name)
	lua_register(m_luaState, "getMonsterHealingSpells", LuaScriptInterface::luaGetMonsterHealingSpells);

	//getMonsterAttackSpells(name)
	lua_register(m_luaState, "getMonsterAttackSpells", LuaScriptInterface::luaGetMonsterAttackSpells);

	//getMonsterLootList(name)
	lua_register(m_luaState, "getMonsterLootList", LuaScriptInterface::luaGetMonsterLootList);

	//doAddCondition(cid, condition)
	lua_register(m_luaState, "doAddCondition", LuaScriptInterface::luaDoAddCondition);

	//doRemoveCondition(cid, type[, subId])
	lua_register(m_luaState, "doRemoveCondition", LuaScriptInterface::luaDoRemoveCondition);

	//doRemoveConditions(cid[, onlyPersistent])
	lua_register(m_luaState, "doRemoveConditions", LuaScriptInterface::luaDoRemoveConditions);

	//doRemoveCreature(cid)
	lua_register(m_luaState, "doRemoveCreature", LuaScriptInterface::luaDoRemoveCreature);

	//doMoveCreature(cid, direction)
	lua_register(m_luaState, "doMoveCreature", LuaScriptInterface::luaDoMoveCreature);

	//doPlayerSetTown(cid, townid)
	lua_register(m_luaState, "doPlayerSetTown", LuaScriptInterface::luaDoPlayerSetTown);

	//doPlayerSetVocation(cid,voc)
	lua_register(m_luaState, "doPlayerSetVocation", LuaScriptInterface::luaDoPlayerSetVocation);

	//doPlayerRemoveItem(cid, itemid[, count[, subType]])
	lua_register(m_luaState, "doPlayerRemoveItem", LuaScriptInterface::luaDoPlayerRemoveItem);

	//doPlayerAddExperience(cid, amount)
	lua_register(m_luaState, "doPlayerAddExperience", LuaScriptInterface::luaDoPlayerAddExperience);

	//doPlayerSetGuildId(cid, id)
	lua_register(m_luaState, "doPlayerSetGuildId", LuaScriptInterface::luaDoPlayerSetGuildId);

	//doPlayerSetGuildRank(cid, rank)
	lua_register(m_luaState, "doPlayerSetGuildRank", LuaScriptInterface::luaDoPlayerSetGuildRank);

	//doPlayerSetGuildNick(cid, nick)
	lua_register(m_luaState, "doPlayerSetGuildNick", LuaScriptInterface::luaDoPlayerSetGuildNick);

	//doPlayerAddOutfit(cid,looktype,addons)
	lua_register(m_luaState, "doPlayerAddOutfit", LuaScriptInterface::luaDoPlayerAddOutfit);

	//doPlayerRemoveOutfit(cid,looktype,addons)
	lua_register(m_luaState, "doPlayerRemoveOutfit", LuaScriptInterface::luaDoPlayerRemoveOutfit);

	//canPlayerWearOutfit(cid, looktype, addons)
	lua_register(m_luaState, "canPlayerWearOutfit", LuaScriptInterface::luaCanPlayerWearOutfit);

	//doSetCreatureLight(cid, lightLevel, lightColor, time)
	lua_register(m_luaState, "doSetCreatureLight", LuaScriptInterface::luaDoSetCreatureLight);

	//getCreatureCondition(cid, condition[, subId])
	lua_register(m_luaState, "getCreatureCondition", LuaScriptInterface::luaGetCreatureCondition);

	//doCreatureSetDropLoot(cid, doDrop)
	lua_register(m_luaState, "doCreatureSetDropLoot", LuaScriptInterface::luaDoCreatureSetDropLoot);

	//getPlayerLossPercent(cid, lossType)
	lua_register(m_luaState, "getPlayerLossPercent", LuaScriptInterface::luaGetPlayerLossPercent);

	//doPlayerSetLossPercent(cid, lossType, newPercent)
	lua_register(m_luaState, "doPlayerSetLossPercent", LuaScriptInterface::luaDoPlayerSetLossPercent);

	//doPlayerSetLossSkill(cid, doLose)
	lua_register(m_luaState, "doPlayerSetLossSkill", LuaScriptInterface::luaDoPlayerSetLossSkill);

	//getPlayerLossSkill(cid)
	lua_register(m_luaState, "getPlayerLossSkill", LuaScriptInterface::luaGetPlayerLossSkill);

	//doPlayerSwitchSaving(cid)
	lua_register(m_luaState, "doPlayerSwitchSaving", LuaScriptInterface::luaDoPlayerSwitchSaving);

	//doPlayerSave(cid)
	lua_register(m_luaState, "doPlayerSave", LuaScriptInterface::luaDoPlayerSave);

	//isPlayer(cid)
	lua_register(m_luaState, "isPlayer", LuaScriptInterface::luaIsPlayer);

	//isPlayerPzLocked(cid)
	lua_register(m_luaState, "isPlayerPzLocked", LuaScriptInterface::luaIsPlayerPzLocked);

	//isPlayerSaving(cid)
	lua_register(m_luaState, "isPlayerSaving", LuaScriptInterface::luaIsPlayerSaving);

	//isMonster(cid)
	lua_register(m_luaState, "isMonster", LuaScriptInterface::luaIsMonster);

	//isNpc(cid)
	lua_register(m_luaState, "isNpc", LuaScriptInterface::luaIsNpc);

	//isCreature(cid)
	lua_register(m_luaState, "isCreature", LuaScriptInterface::luaIsCreature);

	//isContainer(uid)
	lua_register(m_luaState, "isContainer", LuaScriptInterface::luaIsContainer);

	//isCorpse(uid)
	lua_register(m_luaState, "isCorpse", LuaScriptInterface::luaIsCorpse);

	//isMovable(uid)
	lua_register(m_luaState, "isMovable", LuaScriptInterface::luaIsMovable);

	//getCreatureByName(name)
	lua_register(m_luaState, "getCreatureByName", LuaScriptInterface::luaGetCreatureByName);

	//getPlayerByNameWildcard(name~)
	lua_register(m_luaState, "getPlayerByNameWildcard", LuaScriptInterface::luaGetPlayerByNameWildcard);

	//getPlayerGUIDByName(name[, multiworld])
	lua_register(m_luaState, "getPlayerGUIDByName", LuaScriptInterface::luaGetPlayerGUIDByName);

	//getPlayerNameByGUID(guid[, multiworld])
	lua_register(m_luaState, "getPlayerNameByGUID", LuaScriptInterface::luaGetPlayerNameByGUID);

	//registerCreatureEvent(uid, eventName)
	lua_register(m_luaState, "registerCreatureEvent", LuaScriptInterface::luaRegisterCreatureEvent);

	//getContainerSize(uid)
	lua_register(m_luaState, "getContainerSize", LuaScriptInterface::luaGetContainerSize);

	//getContainerCap(uid)
	lua_register(m_luaState, "getContainerCap", LuaScriptInterface::luaGetContainerCap);

	//getContainerCapById(itemid)
	lua_register(m_luaState, "getContainerCapById", LuaScriptInterface::luaGetContainerCapById);

	//getContainerItem(uid, slot)
	lua_register(m_luaState, "getContainerItem", LuaScriptInterface::luaGetContainerItem);

	//doAddContainerItem(uid, itemid[, count/subType])
	lua_register(m_luaState, "doAddContainerItem", LuaScriptInterface::luaDoAddContainerItem);

	//getHouseOwner(houseid)
	lua_register(m_luaState, "getHouseOwner", LuaScriptInterface::luaGetHouseOwner);

	//getHouseName(houseid)
	lua_register(m_luaState, "getHouseName", LuaScriptInterface::luaGetHouseName);

	//getHouseEntry(houseid)
	lua_register(m_luaState, "getHouseEntry", LuaScriptInterface::luaGetHouseEntry);

	//getHouseRent(houseid)
	lua_register(m_luaState, "getHouseRent", LuaScriptInterface::luaGetHouseRent);

	//getHousePrice(houseid)
	lua_register(m_luaState, "getHousePrice", LuaScriptInterface::luaGetHousePrice);

	//getHouseTown(houseid)
	lua_register(m_luaState, "getHouseTown", LuaScriptInterface::luaGetHouseTown);

	//getHouseAccessList(houseid, listid)
	lua_register(m_luaState, "getHouseAccessList", LuaScriptInterface::luaGetHouseAccessList);

	//getHouseByPlayerGUID(playerGUID)
	lua_register(m_luaState, "getHouseByPlayerGUID", LuaScriptInterface::luaGetHouseByPlayerGUID);

	//getHouseFromPos(pos)
	lua_register(m_luaState, "getHouseFromPos", LuaScriptInterface::luaGetHouseFromPos);

	//getHouseTilesSize(houseid)
	lua_register(m_luaState, "getHouseTilesSize", LuaScriptInterface::luaGetHouseTilesSize);

	//setHouseAccessList(houseid, listid, listtext)
	lua_register(m_luaState, "setHouseAccessList", LuaScriptInterface::luaSetHouseAccessList);

	//getDepotId(uid)
	lua_register(m_luaState, "getDepotId", LuaScriptInterface::luaGetDepotId);

	//setHouseOwner(houseid, ownerGUID[, clean])
	lua_register(m_luaState, "setHouseOwner", LuaScriptInterface::luaSetHouseOwner);

	//getWorldType()
	lua_register(m_luaState, "getWorldType", LuaScriptInterface::luaGetWorldType);

	//setWorldType(type)
	lua_register(m_luaState, "setWorldType", LuaScriptInterface::luaSetWorldType);

	//getWorldTime()
	lua_register(m_luaState, "getWorldTime", LuaScriptInterface::luaGetWorldTime);

	//getWorldLight()
	lua_register(m_luaState, "getWorldLight", LuaScriptInterface::luaGetWorldLight);

	//getWorldCreatures(type)
	//0 players, 1 monsters, 2 npcs, 3 all
	lua_register(m_luaState, "getWorldCreatures", LuaScriptInterface::luaGetWorldCreatures);

	//getWorldUpTime()
	lua_register(m_luaState, "getWorldUpTime", LuaScriptInterface::luaGetWorldUpTime);

	//doBroadcastMessage(message, type)
	lua_register(m_luaState, "doBroadcastMessage", LuaScriptInterface::luaDoBroadcastMessage);

	//doPlayerBroadcastMessage(cid, message[, type])
	lua_register(m_luaState, "doPlayerBroadcastMessage", LuaScriptInterface::luaDoPlayerBroadcastMessage);

	//getGuildId(guildName)
	lua_register(m_luaState, "getGuildId", LuaScriptInterface::luaGetGuildId);

	//getGuildMotd(guildId)
	lua_register(m_luaState, "getGuildMotd", LuaScriptInterface::luaGetGuildMotd);

	//getPlayerSex(cid)
	lua_register(m_luaState, "getPlayerSex", LuaScriptInterface::luaGetPlayerSex);

	//doPlayerSetSex(cid, newSex)
	lua_register(m_luaState, "doPlayerSetSex", LuaScriptInterface::luaDoPlayerSetSex);

	//createCombatArea({area}[, {extArea}])
	lua_register(m_luaState, "createCombatArea", LuaScriptInterface::luaCreateCombatArea);

	//createConditionObject(type[, ticks[, buff[, subId]]])
	lua_register(m_luaState, "createConditionObject", LuaScriptInterface::luaCreateConditionObject);

	//setCombatArea(combat, area)
	lua_register(m_luaState, "setCombatArea", LuaScriptInterface::luaSetCombatArea);

	//setCombatCondition(combat, condition)
	lua_register(m_luaState, "setCombatCondition", LuaScriptInterface::luaSetCombatCondition);

	//setCombatParam(combat, key, value)
	lua_register(m_luaState, "setCombatParam", LuaScriptInterface::luaSetCombatParam);

	//setConditionParam(condition, key, value)
	lua_register(m_luaState, "setConditionParam", LuaScriptInterface::luaSetConditionParam);

	//addDamageCondition(condition, rounds, time, value)
	lua_register(m_luaState, "addDamageCondition", LuaScriptInterface::luaAddDamageCondition);

	//addOutfitCondition(condition, lookTypeEx, lookType, lookHead, lookBody, lookLegs, lookFeet)
	lua_register(m_luaState, "addOutfitCondition", LuaScriptInterface::luaAddOutfitCondition);

	//setCombatCallBack(combat, key, function_name)
	lua_register(m_luaState, "setCombatCallback", LuaScriptInterface::luaSetCombatCallBack);

	//setCombatFormula(combat, type, mina, minb, maxa, maxb)
	lua_register(m_luaState, "setCombatFormula", LuaScriptInterface::luaSetCombatFormula);

	//setConditionFormula(combat, mina, minb, maxa, maxb)
	lua_register(m_luaState, "setConditionFormula", LuaScriptInterface::luaSetConditionFormula);

	//doCombat(cid, combat, param)
	lua_register(m_luaState, "doCombat", LuaScriptInterface::luaDoCombat);

	//createCombatObject()
	lua_register(m_luaState, "createCombatObject", LuaScriptInterface::luaCreateCombatObject);

	//doAreaCombatHealth(cid, type, pos, area, min, max, effect)
	lua_register(m_luaState, "doAreaCombatHealth", LuaScriptInterface::luaDoAreaCombatHealth);

	//doTargetCombatHealth(cid, target, type, min, max, effect)
	lua_register(m_luaState, "doTargetCombatHealth", LuaScriptInterface::luaDoTargetCombatHealth);

	//doAreaCombatMana(cid, pos, area, min, max, effect)
	lua_register(m_luaState, "doAreaCombatMana", LuaScriptInterface::luaDoAreaCombatMana);

	//doTargetCombatMana(cid, target, min, max, effect)
	lua_register(m_luaState, "doTargetCombatMana", LuaScriptInterface::luaDoTargetCombatMana);

	//doAreaCombatCondition(cid, pos, area, condition, effect)
	lua_register(m_luaState, "doAreaCombatCondition", LuaScriptInterface::luaDoAreaCombatCondition);

	//doTargetCombatCondition(cid, target, condition, effect)
	lua_register(m_luaState, "doTargetCombatCondition", LuaScriptInterface::luaDoTargetCombatCondition);

	//doAreaCombatDispel(cid, pos, area, type, effect)
	lua_register(m_luaState, "doAreaCombatDispel", LuaScriptInterface::luaDoAreaCombatDispel);

	//doTargetCombatDispel(cid, target, type, effect)
	lua_register(m_luaState, "doTargetCombatDispel", LuaScriptInterface::luaDoTargetCombatDispel);

	//doChallengeCreature(cid, target)
	lua_register(m_luaState, "doChallengeCreature", LuaScriptInterface::luaDoChallengeCreature);

	//numberToVariant(number)
	lua_register(m_luaState, "numberToVariant", LuaScriptInterface::luaNumberToVariant);

	//stringToVariant(string)
	lua_register(m_luaState, "stringToVariant", LuaScriptInterface::luaStringToVariant);

	//positionToVariant(pos)
	lua_register(m_luaState, "positionToVariant", LuaScriptInterface::luaPositionToVariant);

	//targetPositionToVariant(pos)
	lua_register(m_luaState, "targetPositionToVariant", LuaScriptInterface::luaTargetPositionToVariant);

	//variantToNumber(var)
	lua_register(m_luaState, "variantToNumber", LuaScriptInterface::luaVariantToNumber);

	//variantToString(var)
	lua_register(m_luaState, "variantToString", LuaScriptInterface::luaVariantToString);

	//variantToPosition(var)
	lua_register(m_luaState, "variantToPosition", LuaScriptInterface::luaVariantToPosition);

	//doChangeSpeed(cid, delta)
	lua_register(m_luaState, "doChangeSpeed", LuaScriptInterface::luaDoChangeSpeed);

	//doCreatureChangeOutfit(cid, outfit)
	lua_register(m_luaState, "doCreatureChangeOutfit", LuaScriptInterface::luaDoCreatureChangeOutfit);

	//doSetMonsterOutfit(cid, name, time)
	lua_register(m_luaState, "doSetMonsterOutfit", LuaScriptInterface::luaSetMonsterOutfit);

	//doSetItemOutfit(cid, item, time)
	lua_register(m_luaState, "doSetItemOutfit", LuaScriptInterface::luaSetItemOutfit);

	//getItemProtection(uid)
	lua_register(m_luaState, "getItemProtection", LuaScriptInterface::luaGetItemProtection);

	//doSetItemProtection(uid, value)
	lua_register(m_luaState, "doSetItemProtection", LuaScriptInterface::luaDoSetItemProtection);

	//doSetCreatureOutfit(cid, outfit, time)
	lua_register(m_luaState, "doSetCreatureOutfit", LuaScriptInterface::luaSetCreatureOutfit);

	//getCreatureOutfit(cid)
	lua_register(m_luaState, "getCreatureOutfit", LuaScriptInterface::luaGetCreatureOutfit);

	//getCreaturePosition(cid)
	lua_register(m_luaState, "getCreaturePosition", LuaScriptInterface::luaGetCreaturePosition);

	//getCreatureName(cid)
	lua_register(m_luaState, "getCreatureName", LuaScriptInterface::luaGetCreatureName);

	//getCreatureSpeed(cid)
	lua_register(m_luaState, "getCreatureSpeed", LuaScriptInterface::luaGetCreatureSpeed);

	//getCreatureBaseSpeed(cid)
	lua_register(m_luaState, "getCreatureBaseSpeed", LuaScriptInterface::luaGetCreatureBaseSpeed);

	//getCreatureTarget(cid)
	lua_register(m_luaState, "getCreatureTarget", LuaScriptInterface::luaGetCreatureTarget);

	//isItemStackable(itemid)
	lua_register(m_luaState, "isItemStackable", LuaScriptInterface::luaIsItemStackable);

	//isItemRune(itemid)
	lua_register(m_luaState, "isItemRune", LuaScriptInterface::luaIsItemRune);

	//isItemFluidContainer(itemid)
	lua_register(m_luaState, "isItemFluidContainer", LuaScriptInterface::luaIsItemFluidContainer);

	//isItemContainer(itemid)
	lua_register(m_luaState, "isItemContainer", LuaScriptInterface::luaIsItemContainer);

	//isItemMovable(itemid)
	lua_register(m_luaState, "isItemMovable", LuaScriptInterface::luaIsItemMovable);

	//isItemDoor(itemid)
	lua_register(m_luaState, "isItemDoor", LuaScriptInterface::luaIsItemDoor);

	//getItemLevelDoor(itemid)
	lua_register(m_luaState, "getItemLevelDoor", LuaScriptInterface::luaGetItemLevelDoor);

	//getItemDescriptionsById(itemid)
	lua_register(m_luaState, "getItemDescriptionsById", LuaScriptInterface::luaGetItemDescriptionsById);

	//getItemWeightById(itemid, count[, precise = TRUE])
	lua_register(m_luaState, "getItemWeightById", LuaScriptInterface::luaGetItemWeightById);

	//getItemDescriptions(uid)
	lua_register(m_luaState, "getItemDescriptions", LuaScriptInterface::luaGetItemDescriptions);

	//getItemWeight(uid[, precise = TRUE])
	lua_register(m_luaState, "getItemWeight", LuaScriptInterface::luaGetItemWeight);

	//setItemName(uid)
	lua_register(m_luaState, "setItemName", LuaScriptInterface::luaSetItemName);

	//setItemPluralName(uid)
	lua_register(m_luaState, "setItemPluralName", LuaScriptInterface::luaSetItemPluralName);

	//setItemArticle(uid)
	lua_register(m_luaState, "setItemArticle", LuaScriptInterface::luaSetItemArticle);

	//getItemAttack(uid)
	lua_register(m_luaState, "getItemAttack", LuaScriptInterface::luaGetItemAttack);

	//setItemAttack(uid, attack)
	lua_register(m_luaState, "setItemAttack", LuaScriptInterface::luaSetItemAttack);

	//getItemExtraAttack(uid)
	lua_register(m_luaState, "getItemExtraAttack", LuaScriptInterface::luaGetItemExtraAttack);

	//setItemExtraAttack(uid, extraattack)
	lua_register(m_luaState, "setItemExtraAttack", LuaScriptInterface::luaSetItemExtraAttack);

	//getItemDefense(uid)
	lua_register(m_luaState, "getItemDefense", LuaScriptInterface::luaGetItemDefense);

	//setItemDefense(uid, defense)
	lua_register(m_luaState, "setItemDefense", LuaScriptInterface::luaSetItemDefense);

	//getItemExtraDefense(uid)
	lua_register(m_luaState, "getItemExtraDefense", LuaScriptInterface::luaGetItemExtraDefense);

	//setItemExtraDefense(uid, extradefense)
	lua_register(m_luaState, "setItemExtraDefense", LuaScriptInterface::luaSetItemExtraDefense);

	//getItemArmor(uid)
	lua_register(m_luaState, "getItemArmor", LuaScriptInterface::luaGetItemArmor);

	//setItemArmor(uid, armor)
	lua_register(m_luaState, "setItemArmor", LuaScriptInterface::luaSetItemArmor);

	//getItemAttackSpeed(uid)
	lua_register(m_luaState, "getItemAttackSpeed", LuaScriptInterface::luaGetItemAttackSpeed);

	//setItemAttackSpeed(uid, attackspeed)
	lua_register(m_luaState, "setItemAttackSpeed", LuaScriptInterface::luaSetItemAttackSpeed);

	//getItemHitChance(uid)
	lua_register(m_luaState, "getItemHitChance", LuaScriptInterface::luaGetItemHitChance);

	//setItemHitChance(uid, hitChance)
	lua_register(m_luaState, "setItemHitChance", LuaScriptInterface::luaSetItemHitChance);

	//getItemShootRange(uid)
	lua_register(m_luaState, "getItemShootRange", LuaScriptInterface::luaGetItemShootRange);

	//setItemShootRange(uid, shootRange)
	lua_register(m_luaState, "setItemShootRange", LuaScriptInterface::luaSetItemShootRange);

	//hasProperty(uid)
	lua_register(m_luaState, "hasProperty", LuaScriptInterface::luaHasProperty);

	//getItemIdByName(name[, reportError])
	lua_register(m_luaState, "getItemIdByName", LuaScriptInterface::luaGetItemIdByName);

	//isSightClear(fromPos, toPos, floorCheck)
	lua_register(m_luaState, "isSightClear", LuaScriptInterface::luaIsSightClear);

	//getFluidSourceType(type)
	lua_register(m_luaState, "getFluidSourceType", LuaScriptInterface::luaGetFluidSourceType);

	//isInArray(array, value)
	lua_register(m_luaState, "isInArray", LuaScriptInterface::luaIsInArray);

	//wait(delay)
	lua_register(m_luaState, "wait", LuaScriptInterface::luaWait);

	//addEvent(callback, delay, ...)
	lua_register(m_luaState, "addEvent", LuaScriptInterface::luaAddEvent);

	//stopEvent(eventid)
	lua_register(m_luaState, "stopEvent", LuaScriptInterface::luaStopEvent);

	//getPlayersByAccountId(accId)
	lua_register(m_luaState, "getPlayersByAccountId", LuaScriptInterface::luaGetPlayersByAccountId);

	//getAccountIdByName(name)
	lua_register(m_luaState, "getAccountIdByName", LuaScriptInterface::luaGetAccountIdByName);

	//getAccountByName(name)
	lua_register(m_luaState, "getAccountByName", LuaScriptInterface::luaGetAccountByName);

	//getAccountIdByAccount(accName)
	lua_register(m_luaState, "getAccountIdByAccount", LuaScriptInterface::luaGetAccountIdByAccount);

	//getAccountByAccountId(accId)
	lua_register(m_luaState, "getAccountByAccountId", LuaScriptInterface::luaGetAccountByAccountId);

	//getIpByName(name)
	lua_register(m_luaState, "getIpByName", LuaScriptInterface::luaGetIpByName);

	//getPlayersByIp(ip[, mask = 0xFFFFFFFF])
	lua_register(m_luaState, "getPlayersByIp", LuaScriptInterface::luaGetPlayersByIp);

	//doPlayerPopupFYI(cid, message)
	lua_register(m_luaState, "doPlayerPopupFYI", LuaScriptInterface::luaDoPlayerPopupFYI);

	//doPlayerSendTutorial(cid, id)
	lua_register(m_luaState, "doPlayerSendTutorial", LuaScriptInterface::luaDoPlayerSendTutorial);

	//doPlayerAddMapMark(cid, pos, type[, description])
	lua_register(m_luaState, "doPlayerAddMapMark", LuaScriptInterface::luaDoPlayerAddMapMark);

	//doPlayerAddPremiumDays(cid, days)
	lua_register(m_luaState, "doPlayerAddPremiumDays", LuaScriptInterface::luaDoPlayerAddPremiumDays);

	//getPlayerPremiumDays(cid)
	lua_register(m_luaState, "getPlayerPremiumDays", LuaScriptInterface::luaGetPlayerPremiumDays);

	//doCreatureSetLookDir(cid, dir)
	lua_register(m_luaState, "doCreatureSetLookDirection", LuaScriptInterface::luaDoCreatureSetLookDir);

	//getCreatureSkullType(cid)
	lua_register(m_luaState, "getCreatureSkullType", LuaScriptInterface::luaGetCreatureSkullType);

	//doCreatureSetSkullType(cid, skull)
	lua_register(m_luaState, "doCreatureSetSkullType", LuaScriptInterface::luaDoCreatureSetSkullType);

	//getPlayerRedSkullTicks(cid)
	lua_register(m_luaState, "getPlayerRedSkullTicks", LuaScriptInterface::luaGetPlayerRedSkullTicks);

	//doPlayerSetRedSkullTicks(cid, amount)
	lua_register(m_luaState, "doPlayerSetRedSkullTicks", LuaScriptInterface::luaDoPlayerSetRedSkullTicks);

	//getPlayerBalance(cid)
	lua_register(m_luaState, "getPlayerBalance", LuaScriptInterface::luaGetPlayerBalance);

	//getPlayerViolationAccess(cid)
	lua_register(m_luaState, "getPlayerViolationAccess", LuaScriptInterface::luaGetPlayerViolationAccess);

	//getPlayerGroupName(cid)
	lua_register(m_luaState, "getPlayerGroupName", LuaScriptInterface::luaGetPlayerGroupName);

	//getPlayerBlessing(cid, blessing)
	lua_register(m_luaState, "getPlayerBlessing", LuaScriptInterface::luaGetPlayerBlessing);

	//doPlayerAddBlessing(cid, blessing)
	lua_register(m_luaState, "doPlayerAddBlessing", LuaScriptInterface::luaDoPlayerAddBlessing);

	//getPlayerStamina(cid)
	lua_register(m_luaState, "getPlayerStamina", LuaScriptInterface::luaGetPlayerStamina);

	//setPlayerStamina(cid, minutes)
	lua_register(m_luaState, "setPlayerStamina", LuaScriptInterface::luaSetPlayerStamina);

	//doPlayerAddStamina(cid, minutes)
	lua_register(m_luaState, "doPlayerAddStamina", LuaScriptInterface::luaDoPlayerAddStamina);

	//getCreatureNoMove(cid)
	lua_register(m_luaState, "getCreatureNoMove", LuaScriptInterface::luaGetCreatureNoMove);

	//doCreatureSetNoMove(cid, block)
	lua_register(m_luaState, "doCreatureSetNoMove", LuaScriptInterface::luaDoCreatureSetNoMove);

	//doPlayerResetIdleTime(cid)
	lua_register(m_luaState, "doPlayerResetIdleTime", LuaScriptInterface::luaDoPlayerResetIdleTime);

	//getPlayerRates(cid)
	lua_register(m_luaState, "getPlayerRates", LuaScriptInterface::luaGetPlayerRates);

	//doPlayerSetRate(cid, type, value)
	lua_register(m_luaState, "doPlayerSetRate", LuaScriptInterface::luaDoPlayerSetRate);

	//getPlayerPartner(cid)
	lua_register(m_luaState, "getPlayerPartner", LuaScriptInterface::luaGetPlayerPartner);

	//setPlayerPartner(cid, guid)
	lua_register(m_luaState, "setPlayerPartner", LuaScriptInterface::luaSetPlayerPartner);

	//getPlayerParty(cid)
	lua_register(m_luaState, "getPlayerParty", LuaScriptInterface::luaGetPlayerParty);

	//doPlayerJoinParty(cid, lid)
	lua_register(m_luaState, "doPlayerJoinParty", LuaScriptInterface::luaDoPlayerJoinParty);

	//getPartyMembers(lid)
	lua_register(m_luaState, "getPartyMembers", LuaScriptInterface::luaGetPartyMembers);

	//getCreatureMaster(cid)
	lua_register(m_luaState, "getCreatureMaster", LuaScriptInterface::luaGetCreatureMaster);

	//getCreatureSummons(cid)
	lua_register(m_luaState, "getCreatureSummons", LuaScriptInterface::luaGetCreatureSummons);

	//getTownId(townName)
	lua_register(m_luaState, "getTownId", LuaScriptInterface::luaGetTownId);

	//getTownName(townId)
	lua_register(m_luaState, "getTownName", LuaScriptInterface::luaGetTownName);

	//getTownTemplePosition(townId[, displayError])
	lua_register(m_luaState, "getTownTemplePosition", LuaScriptInterface::luaGetTownTemplePosition);

	//getTownHouses(townId)
	lua_register(m_luaState, "getTownHouses", LuaScriptInterface::luaGetTownHouses);

	//getSpectators(centerPos, rangex, rangey, multifloor)
	lua_register(m_luaState, "getSpectators", LuaScriptInterface::luaGetSpectators);

	//getVocationInfo(id)
	lua_register(m_luaState, "getVocationInfo", LuaScriptInterface::luaGetVocationInfo);

	//getGroupInfo(id)
	lua_register(m_luaState, "getGroupInfo", LuaScriptInterface::luaGetGroupInfo);

	//isIpBanished(ip[, mask])
	lua_register(m_luaState, "isIpBanished", LuaScriptInterface::luaIsIpBanished);

	//isPlayerNamelocked(name)
	lua_register(m_luaState, "isPlayerNamelocked", LuaScriptInterface::luaIsPlayerNamelocked);

	//isAccountBanished(accId)
	lua_register(m_luaState, "isAccountBanished", LuaScriptInterface::luaIsAccountBanished);

	//isAccountDeleted(accId)
	lua_register(m_luaState, "isAccountDeleted", LuaScriptInterface::luaIsAccountDeleted);

	//doAddIpBanishment(...)
	lua_register(m_luaState, "doAddIpBanishment", LuaScriptInterface::luaDoAddIpBanishment);

	//doAddNamelock(...)
	lua_register(m_luaState, "doAddNamelock", LuaScriptInterface::luaDoAddNamelock);

	//doAddBanishment(...)
	lua_register(m_luaState, "doAddBanishment", LuaScriptInterface::luaDoAddBanishment);

	//doAddDeletion(...)
	lua_register(m_luaState, "doAddDeletion", LuaScriptInterface::luaDoAddDeletion);

	//doAddNotation(...)
	lua_register(m_luaState, "doAddNotation", LuaScriptInterface::luaDoAddNotation);

	//doRemoveIpBanishment(ip[, mask])
	lua_register(m_luaState, "doRemoveIpBanishment", LuaScriptInterface::luaDoRemoveIpBanishment);

	//doRemoveNamelock(name)
	lua_register(m_luaState, "doRemoveNamelock", LuaScriptInterface::luaDoRemoveNamelock);

	//doRemoveBanishment(accId)
	lua_register(m_luaState, "doRemoveBanishment", LuaScriptInterface::luaDoRemoveBanishment);

	//doRemoveDeletion(accId)
	lua_register(m_luaState, "doRemoveDeletion", LuaScriptInterface::luaDoRemoveDeletion);

	//doRemoveNotations(accId)
	lua_register(m_luaState, "doRemoveNotations", LuaScriptInterface::luaDoRemoveNotations);

	//getNotationsCount(accId)
	lua_register(m_luaState, "getNotationsCount", LuaScriptInterface::luaGetNotationsCount);

	//getBanData(value)
	lua_register(m_luaState, "getBanData", LuaScriptInterface::luaGetBanData);

	//getBanReason(id)
	lua_register(m_luaState, "getBanReason", LuaScriptInterface::luaGetBanReason);

	//getBanAction(id)
	lua_register(m_luaState, "getBanAction", LuaScriptInterface::luaGetBanAction);

	//getBanList(type[, value])
	lua_register(m_luaState, "getBanList", LuaScriptInterface::luaGetBanList);

	//getExperienceStage(level)
	lua_register(m_luaState, "getExperienceStage", LuaScriptInterface::luaGetExperienceStage);

	//getDataDir()
	lua_register(m_luaState, "getDataDir", LuaScriptInterface::luaGetDataDir);

	//getLogsDir()
	lua_register(m_luaState, "getLogsDir", LuaScriptInterface::luaGetLogsDir);

	//getConfigFile()
	lua_register(m_luaState, "getConfigFile", LuaScriptInterface::luaGetConfigFile);

	//getConfigValue(key)
	lua_register(m_luaState, "getConfigValue", LuaScriptInterface::luaGetConfigValue);

	//getHighscoreString(skillId)
	lua_register(m_luaState, "getHighscoreString", LuaScriptInterface::luaGetHighscoreString);

	//getWaypointPosition(name)
	lua_register(m_luaState, "getWaypointPosition", LuaScriptInterface::luaGetWaypointPosition);

	//doWaypointAddTemporial(name, pos)
	lua_register(m_luaState, "doWaypointAddTemporial", LuaScriptInterface::luaDoWaypointAddTemporial);

	//getGameState()
	lua_register(m_luaState, "getGameState", LuaScriptInterface::luaGetGameState);

	//doSetGameState(id)
	lua_register(m_luaState, "doSetGameState", LuaScriptInterface::luaDoSetGameState);

	//doExecuteRaid(name)
	lua_register(m_luaState, "doExecuteRaid", LuaScriptInterface::luaDoExecuteRaid);

	//doReloadInfo(id[, cid])
	lua_register(m_luaState, "doReloadInfo", LuaScriptInterface::luaDoReloadInfo);

	//doSaveServer()
	lua_register(m_luaState, "doSaveServer", LuaScriptInterface::luaDoSaveServer);

	//doCleanHouse(houseId)
	lua_register(m_luaState, "doCleanHouse", LuaScriptInterface::luaDoCleanHouse);

	//doCleanMap()
	lua_register(m_luaState, "doCleanMap", LuaScriptInterface::luaDoCleanMap);

	//doRefreshMap()
	lua_register(m_luaState, "doRefreshMap", LuaScriptInterface::luaDoRefreshMap);

	//db table
	luaL_register(m_luaState, "db", LuaScriptInterface::luaDatabaseReg);

	//result table
	luaL_register(m_luaState, "result", LuaScriptInterface::luaResultReg);

	//bit table
	luaL_register(m_luaState, "bit", LuaScriptInterface::luaBitReg);
}

const luaL_Reg LuaScriptInterface::luaDatabaseReg[] =
{
	//db.executeQuery(query)
	{"executeQuery", LuaScriptInterface::luaDatabaseExecute},

	//db.storeQuery(query)
	{"storeQuery", LuaScriptInterface::luaDatabaseStoreQuery},

	//db.escapeString(str)
	{"escapeString", LuaScriptInterface::luaDatabaseEscapeString},

	//db.escapeBlob(s, length)
	{"escapeBlob", LuaScriptInterface::luaDatabaseEscapeBlob},

	//db.stringComparisonOperator()
	{"stringComparisonOperator", LuaScriptInterface::luaDatabaseStringComparisonOperator},

	{NULL,NULL}
};

const luaL_Reg LuaScriptInterface::luaResultReg[] =
{
	//result.getDataInt(resId, s)
	{"getDataInt", LuaScriptInterface::luaResultGetDataInt},

	//result.getDataLong(resId, s)
	{"getDataLong", LuaScriptInterface::luaResultGetDataLong},

	//result.getDataString(resId, s)
	{"getDataString", LuaScriptInterface::luaResultGetDataString},

	//result.getDataStream(resId, s, length)
	{"getDataStream", LuaScriptInterface::luaResultGetDataStream},

	//result.next(resId)
	{"next", LuaScriptInterface::luaResultNext},

	//result.free(resId)
	{"free", LuaScriptInterface::luaResultFree},

	{NULL,NULL}
};

const luaL_Reg LuaScriptInterface::luaBitReg[] =
{
	//{"cast", LuaScriptInterface::luaBitCast},
	{"bnot", LuaScriptInterface::luaBitNot},
	{"band", LuaScriptInterface::luaBitAnd},
	{"bor", LuaScriptInterface::luaBitOr},
	{"bxor", LuaScriptInterface::luaBitXor},
	{"lshift", LuaScriptInterface::luaBitLeftShift},
	{"rshift", LuaScriptInterface::luaBitRightShift},
	//{"arshift", LuaScriptInterface::luaBitArithmeticalRightShift},

	//Unsigned
	//{"ucast", LuaScriptInterface::luaBitUCast},
	{"ubnot", LuaScriptInterface::luaBitUNot},
	{"uband", LuaScriptInterface::luaBitUAnd},
	{"ubor", LuaScriptInterface::luaBitUOr},
	{"ubxor", LuaScriptInterface::luaBitUXor},
	{"ulshift", LuaScriptInterface::luaBitULeftShift},
	{"urshift", LuaScriptInterface::luaBitURightShift},
	//{"uarshift", LuaScriptInterface::luaBitUArithmeticalRightShift},

	{NULL,NULL}
};

int32_t LuaScriptInterface::internalGetPlayerInfo(lua_State* L, PlayerInfo_t info)
{
	int64_t value = 0;
	ScriptEnviroment* env = getScriptEnv();
	if(const Player* player = env->getPlayerByUID(popNumber(L)))
	{
		Position pos;
		switch(info)
		{
			case PlayerInfoNameDescription:
				lua_pushstring(L, player->getNameDescription().substr(player->getName().length()).c_str());
				return 1;
			case PlayerInfoAccess:
				value = player->getAccess();
				break;
			case PlayerInfoLevel:
				value = player->getLevel();
				break;
			case PlayerInfoExperience:
				value = player->getExperience();
				break;
			case PlayerInfoManaSpent:
				value = player->getSpentMana();
				break;
			case PlayerInfoTown:
				value = player->getTown();
				break;
			case PlayerInfoPromotionLevel:
				value = player->promotionLevel;
				break;
			case PlayerInfoGUID:
				value = player->getGUID();
				break;
			case PlayerInfoAccountId:
				value = player->getAccount();
				break;
			case PlayerInfoAccount:
				lua_pushstring(L, player->getAccountName().c_str());
				return 1;
			case PlayerInfoPremiumDays:
				value = player->premiumDays;
				break;
			case PlayerInfoFood:
			{
				if(Condition* condition = player->getCondition(CONDITION_REGENERATION, CONDITIONID_DEFAULT))
					value = condition->getTicks() / 1000;

				break;
			}
			case PlayerInfoVocation:
				value = player->getVocationId();
				break;
			case PlayerInfoSoul:
				value = player->getSoul();
				break;
			case PlayerInfoFreeCap:
				value = player->getFreeCapacity();
				break;
			case PlayerInfoGuildId:
				value = player->getGuildId();
				break;
			case PlayerInfoGuildName:
				lua_pushstring(L, player->getGuildName().c_str());
				return 1;
			case PlayerInfoGuildRankId:
				value = player->getGuildRankId();
				break;
			case PlayerInfoGuildRank:
				lua_pushstring(L, player->getGuildRank().c_str());
				return 1;
			case PlayerInfoGuildLevel:
				value = player->getGuildLevel();
				break;
			case PlayerInfoGuildNick:
				lua_pushstring(L, player->getGuildNick().c_str());
				return 1;
			case PlayerInfoSex:
				value = player->getSex();
				break;
			case PlayerInfoGroupId:
				value = player->getGroupId();
				break;
			case PlayerInfoGroupName:
				lua_pushstring(L, player->getGroup()->getName().c_str());
				return 1;
			case PlayerInfoBalance:
				value = (g_config.getBool(ConfigManager::BANK_SYSTEM) ? player->balance : 0);
				break;
			case PlayerInfoViolationAccess:
				value = player->getViolationAccess();
				break;
			case PlayerInfoStamina:
				value = player->getStaminaMinutes();
				break;
			case PlayerInfoLossSkill:
				lua_pushboolean(L, player->getLossSkill() ? LUA_TRUE : LUA_FALSE);
				return 1;
			case PlayerInfoMarriage:
				value = player->marriage;
				break;
			case PlayerInfoPzLock:
				lua_pushboolean(L, player->isPzLocked() ? LUA_TRUE : LUA_FALSE);
				return 1;
			case PlayerInfoSaving:
				lua_pushboolean(L, player->isSaving() ? LUA_TRUE : LUA_FALSE);
				return 1;
			case PlayerInfoIp:
				value = player->getIP();
				break;
			case PlayerInfoRedSkullTicks:
				value = player->getRedSkullTicks();
				break;
			case PlayerInfoOutfitWindow:
				player->sendOutfitWindow();
				lua_pushboolean(L, LUA_NO_ERROR);
				return 1;
			default:
				reportErrorFunc("Unknown player info - " + info);
				value = 0;
				break;
		}

		lua_pushnumber(L, value);
		return 1;
	}
	else
	{
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

//getPlayer[Info](uid)
int32_t LuaScriptInterface::luaGetPlayerNameDescription(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoNameDescription);
}

int32_t LuaScriptInterface::luaGetPlayerFood(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoFood);
}

int32_t LuaScriptInterface::luaGetPlayerAccess(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoAccess);
}

int32_t LuaScriptInterface::luaGetPlayerLevel(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoLevel);
}

int32_t LuaScriptInterface::luaGetPlayerExperience(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoExperience);
}

int32_t LuaScriptInterface::luaGetPlayerSpentMana(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoManaSpent);
}

int32_t LuaScriptInterface::luaGetPlayerVocation(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoVocation);
}

int32_t LuaScriptInterface::luaGetPlayerSoul(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoSoul);
}

int32_t LuaScriptInterface::luaGetPlayerFreeCap(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoFreeCap);
}

int32_t LuaScriptInterface::luaGetPlayerGuildId(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoGuildId);
}

int32_t LuaScriptInterface::luaGetPlayerGuildName(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoGuildName);
}

int32_t LuaScriptInterface::luaGetPlayerGuildRankId(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoGuildRankId);
}

int32_t LuaScriptInterface::luaGetPlayerGuildRank(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoGuildRank);
}

int32_t LuaScriptInterface::luaGetPlayerGuildLevel(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoGuildLevel);
}

int32_t LuaScriptInterface::luaGetPlayerGuildNick(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoGuildNick);
}

int32_t LuaScriptInterface::luaGetPlayerSex(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoSex);
}

int32_t LuaScriptInterface::luaGetPlayerTown(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoTown);
}

int32_t LuaScriptInterface::luaGetPlayerPromotionLevel(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoPromotionLevel);
}

int32_t LuaScriptInterface::luaGetPlayerGroupId(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoGroupId);
}

int32_t LuaScriptInterface::luaGetPlayerGroupName(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoGroupName);
}

int32_t LuaScriptInterface::luaGetPlayerGUID(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoGUID);
}

int32_t LuaScriptInterface::luaGetPlayerAccountId(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoAccountId);
}

int32_t LuaScriptInterface::luaGetPlayerAccount(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoAccount);
}

int32_t LuaScriptInterface::luaGetPlayerPremiumDays(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoPremiumDays);
}

int32_t LuaScriptInterface::luaGetPlayerBalance(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoBalance);
}

int32_t LuaScriptInterface::luaGetPlayerViolationAccess(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoViolationAccess);
}

int32_t LuaScriptInterface::luaGetPlayerStamina(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoStamina);
}

int32_t LuaScriptInterface::luaGetPlayerLossSkill(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoLossSkill);
}

int32_t LuaScriptInterface::luaGetPlayerPartner(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoMarriage);
}

int32_t LuaScriptInterface::luaIsPlayerPzLocked(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoPzLock);
}

int32_t LuaScriptInterface::luaIsPlayerSaving(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoSaving);
}

int32_t LuaScriptInterface::luaGetPlayerIp(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoIp);
}

int32_t LuaScriptInterface::luaGetPlayerRedSkullTicks(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoRedSkullTicks);
}

int32_t LuaScriptInterface::luaDoPlayerSendOutfitWindow(lua_State* L)
{
	return internalGetPlayerInfo(L, PlayerInfoOutfitWindow);
}
//

int32_t LuaScriptInterface::luaDoPlayerSetNameDescription(lua_State* L)
{
	//doPlayerSetNameDescription(cid, description)
	std::string description = popString(L);

	ScriptEnviroment* env = getScriptEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		player->nameDescription = player->getName() + description;
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerMagLevel(lua_State* L)
{
	//getPlayerMagLevel(cid[, ignoreBuffs])
	bool ignoreBuffs = false;
	if(lua_gettop(L) >= 2)
		ignoreBuffs = popNumber(L) == LUA_TRUE;

	ScriptEnviroment* env = getScriptEnv();
	if(const Player* player = env->getPlayerByUID(popNumber(L)))
		lua_pushnumber(L, (ignoreBuffs ? player->magLevel : player->getMagicLevel()));
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerRequiredMana(lua_State* L)
{
	//getPlayerRequiredMana(cid, magicLevel)
	uint32_t magLevel = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
		lua_pushnumber(L, player->vocation->getReqMana(magLevel));
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerRequiredSkillTries(lua_State* L)
{
	//getPlayerRequiredSkillTries(cid, skillId, skillLevel)
	int32_t sLevel = popNumber(L), sId = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
		lua_pushnumber(L, player->vocation->getReqSkillTries(sId, sLevel));
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerFlagValue(lua_State* L)
{
	//getPlayerFlagValue(cid, flag)
	uint32_t index = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		if(index < PlayerFlag_LastFlag)
			lua_pushboolean(L, player->hasFlag((PlayerFlags)index) ? LUA_TRUE : LUA_FALSE);
		else
		{
			reportErrorFunc("No valid flag index.");
			lua_pushboolean(L, LUA_ERROR);
		}
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerCustomFlagValue(lua_State* L)
{
	//getPlayerCustomFlagValue(cid, flag)
	uint32_t index = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		if(index < PlayerCustomFlag_LastFlag)
			lua_pushboolean(L, player->hasCustomFlag((PlayerCustomFlags)index) ? LUA_TRUE : LUA_FALSE);
		else
		{
			reportErrorFunc("No valid flag index.");
			lua_pushboolean(L, LUA_ERROR);
		}
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerLearnInstantSpell(lua_State* L)
{
	//doPlayerLearnInstantSpell(cid, name)
	std::string spellName = popString(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(!player)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	InstantSpell* spell = g_spells->getInstantSpellByName(spellName);
	if(!spell)
	{
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	player->learnInstantSpell(spell->getName());
	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerUnlearnInstantSpell(lua_State* L)
{
	//doPlayerUnlearnInstantSpell(cid, name)
	std::string spellName = popString(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(!player)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	InstantSpell* spell = g_spells->getInstantSpellByName(spellName);
	if(!spell)
	{
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	player->unlearnInstantSpell(spell->getName());
	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerLearnedInstantSpell(lua_State* L)
{
	//getPlayerLearnedInstantSpell(cid, name)
	std::string spellName = popString(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(!player)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	InstantSpell* spell = g_spells->getInstantSpellByName(spellName);
	if(!spell)
	{
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	if(player->hasLearnedInstantSpell(spellName))
		lua_pushboolean(L, LUA_TRUE);
	else
		lua_pushboolean(L, LUA_FALSE);

	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerInstantSpellCount(lua_State* L)
{
	//getPlayerInstantSpellCount(cid)
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	if(Player* player = env->getPlayerByUID(cid))
		lua_pushnumber(L, g_spells->getInstantSpellCount(player));
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerInstantSpellInfo(lua_State* L)
{
	//getPlayerInstantSpellInfo(cid, index)
	uint32_t index = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(!player)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	InstantSpell* spell = g_spells->getInstantSpellByIndex(player, index);
	if(!spell)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_SPELL_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	lua_newtable(L);
	setField(L, "name", spell->getName());
	setField(L, "words", spell->getWords());
	setField(L, "level", spell->getLevel());
	setField(L, "mlevel", spell->getMagicLevel());
	setField(L, "mana", spell->getManaCost(player));
	setField(L, "manapercent", spell->getManaPercent());
	return 1;
}

int32_t LuaScriptInterface::luaGetInstantSpellInfo(lua_State* L)
{
	//getInstantSpellInfo(name)
	std::string spellName = popString(L);

	InstantSpell* spell = g_spells->getInstantSpellByName(spellName);
	if(!spell)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_SPELL_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	lua_newtable(L);
	setField(L, "name", spell->getName());
	setField(L, "words", spell->getWords());
	setField(L, "level", spell->getLevel());
	setField(L, "mlevel", spell->getMagicLevel());
	setField(L, "mana", spell->getManaCost(NULL));
	setField(L, "manapercent", spell->getManaPercent());
	return 1;
}

int32_t LuaScriptInterface::luaDoRemoveItem(lua_State* L)
{
	//doRemoveItem(uid[, count])
	int32_t parameters = lua_gettop(L);

	int32_t count = -1;
	if(parameters > 1)
		count = popNumber(L);

	uint32_t uid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Item* item = env->getItemByUID(uid);
	if(!item)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	ReturnValue ret = g_game.internalRemoveItem(NULL, item, count);
	if(ret != RET_NOERROR)
	{
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerRemoveItem(lua_State* L)
{
	//doPlayerRemoveItem(cid, itemid, count[, subType])
	int32_t parameters = lua_gettop(L);

	int32_t subType = -1;
	if(parameters > 3)
		subType = popNumber(L);

	uint32_t count = popNumber(L);
	uint16_t itemId = (uint16_t)popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		if(g_game.removeItemOfType(player, itemId, count, subType))
			lua_pushboolean(L, LUA_TRUE);
		else
			lua_pushboolean(L, LUA_FALSE);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoFeedPlayer(lua_State* L)
{
	//doFeedPlayer(cid, food)
	int32_t food = (int32_t)popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		player->addDefaultRegeneration((food * 1000) * 3);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSendCancel(lua_State* L)
{
	//doPlayerSendCancel(cid, text)
	const char * text = popString(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	const Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		player->sendCancel(text);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoSendDefaultCancel(lua_State* L)
{
	//doPlayerSendDefaultCancel(cid, ReturnValue)
	ReturnValue ret = (ReturnValue)popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	const Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		player->sendCancelMessage(ret);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetSearchString(lua_State* L)
{
	//getSearchString(fromPosition, toPosition[, fromIsCreature[, toIsCreature]])
	int32_t params = lua_gettop(L);
	bool toIsCreature = false, fromIsCreature = false;

	if(params >= 4)
		toIsCreature = popNumber(L) == LUA_TRUE;

	if(params >= 3)
		fromIsCreature = popNumber(L) == LUA_TRUE;

	PositionEx toPos, fromPos;
	popPosition(L, toPos);
	popPosition(L, fromPos);

	if(toPos.x > 0 && fromPos.x > 0)
		lua_pushstring(L, g_game.getSearchString(fromPos, toPos, fromIsCreature, toIsCreature).c_str());
	else
	{
		reportErrorFunc("wrong position(s) specified.");
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetClosestFreeTile(lua_State* L)
{
	//getClosestFreeTile(cid, targetPos[, extended[, ignoreHouse]])
	uint32_t params = lua_gettop(L);
	bool ignoreHouse = true, extended = false;
	if(params >= 4)
		ignoreHouse = popNumber(L) == LUA_TRUE;

	if(params >= 3)
		extended = popNumber(L) == LUA_TRUE;

	PositionEx pos;
	popPosition(L, pos);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	if(Creature* creature = env->getCreatureByUID(cid))
	{
		Position newPos = g_game.getClosestFreeTile(creature, pos, extended, ignoreHouse);
		if(newPos.x != 0)
			pushPosition(L, newPos, 0);
		else
			lua_pushboolean(L, LUA_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoTeleportThing(lua_State* L)
{
	//doTeleportThing(cid, newpos[, pushmove = TRUE])
	int32_t parameters = lua_gettop(L);

	bool pushMove = true;
	if(parameters > 2)
		pushMove = popNumber(L) == 1;

	PositionEx pos;
	popPosition(L, pos);
	uint32_t uid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Thing* tmp = env->getThingByUID(uid);
	if(tmp)
	{
		if(g_game.internalTeleport(tmp, pos, pushMove) == RET_NOERROR)
			lua_pushboolean(L, LUA_NO_ERROR);
		else
			lua_pushboolean(L, LUA_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_THING_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoTransformItem(lua_State* L)
{
	//doTransformItem(uid, newId[, count/subType])
	int32_t parameters = lua_gettop(L);

	int32_t count = -1;
	if(parameters > 2)
		count = popNumber(L);

	uint16_t toId = (uint16_t)popNumber(L);
	uint32_t uid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Item* item = env->getItemByUID(uid);
	if(!item)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	const ItemType& it = Item::items[toId];
	if(it.stackable && count > 100)
	{
		reportErrorFunc("Stack count cannot be higher than 100.");
		count = 100;
	}

	Item* newItem = g_game.transformItem(item, toId, count);

	if(item->isRemoved())
		env->removeItemByUID(uid);

	if(newItem && newItem != item)
		env->insertThing(uid, newItem);

	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaDoCreatureSay(lua_State* L)
{
	//doCreatureSay(uid,text,type[,pos])
	uint32_t params = lua_gettop(L);

	PositionEx pos;
	if(params >= 4)
		popPosition(L, pos);

	uint32_t type = popNumber(L);
	std::string text = popString(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	if(Creature* creature = env->getCreatureByUID(cid))
	{
		if(params >= 4)
		{
			if(pos.x == 0 || pos.y == 0)
			{
				reportErrorFunc("Invalid position specified.");
				lua_pushboolean(L, LUA_ERROR);
				return 1;
			}

			g_game.internalCreatureSay(creature, (SpeakClasses)type, text, &pos);
		}
		else
			g_game.internalCreatureSay(creature, (SpeakClasses)type, text);

		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoSendMagicEffect(lua_State* L)
{
	//doSendMagicEffect(pos, type[, player])
	ScriptEnviroment* env = getScriptEnv();

	SpectatorVec list;
	if(lua_gettop(L) >= 3)
	{
		if(Creature* creature = env->getCreatureByUID(popNumber(L)))
			list.push_back(creature);
	}

	uint32_t type = popNumber(L);
	PositionEx pos;
	popPosition(L, pos);

	if(pos.x == 0xFFFF)
		pos = env->getRealPos();

	if(!list.empty())
		g_game.addMagicEffect(list, pos, type);
	else
		g_game.addMagicEffect(pos, type);

	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaDoSendDistanceShoot(lua_State* L)
{
	//doSendDistanceShoot(fromPos, toPos, type[, player])
	ScriptEnviroment* env = getScriptEnv();

	SpectatorVec list;
	if(lua_gettop(L) >= 4)
	{
		if(Creature* creature = env->getCreatureByUID(popNumber(L)))
			list.push_back(creature);
	}

	uint32_t type = popNumber(L);
	PositionEx toPos, fromPos;
	popPosition(L, toPos);
	popPosition(L, fromPos);

	if(fromPos.x == 0xFFFF)
		fromPos = env->getRealPos();

	if(toPos.x == 0xFFFF)
		toPos = env->getRealPos();

	if(!list.empty())
		g_game.addDistanceEffect(list, fromPos, toPos, type);
	else
		g_game.addDistanceEffect(fromPos, toPos, type);

	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaDoChangeTypeItem(lua_State* L)
{
	//doChangeTypeItem(uid, new_type)
	int32_t subtype = (int32_t)popNumber(L);
	uint32_t uid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Item* item = env->getItemByUID(uid);
	if(!item)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	Item* newItem = g_game.transformItem(item, item->getID(), subtype);

	if(item->isRemoved())
		env->removeItemByUID(uid);

	if(newItem && newItem != item)
		env->insertThing(uid, newItem);

	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerAddSkillTry(lua_State* L)
{
	//doPlayerAddSkillTry(uid, skillid, n)
	uint32_t n = popNumber(L);
	uint32_t skillid = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		player->addSkillAdvance((skills_t)skillid, n);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}


int32_t LuaScriptInterface::luaDoCreatureAddHealth(lua_State* L)
{
	//doCreatureAddHealth(uid, health[, force])
	bool force = false;
	if(lua_gettop(L) >= 3)
		force = popNumber(L) == LUA_TRUE;

	int32_t healthChange = (int32_t)popNumber(L);
	ScriptEnviroment* env = getScriptEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
	{
		g_game.combatChangeHealth(healthChange >= 0 ? COMBAT_HEALING : COMBAT_UNDEFINEDDAMAGE,
			NULL, creature, healthChange, force);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoCreatureAddMana(lua_State* L)
{
	//doCreatureAddMana(uid, mana[, aggressive])
	bool aggressive = true;
	if(lua_gettop(L) >= 3)
		aggressive = popNumber(L) == LUA_TRUE;

	int32_t manaChange = popNumber(L);
	ScriptEnviroment* env = getScriptEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
	{
		if(aggressive)
			g_game.combatChangeMana(NULL, creature, manaChange);
		else
			creature->changeMana(manaChange);

		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerAddSpentMana(lua_State* L)
{
	//doPlayerAddSpentMana(cid, amount)
	uint32_t amount = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		player->addManaSpent(amount);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerAddItem(lua_State* L)
{
	//doPlayerAddItem(cid, itemid[, count/subType[, canDropOnMap = TRUE]])
	uint32_t params = lua_gettop(L);

	bool canDropOnMap = true;
	if(params >= 4)
		canDropOnMap = (popNumber(L) == LUA_TRUE);

	uint32_t count = 0;
	if(params >= 3)
		count = popNumber(L);

	uint32_t itemId = (uint32_t)popNumber(L);
	ScriptEnviroment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(popNumber(L));
	if(!player)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	const ItemType& it = Item::items[itemId];
	if(it.stackable && count > 100)
	{
		int32_t subCount = count;
		while(subCount > 0)
		{
			int32_t stackCount = std::min((int32_t)100, (int32_t)subCount);
			Item* newItem = Item::CreateItem(itemId, stackCount);

			if(!newItem)
			{
				reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
				lua_pushboolean(L, LUA_ERROR);
				return 1;
			}

			ReturnValue ret = g_game.internalPlayerAddItem(NULL, player, newItem, canDropOnMap);
			if(ret != RET_NOERROR)
			{
				delete newItem;
				lua_pushboolean(L, LUA_ERROR);
				return 1;
			}

			subCount = subCount - stackCount;
			if(subCount == 0)
			{
				if(newItem->getParent())
					lua_pushnumber(L, env->addThing((Thing*)newItem));
				else //stackable item stacked with existing object, newItem will be released
					lua_pushnil(L);

				return 1;
			}
		}
	}
	else
	{
		Item* newItem = Item::CreateItem(itemId, count);
		if(!newItem)
		{
			reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
			lua_pushboolean(L, LUA_ERROR);
			return 1;
		}

		ReturnValue ret = g_game.internalPlayerAddItem(NULL, player, newItem, canDropOnMap);
		if(ret != RET_NOERROR)
		{
			delete newItem;
			lua_pushboolean(L, LUA_ERROR);
			return 1;
		}

		if(newItem->getParent())
			lua_pushnumber(L, env->addThing((Thing*)newItem));
		else //stackable item stacked with existing object, newItem will be released
			lua_pushnil(L);

		return 1;
	}
	return 0;
}

int32_t LuaScriptInterface::luaDoPlayerAddItemEx(lua_State* L)
{
	//doPlayerAddItemEx(cid, uid[, canDropOnMap = FALSE])
	bool canDropOnMap = false;
	if(lua_gettop(L) >= 3)
		canDropOnMap = (popNumber(L) == 1);

	uint32_t uid = (uint32_t)popNumber(L);
	ScriptEnviroment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(popNumber(L));
	if(!player)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	Item* item = env->getItemByUID(uid);
	if(!item)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	if(item->getParent() != VirtualCylinder::virtualCylinder)
	{
		reportErrorFunc("Item already has a parent");
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	lua_pushnumber(L, g_game.internalPlayerAddItem(NULL, player, item, canDropOnMap));
	return 1;
}

int32_t LuaScriptInterface::luaDoTileAddItemEx(lua_State* L)
{
	//doTileAddItemEx(pos, uid)
	uint32_t uid = (uint32_t)popNumber(L);
	PositionEx pos;
	popPosition(L, pos);

	ScriptEnviroment* env = getScriptEnv();
	Tile* tile = g_game.getTile(pos.x, pos.y, pos.z);
	if(!tile)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_TILE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	Item* item = env->getItemByUID(uid);
	if(!item)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	if(item->getParent() != VirtualCylinder::virtualCylinder)
	{
		reportErrorFunc("Item already has a parent");
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	lua_pushnumber(L, g_game.internalAddItem(NULL, tile, item));
	return 1;
}

int32_t LuaScriptInterface::luaDoRelocate(lua_State* L)
{
	//doRelocate(pos, posTo)
	//Moves all moveable objects from pos to posTo

	PositionEx toPos;
	popPosition(L, toPos);

	PositionEx fromPos;
	popPosition(L, fromPos);

	Tile* fromTile = g_game.getTile(fromPos.x, fromPos.y, fromPos.z);
	if(!fromTile)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_TILE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	Tile* toTile = g_game.getTile(toPos.x, toPos.y, toPos.z);
	if(!toTile)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_TILE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	if(fromTile != toTile)
	{
		int32_t thingCount = fromTile->getThingCount();
		for(int32_t i = thingCount - 1; i >= 0; --i)
		{
			Thing* thing = fromTile->__getThing(i);
			if(thing)
			{
				if(Item* item = thing->getItem())
				{
					const ItemType& it = Item::items[item->getID()];
					if(!it.isGroundTile() && !it.alwaysOnTop && !it.isMagicField())
						g_game.internalTeleport(item, toPos, false, FLAG_IGNORENOTMOVEABLE);
				}
				else if(Creature* creature = thing->getCreature())
					g_game.internalTeleport(creature, toPos, true);
			}
		}
	}

	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSendTextMessage(lua_State* L)
{
	//doPlayerSendTextMessage(cid, MessageClasses, message)
	std::string text = popString(L);
	uint32_t messageClass = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(popNumber(L));
	if(!player)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	player->sendTextMessage((MessageClasses)messageClass, text);
	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSendChannelMessage(lua_State* L)
{
	//doPlayerSendChannelMessage(cid, author, message, SpeakClasses, channel)
	uint16_t channelId = popNumber(L);
	uint32_t speakClass = popNumber(L);
	std::string text = popString(L);
	std::string name = popString(L);

	ScriptEnviroment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(popNumber(L));
	if(!player)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	player->sendChannelMessage(name, text, (SpeakClasses)speakClass, channelId);
	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSendToChannel(lua_State* L)
{
	//doPlayerSendToChannel(cid, targetId, SpeakClasses, message, channel[, time])
	ScriptEnviroment* env = getScriptEnv();
	uint32_t time = 0;
	if(lua_gettop(L) >= 6)
		time = popNumber(L);

	uint16_t channelId = popNumber(L);
	std::string text = popString(L);
	uint32_t speakClass = popNumber(L);
	uint32_t targetId = popNumber(L);

	Player* player = env->getPlayerByUID(popNumber(L));
	if(!player)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	Creature* creature = env->getCreatureByUID(targetId);
	if(!creature)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	player->sendToChannel(creature, (SpeakClasses)speakClass, text, channelId, time);
	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaDoSendAnimatedText(lua_State* L)
{
	//doSendAnimatedText(pos, text, color[, player])
	ScriptEnviroment* env = getScriptEnv();

	SpectatorVec list;
	if(lua_gettop(L) >= 4)
	{
		if(Creature* creature = env->getCreatureByUID(popNumber(L)))
			list.push_back(creature);
	}

	uint32_t color = popNumber(L);
	std::string text = popString(L);
	PositionEx pos;
	popPosition(L, pos);

	if(pos.x == 0xFFFF)
		pos = env->getRealPos();

	if(!list.empty())
		g_game.addAnimatedText(list, pos, color, text);
	else
		g_game.addAnimatedText(pos, color, text);

	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerSkillLevel(lua_State* L)
{
	//getPlayerSkillLevel(cid, skillid)
	uint32_t skillId = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	if(const Player* player = env->getPlayerByUID(popNumber(L)))
	{
		if(skillId <= 6)
			lua_pushnumber(L, player->skills[skillId][SKILL_LEVEL]);
		else
		{
			reportErrorFunc("Invalid skillId");
			lua_pushboolean(L, LUA_ERROR);
		}
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerSkillTries(lua_State* L)
{
	//getPlayerSkillTries(cid, skillid)
	uint32_t skillid = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	const Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		if(skillid <= 6)
			lua_pushnumber(L, player->skills[skillid][SKILL_TRIES]);
		else
		{
			reportErrorFunc("No valid skillId");
			lua_pushboolean(L, LUA_ERROR);
		}
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoCreatureSetDropLoot(lua_State* L)
{
	//doCreatureSetDropLoot(cid, doDrop)
	bool doDrop = popNumber(L) == LUA_TRUE;
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	if(Creature* creature = env->getCreatureByUID(cid))
	{
		creature->setDropLoot(doDrop ? LOOT_DROP_FULL : LOOT_DROP_NONE);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerLossPercent(lua_State *L)
{
	//getPlayerLossPercent(cid, lossType)
	uint8_t lossType = (uint8_t)popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	const Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		if(lossType <= LOSS_LAST)
		{
			uint32_t value = player->getLossPercent((lossTypes_t)lossType);
			lua_pushnumber(L, value);
		}
		else
		{
			reportErrorFunc("No valid lossType");
			lua_pushboolean(L, LUA_ERROR);
		}
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetLossPercent(lua_State *L)
{
	//doPlayerSetLossPercent(cid, lossType, newPercent)
	uint32_t newPercent = popNumber(L);
	uint8_t lossType = (uint8_t)popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		if(lossType <= LOSS_LAST)
		{
			if(newPercent <= 100)
			{
				player->setLossPercent((lossTypes_t)lossType, newPercent);
				lua_pushboolean(L, LUA_NO_ERROR);
			}
			else
			{
				reportErrorFunc("lossPercent value higher than 100");
				lua_pushboolean(L, LUA_ERROR);
			}
		}
		else
		{
			reportErrorFunc("No valid lossType");
			lua_pushboolean(L, LUA_ERROR);
		}
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetLossSkill(lua_State* L)
{
	//doPlayerSetLossSkill(cid, doLose)
	bool doLose = popNumber(L) == 1;
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		player->setLossSkill(doLose);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoShowTextDialog(lua_State* L)
{
	//doShowTextDialog(cid, itemid, text)
	const char* text = popString(L);
	uint32_t itemId = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		player->setWriteItem(NULL, 0);
		player->sendTextWindow(itemId, text);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetItemWeaponType(lua_State* L)
{
	//getItemWeaponType(uid)
	ScriptEnviroment* env = getScriptEnv();
	if(const Item* item = env->getItemByUID(popNumber(L)))
		lua_pushnumber(L, item->getWeaponType());
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetItemRWInfo(lua_State* L)
{
	//getItemRWInfo(uid)
	ScriptEnviroment* env = getScriptEnv();
	if(const Item* item = env->getItemByUID(popNumber(L)))
	{
		uint32_t flags = 0;
		if(item->isReadable())
			flags |= 1;

		if(item->canWriteText())
			flags |= 2;

		lua_pushnumber(L, flags);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoDecayItem(lua_State* L)
{
	//doDecayItem(uid)
	//Note: to stop decay set decayTo = 0 in items.otb
	uint32_t uid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Item* item = env->getItemByUID(uid);
	if(item)
	{
		g_game.startDecay(item);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetThingFromPos(lua_State* L)
{
	//Consider using getTileItemById/getTileItemByType/getTileThingByPos/getTopCreature instead.

	//getThingFromPos(pos[, displayError])
	//Note:
	//	stackpos = 255. Get the top thing(item moveable or creature)
	//	stackpos = 254. Get MagicFieldtItem
	//	stackpos = 253. Get the top creature (moveable creature)

	bool displayError = true;
	if(lua_gettop(L) >= 2)
		displayError = popNumber(L) == LUA_TRUE;

	PositionEx pos;
	popPosition(L, pos);

	ScriptEnviroment* env = getScriptEnv();

	Tile* tile = g_game.getMap()->getTile(pos);
	Thing *thing = NULL;

	if(tile)
	{
		if(pos.stackpos == 255)
		{
			thing = tile->getTopCreature();
			if(thing == NULL)
			{
				Item* item = tile->getTopDownItem();
				if(item && !item->isNotMoveable())
					thing = item;
			}
		}
		else if(pos.stackpos == 254)
			thing = tile->getFieldItem();
		else if(pos.stackpos == 253)
			thing = tile->getTopCreature();
		else
			thing = tile->__getThing(pos.stackpos);

		if(thing)
			pushThing(L, thing, env->addThing(thing));
		else
			pushThing(L, NULL, 0);

		return 1;
	}

	if(displayError)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_TILE_NOT_FOUND));
		pushThing(L, NULL, 0);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetTileItemById(lua_State* L)
{
	//getTileItemById(pos, itemId[, subType])
	ScriptEnviroment* env = getScriptEnv();

	uint32_t parameters = lua_gettop(L);

	int32_t subType = -1;
	if(parameters > 2)
		subType = (int32_t)popNumber(L);

	int32_t itemId = (int32_t)popNumber(L);

	PositionEx pos;
	popPosition(L, pos);

	Tile* tile = g_game.getTile(pos.x, pos.y, pos.z);
	if(!tile)
	{
		pushThing(L, NULL, 0);
		return 1;
	}

	Item* item = g_game.findItemOfType(tile, itemId, false, subType);
	if(!item)
	{
		pushThing(L, NULL, 0);
		return 1;
	}

	pushThing(L, item, env->addThing(item));
	return 1;
}

int32_t LuaScriptInterface::luaGetTileItemByType(lua_State* L)
{
	//getTileItemByType(pos, type)

	ScriptEnviroment* env = getScriptEnv();

	uint32_t rType = (uint32_t)popNumber(L);

	if(rType >= ITEM_TYPE_LAST)
	{
		reportErrorFunc("Not a valid item type");
		pushThing(L, NULL, 0);
		return 1;
	}

	PositionEx pos;
	popPosition(L, pos);

	Tile* tile = g_game.getTile(pos.x, pos.y, pos.z);
	if(!tile)
	{
		pushThing(L, NULL, 0);
		return 1;
	}

	for(uint32_t i = 0; i < tile->getThingCount(); ++i)
	{
		if(Item* item = tile->__getThing(i)->getItem())
		{
			const ItemType& it = Item::items[item->getID()];
			if(it.type == (ItemTypes_t) rType)
			{
				pushThing(L, item, env->addThing(item));
				return 1;
			}
		}
	}

	pushThing(L, NULL, 0);
	return 1;
}

int32_t LuaScriptInterface::luaGetTileThingByPos(lua_State* L)
{
	//getTileThingByPos(pos)

	PositionEx pos;
	popPosition(L, pos);

	ScriptEnviroment* env = getScriptEnv();

	Tile* tile = g_game.getTile(pos.x, pos.y, pos.z);
	if(!tile)
	{
		if(pos.stackpos == -1)
		{
			lua_pushnumber(L, -1);
			return 1;
		}
		else
		{
			pushThing(L, NULL, 0);
			return 1;
		}
	}

	if(pos.stackpos == -1)
	{
		lua_pushnumber(L, tile->getThingCount());
		return 1;
	}

	Thing* thing = tile->__getThing(pos.stackpos);
	if(!thing)
	{
		pushThing(L, NULL, 0);
		return 1;
	}

	pushThing(L, thing, env->addThing(thing));
	return 1;
}

int32_t LuaScriptInterface::luaGetTopCreature(lua_State* L)
{
	//getTopCreature(pos)
	PositionEx pos;
	popPosition(L, pos);

	ScriptEnviroment* env = getScriptEnv();

	Tile* tile = g_game.getTile(pos.x, pos.y, pos.z);
	if(!tile)
	{
		pushThing(L, NULL, 0);
		return 1;
	}

	Thing* thing = tile->getTopCreature();
	if(!thing || !thing->getCreature())
	{
		pushThing(L, NULL, 0);
		return 1;
	}

	pushThing(L, thing, env->addThing(thing));
	return 1;
}

int32_t LuaScriptInterface::luaDoCreateItem(lua_State* L)
{
	//doCreateItem(itemid[, type/count], pos)
	//Returns uid of the created item, only works on tiles.

	uint32_t parameters = lua_gettop(L);

	PositionEx pos;
	popPosition(L, pos);

	uint32_t count = 0;
	if(parameters > 2)
		count = popNumber(L);

	uint32_t itemId = (uint32_t)popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Tile* tile = g_game.getTile(pos.x, pos.y, pos.z);
	if(!tile)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_TILE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	const ItemType& it = Item::items[itemId];
	if(it.stackable && count > 100)
	{
		int32_t subCount = count;
		while(subCount > 0)
		{
			int32_t stackCount = std::min((int32_t)100, (int32_t)subCount);
			Item* newItem = Item::CreateItem(itemId, stackCount);
			if(!newItem)
			{
				reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
				lua_pushboolean(L, LUA_ERROR);
				return 1;
			}

			ReturnValue ret = g_game.internalAddItem(NULL, tile, newItem, INDEX_WHEREEVER, FLAG_NOLIMIT);
			if(ret != RET_NOERROR)
			{
				delete newItem;
				lua_pushboolean(L, LUA_ERROR);
				return 1;
			}

			subCount = subCount - stackCount;
			if(subCount == 0)
			{
				if(newItem->getParent())
					lua_pushnumber(L, env->addThing((Thing*)newItem));
				else //stackable item stacked with existing object, newItem will be released
					lua_pushnil(L);

				return 1;
			}
		}
	}
	else
	{
		Item* newItem = Item::CreateItem(itemId, count);

		ReturnValue ret = g_game.internalAddItem(NULL, tile, newItem, INDEX_WHEREEVER, FLAG_NOLIMIT);
		if(ret != RET_NOERROR)
		{
			delete newItem;
			reportErrorFunc("Cannot add Item");
			lua_pushboolean(L, LUA_ERROR);
			return 1;
		}

		if(newItem->getParent())
			lua_pushnumber(L, env->addThing(newItem));
		else //stackable item stacked with existing object, newItem will be released
			lua_pushnil(L);

		return 1;
	}
	return 0;
}

int32_t LuaScriptInterface::luaDoCreateItemEx(lua_State* L)
{
	//doCreateItemEx(itemid[, count/subType])
	uint32_t count = 0;
	if(lua_gettop(L) >= 2)
		count = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	const ItemType& it = Item::items[(uint32_t)popNumber(L)];
	if(it.stackable && count > 100)
	{
		reportErrorFunc("Stack count cannot be higher than 100.");
		count = 100;
	}

	Item* newItem = Item::CreateItem(it.id, count);
	if(!newItem)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	newItem->setParent(VirtualCylinder::virtualCylinder);
	env->addTempItem(newItem);

	lua_pushnumber(L, env->addThing(newItem));
	return 1;
}

int32_t LuaScriptInterface::luaDoCreateTeleport(lua_State* L)
{
	//doCreateTeleport(itemid, topos, createpos)
	PositionEx createPos;
	popPosition(L, createPos);
	PositionEx toPos;
	popPosition(L, toPos);
	uint32_t itemId = (uint32_t)popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Tile* tile = g_game.getMap()->getTile(createPos);
	if(!tile)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_TILE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	Item* newItem = Item::CreateItem(itemId);
	Teleport* newTeleport = newItem->getTeleport();

	if(!newTeleport)
	{
		delete newItem;
		reportErrorFunc("Invalid teleport ItemID.");
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	newTeleport->setDestPos(toPos);
	ReturnValue ret = g_game.internalAddItem(NULL, tile, newTeleport, INDEX_WHEREEVER, FLAG_NOLIMIT);
	if(ret != RET_NOERROR)
	{
		delete newItem;
		reportErrorFunc("Cannot add Item");
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	if(newItem->getParent())
		lua_pushnumber(L, env->addThing(newItem));
	else
	{
		//stackable item stacked with existing object, newItem will be released
		lua_pushnil(L);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerStorageValue(lua_State* L)
{
	//getPlayerStorageValue(cid, key)
	uint32_t key = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		std::string strValue;
		if(player->getStorageValue(key, strValue))
		{
			int32_t intValue = atoi(strValue.c_str());
			if(intValue || strValue == "0")
				lua_pushnumber(L, intValue);
			else
				lua_pushstring(L, strValue.c_str());
		}
		else
			lua_pushnumber(L, -1);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}

	return 1;
}

int32_t LuaScriptInterface::luaSetPlayerStorageValue(lua_State* L)
{
	//setPlayerStorageValue(cid, key, value)
	std::string value;
	bool nil = false;
	if(lua_isnil(L, -1))
	{
		nil = true;
		lua_pop(L, 1);
	}
	else
		value = popString(L);

	uint32_t key = popNumber(L), cid = popNumber(L);
	if(IS_IN_KEYRANGE(key, RESERVED_RANGE))
	{
		char buffer[60];
		sprintf(buffer, "Accessing reserved range: %d", key);

		reportErrorFunc(buffer);
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	ScriptEnviroment* env = getScriptEnv();
	if(Player* player = env->getPlayerByUID(cid))
	{
		if(!nil)
			nil = player->addStorageValue(key, value);
		else
			nil = player->eraseStorageValue(key);

		lua_pushboolean(L, nil ? LUA_NO_ERROR : LUA_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoSetItemActionId(lua_State* L)
{
	//doSetItemActionId(uid, actionid)
	uint32_t actionid = popNumber(L);
	uint32_t uid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Item* item = env->getItemByUID(uid);
	if(item)
	{
		item->setActionId(actionid);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoSetItemText(lua_State* L)
{
	//doSetItemText(uid, text[, writer[, date]])
	std::string writer, text;
	uint32_t params = lua_gettop(L), date = 0;
	if(params >= 4)
		date = popNumber(L);

	if(params >= 3)
		writer = popString(L);

	text = popString(L);
	ScriptEnviroment* env = getScriptEnv();
	if(Item* item = env->getItemByUID(popNumber(L)))
	{
		item->setText(text);
		if(writer != "")
			item->setWriter(writer);

		if(date)
			item->setDate(date);

		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoSetItemSpecialDescription(lua_State* L)
{
	//doSetItemSpecialDescription(uid, desc)
	std::string str = popString(L);

	ScriptEnviroment* env = getScriptEnv();
	if(Item* item = env->getItemByUID(popNumber(L)))
	{
		if(str == "")
			item->resetSpecialDescription();
		else
			item->setSpecialDescription(str);

		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetTileInfo(lua_State* L)
{
	//getTileInfo(pos)
	PositionEx pos;

	popPosition(L, pos);
	if(Tile* tile = g_game.getMap()->getTile(pos))
	{
		lua_newtable(L);
		setFieldBool(L, "protection", tile->hasFlag(TILESTATE_PROTECTIONZONE));
		setFieldBool(L, "nopvp", tile->hasFlag(TILESTATE_NOPVPZONE));
		setFieldBool(L, "nologout", tile->hasFlag(TILESTATE_NOLOGOUT));
		setFieldBool(L, "pvp", tile->hasFlag(TILESTATE_PVPZONE));
		setFieldBool(L, "refresh", tile->hasFlag(TILESTATE_REFRESH));
		setFieldBool(L, "trashed", tile->hasFlag(TILESTATE_TRASHED));
		setFieldBool(L, "house", tile->hasFlag(TILESTATE_HOUSE));
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_TILE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetHouseFromPos(lua_State* L)
{
	//getHouseFromPos(pos)
	PositionEx pos;
	popPosition(L, pos);
	if(Tile* tile = g_game.getMap()->getTile(pos))
	{
		uint32_t houseId = LUA_ERROR;
		if(HouseTile* houseTile = dynamic_cast<HouseTile*>(tile))
		{
			if(House* house = houseTile->getHouse())
				houseId = house->getHouseId();
		}

		lua_pushnumber(L, houseId);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_TILE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoCreateMonster(lua_State* L)
{
	//doCreateMonster(name, pos[, displayError])
	bool displayError = true;
	if(lua_gettop(L) >= 3)
		displayError = popNumber(L) == LUA_TRUE;

	PositionEx pos;
	popPosition(L, pos);

	std::string name = popString(L);
	Monster* monster = Monster::createMonster(name.c_str());
	if(!monster)
	{
		if(displayError)
		{
			std::string tmp = (std::string)"Monster name(" + name + (std::string)") not found";
			reportErrorFunc(tmp);
		}

		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	if(!g_game.placeCreature(monster, pos))
	{
		delete monster;
		if(displayError)
		{
			std::string tmp = (std::string)"Cannot create monster: " + name;
			reportErrorFunc(tmp);
		}

		lua_pushboolean(L, LUA_NO_ERROR); //for scripting compatibility
		return 1;
	}

	ScriptEnviroment* env = getScriptEnv();
	lua_pushnumber(L, env->addThing((Thing*)monster));
	return 1;
}

int32_t LuaScriptInterface::luaDoCreateNpc(lua_State* L)
{
	//doCreateNpc(name, pos[, displayError])
	bool displayError = true;
	if(lua_gettop(L) >= 3)
		displayError = popNumber(L) == LUA_TRUE;

	PositionEx pos;
	popPosition(L, pos);

	std::string name = popString(L);
	Npc* npc = Npc::createNpc(name.c_str());
	if(!npc)
	{
		if(displayError)
		{
			std::string tmp = (std::string)"Npc name(" + name + (std::string)") not found";
			reportErrorFunc(tmp);
		}

		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	if(!g_game.placeCreature(npc, pos))
	{
		delete npc;
		if(displayError)
		{
			std::string tmp = (std::string)"Cannot create npc: " + name;
			reportErrorFunc(tmp);
		}

		lua_pushboolean(L, LUA_NO_ERROR); //for scripting compatibility
		return 1;
	}

	ScriptEnviroment* env = getScriptEnv();
	lua_pushnumber(L, env->addThing((Thing*)npc));
	return 1;
}

int32_t LuaScriptInterface::luaDoRemoveCreature(lua_State* L)
{
	//doRemoveCreature(cid)
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	if(Creature* creature = env->getCreatureByUID(cid))
	{
		Player* player = creature->getPlayer();
		if(player)
			player->kickPlayer(true); //Players will get kicked without restrictions
		else
			g_game.removeCreature(creature); //Monsters/NPCs will get removed

		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerAddMoney(lua_State *L)
{
	//doPlayerAddMoney(cid, money)
	uint64_t money = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		g_game.addMoney(player, money);
		lua_pushboolean(L, LUA_TRUE);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerRemoveMoney(lua_State* L)
{
	//doPlayerRemoveMoney(cid,money)
	uint64_t money = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Player* player = env->getPlayerByUID(cid);
	if(player)
		lua_pushboolean(L, g_game.removeMoney(player, money) ? LUA_TRUE : LUA_FALSE);
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerWithdrawMoney(lua_State *L)
{
	//doPlayerWithdrawMoney(cid, money)
	uint64_t money = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Player* player = env->getPlayerByUID(cid);
	if(player)
		lua_pushboolean(L, player->withdrawMoney(money) ? LUA_TRUE : LUA_FALSE);
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerDepositMoney(lua_State *L)
{
	//doPlayerDepositMoney(cid, money)
	uint64_t money = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Player* player = env->getPlayerByUID(cid);
	if(player)
		lua_pushboolean(L, player->depositMoney(money) ? LUA_TRUE : LUA_FALSE);
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerTransferMoneyTo(lua_State *L)
{
	//doPlayerTransferMoneyTo(cid, target, money)
	uint64_t money = popNumber(L);
	std::string target = popString(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Player* player = env->getPlayerByUID(cid);
	if(player)
		lua_pushboolean(L, player->transferMoneyTo(target, money) ? LUA_TRUE : LUA_FALSE);
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetTown(lua_State* L)
{
	//doPlayerSetTown(cid, townid)
	uint32_t townid = (uint32_t)popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		Town* town = Towns::getInstance().getTown(townid);
		if(town)
		{
			player->masterPos = town->getTemplePosition();
			player->setTown(townid);
			lua_pushboolean(L, LUA_NO_ERROR);
		}
		else
		{
			reportErrorFunc("Not found townid");
			lua_pushboolean(L, LUA_ERROR);
		}
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetVocation(lua_State* L)
{
	//doPlayerSetVocation(cid, voc)
	uint32_t voc = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		player->setVocation(voc);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetSex(lua_State* L)
{
	//doPlayerSetSex(cid,voc)
	uint32_t newSex = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		player->setSex((PlayerSex_t)newSex);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDebugPrint(lua_State* L)
{
	//debugPrint(text)
	const char * text = popString(L);
	reportErrorFunc(std::string(text));
	return 0;
}

int32_t LuaScriptInterface::luaDoPlayerAddSoul(lua_State* L)
{
	//doPlayerAddSoul(cid,soul)
	int32_t addsoul = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		player->changeSoul(addsoul);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerItemCount(lua_State* L)
{
	//getPlayerItemCount(cid,itemid)
	uint32_t itemId = (uint32_t)popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	const Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		uint32_t n = player->__getItemTypeCount(itemId);
		lua_pushnumber(L, n);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerMoney(lua_State* L)
{
	//getPlayerMoney(cid)
	ScriptEnviroment* env = getScriptEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
		lua_pushnumber(L, g_game.getMoney(player));
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetHouseOwner(lua_State* L)
{
	//getHouseOwner(houseid)
	uint32_t houseid = popNumber(L);

	House* house = Houses::getInstance().getHouse(houseid);
	if(house)
	{
		uint32_t owner = house->getHouseOwner();
		lua_pushnumber(L, owner);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_HOUSE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetHouseName(lua_State* L)
{
	//getHouseName(houseid)
	uint32_t houseid = popNumber(L);

	House* house = Houses::getInstance().getHouse(houseid);
	if(house)
		lua_pushstring(L, house->getName().c_str());
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_HOUSE_NOT_FOUND));
		lua_pushnil(L);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetHouseEntry(lua_State* L)
{
	//getHouseEntry(houseid)
	uint32_t houseid = popNumber(L);

	House* house = Houses::getInstance().getHouse(houseid);
	if(house)
		pushPosition(L, house->getEntryPosition(), 0);
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_HOUSE_NOT_FOUND));
		lua_pushnil(L);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetHouseRent(lua_State* L)
{
	//getHouseRent(houseid)
	uint32_t houseid = popNumber(L);

	House* house = Houses::getInstance().getHouse(houseid);
	if(house)
		lua_pushnumber(L, house->getRent());
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_HOUSE_NOT_FOUND));
		lua_pushnil(L);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetHousePrice(lua_State* L)
{
	//getHousePrice(houseid)
	uint32_t houseid = popNumber(L);

	House* house = Houses::getInstance().getHouse(houseid);
	if(house)
		lua_pushnumber(L, house->getPrice());
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_HOUSE_NOT_FOUND));
		lua_pushnil(L);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetHouseTown(lua_State* L)
{
	//getHouseTown(houseid)
	uint32_t houseid = popNumber(L);

	House* house = Houses::getInstance().getHouse(houseid);
	if(house)
		lua_pushnumber(L, house->getTownId());
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_HOUSE_NOT_FOUND));
		lua_pushnil(L);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetHouseAccessList(lua_State* L)
{
	//getHouseAccessList(houseid, listid)
	uint32_t listid = popNumber(L);
	uint32_t houseid = popNumber(L);

	House* house = Houses::getInstance().getHouse(houseid);
	if(house)
	{
		std::string list;
		if(house->getAccessList(listid, list))
			lua_pushstring(L, list.c_str());
		else
		{
			reportErrorFunc("No valid listid.");
			lua_pushnil(L);
		}
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_HOUSE_NOT_FOUND));
		lua_pushnil(L);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetHouseByPlayerGUID(lua_State* L)
{
	//getHouseByPlayerGUID(playerGUID)
	uint32_t guid = popNumber(L);

	House* house = Houses::getInstance().getHouseByPlayerId(guid);
	if(house)
		lua_pushnumber(L, house->getHouseId());
	else
		lua_pushnil(L);
	return 1;
}

int32_t LuaScriptInterface::luaGetHouseTilesSize(lua_State* L)
{
	//getHouseTilesSize(houseid)
	uint32_t houseid = popNumber(L);

	House* house = Houses::getInstance().getHouse(houseid);
	if(house)
		lua_pushnumber(L, house->getHouseTileSize());
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_HOUSE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaSetHouseAccessList(lua_State* L)
{
	//setHouseAccessList(houseid, listid, listtext)
	std::string list = popString(L);
	uint32_t listid = popNumber(L);
	uint32_t houseid = popNumber(L);

	House* house = Houses::getInstance().getHouse(houseid);
	if(house)
	{
		house->setAccessList(listid, list);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_HOUSE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetDepotId(lua_State *L)
{
	//getDepotId(uid)
	uint32_t uid = popNumber(L);

    ScriptEnviroment* env = getScriptEnv();
	Container* container = env->getContainerByUID(uid);
	if(container)
	{
		Depot* depot = container->getDepot();
		if(depot)
			lua_pushnumber(L, depot->getDepotId());
		else
		{
			reportErrorFunc("Depot not found.");
			lua_pushboolean(L, LUA_ERROR);
		}
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CONTAINER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}

	return 1;
}

int32_t LuaScriptInterface::luaSetHouseOwner(lua_State* L)
{
	//setHouseOwner(houseid, owner[, clean])
	bool clean = true;
	int32_t parameters = lua_gettop(L);
	if(parameters >= 3)
		clean = popNumber(L);

	uint32_t owner = popNumber(L);
	uint32_t houseid = popNumber(L);

	House* house = Houses::getInstance().getHouse(houseid);
	if(house)
	{
		house->setHouseOwner(owner, clean);
		lua_pushboolean(L, LUA_TRUE);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_HOUSE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetWorldType(lua_State* L)
{
	//getWorldType()
	switch(g_game.getWorldType())
	{
		case WORLD_TYPE_NO_PVP:
			lua_pushnumber(L, 1);
			break;
		case WORLD_TYPE_PVP:
			lua_pushnumber(L, 2);
			break;
		case WORLD_TYPE_PVP_ENFORCED:
			lua_pushnumber(L, 3);
			break;
		default:
			lua_pushboolean(L, LUA_ERROR);
			break;
	}
	return 1;
}

int32_t LuaScriptInterface::luaSetWorldType(lua_State* L)
{
	//setWorldType(type)
	WorldType_t type = (WorldType_t)popNumber(L);

	if(type >= WORLD_TYPE_FIRST && type <= WORLD_TYPE_LAST)
	{
		g_game.setWorldType(type);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc("Bad worldType type.");
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetWorldTime(lua_State* L)
{
	//getWorldTime()
	uint32_t time = g_game.getLightHour();
	lua_pushnumber(L, time);
	return 1;
}

int32_t LuaScriptInterface::luaGetWorldLight(lua_State* L)
{
	//getWorldLight()
	LightInfo lightInfo;
	g_game.getWorldLightInfo(lightInfo);
	lua_pushnumber(L, lightInfo.level);
	lua_pushnumber(L, lightInfo.color);
	return 1;
}

int32_t LuaScriptInterface::luaGetWorldCreatures(lua_State* L)
{
	//getWorldCreatures(type)
	//0 players, 1 monsters, 2 npcs, 3 all
	uint32_t type = popNumber(L);
	uint32_t value;
	switch(type)
	{
		case 0:
			value = g_game.getPlayersOnline();
			break;
		case 1:
			value = g_game.getMonstersOnline();
			break;
		case 2:
			value = g_game.getNpcsOnline();
			break;
		case 3:
			value = g_game.getCreaturesOnline();
			break;
		default:
			reportErrorFunc("Wrong creature type.");
			lua_pushboolean(L, LUA_ERROR);
			return 1;
			break;
	}
	lua_pushnumber(L, value);
	return 1;
}

int32_t LuaScriptInterface::luaGetWorldUpTime(lua_State* L)
{
	//getWorldUpTime()
	uint32_t uptime = 0;
	Status* status = Status::getInstance();
	if(status)
		uptime = status->getUptime();
	lua_pushnumber(L, uptime);
	return 1;
}

int32_t LuaScriptInterface::luaDoBroadcastMessage(lua_State* L)
{
	//doBroadcastMessage(message, type)
	uint32_t type = MSG_STATUS_WARNING;
	int32_t parameters = lua_gettop(L);
	if(parameters >= 2)
		type = popNumber(L);

	std::string message = popString(L);
	if(g_game.broadcastMessage(message, (MessageClasses)type))
		lua_pushboolean(L, LUA_NO_ERROR);
	else
	{
		reportErrorFunc("Bad messageClass type.");
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerBroadcastMessage(lua_State* L)
{
	//doPlayerBroadcastMessage(cid, message[, type = SPEAK_BROADCAST])
	uint32_t type = SPEAK_BROADCAST;
	int32_t parameters = lua_gettop(L);
	if(parameters >= 3)
		type = popNumber(L);

	std::string message = popString(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		if(g_game.playerBroadcastMessage(player, message, (SpeakClasses)type))
			lua_pushboolean(L, LUA_NO_ERROR);
		else
		{
			reportErrorFunc("Bad speakClass type.");
			lua_pushboolean(L, LUA_ERROR);
		}
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerLight(lua_State* L)
{
	//getPlayerLight(cid)
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	const Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		LightInfo lightInfo;
		player->getCreatureLight(lightInfo);
		lua_pushnumber(L, lightInfo.level);
		lua_pushnumber(L, lightInfo.color);//color
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerAddExperience(lua_State* L)
{
	//doPlayerAddExperience(cid, amount)
	int64_t amount = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		if(amount > 0)
			player->addExperience(amount);
		else if(amount < 0)
			player->removeExperience(std::abs(amount));
		else
		{
			lua_pushboolean(L, LUA_FALSE);
			return 1;
		}

		lua_pushboolean(L, LUA_TRUE);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerSlotItem(lua_State* L)
{
	//getPlayerSlotItem(cid, slot)
	uint32_t slot = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	if(const Player* player = env->getPlayerByUID(cid))
	{
		if(Thing* thing = player->__getThing(slot))
			pushThing(L, thing, env->addThing(thing));
		else
			pushThing(L, NULL, 0);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		pushThing(L, NULL, 0);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerWeapon(lua_State* L)
{
	//getPlayerWeapon(cid[, ignoreAmmo = FALSE])
	bool ignoreAmmo = false;
	if(lua_gettop(L) >= 2)
		ignoreAmmo = popNumber(L) == LUA_TRUE;

	ScriptEnviroment* env = getScriptEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		if(Item* weapon = player->getWeapon(ignoreAmmo))
			pushThing(L, weapon, env->addThing(weapon));
		else
			pushThing(L, NULL, 0);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushnil(L);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerItemById(lua_State* L)
{
	//getPlayerItemById(cid, deepSearch, itemId[, subType])
	ScriptEnviroment* env = getScriptEnv();

	uint32_t parameters = lua_gettop(L);

	int32_t subType = -1;
	if(parameters > 3)
		subType = (int32_t)popNumber(L);

	int32_t itemId = (int32_t)popNumber(L);
	bool deepSearch = popNumber(L) == 1;
	uint32_t cid = popNumber(L);

	Player* player = env->getPlayerByUID(cid);
	if(!player)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		pushThing(L, NULL, 0);
		return 1;
	}

	Item* item = g_game.findItemOfType(player, itemId, deepSearch, subType);
	if(!item)
	{
		pushThing(L, NULL, 0);
		return 1;
	}

	pushThing(L, item, env->addThing(item));
	return 1;
}

int32_t LuaScriptInterface::luaGetThing(lua_State* L)
{
	//getThing(uid)
	uint32_t uid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Thing* thing = env->getThingByUID(uid);
	if(thing)
		pushThing(L, thing, uid);
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_THING_NOT_FOUND));
		pushThing(L, NULL, 0);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoTileQueryAdd(lua_State* L)
{
	//doTileQueryAdd(uid, pos[, flags])
	uint32_t flags = 0;
	if(lua_gettop(L) >= 3)
		flags = popNumber(L);

	PositionEx pos;
	popPosition(L, pos);
	uint32_t uid = popNumber(L);
	ScriptEnviroment* env = getScriptEnv();

	Tile* tile = g_game.getTile(pos.x, pos.y, pos.z);
	if(!tile)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_TILE_NOT_FOUND));
		lua_pushnumber(L, (uint32_t)RET_NOTPOSSIBLE);
		return 1;
	}

	Thing* thing = env->getThingByUID(uid);
	if(!thing)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_THING_NOT_FOUND));
		lua_pushnumber(L, (uint32_t)RET_NOTPOSSIBLE);
		return 1;
	}

	ReturnValue ret = tile->__queryAdd(0, thing, 1, flags);
	lua_pushnumber(L, (uint32_t)ret);
	return 1;
}

int32_t LuaScriptInterface::luaGetThingPos(lua_State* L)
{
	//getThingPos(uid)
	uint32_t uid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Thing* thing = env->getThingByUID(uid);
	Position pos(0, 0, 0);
	uint32_t stackpos = 0;
	if(thing)
	{
		pos = thing->getPosition();
		if(Tile* tile = thing->getTile())
			stackpos = tile->__getIndexOfThing(thing);
	}
	else
		reportErrorFunc(getErrorDesc(LUA_ERROR_THING_NOT_FOUND));

	pushPosition(L, pos, stackpos);
	return 1;
}

int32_t LuaScriptInterface::luaCreateCombatObject(lua_State* L)
{
	//createCombatObject()
	ScriptEnviroment* env = getScriptEnv();

	if(env->getScriptId() != EVENT_ID_LOADING)
	{
		reportError(__FUNCTION__, "This function can only be used while loading the script.");
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	Combat* combat = new Combat;

	if(!combat)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_COMBAT_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	uint32_t newCombatId = env->addCombatObject(combat);
	lua_pushnumber(L, newCombatId);
	return 1;
}

bool LuaScriptInterface::getArea(lua_State* L, std::list<uint32_t>& list, uint32_t& rows)
{
	rows = 0;
	uint32_t i = 0, j = 0;
	lua_pushnil(L);  //first key //

	while(lua_next(L, -2) != 0)
	{
		lua_pushnil(L);
		while(lua_next(L, -2) != 0)
		{
			list.push_back((uint32_t)lua_tonumber(L, -1));

			lua_pop(L, 1);  //removes `value'; keeps `key' for next iteration //
			j++;
		}

		++rows;

		j = 0;
		lua_pop(L, 1);  //removes `value'; keeps `key' for next iteration //
		i++;
	}

	lua_pop(L, 1);
	return (rows != 0);
}

int32_t LuaScriptInterface::luaCreateCombatArea(lua_State* L)
{
	//createCombatArea( {area}[, {extArea}])
	ScriptEnviroment* env = getScriptEnv();

	if(env->getScriptId() != EVENT_ID_LOADING)
	{
		reportError(__FUNCTION__, "This function can only be used while loading the script.");
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	int32_t parameters = lua_gettop(L);

	AreaCombat* area = new AreaCombat;

	if(parameters > 1)
	{
		//has extra parameter with diagonal area information

		uint32_t rowsExtArea;
		std::list<uint32_t> listExtArea;
		getArea(L, listExtArea, rowsExtArea);

		/*setup all possible rotations*/
		area->setupExtArea(listExtArea, rowsExtArea);
	}

	if(lua_isnil(L, -1) == 1) //prevent crash
	{
		lua_pop(L, 2);
		lua_pushboolean(L, LUA_FALSE);
		return 1;
	}

	uint32_t rowsArea = 0;
	std::list<uint32_t> listArea;
	getArea(L, listArea, rowsArea);

	/*setup all possible rotations*/
	area->setupArea(listArea, rowsArea);

	uint32_t newAreaId = env->addCombatArea(area);

	lua_pushnumber(L, newAreaId);
	return 1;
}

int32_t LuaScriptInterface::luaCreateConditionObject(lua_State* L)
{
	//createConditionObject(type[, ticks[, buff[, subId]]])
	uint32_t params = lua_gettop(L), subId = 0;
	if(params >= 4)
		subId = popNumber(L);

	bool buff = false;
	if(params >= 3)
		buff = popNumber(L) == LUA_TRUE;

	int32_t ticks = 0;
	if(params >= 2)
		ticks = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	if(env->getScriptId() != EVENT_ID_LOADING)
	{
		reportError(__FUNCTION__, "This function can only be used while loading the script.");
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	ConditionType_t type = (ConditionType_t)popNumber(L);
	if(Condition* condition = Condition::createCondition(CONDITIONID_COMBAT, type, ticks, 0, buff, subId))
	{
		uint32_t conditionId = env->addConditionObject(condition);
		lua_pushnumber(L, conditionId);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CONDITION_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}

	return 1;
}

int32_t LuaScriptInterface::luaSetCombatArea(lua_State* L)
{
	//setCombatArea(combat, area)
	uint32_t areaId = popNumber(L);
	uint32_t combatId = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	if(env->getScriptId() != EVENT_ID_LOADING)
	{
		reportError(__FUNCTION__, "This function can only be used while loading the script.");
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	Combat* combat = env->getCombatObject(combatId);

	if(!combat)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_COMBAT_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	const AreaCombat* area = env->getCombatArea(areaId);
	if(!area)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_AREA_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	combat->setArea(new AreaCombat(*area));

	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaSetCombatCondition(lua_State* L)
{
	//setCombatCondition(combat, condition)
	uint32_t conditionId = popNumber(L);
	uint32_t combatId = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	if(env->getScriptId() != EVENT_ID_LOADING)
	{
		reportError(__FUNCTION__, "This function can only be used while loading the script.");
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	Combat* combat = env->getCombatObject(combatId);
	if(!combat)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_COMBAT_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	const Condition* condition = env->getConditionObject(conditionId);
	if(!condition)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CONDITION_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	combat->setCondition(condition->clone());
	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaSetCombatParam(lua_State* L)
{
	//setCombatParam(combat, key, value)
	uint32_t value = popNumber(L);
	CombatParam_t key = (CombatParam_t)popNumber(L);
	uint32_t combatId = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	if(env->getScriptId() != EVENT_ID_LOADING)
	{
		reportError(__FUNCTION__, "This function can only be used while loading the script.");
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	Combat* combat = env->getCombatObject(combatId);
	if(!combat)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_COMBAT_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	else
	{
		combat->setParam(key, value);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaSetConditionParam(lua_State* L)
{
	//setConditionParam(condition, key, value)
	int32_t value = (int32_t)popNumber(L);
	ConditionParam_t key = (ConditionParam_t)popNumber(L);
	uint32_t conditionId = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	if(env->getScriptId() != EVENT_ID_LOADING)
	{
		reportError(__FUNCTION__, "This function can only be used while loading the script.");
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	if(Condition* condition = env->getConditionObject(conditionId))
	{
		condition->setParam(key, value);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CONDITION_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaAddDamageCondition(lua_State* L)
{
	//addDamageCondition(condition, rounds, time, value)
	int32_t value = popNumber(L), time = popNumber(L), rounds = popNumber(L);
	uint32_t conditionId = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	if(env->getScriptId() != EVENT_ID_LOADING)
	{
		reportError(__FUNCTION__, "This function can only be used while loading the script.");
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	if(ConditionDamage* condition = dynamic_cast<ConditionDamage*>(env->getConditionObject(conditionId)))
	{
		condition->addDamage(rounds, time, value);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CONDITION_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaAddOutfitCondition(lua_State* L)
{
	//addOutfitCondition(condition, lookTypeEx, lookType, lookHead, lookBody, lookLegs, lookFeet)
	Outfit_t outfit;
	outfit.lookFeet = popNumber(L);
	outfit.lookLegs = popNumber(L);
	outfit.lookBody = popNumber(L);
	outfit.lookHead = popNumber(L);
	outfit.lookType = popNumber(L);
	outfit.lookTypeEx = popNumber(L);
	uint32_t conditionId = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	if(env->getScriptId() != EVENT_ID_LOADING)
	{
		reportError(__FUNCTION__, "This function can only be used while loading the script.");
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	if(ConditionOutfit* condition = dynamic_cast<ConditionOutfit*>(env->getConditionObject(conditionId)))
	{
		condition->addOutfit(outfit);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CONDITION_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaSetCombatCallBack(lua_State* L)
{
	//setCombatCallBack(combat, key, function_name)
	const char* function = popString(L);
	std::string function_str(function);
	CallBackParam_t key = (CallBackParam_t)popNumber(L);
	uint32_t combatId = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	if(env->getScriptId() != EVENT_ID_LOADING)
	{
		reportError(__FUNCTION__, "This function can only be used while loading the script.");
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	Combat* combat = env->getCombatObject(combatId);

	if(!combat)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_COMBAT_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	LuaScriptInterface* scriptInterface = env->getScriptInterface();

	combat->setCallback(key);
	CallBack* callback = combat->getCallback(key);
	if(!callback)
	{
		char buffer[50];
		sprintf(buffer, "%d is not a valid callback key.", key);
		reportError(__FUNCTION__, buffer);
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	if(!callback->loadCallBack(scriptInterface, function_str))
	{
		reportError(__FUNCTION__, "Cannot load callback");
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaSetCombatFormula(lua_State* L)
{
	//setCombatFormula(combat, type, mina, minb, maxa, maxb)
	ScriptEnviroment* env = getScriptEnv();

	if(env->getScriptId() != EVENT_ID_LOADING)
	{
		reportError(__FUNCTION__, "This function can only be used while loading the script.");
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	double maxb = popFloatNumber(L);
	double maxa = popFloatNumber(L);
	double minb = popFloatNumber(L);
	double mina = popFloatNumber(L);

	formulaType_t type = (formulaType_t)popNumber(L);
	uint32_t combatId = popNumber(L);

	Combat* combat = env->getCombatObject(combatId);

	if(combat)
	{
		combat->setPlayerCombatValues(type, mina, minb, maxa, maxb);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_COMBAT_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaSetConditionFormula(lua_State* L)
{
	//setConditionFormula(condition, mina, minb, maxa, maxb)
	ScriptEnviroment* env = getScriptEnv();

	if(env->getScriptId() != EVENT_ID_LOADING)
	{
		reportError(__FUNCTION__, "This function can only be used while loading the script.");
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	double maxb = popFloatNumber(L);
	double maxa = popFloatNumber(L);
	double minb = popFloatNumber(L);
	double mina = popFloatNumber(L);

	uint32_t conditionId = popNumber(L);

	if(ConditionSpeed* condition = dynamic_cast<ConditionSpeed*>(env->getConditionObject(conditionId)))
	{
		condition->setFormulaVars(mina, minb, maxa, maxb);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CONDITION_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoCombat(lua_State* L)
{
	//doCombat(cid, combat, param)
	ScriptEnviroment* env = getScriptEnv();

	LuaVariant var = popVariant(L);
	uint32_t combatId = (uint32_t)popNumber(L);
	uint32_t cid = (uint32_t)popNumber(L);

	Creature* creature = NULL;

	if(cid != 0)
	{
		creature = env->getCreatureByUID(cid);

		if(!creature)
		{
			reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushboolean(L, LUA_ERROR);
			return 1;
		}
	}

	const Combat* combat = env->getCombatObject(combatId);

	if(!combat)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_COMBAT_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	if(var.type == VARIANT_NONE)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_VARIANT_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	switch(var.type)
	{
		case VARIANT_NUMBER:
		{
			Creature* target = g_game.getCreatureByID(var.number);
			if(!target || (target->isInGhostMode() && !creature->canSeeGhost(target)))
			{
				lua_pushboolean(L, LUA_ERROR);
				return 1;
			}

			if(combat->hasArea())
			{
				combat->doCombat(creature, target->getPosition());
				//std::cout << "Combat->hasArea()" << std::endl;
			}
			else
			{
				combat->doCombat(creature, target);
			}
			break;
		}

		case VARIANT_POSITION:
		{
			combat->doCombat(creature, var.pos);
			break;
		}

		case VARIANT_TARGETPOSITION:
		{
			if(combat->hasArea())
				combat->doCombat(creature, var.pos);
			else
			{
				combat->postCombatEffects(creature, var.pos);
				g_game.addMagicEffect(var.pos, NM_ME_POFF);
			}
			break;
		}

		case VARIANT_STRING:
		{
			Player* target = g_game.getPlayerByName(var.text);
			if(!target || (target->isInGhostMode() && !creature->canSeeGhost(target)))
			{
				lua_pushboolean(L, LUA_ERROR);
				return 1;
			}

			combat->doCombat(creature, target);
			break;
		}

		default:
		{
			reportErrorFunc(getErrorDesc(LUA_ERROR_VARIANT_UNKNOWN));
			lua_pushboolean(L, LUA_ERROR);
			return 1;
			break;
		}
	}

	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaDoAreaCombatHealth(lua_State* L)
{
	//doAreaCombatHealth(cid, type, pos, area, min, max, effect)
	uint8_t effect = (uint8_t)popNumber(L);
	int32_t maxChange = (int32_t)popNumber(L);
	int32_t minChange = (int32_t)popNumber(L);
	uint32_t areaId = popNumber(L);

	PositionEx pos;
	popPosition(L, pos);

	CombatType_t combatType = (CombatType_t)popNumber(L);
	uint32_t cid = (uint32_t)popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Creature* creature = NULL;

	if(cid != 0)
	{
		creature = env->getCreatureByUID(cid);

		if(!creature)
		{
			reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushboolean(L, LUA_ERROR);
			return 1;
		}
	}

	const AreaCombat* area = env->getCombatArea(areaId);
	if(area || areaId == 0)
	{
		CombatParams params;
		params.combatType = combatType;
		params.impactEffect = effect;
		Combat::doCombatHealth(creature, pos, area, minChange, maxChange, params);

		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_AREA_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoTargetCombatHealth(lua_State* L)
{
	//doTargetCombatHealth(cid, target, type, min, max, effect)
	uint8_t effect = (uint8_t)popNumber(L);
	int32_t maxChange = (int32_t)popNumber(L);
	int32_t minChange = (int32_t)popNumber(L);
	CombatType_t combatType = (CombatType_t)popNumber(L);
	uint32_t targetCid = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Creature* creature = NULL;

	if(cid != 0)
	{
		creature = env->getCreatureByUID(cid);

		if(!creature)
		{
			reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushboolean(L, LUA_ERROR);
			return 1;
		}
	}

	Creature* target = env->getCreatureByUID(targetCid);
	if(target)
	{
		CombatParams params;
		params.combatType = combatType;
		params.impactEffect = effect;
		Combat::doCombatHealth(creature, target, minChange, maxChange, params);

		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoAreaCombatMana(lua_State* L)
{
	//doAreaCombatMana(cid, pos, area, min, max, effect)
	uint8_t effect = (uint8_t)popNumber(L);
	int32_t maxChange = (int32_t)popNumber(L);
	int32_t minChange = (int32_t)popNumber(L);
	uint32_t areaId = popNumber(L);

	PositionEx pos;
	popPosition(L, pos);

	uint32_t cid = (uint32_t)popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Creature* creature = NULL;

	if(cid != 0)
	{
		creature = env->getCreatureByUID(cid);

		if(!creature)
		{
			reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushboolean(L, LUA_ERROR);
			return 1;
		}
	}

	const AreaCombat* area = env->getCombatArea(areaId);
	if(area || areaId == 0)
	{
		CombatParams params;
		params.impactEffect = effect;
		Combat::doCombatMana(creature, pos, area, minChange, maxChange, params);

		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_AREA_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoTargetCombatMana(lua_State* L)
{
	//doTargetCombatMana(cid, target, min, max, effect)
	Creature* creature = NULL;

	uint8_t effect = (uint8_t)popNumber(L);
	int32_t maxChange = (int32_t)popNumber(L);
	int32_t minChange = (int32_t)popNumber(L);
	uint32_t targetCid = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	if(cid != 0)
	{
		creature = env->getCreatureByUID(cid);

		if(!creature)
		{
			reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushboolean(L, LUA_ERROR);
			return 1;
		}
	}

	Creature* target = env->getCreatureByUID(targetCid);
	if(target)
	{
		CombatParams params;
		params.impactEffect = effect;
		Combat::doCombatMana(creature, target, minChange, maxChange, params);

		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoAreaCombatCondition(lua_State* L)
{
	//doAreaCombatCondition(cid, pos, area, condition, effect)
	Creature* creature = NULL;
	PositionEx pos;

	uint8_t effect = (uint8_t)popNumber(L);
	uint32_t conditionId = popNumber(L);
	uint32_t areaId = popNumber(L);
	popPosition(L, pos);
	uint32_t cid = (uint32_t)popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	if(cid != 0)
	{
		creature = env->getCreatureByUID(cid);
		if(!creature)
		{
			reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushboolean(L, LUA_ERROR);
			return 1;
		}
	}

	if(const Condition* condition = env->getConditionObject(conditionId))
	{
		const AreaCombat* area = env->getCombatArea(areaId);
		if(area || areaId == 0)
		{
			CombatParams params;
			params.impactEffect = effect;
			params.conditionList.push_back(condition);
			Combat::doCombatCondition(creature, pos, area, params);

			lua_pushboolean(L, LUA_NO_ERROR);
		}
		else
		{
			reportErrorFunc(getErrorDesc(LUA_ERROR_AREA_NOT_FOUND));
			lua_pushboolean(L, LUA_ERROR);
		}
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CONDITION_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoTargetCombatCondition(lua_State* L)
{
	//doTargetCombatCondition(cid, target, condition, effect)
	uint8_t effect = (uint8_t)popNumber(L);
	uint32_t conditionId = popNumber(L);
	uint32_t targetCid = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Creature* creature = NULL;
	if(cid != 0)
	{
		creature = env->getCreatureByUID(cid);
		if(!creature)
		{
			reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushboolean(L, LUA_ERROR);
			return 1;
		}
	}

	if(Creature* target = env->getCreatureByUID(targetCid))
	{
		if(const Condition* condition = env->getConditionObject(conditionId))
		{
			CombatParams params;
			params.impactEffect = effect;
			params.conditionList.push_back(condition);
			Combat::doCombatCondition(creature, target, params);

			lua_pushboolean(L, LUA_NO_ERROR);
		}
		else
		{
			reportErrorFunc(getErrorDesc(LUA_ERROR_CONDITION_NOT_FOUND));
			lua_pushboolean(L, LUA_ERROR);
		}
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoAreaCombatDispel(lua_State* L)
{
	//doAreaCombatDispel(cid, pos, area, type, effect)
	uint8_t effect = (uint8_t)popNumber(L);
	ConditionType_t dispelType = (ConditionType_t)popNumber(L);
	uint32_t areaId = popNumber(L);
	PositionEx pos;
	popPosition(L, pos);
	uint32_t cid = (uint32_t)popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Creature* creature = NULL;

	if(cid != 0)
	{
		creature = env->getCreatureByUID(cid);

		if(!creature)
		{
			reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushboolean(L, LUA_ERROR);
			return 1;
		}
	}

	const AreaCombat* area = env->getCombatArea(areaId);
	if(area || areaId == 0)
	{
		CombatParams params;
		params.impactEffect = effect;
		params.dispelType = dispelType;
		Combat::doCombatDispel(creature, pos, area, params);

		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_AREA_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoTargetCombatDispel(lua_State* L)
{
	//doTargetCombatDispel(cid, target, type, effect)
	uint8_t effect = (uint8_t)popNumber(L);
	ConditionType_t dispelType = (ConditionType_t)popNumber(L);
	uint32_t targetCid = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Creature* creature = NULL;

	if(cid != 0)
	{
		creature = env->getCreatureByUID(cid);

		if(!creature)
		{
			reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushboolean(L, LUA_ERROR);
			return 1;
		}
	}

	Creature* target = env->getCreatureByUID(targetCid);
	if(target)
	{
		CombatParams params;
		params.impactEffect = effect;
		params.dispelType = dispelType;
		Combat::doCombatDispel(creature, target, params);

		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoChallengeCreature(lua_State* L)
{
	//doChallengeCreature(cid, target)
	ScriptEnviroment* env = getScriptEnv();
	uint32_t cid = popNumber(L);

	Creature* creature = env->getCreatureByUID(popNumber(L));
	if(!creature)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	Creature* target = env->getCreatureByUID(cid);
	if(!target)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	target->challengeCreature(creature);
	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaDoSummonMonster(lua_State* L)
{
	//doSummonMonster(cid, name)
	std::string name = popString(L);
	ScriptEnviroment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(popNumber(L));
	if(!creature)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	lua_pushnumber(L, g_game.placeSummon(creature, name));
	return 1;
}

int32_t LuaScriptInterface::luaDoConvinceCreature(lua_State* L)
{
	//doConvinceCreature(cid, target)
	uint32_t cid = popNumber(L);
	ScriptEnviroment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(popNumber(L));
	if(!creature)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	Creature* target = env->getCreatureByUID(cid);
	if(!target)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	target->convinceCreature(creature);
	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaGetMonsterTargetList(lua_State* L)
{
	//getMonsterTargetList(cid)
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(!creature)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	Monster* monster = creature->getMonster();
	if(!monster)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	const CreatureList& targetList = monster->getTargetList();
	CreatureList::const_iterator it = targetList.begin();

	lua_newtable(L);
	for(uint32_t i = 1; it != targetList.end(); ++it, ++i)
	{
		if(monster->isTarget(*it))
		{
			lua_pushnumber(L, i);
			lua_pushnumber(L, env->addThing(*it));
			lua_settable(L, -3);
		}
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetMonsterFriendList(lua_State* L)
{
	//getMonsterFriendList(cid)
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(!creature)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	Monster* monster = creature->getMonster();
	if(!monster)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	Creature* friendCreature;
	const CreatureList& friendList = monster->getFriendList();
	CreatureList::const_iterator it = friendList.begin();

	lua_newtable(L);
	for(uint32_t i = 1; it != friendList.end(); ++it, ++i)
	{
		friendCreature = (*it);
		if(!friendCreature->isRemoved() && friendCreature->getPosition().z == monster->getPosition().z)
		{
			lua_pushnumber(L, i);
			lua_pushnumber(L, env->addThing(*it));
			lua_settable(L, -3);
		}
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoMonsterSetTarget(lua_State* L)
{
	//doMonsterSetTarget(cid, target)
	uint32_t targetCid = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(!creature)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	Monster* monster = creature->getMonster();
	if(!monster)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	Creature* target = env->getCreatureByUID(targetCid);
	if(!target)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	if(!monster->isSummon())
		monster->selectTarget(target);

	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaDoMonsterChangeTarget(lua_State* L)
{
	//doMonsterChangeTarget(cid)
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(!creature)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	Monster* monster = creature->getMonster();
	if(!monster)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	if(!monster->isSummon())
		monster->searchTarget(TARGETSEARCH_RANDOM);

	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaGetMonsterInfo(lua_State* L)
{
	//getMonsterInfo(name)
	const MonsterType* mType = g_monsters.getMonsterType(popString(L));
	if(!mType)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	lua_newtable(L);
	setField(L, "name", mType->name.c_str());
	setField(L, "experience", mType->experience);
	setField(L, "health", mType->health);
	setField(L, "healthMax", mType->health_max);
	setField(L, "manaCost", mType->manaCost);
	setField(L, "defense", mType->defense);
	setField(L, "armor", mType->armor);
	setField(L, "baseSpeed", mType->base_speed);
	setField(L, "lookCorpse", mType->lookcorpse);
	setField(L, "race", mType->race);
	setField(L, "skull", mType->skull);
	setField(L, "partyShield", mType->partyShield);
	setFieldBool(L, "summonable", mType->isSummonable);
	setFieldBool(L, "illusionable", mType->isIllusionable);
	setFieldBool(L, "convinceable", mType->isConvinceable);
	setFieldBool(L, "attackable", mType->isAttackable);
	setFieldBool(L, "hostile", mType->isHostile);
	return 1;
}

int32_t LuaScriptInterface::luaGetMonsterHealingSpells(lua_State* L)
{
	//getMonsterHealingSpells(name)
	const MonsterType* mType = g_monsters.getMonsterType(popString(L));
	SpellList::const_iterator it = mType->spellDefenseList.begin();

	lua_newtable(L);
	for(uint32_t i = 1; it != mType->spellDefenseList.end(); ++it, ++i)
	{
		lua_pushnumber(L, i);
		lua_newtable(L);

		setField(L, "speed", (*it).speed);
		setField(L, "chance", (*it).chance);
		setField(L, "range", (*it).range);
		setField(L, "minCombatValue", (*it).minCombatValue);
		setField(L, "maxCombatValue", (*it).maxCombatValue);
		setFieldBool(L, "isMelee", (*it).isMelee);
		lua_settable(L, -3);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetMonsterAttackSpells(lua_State* L)
{
	//getMonsterAttackSpells(name)
	const MonsterType* mType = g_monsters.getMonsterType(popString(L));
	SpellList::const_iterator it = mType->spellAttackList.begin();

	lua_newtable(L);
	for(uint32_t i = 1; it != mType->spellAttackList.end(); ++it, ++i)
	{
		lua_pushnumber(L, i);
		lua_newtable(L);

		setField(L, "speed", (*it).speed);
		setField(L, "chance", (*it).chance);
		setField(L, "range", (*it).range);
		setField(L, "minCombatValue", (*it).minCombatValue);
		setField(L, "maxCombatValue", (*it).maxCombatValue);
		setFieldBool(L, "isMelee", (*it).isMelee);
		lua_settable(L, -3);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetMonsterLootList(lua_State* L)
{
	//getMonsterLootList(name)
	const MonsterType* mType = g_monsters.getMonsterType(popString(L));
	LootItems::const_iterator it = mType->lootItems.begin();

	lua_newtable(L);
	for(uint32_t i = 1; it != mType->lootItems.end(); ++it, ++i)
	{
		lua_pushnumber(L, i);
		lua_newtable(L);

		setField(L, "id", (*it).id);
		setField(L, "countMax", (*it).countmax);
		setField(L, "chance", (*it).chance);
		setField(L, "subType", (*it).subType);
		setField(L, "actionId", (*it).actionId);
		setField(L, "uniqueId", (*it).uniqueId);
		setField(L, "text", (*it).text);
		if((*it).childLoot.size() > 0)
		{
			LootItems::const_iterator cit = (*it).childLoot.begin();

			lua_newtable(L);
			for(uint32_t j = 1; cit != (*it).childLoot.end(); ++cit, ++j)
			{
				lua_pushnumber(L, j);
				lua_newtable(L);

				setField(L, "id", (*cit).id);
				setField(L, "countMax", (*cit).countmax);
				setField(L, "chance", (*cit).chance);
				setField(L, "subType", (*cit).subType);
				setField(L, "actionId", (*cit).actionId);
				setField(L, "uniqueId", (*cit).uniqueId);
				setField(L, "text", (*cit).text);
				lua_settable(L, -3);
			}
		}

		lua_settable(L, -3);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoAddCondition(lua_State* L)
{
	//doAddCondition(cid, condition)

	uint32_t conditionId = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(!creature)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	Condition* condition = env->getConditionObject(conditionId);
	if(!condition)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CONDITION_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	creature->addCondition(condition->clone());
	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaDoRemoveCondition(lua_State* L)
{
	//doRemoveCondition(cid, type[, subId])
	uint32_t subId = 0;
	if(lua_gettop(L) >= 3)
		subId = popNumber(L);

	ConditionType_t conditionType = (ConditionType_t)popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	Creature* creature = env->getCreatureByUID(cid);
	if(!creature)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	Condition* condition = creature->getCondition(conditionType, CONDITIONID_COMBAT, subId);
	if(!condition)
		condition = creature->getCondition(conditionType, CONDITIONID_DEFAULT, subId);

	if(condition)
		creature->removeCondition(condition);

	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaDoRemoveConditions(lua_State* L)
{
	//doRemoveConditions(cid[, onlyPersistent])
	bool onlyPersistent = true;
	if(lua_gettop(L))
		onlyPersistent = popNumber(L) == LUA_TRUE;

	ScriptEnviroment* env = getScriptEnv();
	Creature* creature = env->getCreatureByUID(popNumber(L));
	if(!creature)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	creature->removeConditions(CONDITIONEND_ABORT, onlyPersistent);
	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaNumberToVariant(lua_State* L)
{
	//numberToVariant(number)
	LuaVariant var;
	var.type = VARIANT_NUMBER;
	var.number = popNumber(L);

	LuaScriptInterface::pushVariant(L, var);
	return 1;
}

int32_t LuaScriptInterface::luaStringToVariant(lua_State* L)
{
	//stringToVariant(string)
	LuaVariant var;
	var.type = VARIANT_STRING;
	var.text = popString(L);

	LuaScriptInterface::pushVariant(L, var);
	return 1;
}

int32_t LuaScriptInterface::luaPositionToVariant(lua_State* L)
{
	//positionToVariant(pos)
	LuaVariant var;
	var.type = VARIANT_POSITION;
	popPosition(L, var.pos);

	LuaScriptInterface::pushVariant(L, var);
	return 1;
}

int32_t LuaScriptInterface::luaTargetPositionToVariant(lua_State* L)
{
	//targetPositionToVariant(pos)
	LuaVariant var;
	var.type = VARIANT_TARGETPOSITION;
	popPosition(L, var.pos);

	LuaScriptInterface::pushVariant(L, var);
	return 1;
}

int32_t LuaScriptInterface::luaVariantToNumber(lua_State* L)
{
	//variantToNumber(var)
	LuaVariant var = popVariant(L);

	uint32_t number = 0;
	if(var.type == VARIANT_NUMBER)
		number = var.number;

	lua_pushnumber(L, number);
	return 1;
}

int32_t LuaScriptInterface::luaVariantToString(lua_State* L)
{
	//variantToString(var)
	LuaVariant var = popVariant(L);

	std::string text = "";
	if(var.type == VARIANT_STRING)
		text = var.text;

	lua_pushstring(L, text.c_str());
	return 1;
}

int32_t LuaScriptInterface::luaVariantToPosition(lua_State* L)
{
	//luaVariantToPosition(var)
	LuaVariant var = popVariant(L);

	PositionEx pos(0, 0, 0, 0);
	if(var.type == VARIANT_POSITION || var.type == VARIANT_TARGETPOSITION)
		pos = var.pos;

	pushPosition(L, pos, pos.stackpos);
	return 1;
}

int32_t LuaScriptInterface::luaDoChangeSpeed(lua_State* L)
{
	//doChangeSpeed(cid, delta)
	int32_t delta = (int32_t)popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(creature)
	{
		g_game.changeSpeed(creature, delta);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaSetCreatureOutfit(lua_State* L)
{
	//doSetCreatureOutfit(cid, outfit, time)
	int32_t time = (int32_t)popNumber(L);
	Outfit_t outfit;
	outfit.lookType = getField(L, "lookType");
	outfit.lookHead = getField(L, "lookHead");
	outfit.lookBody = getField(L, "lookBody");
	outfit.lookLegs = getField(L, "lookLegs");
	outfit.lookFeet = getField(L, "lookFeet");
	outfit.lookAddons = getField(L, "lookAddons");
	lua_pop(L, 1);

	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);

	if(creature)
	{
		ReturnValue ret = Spell::CreateIllusion(creature, outfit, time);
		if(ret == RET_NOERROR)
			lua_pushboolean(L, LUA_NO_ERROR);
		else
			lua_pushboolean(L, LUA_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureOutfit(lua_State* L)
{
	//getCreatureOutfit(cid)
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(creature)
	{
		const Outfit_t outfit = creature->getCurrentOutfit();

		lua_newtable(L);
		setField(L, "lookType", outfit.lookType);
		setField(L, "lookHead", outfit.lookHead);
		setField(L, "lookBody", outfit.lookBody);
		setField(L, "lookLegs", outfit.lookLegs);
		setField(L, "lookFeet", outfit.lookFeet);
		setField(L, "lookAddons", outfit.lookAddons);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaSetMonsterOutfit(lua_State* L)
{
	//doSetMonsterOutfit(cid, name, time)
	int32_t time = (int32_t)popNumber(L);
	std::string name = popString(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(creature)
	{
		ReturnValue ret = Spell::CreateIllusion(creature, name, time);
		if(ret == RET_NOERROR)
			lua_pushboolean(L, LUA_NO_ERROR);
		else
			lua_pushboolean(L, LUA_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaSetItemOutfit(lua_State* L)
{
	//doSetItemOutfit(cid, item, time)
	int32_t time = (int32_t)popNumber(L);
	uint32_t item = (uint32_t)popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
	{
		if(Spell::CreateIllusion(creature, item, time) == RET_NOERROR)
			lua_pushboolean(L, LUA_NO_ERROR);
		else
			lua_pushboolean(L, LUA_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetItemProtection(lua_State* L)
{
	//getItemProtection(uid)
	ScriptEnviroment* env = getScriptEnv();

	Item* item = env->getItemByUID(popNumber(L));
	if(!item)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	lua_pushboolean(L, (item->isScriptProtected() ? LUA_TRUE : LUA_FALSE));
	return 1;
}

int32_t LuaScriptInterface::luaDoSetItemProtection(lua_State* L)
{
	//doSetItemProtection(uid, value)
	bool value = popNumber(L) == LUA_TRUE;

	ScriptEnviroment* env = getScriptEnv();
	if(Item* item = env->getItemByUID(popNumber(L)))
	{
		item->setScriptProtected(value);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetGlobalStorageValue(lua_State* L)
{
	//getGlobalStorageValue(key)
	ScriptEnviroment* env = getScriptEnv();

	std::string strValue;
	if(env->getGlobalStorageValue(popNumber(L), strValue))
	{
		int32_t intValue = atoi(strValue.c_str());
		if(intValue || strValue == "0")
			lua_pushnumber(L, intValue);
		else
			lua_pushstring(L, strValue.c_str());
	}
	else
		lua_pushnumber(L, -1);

	return 1;
}

int32_t LuaScriptInterface::luaSetGlobalStorageValue(lua_State* L)
{
	//setGlobalStorageValue(value, key)
	std::string value;
	bool nil = false;
	if(lua_isnil(L, -1))
	{
		nil = true;
		lua_pop(L, 1);
	}
	else
		value = popString(L);

	ScriptEnviroment* env = getScriptEnv();
	if(!nil)
		nil = env->addGlobalStorageValue(popNumber(L), value);
	else
		nil = env->eraseGlobalStorageValue(popNumber(L));

	lua_pushboolean(L, nil ? LUA_NO_ERROR : LUA_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerDepotItems(lua_State* L)
{
	//getPlayerDepotItems(cid, depotid)
	uint32_t depotid = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		const Depot* depot = player->getDepot(depotid, true);
		if(depot)
			lua_pushnumber(L, depot->getItemHoldingCount());
		else
		{
			reportErrorFunc("Depot not found");
			lua_pushboolean(L, LUA_ERROR);
		}
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetGuildId(lua_State* L)
{
	//doPlayerSetGuildId(cid, id)
	uint32_t id = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		if(player->guildId == 0)
		{
			if(IOGuild::getInstance()->guildExists(id))
				IOGuild::getInstance()->joinGuild(player, id);
		}
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetGuildRank(lua_State* L)
{
	//doPlayerSetGuildRank(cid, rank)
	const char* rank = popString(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		player->setGuildRank(std::string(rank));
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetGuildNick(lua_State* L)
{
	//doPlayerSetGuildNick(cid, nick)
	const char* nick = popString(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		player->setGuildNick(std::string(nick));
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetGuildId(lua_State* L)
{
	//getGuildId(guildName)
	uint32_t guildId;
	if(IOGuild::getInstance()->getGuildIdByName(guildId, popString(L)))
		lua_pushnumber(L, guildId);
	else
	{
		reportErrorFunc("Guild not found");
		lua_pushboolean(L, LUA_ERROR);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetGuildMotd(lua_State* L)
{
	//getGuildMotd(guildId)
	uint32_t guildId = popNumber(L);
	if(IOGuild::getInstance()->guildExists(guildId))
		lua_pushstring(L, IOGuild::getInstance()->getMotd(guildId).c_str());
	else
	{
		reportErrorFunc("Guild not found");
		lua_pushboolean(L, LUA_ERROR);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoMoveCreature(lua_State* L)
{
	//doMoveCreature(cid, direction)
	uint32_t direction = popNumber(L), cid = popNumber(L);
	if(direction < NORTH || direction > NORTHEAST)
	{
		reportErrorFunc("No valid direction");
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	ScriptEnviroment* env = getScriptEnv();
	if(Creature* creature = env->getCreatureByUID(cid))
		lua_pushnumber(L, g_game.internalMoveCreature(creature, (Direction)direction, FLAG_NOLIMIT));
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaIsPlayer(lua_State* L)
{
	//isPlayer(cid)
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	if(env->getPlayerByUID(cid))
		lua_pushboolean(L, LUA_TRUE);
	else
		lua_pushboolean(L, LUA_FALSE);

	return 1;
}

int32_t LuaScriptInterface::luaIsMonster(lua_State* L)
{
	//isMonster(cid)
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Creature* c = env->getCreatureByUID(cid);
	if(c && c->getMonster())
		lua_pushboolean(L, LUA_TRUE);
	else
		lua_pushboolean(L, LUA_FALSE);

	return 1;
}

int32_t LuaScriptInterface::luaIsNpc(lua_State* L)
{
	//isNpc(cid)
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Creature* c = env->getCreatureByUID(cid);
	if(c && c->getNpc())
		lua_pushboolean(L, LUA_TRUE);
	else
		lua_pushboolean(L, LUA_FALSE);

	return 1;
}

int32_t LuaScriptInterface::luaIsCreature(lua_State* L)
{
	//isCreature(cid)
	ScriptEnviroment* env = getScriptEnv();
	lua_pushboolean(L, env->getCreatureByUID(popNumber(L)) ? LUA_TRUE: LUA_FALSE);
	return 1;
}

int32_t LuaScriptInterface::luaIsContainer(lua_State* L)
{
	//isContainer(uid)
	uint32_t uid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	if(env->getContainerByUID(uid))
		lua_pushboolean(L, LUA_TRUE);
	else
		lua_pushboolean(L, LUA_FALSE);

	return 1;
}

int32_t LuaScriptInterface::luaIsCorpse(lua_State* L)
{
	//isCorpse(uid)
	ScriptEnviroment* env = getScriptEnv();
	if(Item* item = env->getItemByUID(popNumber(L)))
	{
		const ItemType& it = Item::items[item->getID()];
		lua_pushboolean(L, (it.corpseType != RACE_NONE ? LUA_TRUE : LUA_FALSE));
	}
	else
		lua_pushboolean(L, LUA_FALSE);

	return 1;
}

int32_t LuaScriptInterface::luaIsMovable(lua_State* L)
{
	//isMovable(uid)
	uint32_t uid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Thing* thing = env->getThingByUID(uid);

	if(thing && thing->isPushable())
		lua_pushboolean(L, LUA_TRUE);
	else
		lua_pushboolean(L, LUA_FALSE);

	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureByName(lua_State* L)
{
	//getCreatureByName(name)
	ScriptEnviroment* env = getScriptEnv();
	if(Creature* creature = g_game.getCreatureByName(popString(L)))
		lua_pushnumber(L, env->addThing(creature));
	else
		lua_pushnil(L);

	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerByNameWildcard(lua_State* L)
{
	//getPlayerByNameWildcard(name~)
	Player* player = NULL;

	ScriptEnviroment* env = getScriptEnv();
	if(g_game.getPlayerByNameWildcard(popString(L), player) == RET_NOERROR)
		lua_pushnumber(L, env->addThing(player));
	else
		lua_pushnil(L);

	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerGUIDByName(lua_State* L)
{
	//getPlayerGUIDByName(name[, multiworld])
	bool multiworld = false;
	if(lua_gettop(L) > 1)
		multiworld = popNumber(L) == LUA_TRUE;

	std::string name = popString(L);
	uint32_t guid;
	if(Player* player = g_game.getPlayerByName(name.c_str()))
		lua_pushnumber(L, player->getGUID());
	else if(IOLoginData::getInstance()->getGuidByName(guid, name, multiworld))
		lua_pushnumber(L, guid);
	else
		lua_pushnil(L);

	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerNameByGUID(lua_State* L)
{
	//getPlayerNameByGUID(guid[, multiworld])
	bool multiworld = false;
	if(lua_gettop(L) > 1)
		multiworld = popNumber(L) == LUA_TRUE;

	uint32_t guid = popNumber(L);
	std::string name;
	if(!IOLoginData::getInstance()->getNameByGuid(guid, name, multiworld))
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushnil(L);
	}

	lua_pushstring(L, name.c_str());
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayersByAccountId(lua_State *L)
{
	//getPlayersByAccountId(accId)
	uint32_t accId = popNumber(L);
	PlayerVector players = g_game.getPlayersByAccount(accId);

	ScriptEnviroment* env = getScriptEnv();
	PlayerVector::iterator it = players.begin();

	lua_newtable(L);
	for(uint32_t i = 1; it != players.end(); ++it, ++i)
	{
		lua_pushnumber(L, i);
		lua_pushnumber(L, env->addThing(*it));
		lua_settable(L, -3);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetIpByName(lua_State *L)
{
	//getIpByName(name)
	std::string name = popString(L);

	if(Player* player = g_game.getPlayerByName(name))
		lua_pushnumber(L, player->getIP());
	else
		lua_pushnumber(L, IOLoginData::getInstance()->getLastIPByName(name));

	return 1;
}

int32_t LuaScriptInterface::luaGetPlayersByIp(lua_State *L)
{
	//getPlayersByIp(ip[, mask])
	uint32_t mask = 0xFFFFFFFF;
	if(lua_gettop(L) >= 2)
		mask = (uint32_t)popNumber(L);

	uint32_t ip = (uint32_t)popNumber(L);
	PlayerVector players = g_game.getPlayersByIP(ip, mask);

	ScriptEnviroment* env = getScriptEnv();
	PlayerVector::iterator it = players.begin();

	lua_newtable(L);
	for(uint32_t i = 1; it != players.end(); ++it, ++i)
	{
		lua_pushnumber(L, i);
		lua_pushnumber(L, env->addThing(*it));
		lua_settable(L, -3);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetAccountIdByName(lua_State *L)
{
	//getAccountIdByName(name)
	std::string name = popString(L);

	if(Player* player = g_game.getPlayerByName(name))
		lua_pushnumber(L, player->getAccount());
	else
		lua_pushnumber(L, IOLoginData::getInstance()->getAccountIdByName(name));

	return 1;
}

int32_t LuaScriptInterface::luaGetAccountByName(lua_State *L)
{
	//getAccountByName(name)
	std::string name = popString(L);

	if(Player* player = g_game.getPlayerByName(name))
		lua_pushstring(L, player->getAccountName().c_str());
	else
	{
		std::string tmp;
		IOLoginData::getInstance()->getAccountName(IOLoginData::getInstance()->getAccountIdByName(name), tmp);
		lua_pushstring(L, tmp.c_str());
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetAccountIdByAccount(lua_State *L)
{
	//getAccountIdByAccount(accName)
	uint32_t value = 0;
	IOLoginData::getInstance()->getAccountId(popString(L), value);
	lua_pushnumber(L, value);
	return 1;
}

int32_t LuaScriptInterface::luaGetAccountByAccountId(lua_State *L)
{
	//getAccountByAccountId(accId)
	std::string value = 0;
	IOLoginData::getInstance()->getAccountName(popNumber(L), value);
	lua_pushstring(L, value.c_str());
	return 1;
}

int32_t LuaScriptInterface::luaRegisterCreatureEvent(lua_State* L)
{
	//registerCreatureEvent(cid, name)
	const char* name = popString(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(creature)
	{
		creature->registerCreatureEvent(name);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetContainerSize(lua_State* L)
{
	//getContainerSize(uid)
	uint32_t uid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	if(Container* container = env->getContainerByUID(uid))
		lua_pushnumber(L, container->size());
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CONTAINER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetContainerCap(lua_State* L)
{
	//getContainerCap(uid)
	uint32_t uid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	if(Container* container = env->getContainerByUID(uid))
		lua_pushnumber(L, container->capacity());
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CONTAINER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetContainerCapById(lua_State* L)
{
	//getContainerCapById(itemid)
	uint32_t itemId = popNumber(L);

	const ItemType& it = Item::items[itemId];
	if(it.isContainer())
		lua_pushnumber(L, it.maxItems);
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CONTAINER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetContainerItem(lua_State* L)
{
	//getContainerItem(uid, slot)
	uint32_t slot = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	if(Container* container = env->getContainerByUID(popNumber(L)))
	{
		if(Item* item = container->getItem(slot))
			pushThing(L, item, env->addThing(item));
		else
			pushThing(L, NULL, 0);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CONTAINER_NOT_FOUND));
		pushThing(L, NULL, 0);
	}
	return 1;

}

int32_t LuaScriptInterface::luaDoAddContainerItemEx(lua_State* L)
{
	//doAddContainerItemEx(uid, virtuid)
	uint32_t virtuid = popNumber(L);
	uint32_t uid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	Container* container = env->getContainerByUID(uid);
	if(container)
	{
		Item* item = env->getItemByUID(virtuid);
		if(!item)
		{
			reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
			lua_pushboolean(L, LUA_ERROR);
			return 1;
		}

		if(item->getParent() != VirtualCylinder::virtualCylinder)
		{
			reportErrorFunc("Item already has a parent");
			lua_pushboolean(L, LUA_ERROR);
			return 1;
		}

		ReturnValue ret = RET_NOERROR;
		ret = g_game.internalAddItem(NULL, container, item);
		if(ret == RET_NOERROR)
			env->removeTempItem(item);

		lua_pushnumber(L, ret);
		return 1;
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CONTAINER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}
}

int32_t LuaScriptInterface::luaDoAddContainerItem(lua_State* L)
{
	//doAddContainerItem(uid, itemid[, count/subType])
	int32_t parameters = lua_gettop(L);

	uint32_t count = 0;
	if(parameters > 2)
		count = popNumber(L);

	uint16_t itemId = (uint16_t)popNumber(L);
	uint32_t uid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	Container* container = env->getContainerByUID(uid);
	if(container)
	{
		const ItemType& it = Item::items[itemId];
		if(it.stackable && count > 100)
		{
			int32_t subCount = count;
			while(subCount > 0)
			{
				int32_t stackCount = std::min((int32_t)100, (int32_t)subCount);
				Item* newItem = Item::CreateItem(itemId, stackCount);

				if(!newItem)
				{
					reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
					lua_pushboolean(L, LUA_ERROR);
					return 1;
				}

				ReturnValue ret = g_game.internalAddItem(NULL, container, newItem);
				if(ret != RET_NOERROR)
				{
					delete newItem;
					lua_pushboolean(L, LUA_ERROR);
					return 1;
				}

				subCount = subCount - stackCount;
				if(subCount == 0)
				{
					if(newItem->getParent())
						lua_pushnumber(L, env->addThing((Thing*)newItem));
					else //stackable item stacked with existing object, newItem will be released
						lua_pushnil(L);

					return 1;
				}
			}
		}
		else
		{
			Item* newItem = Item::CreateItem(itemId, count);
			ReturnValue ret = g_game.internalAddItem(NULL, container, newItem);
			if(ret != RET_NOERROR)
			{
				delete newItem;
				lua_pushboolean(L, LUA_ERROR);
				return 1;
			}

			if(newItem->getParent())
				lua_pushnumber(L, env->addThing((Thing*)newItem));
			else //stackable item stacked with existing object, newItem will be released
				lua_pushnil(L);

			return 1;
		}
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CONTAINER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}
	return 0;
}

int32_t LuaScriptInterface::luaGetFluidSourceType(lua_State* L)
{
	//getFluidSourceType(type)
	uint32_t type = popNumber(L);

	const ItemType& it = Item::items[type];
	if(it.id != 0)
		lua_pushnumber(L, it.fluidSource);
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaIsInArray(lua_State* L)
{
	//isInArray(array, value[, lower])
	bool lower = true;
	if(lua_gettop(L) >= 3)
		lower = (bool)popNumber(L);

	std::string value;
	if(lua_isnil(L, -1) == 1)
		lua_pop(L, 1);
	else if(lua_isnumber(L, -1) == 1)
	{
		char tmp[40];
		sprintf(tmp, "%d", (int32_t)popNumber(L));
		value = tmp;
	}
	else if(lua_isboolean(L, -1) == 1)
	{
		char tmp[3];
		sprintf(tmp, "%d", (int32_t)popBoolean(L));
		value = tmp;
	}
	else if(lua_isstring(L, -1) == 1)
		value = popString(L);
	else
	{
		lua_pop(L, 1);
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	if(lower)
		toLowerCaseString(value);

	int32_t i = 1;
	while(true)
	{
		lua_pushnumber(L, i);
		lua_gettable(L, -2);
		if(lua_isnil(L, -1) == 1)
		{
			lua_pop(L, 2);
			lua_pushboolean(L, LUA_FALSE);
			return 1;
		}
		else if(lua_isnumber(L, -1) == 1)
		{
			int32_t arrayValue = (int32_t)popNumber(L);
			if(arrayValue == atoi(value.c_str()))
			{
				lua_pop(L, 1);
				lua_pushboolean(L, LUA_TRUE);
				return 1;
			}
		}
		else if(lua_isboolean(L, -1) == 1)
		{
			int32_t arrayValue = (int32_t)popBoolean(L);
			if(arrayValue == atoi(value.c_str()))
			{
				lua_pop(L, 1);
				lua_pushboolean(L, LUA_TRUE);
				return 1;
			}
		}
		else if(lua_isstring(L, -1) == 1)
		{
			std::string arrayValue = popString(L);
			if(lower)
				toLowerCaseString(arrayValue);

			if(arrayValue == value)
			{
				lua_pop(L, 1);
				lua_pushboolean(L, LUA_TRUE);
				return 1;
			}
		}
		else
			break;

		++i;
	}

	lua_pop(L, 2);
	lua_pushboolean(L, LUA_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerAddOutfit(lua_State* L)
{
	//doPlayerAddOutfit(cid, looktype, addon)
	uint32_t addon = popNumber(L);
	uint32_t looktype = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		player->addOutfit(looktype, addon);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerRemoveOutfit(lua_State* L)
{
	//doPlayerRemoveOutfit(cid, looktype, addon)
	uint32_t addon = popNumber(L);
	uint32_t looktype = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		player->remOutfit(looktype, addon);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaCanPlayerWearOutfit(lua_State* L)
{
	//canPlayerWearOutfit(cid, looktype, addon)
	uint32_t addon = popNumber(L);
	uint32_t looktype = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		lua_pushboolean(L, player->canWear(looktype, addon) ? LUA_TRUE : LUA_FALSE);
		return 1;
	}

	reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
	lua_pushboolean(L, LUA_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaDoCreatureChangeOutfit(lua_State* L)
{
	//doCreatureChangeOutfit(cid, outfit)
	Outfit_t outfit;
	outfit.lookType = getField(L, "lookType");
	outfit.lookHead = getField(L, "lookHead");
	outfit.lookBody = getField(L, "lookBody");
	outfit.lookLegs = getField(L, "lookLegs");
	outfit.lookFeet = getField(L, "lookFeet");
	outfit.lookAddons = getField(L, "lookAddons");
	lua_pop(L, 1);

	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	Creature* creature = env->getCreatureByUID(cid);
	if(creature)
	{
		creature->defaultOutfit = outfit;
		g_game.internalCreatureChangeOutfit(creature, outfit);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoSetCreatureLight(lua_State* L)
{
	//doSetCreatureLight(cid, lightLevel, lightColor, time)
	uint32_t time = popNumber(L);
	uint8_t color = (uint8_t)popNumber(L);
	uint8_t level = (uint8_t)popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	Creature* creature = env->getCreatureByUID(cid);
	if(creature)
	{
		Condition* condition = Condition::createCondition(CONDITIONID_COMBAT, CONDITION_LIGHT, time, level | (color << 8));
		creature->addCondition(condition);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerPopupFYI(lua_State* L)
{
	//doPlayerPopupFYI(cid, message)
	std::string message = popString(L);
	uint32_t cid = (uint32_t)popNumber(L);
	ScriptEnviroment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		player->sendFYIBox(message);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSendTutorial(lua_State* L)
{
	//doPlayerSendTutorial(cid, id)
	uint8_t id = (uint8_t)popNumber(L);
	uint32_t cid = (uint32_t)popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Player* player = env->getPlayerByUID(cid);
	if(!player)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	player->sendTutorial(id);
	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerAddMapMark(lua_State* L)
{
	//doPlayerAddMapMark(cid, pos, type[, description])
	int32_t parameters = lua_gettop(L);
	std::string description = "";
	Position pos;
	uint32_t stackpos;

	if(parameters > 3)
		description = popString(L);

	MapMarks_t type = (MapMarks_t)popNumber(L);
	popPosition(L, pos, stackpos);
	uint32_t cid = (uint32_t)popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Player* player = env->getPlayerByUID(cid);
	if(!player)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	player->sendAddMarker(pos, type, description);
	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerAddPremiumDays(lua_State* L)
{
	//doPlayerAddPremiumDays(cid, days)
	int32_t days = popNumber(L);
	uint32_t cid = popNumber(L);
	ScriptEnviroment* env = getScriptEnv();
	if(Player* player = env->getPlayerByUID(cid))
	{
		Account account = IOLoginData::getInstance()->loadAccount(player->getAccount());
		if(player->premiumDays < 65535)
		{
			if(days < 0)
			{
				account.premiumDays = std::max((uint32_t)0, uint32_t(account.premiumDays + (int32_t)days));
				player->premiumDays = std::max((uint32_t)0, uint32_t(player->premiumDays + (int32_t)days));
			}
			else
			{
				account.premiumDays = std::min((uint32_t)65534, uint32_t(account.premiumDays + (uint32_t)days));
				player->premiumDays = std::min((uint32_t)65534, uint32_t(player->premiumDays + (uint32_t)days));
			}
			IOLoginData::getInstance()->saveAccount(account);
		}
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetItemDescriptionsById(lua_State* L)
{
	//getItemDescriptionsById(itemid)
	uint32_t itemid = popNumber(L);
	const ItemType& it = Item::items[itemid];

	lua_newtable(L);
	setField(L, "name", it.name.c_str());
	setField(L, "article", it.article.c_str());
	setField(L, "plural", it.pluralName.c_str());
	return 1;
}

int32_t LuaScriptInterface::luaGetItemWeightById(lua_State* L)
{
	//getItemWeightById(itemid, count[, precise = TRUE])
	int32_t parameters = lua_gettop(L);

	bool precise = true;
	if(parameters > 2)
		precise = popNumber(L) == LUA_TRUE;

	int32_t count = popNumber(L);
	uint32_t itemid = popNumber(L);

	const ItemType& it = Item::items[itemid];
	double weight = it.weight * std::max(1, count);
	if(precise)
	{
		std::stringstream ws;
		ws << std::fixed << std::setprecision(2) << weight;
		weight = atof(ws.str().c_str());
	}

	lua_pushnumber(L, weight);
	return 1;
}

int32_t LuaScriptInterface::luaHasProperty(lua_State* L)
{
	//hasProperty(uid, prop)
	uint32_t prop = popNumber(L);
	uint32_t uid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Item* item = env->getItemByUID(uid);
	if(!item)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	//Check if the item is a tile, so we can get more accurate properties
	bool hasProp = item->hasProperty((ITEMPROPERTY)prop);
	if(item->getTile() && item->getTile()->ground == item)
		hasProp = item->getTile()->hasProperty((ITEMPROPERTY)prop);

	lua_pushboolean(L, hasProp ? LUA_TRUE : LUA_FALSE);
	return 1;
}

int32_t LuaScriptInterface::luaGetItemIdByName(lua_State* L)
{
	//getItemIdByName(name[, reportError])
	bool _reportError = true;
	if(lua_gettop(L) >= 2)
		_reportError = popNumber(L) == LUA_TRUE;

	int32_t itemId = Item::items.getItemIdByName(popString(L));
	if(itemId == -1)
	{
		if(_reportError)
			reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));

		lua_pushboolean(L, LUA_ERROR);
	}
	else
		lua_pushnumber(L, itemId);

	return 1;
}

int32_t LuaScriptInterface::luaIsSightClear(lua_State* L)
{
	//isSightClear(fromPos, toPos, floorCheck)
	PositionEx fromPos, toPos;
	bool floorCheck = (popNumber(L) == 1);
	popPosition(L, toPos);
	popPosition(L, fromPos);

	bool result = g_game.isSightClear(fromPos, toPos, floorCheck);
	lua_pushboolean(L, (result ? LUA_TRUE : LUA_FALSE));
	return 1;
}

int32_t LuaScriptInterface::luaGetCreaturePosition(lua_State* L)
{
	//getCreaturePosition(cid)
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(creature)
	{
		Position pos = creature->getPosition();
		uint32_t stackpos = 0;
		if(Tile* tile = creature->getTile())
			stackpos = tile->__getIndexOfThing(creature);
		pushPosition(L, pos, stackpos);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureName(lua_State* L)
{
	//getCreatureName(cid)
	ScriptEnviroment* env = getScriptEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
		lua_pushstring(L, creature->getName().c_str());
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureNoMove(lua_State* L)
{
	//getCreatureNoMove(cid)
	ScriptEnviroment* env = getScriptEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
		lua_pushboolean(L, creature->getNoMove() ? LUA_TRUE : LUA_FALSE);
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureSkullType(lua_State* L)
{
	//getCreatureSkullType(cid)
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	if(Creature* creature = env->getCreatureByUID(cid))
		lua_pushnumber(L, creature->getSkull());
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoCreatureSetLookDir(lua_State* L)
{
	//doCreatureSetLookDir(cid, dir)
	Direction dir = (Direction)popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
	{
		if(dir < NORTH || dir > WEST)
		{
			lua_pushboolean(L, LUA_ERROR);
			return 1;
		}

		g_game.internalCreatureTurn(creature, dir);
		if(Player* player = creature->getPlayer())
			player->resetIdleTime();

		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoCreatureSetSkullType(lua_State* L)
{
	//doCreatureSetSkullType(cid, skull)
	Skulls_t skull = (Skulls_t)popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	if(Creature* creature = env->getCreatureByUID(cid))
	{
		creature->setSkull(skull);
		g_game.updateCreatureSkull(creature);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetRedSkullTicks(lua_State* L)
{
	//doPlayerSetRedSkullTicks(cid, amount)
	int64_t amount = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	if(Player* player = env->getPlayerByUID(cid))
	{
		player->setRedSkullTicks(std::max((int64_t)0, amount));
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureSpeed(lua_State* L)
{
	//getCreatureSpeed(cid)
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(creature)
		lua_pushnumber(L, creature->getSpeed());
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureBaseSpeed(lua_State* L)
{
	//getCreatureBaseSpeed(cid)
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(creature)
		lua_pushnumber(L, creature->getBaseSpeed());
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureTarget(lua_State* L)
{
	//getCreatureTarget(cid)
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(creature)
	{
		Creature* target = creature->getAttackedCreature();
		lua_pushnumber(L, target ? env->addThing(target) : 0);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaIsItemStackable(lua_State* L)
{
	//isItemStackable(itemid)
	const ItemType& it = Item::items[popNumber(L)];
	lua_pushboolean(L, it.stackable ? LUA_TRUE : LUA_FALSE);
	return 1;
}

int32_t LuaScriptInterface::luaIsItemRune(lua_State* L)
{
	//isItemRune(itemid)
	const ItemType& it = Item::items[popNumber(L)];
	lua_pushboolean(L, it.isRune() ? LUA_TRUE : LUA_FALSE);
	return 1;
}

int32_t LuaScriptInterface::luaIsItemDoor(lua_State* L)
{
	//isItemDoor(itemid)
	const ItemType& it = Item::items[popNumber(L)];
	lua_pushboolean(L, it.isDoor() ? LUA_TRUE : LUA_FALSE);
	return 1;
}

int32_t LuaScriptInterface::luaGetItemLevelDoor(lua_State* L)
{
	//getItemLevelDoor(itemid)
	lua_pushnumber(L, Item::items[popNumber(L)].levelDoor);
	return 1;
}

int32_t LuaScriptInterface::luaIsItemContainer(lua_State* L)
{
	//isItemContainer(itemid)
	const ItemType& it = Item::items[popNumber(L)];
	lua_pushboolean(L, it.isContainer() ? LUA_TRUE : LUA_FALSE);
	return 1;
}

int32_t LuaScriptInterface::luaIsItemFluidContainer(lua_State* L)
{
	//isItemFluidContainer(itemid)
	const ItemType& it = Item::items[popNumber(L)];
	lua_pushboolean(L, it.isFluidContainer() ? LUA_TRUE : LUA_FALSE);
	return 1;
}

int32_t LuaScriptInterface::luaIsItemMovable(lua_State* L)
{
	const ItemType& it = Item::items[popNumber(L)];
	lua_pushboolean(L, it.moveable ? LUA_TRUE : LUA_FALSE);
	return 1;
}

int32_t LuaScriptInterface::luaWait(lua_State* L)
{
	//wait(delay)
	return lua_yield(L, 1);
}

int32_t LuaScriptInterface::luaAddEvent(lua_State* L)
{
	//addEvent(callback, delay, ...)
	ScriptEnviroment* env = getScriptEnv();

	LuaScriptInterface* script_interface = env->getScriptInterface();
	if(!script_interface)
	{
		reportError(__FUNCTION__, "No valid script interface!");
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	int32_t parameters = lua_gettop(L);
	if(lua_isfunction(L, -parameters) == 0) //-parameters means the first parameter from left to right
	{
		reportError(__FUNCTION__, "callback parameter should be a function.");
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	LuaTimerEventDesc eventDesc;
	std::list<int32_t> params;
	for(int32_t i = 0; i < parameters - 2; ++i) //-2 because addEvent needs at least two parameters
		params.push_back(luaL_ref(L, LUA_REGISTRYINDEX));

	eventDesc.parameters = params;

	uint32_t delay = std::max((int64_t)100, popNumber(L));
	eventDesc.function = luaL_ref(L, LUA_REGISTRYINDEX);

	eventDesc.scriptId = env->getScriptId();

	script_interface->m_lastEventTimerId++;
	script_interface->m_timerEvents[script_interface->m_lastEventTimerId] = eventDesc;

	Scheduler::getScheduler().addEvent(createSchedulerTask(delay, boost::bind(&LuaScriptInterface::executeTimerEvent, script_interface, script_interface->m_lastEventTimerId)));
	lua_pushnumber(L, script_interface->m_lastEventTimerId);
	return 1;
}

int32_t LuaScriptInterface::luaStopEvent(lua_State* L)
{
	//stopEvent(eventid)
	uint32_t eventId = popNumber(L);
	ScriptEnviroment* env = getScriptEnv();

	LuaScriptInterface* script_interface = env->getScriptInterface();
	if(!script_interface)
	{
		reportError(__FUNCTION__, "No valid script interface!");
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	LuaTimerEvents::iterator it = script_interface->m_timerEvents.find(eventId);
	if(it != script_interface->m_timerEvents.end())
	{
		for(std::list<int32_t>::iterator lt = it->second.parameters.begin(); lt != it->second.parameters.end(); ++lt)
			luaL_unref(script_interface->m_luaState, LUA_REGISTRYINDEX, *lt);
		it->second.parameters.clear();

		luaL_unref(script_interface->m_luaState, LUA_REGISTRYINDEX, it->second.function);
		script_interface->m_timerEvents.erase(it);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
		lua_pushboolean(L, LUA_ERROR);

	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureCondition(lua_State* L)
{
	//getCreatureCondition(cid, condition[, subId])
	uint32_t subId = 0, condition;
	if(lua_gettop(L) >= 3)
		subId = popNumber(L);

	condition = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
		lua_pushboolean(L, creature->hasCondition((ConditionType_t)condition, subId) ? LUA_TRUE : LUA_FALSE);
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_FALSE);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerBlessing(lua_State* L)
{
	//getPlayerBlessings(cid, blessing)
	int16_t blessing = popNumber(L) - 1;

	ScriptEnviroment* env = getScriptEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
		lua_pushboolean(L, (player->hasBlessing(blessing) ? LUA_TRUE : LUA_FALSE));
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerAddBlessing(lua_State* L)
{
	//doPlayerAddBlessing(cid, blessing)
	int16_t blessing = popNumber(L) - 1;
	ScriptEnviroment* env = getScriptEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		if(!player->hasBlessing(blessing))
		{
			player->addBlessing(1 << blessing);
			lua_pushboolean(L, LUA_TRUE);
		}
		else
			lua_pushboolean(L, LUA_FALSE);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaSetPlayerPromotionLevel(lua_State* L)
{
	//setPlayerPromotionLevel(cid, level)
	uint32_t level = popNumber(L);
	ScriptEnviroment* env = getScriptEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		player->setPromotionLevel(level);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaSetPlayerGroupId(lua_State* L)
{
	//setPlayerGroupId(cid, groupId)
	uint32_t groupId = popNumber(L);
	ScriptEnviroment* env = getScriptEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		if(Group* group = Groups::getInstance()->getGroup(groupId))
		{
			player->setGroup(group);
			lua_pushboolean(L, LUA_TRUE);
		}
		else
			lua_pushboolean(L, LUA_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_FALSE);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureMana(lua_State* L)
{
	//getCreatureMana(cid)
	uint32_t cid = popNumber(L);
	ScriptEnviroment* env = getScriptEnv();
	Creature* creature = env->getCreatureByUID(cid);
	if(creature)
		lua_pushnumber(L, creature->getMana());
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureMaxMana(lua_State* L)
{
	//getCreatureMaxMana(cid)
	ScriptEnviroment* env = getScriptEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
		lua_pushnumber(L, creature->getMaxMana());
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureHealth(lua_State* L)
{
	//getCreatureHealth(cid)
	ScriptEnviroment* env = getScriptEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
		lua_pushnumber(L, creature->getHealth());
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureLookDirection(lua_State* L)
{
	//getCreatureLookDirection(cid)
	ScriptEnviroment* env = getScriptEnv();

	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
		lua_pushnumber(L, creature->getDirection());
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureMaxHealth(lua_State* L)
{
	//getCreatureMaxHealth(cid)
	uint32_t cid = popNumber(L);
	ScriptEnviroment* env = getScriptEnv();
	Creature* creature = env->getCreatureByUID(cid);
	if(creature)
		lua_pushnumber(L, creature->getMaxHealth());
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaSetPlayerStamina(lua_State* L)
{
	//setPlayerStamina(cid, minutes)
	uint32_t minutes = popNumber(L);
	uint32_t cid = popNumber(L);
	ScriptEnviroment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		player->setStaminaMinutes(minutes);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerAddStamina(lua_State* L)
{
	//doPlayerAddStamina(cid, minutes)
	int32_t minutes = popNumber(L);
	uint32_t cid = popNumber(L);
	ScriptEnviroment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		player->setStaminaMinutes(player->getStaminaMinutes() + minutes);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaSetPlayerPartner(lua_State* L)
{
	//setPlayerPartner(cid, guid)
	uint32_t guid = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(!player)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	player->marriage = guid;
	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerParty(lua_State* L)
{
	//getPlayerParty(cid)
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	if(Player* player = env->getPlayerByUID(cid))
	{
		if(Party* party = player->getParty())
			lua_pushnumber(L, env->addThing(party->getLeader()));
		else
			lua_pushnil(L);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerJoinParty(lua_State* L)
{
	//doPlayerJoinParty(cid, lid)
	ScriptEnviroment* env = getScriptEnv();

	Player* leader = env->getPlayerByUID(popNumber(L));
	if(!leader)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_FALSE);
	}

	Player* player = env->getPlayerByUID(popNumber(L));
	if(!player)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_FALSE);
	}

	g_game.playerJoinParty(player->getID(), leader->getID());
	lua_pushboolean(L, LUA_TRUE);
	return 1;
}

int32_t LuaScriptInterface::luaGetPartyMembers(lua_State* L)
{
	//getPartyMembers(cid)
	ScriptEnviroment* env = getScriptEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		if(Party* party = player->getParty())
		{
			PlayerVector list = party->getMembers();
			list.push_back(party->getLeader());

			PlayerVector::const_iterator it = list.begin();
			lua_newtable(L);
			for(uint32_t i = 1; it != list.end(); ++it, ++i)
			{
				lua_pushnumber(L, i);
				lua_pushnumber(L, (*it)->getID());
				lua_settable(L, -3);
			}

			return 1;
		}
	}

	lua_pushboolean(L, LUA_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaGetVocationInfo(lua_State* L)
{
	//getVocationInfo(id)
	uint32_t id = popNumber(L);
	Vocation* voc = Vocations::getInstance()->getVocation(id);
	if(!voc)
	{
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	lua_newtable(L);
	setField(L, "id", voc->getId());
	setField(L, "name", voc->getName().c_str());
	setField(L, "description", voc->getDescription().c_str());
	setField(L, "healthGain", voc->getHealthGain());
	setField(L, "healthGainTicks", voc->getHealthGainTicks());
	setField(L, "healthGainAmount", voc->getHealthGainAmount());
	setField(L, "manaGain", voc->getManaGain());
	setField(L, "manaGainTicks", voc->getManaGainTicks());
	setField(L, "manaGainAmount", voc->getManaGainAmount());
	setField(L, "attackSpeed", voc->getAttackSpeed());
	setField(L, "baseSpeed", voc->getBaseSpeed());
	setField(L, "fromVocation", voc->getFromVocation());
	setField(L, "promotedVocation", Vocations::getInstance()->getPromotedVocation(id));
	setField(L, "soul", voc->getSoulMax());
	setField(L, "soulTicks", voc->getSoulGainTicks());
	setField(L, "capacity", voc->getCapGain());
	setFieldBool(L, "attackable", voc->isAttackable());
	setFieldBool(L, "needPremium", voc->isPremiumNeeded());
	return 1;
}

int32_t LuaScriptInterface::luaGetGroupInfo(lua_State* L)
{
	//getGroupInfo(id[, premium])
	bool premium = false;
	if(lua_gettop(L) >= 2)
		premium = popNumber(L);

	Group* group = Groups::getInstance()->getGroup(popNumber(L));
	if(!group)
	{
		lua_pushboolean(L, LUA_FALSE);
		return 1;
	}

	lua_newtable(L);
	setField(L, "id", group->getId());
	setField(L, "name", group->getName().c_str());
	setField(L, "access", group->getAccess());
	setField(L, "violationAccess", group->getViolationAccess());
	setField(L, "depotLimit", group->getDepotLimit(premium));
	setField(L, "maxVips", group->getMaxVips(premium));
	setField(L, "outfit", group->getOutfit());
	return 1;
}

int32_t LuaScriptInterface::luaGetPlayersOnline(lua_State* L)
{
	//getPlayersOnline()
	ScriptEnviroment* env = getScriptEnv();
	AutoList<Player>::listiterator it = Player::listPlayer.list.begin();

	lua_newtable(L);
	for(int32_t i = 1; it != Player::listPlayer.list.end(); ++it, ++i)
	{
		lua_pushnumber(L, i);
		lua_pushnumber(L, env->addThing(it->second));
		lua_settable(L, -3);
	}
	return 1;
}

int32_t LuaScriptInterface::luaSetCreatureMaxHealth(lua_State* L)
{
	//setCreatureMaxHealth(uid, health)
	uint32_t maxHealth = (uint32_t)popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	Creature* creature = env->getCreatureByUID(cid);
	if(creature)
	{
		creature->changeMaxHealth(maxHealth);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaSetCreatureMaxMana(lua_State* L)
{
	//setCreatureMaxMana(uid, mana)
	uint32_t maxMana = (uint32_t)popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	Creature* creature = env->getCreatureByUID(cid);
	if(creature)
	{
		creature->changeMaxMana(maxMana);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureMaster(lua_State *L)
{
	//getCreatureMaster(cid)
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(!creature)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	Creature* master = creature->getMaster();
	if(!master)
	{
		lua_pushnumber(L, cid);
		return 1;
	}

	lua_pushnumber(L, env->addThing(master));
	return 1;
}

int32_t LuaScriptInterface::luaGetCreatureSummons(lua_State *L)
{
	//getCreatureSummons(cid)
	uint32_t cid = popNumber(L);
	ScriptEnviroment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(!creature)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	const std::list<Creature*>& summons = creature->getSummons();
	CreatureList::const_iterator it = summons.begin();

	lua_newtable(L);
	for(uint32_t i = 1; it != summons.end(); ++it, ++i)
	{
		lua_pushnumber(L, i);
		lua_pushnumber(L, env->addThing(*it));
		lua_settable(L, -3);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerResetIdleTime(lua_State* L)
{
	//doPlayerResetIdleTime(cid)
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player)
	{
		player->resetIdleTime();
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoCreatureSetNoMove(lua_State* L)
{
	//doCreatureSetNoMove(cid, block)
	bool block = popNumber(L) == 1;

	ScriptEnviroment* env = getScriptEnv();
	if(Creature* creature = env->getCreatureByUID(popNumber(L)))
	{
		creature->setNoMove(block);
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetPlayerRates(lua_State* L)
{
	//getPlayerRates(cid)
	ScriptEnviroment* env = getScriptEnv();

	Player* player = env->getPlayerByUID(popNumber(L));
	if(!player)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	lua_newtable(L);
	for(uint32_t i = SKILL_FIRST; i <= SKILL__LAST; ++i)
	{
		lua_pushnumber(L, i);
		lua_pushnumber(L, player->rates[(skills_t)i]);
		lua_settable(L, -3);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSetRate(lua_State* L)
{
	//doPlayerSetRate(cid, type, value)
	float value = popFloatNumber(L);
	uint32_t type = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	if(Player* player = env->getPlayerByUID(popNumber(L)))
	{
		if(type > SKILL__LAST)
		{
			reportErrorFunc("Invalid type");
			lua_pushboolean(L, LUA_ERROR);
		}
		else
		{
			player->rates[(skills_t)type] = value;
			lua_pushboolean(L, LUA_NO_ERROR);
		}
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}

	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSwitchSaving(lua_State* L)
{
	//doPlayerSwitchSaving(cid)
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	if(Player* player = env->getPlayerByUID(cid))
	{
		player->switchSaving();
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoPlayerSave(lua_State* L)
{
	//doPlayerSave(cid)
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	if(Player* player = env->getPlayerByUID(cid))
		lua_pushboolean(L, IOLoginData::getInstance()->savePlayer(player, false) ? LUA_TRUE : LUA_FALSE);
	else
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetTownId(lua_State* L)
{
	//getTownId(townName)
	std::string townName = popString(L);
	if(Town* town = Towns::getInstance().getTown(townName))
		lua_pushnumber(L, town->getTownID());
	else
	{
		reportError(__FUNCTION__, "town not found.");
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetTownName(lua_State* L)
{
	//getTownName(townId)
	uint32_t townId = popNumber(L);
	if(Town* town = Towns::getInstance().getTown(townId))
		lua_pushstring(L, town->getName().c_str());
	else
	{
		reportError(__FUNCTION__, "town not found.");
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetTownTemplePosition(lua_State* L)
{
	//getTownTemplePosition(townId[, displayError])
	bool displayError = true;
	if(lua_gettop(L) >= 2)
		displayError = popNumber(L) == LUA_TRUE;

	uint32_t townId = popNumber(L);

	if(Town* town = Towns::getInstance().getTown(townId))
		pushPosition(L, town->getTemplePosition(), 255);
	else
	{
		if(displayError)
			reportError(__FUNCTION__, "town not found.");

		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaGetTownHouses(lua_State* L)
{
	//getTownHouses(townId)
	uint32_t townId = 0;
	if(lua_gettop(L) > 0)
		townId = popNumber(L);

	HouseMap::iterator it = Houses::getInstance().getHouseBegin();

	lua_newtable(L);
	for(uint32_t i = 1; it != Houses::getInstance().getHouseEnd(); ++i, ++it)
	{
		if(townId != 0 && it->second->getTownId() != townId)
			continue;

		lua_pushnumber(L, i);
		lua_pushnumber(L, it->second->getHouseId());
		lua_settable(L, -3);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetSpectators(lua_State *L)
{
	//getSpectators(centerPos, rangex, rangey[, multifloor])
	bool multifloor = false;
	if(lua_gettop(L) >= 4)
		multifloor = popNumber(L) == LUA_TRUE;

	uint32_t rangey = popNumber(L), rangex = popNumber(L);
	PositionEx centerPos;
	popPosition(L, centerPos);

	SpectatorVec list;
	g_game.getSpectators(list, centerPos, false, multifloor, rangex, rangex, rangey, rangey);
	if(list.empty())
	{
		lua_pushnil(L);
		return 1;
	}

	ScriptEnviroment* env = getScriptEnv();
	SpectatorVec::const_iterator it = list.begin();

	lua_newtable(L);
	for(uint32_t i = 1; it != list.end(); ++it, ++i)
	{
		lua_pushnumber(L, i);
		lua_pushnumber(L, env->addThing(*it));
		lua_settable(L, -3);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetHighscoreString(lua_State* L)
{
	//getHighscoreString(skillId)
	uint16_t skillId = popNumber(L);
	if(skillId > 8)
	{
		reportErrorFunc("Invalid skillId");
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	lua_pushstring(L, g_game.getHighscoreString(skillId).c_str());
	return 1;
}

int32_t LuaScriptInterface::luaGetWaypointPosition(lua_State* L)
{
	//getWaypointPosition(name)
	if(WaypointPtr waypoint = g_game.getMap()->waypoints.getWaypointByName(popString(L)))
		pushPosition(L, waypoint->pos, 0);
	else
		lua_pushboolean(L, LUA_ERROR);

	return 1;
}

int32_t LuaScriptInterface::luaDoWaypointAddTemporial(lua_State* L)
{
	//doWaypointAddTemporial(name, pos)
	PositionEx pos;
	popPosition(L, pos);

	g_game.getMap()->waypoints.addWaypoint(WaypointPtr(new Waypoint(popString(L), pos)));
	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaGetGameState(lua_State* L)
{
	//getGameState()
	lua_pushnumber(L, g_game.getGameState());
	return 1;
}

int32_t LuaScriptInterface::luaDoSetGameState(lua_State* L)
{
	//doSetGameState(id)
	uint32_t id = popNumber(L);
	if(id < GAME_STATE_FIRST || id > GAME_STATE_LAST)
	{
		lua_pushboolean(L, LUA_ERROR);
		reportErrorFunc("Invalid game state id.");
		return 1;
	}

	Dispatcher::getDispatcher().addTask(createTask(boost::bind(&Game::setGameState, &g_game, (GameState_t)id)));
	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaDoExecuteRaid(lua_State* L)
{
	//doExecuteRaid(name)
	Raid* raid = Raids::getInstance()->getRaidByName(popString(L));
	if(!raid || !raid->isLoaded())
	{
		reportErrorFunc("Such raid does not exists.");
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	if(Raids::getInstance()->getRunning())
	{
		lua_pushboolean(L, LUA_FALSE);
		return 1;
	}

	Raids::getInstance()->setRunning(raid);
	RaidEvent* event = raid->getNextRaidEvent();
	if(!event)
	{
		lua_pushboolean(L, LUA_FALSE);
		return 1;
	}

	raid->setState(RAIDSTATE_EXECUTING);
	uint32_t ticks = event->getDelay();
	if(ticks > 0)
		Scheduler::getScheduler().addEvent(createSchedulerTask(ticks,
			boost::bind(&Raid::executeRaidEvent, raid, event)));
	else
		Dispatcher::getDispatcher().addTask(createTask(
			boost::bind(&Raid::executeRaidEvent, raid, event)));

	lua_pushboolean(L, LUA_TRUE);
	return 1;
}

int32_t LuaScriptInterface::luaDoReloadInfo(lua_State* L)
{
	//doReloadInfo(id[, cid])
	uint32_t cid = 0;
	if(lua_gettop(L) >= 2)
		cid = popNumber(L);

	uint32_t id = popNumber(L);
	if(id < RELOAD_FIRST || id > RELOAD_LAST)
	{
		lua_pushboolean(L, LUA_ERROR);
		reportErrorFunc("Invalid reload info id.");
		return 1;
	}

	Scheduler::getScheduler().addEvent(createSchedulerTask(SCHEDULER_MINTICKS,
		boost::bind(&Game::reloadInfo, &g_game, (ReloadInfo_t)id, cid)));
	return 1;
}

int32_t LuaScriptInterface::luaDoSaveServer(lua_State* L)
{
	//doSaveServer()
	Dispatcher::getDispatcher().addTask(createTask(boost::bind(&Game::saveGameState, &g_game, true)));
	return 1;
}

int32_t LuaScriptInterface::luaDoCleanHouse(lua_State *L)
{
	//doCleanHouse(houseId)
	uint32_t houseId = popNumber(L);
	if(House* house = Houses::getInstance().getHouse(houseId))
	{
		house->clean();
		lua_pushboolean(L, LUA_NO_ERROR);
	}
	else
	{
		reportErrorFunc("House not found.");
		lua_pushboolean(L, LUA_ERROR);
	}
	return 1;
}

int32_t LuaScriptInterface::luaDoCleanMap(lua_State* L)
{
	//doCleanMap()
	uint32_t count = 0;
	g_game.cleanMap(count);
	lua_pushnumber(L, count);
	return 1;
}

int32_t LuaScriptInterface::luaDoRefreshMap(lua_State* L)
{
	//doRefreshMap()
	g_game.proceduralRefresh();
	return 1;
}

int32_t LuaScriptInterface::luaGetItemDescriptions(lua_State* L)
{
	ScriptEnviroment* env = getScriptEnv();
	Item* item = env->getItemByUID(popNumber(L));
	if(!item)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	lua_newtable(L);
	setField(L, "name", item->getName().c_str());
	setField(L, "article", item->getArticle().c_str());
	setField(L, "plural", item->getPluralName().c_str());
	setField(L, "text", item->getText().c_str());
	setField(L, "special", item->getSpecialDescription().c_str());
	setField(L, "writer", item->getWriter().c_str());
	setField(L, "date", item->getDate());
	return 1;
}

int32_t LuaScriptInterface::luaGetItemWeight(lua_State* L)
{
	//getItemWeight(itemid[, precise = TRUE])
	bool precise = true;
	if(lua_gettop(L) > 2)
		precise = popNumber(L) == LUA_TRUE;

	ScriptEnviroment* env = getScriptEnv();
	Item* item = env->getItemByUID(popNumber(L));
	if(!item)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	double weight = item->getWeight();
	if(precise)
	{
		std::stringstream ws;
		ws << std::fixed << std::setprecision(2) << weight;
		weight = atof(ws.str().c_str());
	}

	lua_pushnumber(L, weight);
	return 1;
}

int32_t LuaScriptInterface::luaSetItemName(lua_State* L)
{
	std::string name = popString(L);
	ScriptEnviroment* env = getScriptEnv();

	Item* item = env->getItemByUID(popNumber(L));
	if(!item)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	item->setName(name);
	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaSetItemPluralName(lua_State* L)
{
	std::string pluralName = popString(L);
	ScriptEnviroment* env = getScriptEnv();

	Item* item = env->getItemByUID(popNumber(L));
	if(!item)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	item->setPluralName(pluralName);
	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaSetItemArticle(lua_State* L)
{
	std::string article = popString(L);
	ScriptEnviroment* env = getScriptEnv();

	Item* item = env->getItemByUID(popNumber(L));
	if(!item)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	item->setArticle(article);
	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaGetItemAttack(lua_State* L)
{
	ScriptEnviroment* env = getScriptEnv();

	Item* item = env->getItemByUID(popNumber(L));
	if(!item)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	lua_pushnumber(L, item->getAttack());
	return 1;
}

int32_t LuaScriptInterface::luaSetItemAttack(lua_State* L)
{
	int32_t value = (int32_t)popNumber(L);
	ScriptEnviroment* env = getScriptEnv();

	Item* item = env->getItemByUID(popNumber(L));
	if(!item)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	item->setAttack(value);
	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaGetItemExtraAttack(lua_State* L)
{
	ScriptEnviroment* env = getScriptEnv();

	Item* item = env->getItemByUID(popNumber(L));
	if(!item)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	lua_pushnumber(L, item->getExtraAttack());
	return 1;
}

int32_t LuaScriptInterface::luaSetItemExtraAttack(lua_State* L)
{
	int32_t value = (int32_t)popNumber(L);
	ScriptEnviroment* env = getScriptEnv();

	Item* item = env->getItemByUID(popNumber(L));
	if(!item)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	item->setExtraAttack(value);
	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaGetItemDefense(lua_State* L)
{
	ScriptEnviroment* env = getScriptEnv();

	Item* item = env->getItemByUID(popNumber(L));
	if(!item)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	lua_pushnumber(L, item->getDefense());
	return 1;
}

int32_t LuaScriptInterface::luaSetItemDefense(lua_State* L)
{
	int32_t value = (int32_t)popNumber(L);
	ScriptEnviroment* env = getScriptEnv();

	Item* item = env->getItemByUID(popNumber(L));
	if(!item)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	item->setDefense(value);
	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaGetItemExtraDefense(lua_State* L)
{
	ScriptEnviroment* env = getScriptEnv();

	Item* item = env->getItemByUID(popNumber(L));
	if(!item)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	lua_pushnumber(L, item->getExtraDefense());
	return 1;

}

int32_t LuaScriptInterface::luaSetItemExtraDefense(lua_State* L)
{
	int32_t value = (int32_t)popNumber(L);
	ScriptEnviroment* env = getScriptEnv();

	Item* item = env->getItemByUID(popNumber(L));
	if(!item)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	item->setExtraDefense(value);
	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaGetItemArmor(lua_State* L)
{
	ScriptEnviroment* env = getScriptEnv();

	Item* item = env->getItemByUID(popNumber(L));
	if(!item)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	lua_pushnumber(L, item->getArmor());
	return 1;
}

int32_t LuaScriptInterface::luaSetItemArmor(lua_State* L)
{
	int32_t value = (int32_t)popNumber(L);
	ScriptEnviroment* env = getScriptEnv();

	Item* item = env->getItemByUID(popNumber(L));
	if(!item)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	item->setArmor(value);
	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaGetItemAttackSpeed(lua_State* L)
{
	ScriptEnviroment* env = getScriptEnv();

	Item* item = env->getItemByUID(popNumber(L));
	if(!item)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	lua_pushnumber(L, item->getAttackSpeed());
	return 1;
}

int32_t LuaScriptInterface::luaSetItemAttackSpeed(lua_State* L)
{
	int32_t value = (int32_t)popNumber(L);
	ScriptEnviroment* env = getScriptEnv();

	Item* item = env->getItemByUID(popNumber(L));
	if(!item)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	item->setAttackSpeed(value);
	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaGetItemHitChance(lua_State* L)
{
	ScriptEnviroment* env = getScriptEnv();

	Item* item = env->getItemByUID(popNumber(L));
	if(!item)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	lua_pushnumber(L, item->getHitChance());
	return 1;
}

int32_t LuaScriptInterface::luaSetItemHitChance(lua_State* L)
{
	int32_t value = (int32_t)popNumber(L);
	ScriptEnviroment* env = getScriptEnv();

	Item* item = env->getItemByUID(popNumber(L));
	if(!item)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	item->setHitChance(value);
	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaGetItemShootRange(lua_State* L)
{
	ScriptEnviroment* env = getScriptEnv();

	Item* item = env->getItemByUID(popNumber(L));
	if(!item)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	lua_pushnumber(L, item->getShootRange());
	return 1;
}

int32_t LuaScriptInterface::luaSetItemShootRange(lua_State* L)
{
	int32_t value = (int32_t)popNumber(L);
	ScriptEnviroment* env = getScriptEnv();

	Item* item = env->getItemByUID(popNumber(L));
	if(!item)
	{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	item->setShootRange(value);
	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaIsIpBanished(lua_State *L)
{
	//isIpBanished(ip[, mask])
	uint32_t mask = 0xFFFFFFFF;
	if(lua_gettop(L) > 1)
		mask = popNumber(L);

	lua_pushboolean(L, IOBan::getInstance()->isIpBanished(
		(uint32_t)popNumber(L), mask) ? LUA_TRUE : LUA_FALSE);
	return 1;
}

int32_t LuaScriptInterface::luaIsPlayerNamelocked(lua_State *L)
{
	//isPlayerNamelocked(name)
	lua_pushboolean(L, IOBan::getInstance()->isNamelocked(
		popString(L)) ? LUA_TRUE : LUA_FALSE);
	return 1;
}

int32_t LuaScriptInterface::luaIsAccountBanished(lua_State *L)
{
	//isAccountBanished(accId)
	lua_pushboolean(L, IOBan::getInstance()->isBanished(
		(uint32_t)popNumber(L)) ? LUA_TRUE : LUA_FALSE);
	return 1;
}

int32_t LuaScriptInterface::luaIsAccountDeleted(lua_State *L)
{
	//isAccountDeleted(accId)
	lua_pushboolean(L, IOBan::getInstance()->isDeleted(
		(uint32_t)popNumber(L)) ? LUA_TRUE : LUA_FALSE);
	return 1;
}

int32_t LuaScriptInterface::luaDoAddIpBanishment(lua_State *L)
{
	//doAddIpBanishment(ip[, mask[, length[, comment[, admin]]]])
	uint32_t admin = 0, mask = 0xFFFFFFFF, params = lua_gettop(L), length = g_config.getNumber(ConfigManager::IPBANISHMENT_LENGTH);
	std::string comment = "No comment.";
	if(params > 4)
		admin = popNumber(L);

	if(params > 3)
		comment = popString(L);

	if(params > 2)
		length = popNumber(L);

	if(params > 1)
		mask = popNumber(L);

	lua_pushboolean(L, IOBan::getInstance()->addIpBanishment(
		(uint32_t)popNumber(L), (time(NULL) + length), comment, admin, mask) ? LUA_NO_ERROR : LUA_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaDoAddNamelock(lua_State *L)
{
	//doAddNamelock(name[, reason[, action[, comment[, admin[, statement]]]]])
	uint32_t admin = 0, action = 0, reason = 0, params = lua_gettop(L);
	std::string statement, comment = "No comment.";
	if(params > 5)
		statement = popString(L);

	if(params > 4)
		admin = popNumber(L);

	if(params > 3)
		comment = popString(L);

	if(params > 2)
		action = popNumber(L);

	if(params > 1)
		reason = popNumber(L);

	lua_pushboolean(L, IOBan::getInstance()->addNamelock(
		popString(L), reason, action, comment, admin, statement) ? LUA_NO_ERROR : LUA_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaDoAddBanishment(lua_State *L)
{
	//doAddBanishment(accId[, length[, reason[, action[, comment[, admin[, statement]]]]]])
	uint32_t admin = 0, action = 0, reason = 0, params = lua_gettop(L), length = g_config.getNumber(ConfigManager::BAN_LENGTH);
	std::string statement, comment = "No comment.";
	if(params > 6)
		statement = popString(L);

	if(params > 5)
		admin = popNumber(L);

	if(params > 4)
		comment = popString(L);

	if(params > 3)
		action = popNumber(L);

	if(params > 2)
		reason = popNumber(L);

	if(params > 1)
		length = popNumber(L);

	lua_pushboolean(L, IOBan::getInstance()->addBanishment(
		(uint32_t)popNumber(L), (time(NULL) + length), reason, action, comment, admin, statement) ? LUA_NO_ERROR : LUA_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaDoAddDeletion(lua_State *L)
{
	//doAddDeletion(accId[, reason[, action[, comment[, admin[, statement]]]]]])
	uint32_t admin = 0, action = 0, reason = 0, params = lua_gettop(L);
	std::string statement, comment = "No comment.";
	if(params > 5)
		statement = popString(L);

	if(params > 4)
		admin = popNumber(L);

	if(params > 3)
		comment = popString(L);

	if(params > 2)
		action = popNumber(L);

	if(params > 1)
		reason = popNumber(L);

	lua_pushboolean(L, IOBan::getInstance()->addDeletion(
		(uint32_t)popNumber(L), reason, action, comment, admin, statement) ? LUA_NO_ERROR : LUA_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaDoAddNotation(lua_State *L)
{
	//doAddNotation(accId[, reason[, action[, comment[, admin[, statement]]]]]])
	uint32_t admin = 0, action = 0, reason = 0, params = lua_gettop(L);
	std::string statement, comment = "No comment.";
	if(params > 5)
		statement = popString(L);

	if(params > 4)
		admin = popNumber(L);

	if(params > 3)
		comment = popString(L);

	if(params > 2)
		action = popNumber(L);

	if(params > 1)
		reason = popNumber(L);

	IOBan::getInstance()->addNotation(
		(uint32_t)popNumber(L), reason, action, comment, admin, statement);
	lua_pushboolean(L, LUA_NO_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaDoRemoveIpBanishment(lua_State *L)
{
	//doRemoveIpBanishment(ip[, mask])
	uint32_t mask = 0xFFFFFFFF;
	if(lua_gettop(L) > 1)
		mask = popNumber(L);

	lua_pushboolean(L, IOBan::getInstance()->removeIpBanishment(
		(uint32_t)popNumber(L), mask) ? LUA_TRUE : LUA_FALSE);
	return 1;
}

int32_t LuaScriptInterface::luaDoRemoveNamelock(lua_State *L)
{
	//doRemoveNamelock(name)
	lua_pushboolean(L, IOBan::getInstance()->removeNamelock(
		popString(L)) ? LUA_TRUE : LUA_FALSE);
	return 1;
}

int32_t LuaScriptInterface::luaDoRemoveBanishment(lua_State *L)
{
	//doRemoveBanisment(accId)
	lua_pushboolean(L, IOBan::getInstance()->removeBanishment(
		(uint32_t)popNumber(L)) ? LUA_TRUE : LUA_FALSE);
	return 1;
}

int32_t LuaScriptInterface::luaDoRemoveDeletion(lua_State *L)
{
	//doRemoveDeletion(accId)
	lua_pushboolean(L, IOBan::getInstance()->removeDeletion(
		(uint32_t)popNumber(L)) ? LUA_TRUE : LUA_FALSE);
	return 1;
}

int32_t LuaScriptInterface::luaDoRemoveNotations(lua_State *L)
{
	//doRemoveNotations(accId)
	IOBan::getInstance()->removeNotations((uint32_t)popNumber(L));
	lua_pushboolean(L, LUA_TRUE);
	return 1;
}

int32_t LuaScriptInterface::luaGetNotationsCount(lua_State *L)
{
	//getNotationsCount(accId)
	lua_pushnumber(L, IOBan::getInstance()->getNotationsCount(
		(uint32_t)popNumber(L)));
	return 1;
}

int32_t LuaScriptInterface::luaGetBanData(lua_State *L)
{
	//getBanData(value)
	Ban tmp;
	if(!IOBan::getInstance()->getData((uint32_t)popNumber(L), tmp))
	{
		lua_pushboolean(L, LUA_ERROR);
		return 1;
	}

	lua_newtable(L);
	setField(L, "id", tmp.id);
	setField(L, "type", tmp.type);
	setField(L, "value", tmp.value);
	setField(L, "param", tmp.param);
	setField(L, "added", tmp.added);
	setField(L, "expires", tmp.expires);
	setField(L, "adminId", tmp.adminid);
	setField(L, "reason", tmp.reason);
	setField(L, "action", tmp.action);
	setField(L, "comment", tmp.comment);
	setField(L, "statement", tmp.statement);
	return 1;
}

int32_t LuaScriptInterface::luaGetBanReason(lua_State *L)
{
	//getBanReason(id)
	lua_pushstring(L, getReason((ViolationActions_t)popNumber(L)).c_str());
	return 1;
}

int32_t LuaScriptInterface::luaGetBanAction(lua_State *L)
{
	//getBanAction(id)
	bool ipBanishment = false;
	if(lua_gettop(L) > 1)
		ipBanishment = popNumber(L) == LUA_TRUE;

	lua_pushstring(L, getAction(popNumber(L), ipBanishment).c_str());
	return 1;
}

int32_t LuaScriptInterface::luaGetBanList(lua_State *L)
{
	//getBanList(type[, value])
	uint32_t value = 0;
	if(lua_gettop(L) >= 2)
		value = popNumber(L);

	BansVec bans = IOBan::getInstance()->getList((BanType_t)popNumber(L), value);
	BansVec::const_iterator it = bans.begin();

	lua_newtable(L);
	for(uint32_t i = 1; it != bans.end(); ++it, ++i)
	{
		lua_pushnumber(L, i);
		lua_newtable(L);

		setField(L, "id", (*it).id);
		setField(L, "type", (*it).type);
		setField(L, "value", (*it).value);
		setField(L, "param", (*it).param);
		setField(L, "added", (*it).added);
		setField(L, "expires", (*it).expires);
		setField(L, "adminId", (*it).adminid);
		setField(L, "reason", (*it).reason);
		setField(L, "action", (*it).action);
		setField(L, "comment", (*it).comment);
		setField(L, "statement", (*it).statement);
		lua_settable(L, -3);
	}

	return 1;
}

int32_t LuaScriptInterface::luaGetExperienceStage(lua_State* L)
{
	//getExperienceStage(level)
	lua_pushnumber(L, g_game.getExperienceStage(popNumber(L)));
	return 1;
}

int32_t LuaScriptInterface::luaGetDataDir(lua_State* L)
{
	//getDataDir()
	lua_pushstring(L, getFilePath(FILE_TYPE_OTHER, "").c_str());
	return 1;
}

int32_t LuaScriptInterface::luaGetLogsDir(lua_State* L)
{
	//getLogsDir()
	lua_pushstring(L, getFilePath(FILE_TYPE_LOG, "").c_str());
	return 1;
}

int32_t LuaScriptInterface::luaGetConfigFile(lua_State* L)
{
	//getConfigFile()
	lua_pushstring(L, g_config.getString(ConfigManager::CONFIG_FILE).c_str());
	return 1;
}

int32_t LuaScriptInterface::luaGetConfigValue(lua_State* L)
{
	//getConfigValue(key)
	g_config.getValue(popString(L), L);
	return 1;
}

int32_t LuaScriptInterface::luaDatabaseExecute(lua_State *L)
{
	//db.executeQuery(query)
	//executes the query on the database
	//Same as Database::getInstance()->executeQuery(query) on C++
	//returns true if worked, false otherwise, plus prints the SQL error on the console
	lua_pushboolean(L, (Database::getInstance()->executeQuery(popString(L)) ? LUA_TRUE : LUA_FALSE));
	return 1;
}

int32_t LuaScriptInterface::luaDatabaseStoreQuery(lua_State *L)
{
	//db.storeQuery(query)
	//executes the query on the database and stores its result
	//mainly used for SELECT queries
	//Returns an integer; The internal ID of the stored result
	//Returns LUA_ERROR if the result could not be stored, plus prints the SQL error on the console
	ScriptEnviroment* env = getScriptEnv();
	if(DBResult* res = Database::getInstance()->storeQuery(popString(L)))
	{
		lua_pushnumber(L, env->addResult(res));
		return 1;
	}

	lua_pushboolean(L, LUA_ERROR);
	return 1;
}

int32_t LuaScriptInterface::luaDatabaseEscapeString(lua_State *L)
{
	//db.escapeString(str)
	//Returns the escaped string ready to use on SQL queries
	lua_pushstring(L, Database::getInstance()->escapeString(popString(L)).c_str());
	return 1;
}

int32_t LuaScriptInterface::luaDatabaseEscapeBlob(lua_State *L)
{
	//db.escapeBlob(s, length)
	//returns the escaped BLOB string to use on SQL queries
	//Used for conditions/item's attributes (un)serialization
	uint32_t length = popNumber(L);
	lua_pushstring(L, Database::getInstance()->escapeBlob(popString(L), length).c_str());
	return 1;
}

int32_t LuaScriptInterface::luaDatabaseStringComparisonOperator(lua_State *L)
{
	//db.stringComparisonOperator()
	lua_pushstring(L, Database::getInstance()->getStringComparisonOperator().c_str());
	return 1;
}

#define CHECK_RESULT() \
	if(!res) \
	{ \
		reportErrorFunc("Result not found."); \
		lua_pushboolean(L, LUA_ERROR); \
		return 1; \
	}
int32_t LuaScriptInterface::luaResultGetDataInt(lua_State *L)
{
	//result.getDataInt(res, s)
	//returns an integer
	const std::string& s = popString(L);

	ScriptEnviroment* env = getScriptEnv();
	DBResult* res = env->getResult(popNumber(L));
	CHECK_RESULT()

	lua_pushnumber(L, res->getDataInt(s));
	return 1;
}

int32_t LuaScriptInterface::luaResultGetDataLong(lua_State *L)
{
	//result.getDataLong(res, s)
	//returns a long
	const std::string& s = popString(L);

	ScriptEnviroment* env = getScriptEnv();
	DBResult* res = env->getResult(popNumber(L));
	CHECK_RESULT()

	lua_pushnumber(L, res->getDataLong(s));
	return 1;
}

int32_t LuaScriptInterface::luaResultGetDataString(lua_State *L)
{
	//result.getDataString(res, s)
	//returns a string
	const std::string& s = popString(L);

	ScriptEnviroment* env = getScriptEnv();
	DBResult* res = env->getResult(popNumber(L));
	CHECK_RESULT()

	lua_pushstring(L, res->getDataString(s).c_str());
	return 1;
}

int32_t LuaScriptInterface::luaResultGetDataStream(lua_State *L)
{
	//result.getDataStream(res, s)
	//Returns a BLOB string and the length of it
	const std::string s = popString(L);

	ScriptEnviroment* env = getScriptEnv();
	DBResult* res = env->getResult(popNumber(L));
	CHECK_RESULT()

	uint64_t length = 0;
	lua_pushstring(L, res->getDataStream(s, length));
	lua_pushnumber(L, length);
	return 2;
}

int32_t LuaScriptInterface::luaResultNext(lua_State *L)
{
	//result.next(res)
	//Goes to the next result of the stored result
	//Returns true if the next result exists, false otherwise
	ScriptEnviroment* env = getScriptEnv();
	DBResult* res = env->getResult(popNumber(L));
	CHECK_RESULT()

	lua_pushboolean(L, (res->next() ? LUA_TRUE : LUA_FALSE));
	return 1;
}

int32_t LuaScriptInterface::luaResultFree(lua_State *L)
{
	//result.free(res)
	//Frees the memory of the result, and removes it from the server
	uint32_t rid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();
	DBResult* res = env->getResult(rid);
	CHECK_RESULT()

	bool ret = env->removeResult(rid);
	res->free();

	lua_pushboolean(L, (ret ? LUA_TRUE : LUA_FALSE));
	return 1;
}
#undef CHECK_RESULT

int32_t LuaScriptInterface::luaBitNot(lua_State *L)
{
	int32_t number = (int32_t)popNumber(L);
	lua_pushnumber(L, ~number);
	return 1;
}

int32_t LuaScriptInterface::luaBitUNot(lua_State *L)
{
	uint32_t number = (uint32_t)popNumber(L);
	lua_pushnumber(L, ~number);
	return 1;
}

#define MULTIOP(type, name, op) \
	int32_t LuaScriptInterface::luaBit##name(lua_State* L) \
	{ \
		int32_t n = lua_gettop(L); \
		type w = (type)popNumber(L); \
		for(int32_t i = 2; i <= n; ++i) \
			w op popNumber(L); \
		lua_pushnumber(L, w); \
		return 1; \
	}

MULTIOP(int32_t, And, &=)
MULTIOP(int32_t, Or, |=)
MULTIOP(int32_t, Xor, ^=)
MULTIOP(uint32_t, UAnd, &=)
MULTIOP(uint32_t, UOr, |=)
MULTIOP(uint32_t, UXor, ^=)

#define SHIFTOP(type, name, op) \
	int32_t LuaScriptInterface::luaBit##name(lua_State* L) \
	{ \
		type n2 = (type)popNumber(L), n1 = (type)popNumber(L); \
		lua_pushnumber(L, (n1 op n2)); \
		return 1; \
	}

SHIFTOP(int32_t, LeftShift, <<)
SHIFTOP(int32_t, RightShift, >>)
SHIFTOP(uint32_t, ULeftShift, <<)
SHIFTOP(uint32_t, URightShift, >>)
