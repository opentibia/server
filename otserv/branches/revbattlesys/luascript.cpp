//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// class which takes care of all data which must get saved in the player
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


#include <string>
#include <iostream>
#include <sstream>

#include "luascript.h"
#include "player.h"
#include "item.h"
#include "game.h"
#include "house.h"
#include "housetile.h"
#include "status.h"
#include "combat.h"
#include "ioplayer.h"

extern Game g_game;

LuaScript::LuaScript()
{
	luaState = NULL;
}


LuaScript::~LuaScript()
{
	if(luaState)
		lua_close(luaState);
}


int LuaScript::openFile(const char *filename)
{
	luaState = lua_open();

	if(luaL_dofile(luaState, filename))
		return false;

  return true;
}


std::string LuaScript::getGlobalString(std::string var, const std::string& defString)
{
	lua_getglobal(luaState, var.c_str());

	if(!lua_isstring(luaState, -1))
		return defString;

	int len = (int)lua_strlen(luaState, -1);
	std::string ret(lua_tostring(luaState, -1), len);
	lua_pop(luaState,1);

	return ret;
}

int LuaScript::getGlobalNumber(std::string var, const int defNum)
{
	lua_getglobal(luaState, var.c_str());

	if(!lua_isnumber(luaState, -1))
  	  return defNum;

	int val = (int)lua_tonumber(luaState, -1);
	lua_pop(luaState,1);

	return val;
}


int LuaScript::setGlobalString(std::string var, std::string val)
{
	return false;
}

int LuaScript::setGlobalNumber(std::string var, int val){
	lua_pushnumber(luaState, val);
	lua_setglobal(luaState, var.c_str());
	return true;
}

std::string LuaScript::getGlobalStringField (std::string var, const int key, const std::string& defString)
{
	lua_getglobal(luaState, var.c_str());

	lua_pushnumber(luaState, key);
	lua_gettable(luaState, -2);  /* get table[key] */
	if(!lua_isstring(luaState, -1))
		return defString;
	
	std::string result = lua_tostring(luaState, -1);
	lua_pop(luaState, 2);  /* remove number and key*/
	return result;
}

int LuaScript::getField (const char *key)
{
	int result;
	lua_pushstring(luaState, key);
	lua_gettable(luaState, -2);  /* get table[key] */
	result = (int)lua_tonumber(luaState, -1);
	lua_pop(luaState, 1);  /* remove number and key*/
	return result;
}

void LuaScript::setField (const char *index, int val)
{
	lua_pushstring(luaState, index);
	lua_pushnumber(luaState, (double)val);
	lua_settable(luaState, -3);
}


int LuaScript::getField (lua_State *L , const char *key)
{
	int result;
	lua_pushstring(L, key);
	lua_gettable(L, -2);  /* get table[key] */
	result = (int)lua_tonumber(L, -1);
	lua_pop(L, 1);  /* remove number and key*/
	return result;
}

void LuaScript::setField (lua_State *L, const char *index, int val)
{
	lua_pushstring(L, index);
	lua_pushnumber(L, (double)val);
	lua_settable(L, -3);
}

ScriptEnviroment::ThingMap ScriptEnviroment::m_globalMap;
ScriptEnviroment::AreaMap ScriptEnviroment::m_areaMap;
ScriptEnviroment::CombatMap ScriptEnviroment::m_combatMap;
ScriptEnviroment::StorageMap ScriptEnviroment::m_globalStorageMap;
/*
ScriptEnviroment::CombatHealthMap ScriptEnviroment::m_combatHealthMap;
ScriptEnviroment::CombatManaMap ScriptEnviroment::m_combatManaMap;
ScriptEnviroment::CombatConditionMap ScriptEnviroment::m_combatConditionMap;
*/

ScriptEnviroment::ScriptEnviroment()
{
	resetEnv();
	m_lastUID = 70000;
	m_lastAreaId = 0;
	m_lastCombatId = 0;
}

ScriptEnviroment::~ScriptEnviroment()
{
	//
}

void ScriptEnviroment::resetEnv()
{
	m_scriptId = 0;
	m_interface = NULL;
	m_localMap.clear();

	for(AreaMap::iterator it = m_areaMap.begin(); it != m_areaMap.end(); ++it){
		delete it->second;
	}
	m_areaMap.clear();

	/*
	for(CombatHealthMap::iterator it = m_combatHealthMap.begin(); it != m_combatHealthMap.end(); ++it){
		delete it->second;
	}
	m_combatHealthMap.clear();

	for(CombatManaMap::iterator it = m_combatManaMap.begin(); it != m_combatManaMap.end(); ++it){
		delete it->second;
	}
	m_combatManaMap.clear();

	for(CombatConditionMap::iterator it = m_combatConditionMap.begin(); it != m_combatConditionMap.end(); ++it){
		delete it->second;
	}
	m_combatConditionMap.clear();
	*/

	for(CombatMap::iterator it = m_combatMap.begin(); it != m_combatMap.end(); ++it){
		delete it->second;
	}
	m_combatMap.clear();

	m_realPos.x = 0;
	m_realPos.y = 0;
	m_realPos.z = 0;
}


void ScriptEnviroment::setRealPos(const Position& realPos)
{
	m_realPos = realPos;	
}

Position ScriptEnviroment::getRealPos()
{
	return m_realPos;
}
	
void ScriptEnviroment::setScriptId(long scriptId, LuaScriptInterface* scriptInterface)
{
	resetEnv();
	m_scriptId = scriptId;
	m_interface = scriptInterface;
}

void ScriptEnviroment::setEventDesc(const std::string& desc)
{
	m_eventdesc = desc;
}
	
void ScriptEnviroment::getEventInfo(long& scriptId, std::string& desc, LuaScriptInterface*& scriptInterface)
{
	scriptId = m_scriptId;
	desc = m_eventdesc;
	scriptInterface = m_interface;
}

void ScriptEnviroment::addUniqueThing(Thing* thing)
{
	Item* item = thing->getItem();
	if(item && item->getUniqueId() != 0 ){
		unsigned short uid = item->getUniqueId();
		
		Thing* tmp = m_globalMap[uid];
		if(!tmp){
			m_globalMap[uid] = thing;
		}
		else{
			std::cout << "Duplicate uniqueId " <<  uid << std::endl;
		}
	}
}

long ScriptEnviroment::addThing(Thing* thing)
{
	if(!thing)
		return 0;
	
	ThingMap::iterator it;
	for(it = m_localMap.begin(); it != m_localMap.end(); ++it){
		if(it->second == thing){
			return it->first;
		}
	}
	
	long newUid;
	if(Creature* creature = thing->getCreature()){
		newUid = creature->getID();
	}
	else{
		++m_lastUID;
		if(m_lastUID < 70000)
			m_lastUID = 70000;
		
		while(m_localMap[m_lastUID]){
			++m_lastUID;
		}
		newUid = m_lastUID;
	}
	
	m_localMap[newUid] = thing;
	return newUid;
}
	
Thing* ScriptEnviroment::getThingByUID(long uid)
{
	Thing* tmp = m_localMap[uid];
	if(tmp && !tmp->isRemoved()){
		return tmp;
	}
	tmp = m_globalMap[uid];
	if(tmp && !tmp->isRemoved()){
		return tmp;
	}
	if(uid >= 0x10000000){ //is a creature id
		tmp = g_game.getCreatureByID(uid);
		if(tmp && !tmp->isRemoved()){
			m_localMap[uid] = tmp;
			return tmp;
		}
	}
	return NULL;
}

Item* ScriptEnviroment::getItemByUID(long uid)
{
	Thing* tmp = getThingByUID(uid);
	if(tmp){
		if(Item* item = tmp->getItem())
			return item;
	}
	return NULL;
}

Container* ScriptEnviroment::getContainerByUID(long uid)
{
	Item* tmp = getItemByUID(uid);
	if(tmp){
		if(Container* container = tmp->getContainer())
			return container;
	}
	return NULL;
}

Creature* ScriptEnviroment::getCreatureByUID(long uid)
{
	Thing* tmp = getThingByUID(uid);
	if(tmp){
		if(Creature* creature = tmp->getCreature())
			return creature;
	}
	return NULL;
}

Player* ScriptEnviroment::getPlayerByUID(long uid)
{
	Thing* tmp = getThingByUID(uid);
	if(tmp){
		if(Creature* creature = tmp->getCreature())
			if(Player* player = creature->getPlayer())
				return player;
	}
	return NULL;
}

uint32_t ScriptEnviroment::addCombatArea(AreaCombat* area)
{
	uint32_t newAreaId = m_lastAreaId + 1;
	m_areaMap[newAreaId] = area;
	
	m_lastAreaId++;
	return newAreaId;
}

AreaCombat* ScriptEnviroment::getCombatArea(uint32_t areaId) const
{
	AreaMap::const_iterator it = m_areaMap.find(areaId);
	if(it != m_areaMap.end()){
		return it->second;
	}

	return NULL;
}

uint32_t ScriptEnviroment::addCombatObject(Combat* combat)
{
	uint32_t newCombatId = m_lastCombatId + 1;
	m_combatMap[newCombatId] = combat;
	
	m_lastCombatId++;
	return newCombatId;
}

Combat* ScriptEnviroment::getCombatObject(uint32_t combatId) const
{
	CombatMap::iterator it = m_combatMap.find(combatId);
	if(it != m_combatMap.end()){
		return it->second;
	}

	return NULL;
}

void ScriptEnviroment::addGlobalStorageValue(const unsigned long key, const long value)
{
	m_globalStorageMap[key] = value;
}

bool ScriptEnviroment::getGlobalStorageValue(const unsigned long key, long &value) const
{
	StorageMap::const_iterator it;
	it = m_globalStorageMap.find(key);
	if(it != m_globalStorageMap.end()){
		value = it->second;
		return true;
	}
	else{
		value = 0;
		return false;
	}
}

std::string LuaScriptInterface::getErrorDesc(ErrorCode_t code){
	switch(code){
	case LUA_ERROR_PLAYER_NOT_FOUND:
		return "Player not found";
		break;
	case LUA_ERROR_CREATURE_NOT_FOUND:
		return "Creature not found";
		break;
	case LUA_ERROR_ITEM_NOT_FOUND:
		return "Item not found";
		break;
	case LUA_ERROR_THING_NOT_FOUND:
		return "Thing not found";
		break;
	case LUA_ERROR_TILE_NOT_FOUND:
		return "Tile not found";
		break;
	case LUA_ERROR_HOUSE_NOT_FOUND:
		return "House not found";
		break;
	case LUA_ERROR_COMBAT_NOT_FOUND:
		return "Combat not found";
		break;
	case LUA_ERROR_CONTAINER_NOT_FOUND:
		return "Container not found";
		break;
	default:
		return "Wrong error code!!";
		break;
	};
}

ScriptEnviroment LuaScriptInterface::m_scriptEnv;

LuaScriptInterface::LuaScriptInterface(std::string interfaceName)
{
	m_luaState = NULL;
	m_interfaceName = interfaceName;
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

long LuaScriptInterface::getLastLuaError()
{
	return m_lastLuaError;
}

void LuaScriptInterface::dumpLuaStack()
{
	int a = lua_gettop(m_luaState);
	std::cout <<  "stack size: " << a << std::endl;
	for(int i = 1; i <= a ; i++){
		std::cout << lua_typename(m_luaState, lua_type(m_luaState,-i)) << " " << lua_topointer(m_luaState, -i) << std::endl;
	}
}

lua_State* LuaScriptInterface::getLuaState()
{
	return m_luaState;
}

long LuaScriptInterface::loadFile(const std::string& file)
{
	/*
	//search if we have loaded that file before
	ScriptsCache::iterator it;
	for(it = m_cacheFiles.begin(); it != m_cacheFiles.end(); ++it){
		if(it->second == file){
			return it->first;
		}
	}
	*/
	
	//loads file as a chunk at stack top
	int ret = luaL_loadfile(m_luaState, file.c_str());
	if(ret != 0){
		m_lastLuaError = ret;
		return -1;
	}
	//check that it is loaded as a function
	if(lua_isfunction(m_luaState, -1) == 0){
		return -1;
	}
	//execute it
	ret = lua_pcall(m_luaState, 0, 0, 0);
	if(ret != 0){
		return -1;
	}
	
	/*++m_runningFileId;
	m_cacheFiles[m_runningFileId] = file;
	return m_runningFileId;
	*/
	m_loadingFile = file;
	return 0;
}

long LuaScriptInterface::getEvent(const std::string& eventName)
{
	//get our events table
	lua_getfield(m_luaState, LUA_REGISTRYINDEX, "EVENTS");
	if(lua_istable(m_luaState, -1) == 0){
		lua_pop(m_luaState, 1);
		return -1;
	}
	//get current event function pointer
	lua_getglobal(m_luaState, eventName.c_str());
	if(lua_isfunction(m_luaState, -1) == 0){
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

const std::string& LuaScriptInterface::getFileById(long scriptId)
{
	static std::string unk = "(Unknow script file)";
	ScriptsCache::iterator it = m_cacheFiles.find(scriptId);
	if(it != m_cacheFiles.end()){
		return it->second;
	}
	else{
		return unk;
	}
}
	
ScriptEnviroment* LuaScriptInterface::getScriptEnv()
{
	return &m_scriptEnv;
}

void LuaScriptInterface::reportError(const char* function, const std::string& error_desc)
{
	ScriptEnviroment* env = getScriptEnv();
	long fileId;
	std::string event_desc;
	LuaScriptInterface* scriptInterface;
	env->getEventInfo(fileId, event_desc, scriptInterface);
	
	std::cout << std::endl << "Lua Script Error: ";
	if(scriptInterface){
		std::cout << "[" << scriptInterface->getInterfaceName() << "] " << std::endl;
		std::cout << scriptInterface->getFileById(fileId) << std::endl;
	}
	std::cout << event_desc << std::endl;
	if(function)
		std::cout << function << "(). ";
	std::cout << error_desc << std::endl;
}

std::string LuaScriptInterface::getInterfaceName()
{
	return m_interfaceName;
}

bool LuaScriptInterface::pushFunction(long functionId)
{
	lua_getfield(m_luaState, LUA_REGISTRYINDEX, "EVENTS");
	if(lua_istable(m_luaState, -1) == 0){
		return false;
	}
	lua_pushnumber(m_luaState, functionId);
	lua_rawget(m_luaState, -2);
	lua_remove(m_luaState, -2);
	if(lua_isfunction(m_luaState, -1) == 0){
		return false;
	}
	return true;
}

bool LuaScriptInterface::initState()
{
	m_luaState = luaL_newstate();
	if(!m_luaState){
		return false;
	}
	luaopen_base(m_luaState);
	luaopen_table(m_luaState);
	luaopen_os(m_luaState);
	luaopen_string(m_luaState);
	luaopen_math(m_luaState);
	
	registerFunctions();
	
	lua_newtable(m_luaState);
	lua_setfield(m_luaState, LUA_REGISTRYINDEX, "EVENTS");
	
	m_runningEventId = 1000;
	return true;
}

bool LuaScriptInterface::closeState()
{
	m_cacheFiles.clear();
	lua_close(m_luaState);
	return true;
}

bool LuaScriptInterface::callFunction(long nParams, long &result)
{
	bool ret;
	int size0 = lua_gettop(m_luaState);
	if(lua_pcall(m_luaState, nParams, 1, 0) != 0){
		LuaScriptInterface::reportError(NULL, std::string(LuaScriptInterface::popString(m_luaState)));
		ret =  false;
	}
	else{
		result = LuaScriptInterface::popNumber(m_luaState);
		ret =  true;
	}
	if((lua_gettop(m_luaState) + nParams  + 1) != size0){
		LuaScriptInterface::reportError(NULL, "Stack size changed!");
	}
	return ret;
}

bool LuaScriptInterface::callFunction(long nParams)
{
	long a;
	return callFunction(nParams, a);
}

void LuaScriptInterface::pushThing(lua_State *L, Thing* thing, long thingid)
{
	lua_newtable(L);
	if(thing && thing->getItem()){	
		const Item* item = thing->getItem();
		setField(L, "uid", thingid);
		setField(L, "itemid", item->getID());

		if(item->hasSubType())
			setField(L, "type", item->getItemCountOrSubtype());
		else
			setField(L, "type", 0);

		setField(L, "actionid", item->getActionId());
	}
	else if(thing && thing->getCreature()){
		const Creature* creature = thing->getCreature();
		setField(L, "uid", thingid);
		setField(L, "itemid", 1);
		char type;
		if(creature->getPlayer()){
			type = 1;
		}
		else if(creature->getMonster()){
			type = 2;
		}
		else{//npc
			type = 3;
		}	
		setField(L, "type", type);
		setField(L, "actionid", 0);
	}	
	else{
		setField(L, "uid", 0);
		setField(L, "itemid", 0);
		setField(L, "type", 0);
		setField(L, "actionid", 0);
	}
}

void LuaScriptInterface::pushPosition(lua_State *L, const Position& position, long stackpos)
{
	lua_newtable(L);
	setField(L, "z", position.z);
	setField(L, "y", position.y);
	setField(L, "x", position.x);
	setField(L, "stackpos", stackpos);
}

void LuaScriptInterface::popPosition(lua_State *L, Position& position, long& stackpos)
{
	position.z = getField(L, "z");
	position.y = getField(L, "y");
	position.x = getField(L, "x");
	stackpos = getField(L, "stackpos");
	lua_pop(L, 1); //table
}

long LuaScriptInterface::popNumber(lua_State *L)
{
	lua_pop(L,1);
	return (unsigned long)lua_tonumber(L, 0);
}

const char* LuaScriptInterface::popString(lua_State *L)
{
	lua_pop(L,1);
	return lua_tostring(L, 0);
}
	
long LuaScriptInterface::getField(lua_State *L, const char *key)
{
	int result;
	lua_pushstring(L, key);
	lua_gettable(L, -2);  // get table[key]
	result = (int)lua_tonumber(L, -1);
	lua_pop(L, 1);  // remove number and key
	return result;
}

void LuaScriptInterface::setField(lua_State *L, const char *index, long val)
{
	lua_pushstring(L, index);
	lua_pushnumber(L, (double)val);
	lua_settable(L, -3);
}
	
void LuaScriptInterface::registerFunctions()
{
	//lua_register(L, "name", C_function);
	
	//getPlayerFood(uid)
	lua_register(m_luaState, "getPlayerFood", LuaScriptInterface::luaGetPlayerFood);
	//getPlayerHealth(uid)	
	lua_register(m_luaState, "getPlayerHealth", LuaScriptInterface::luaGetPlayerHealth);
	//getPlayerMana(uid)
	lua_register(m_luaState, "getPlayerMana", LuaScriptInterface::luaGetPlayerMana);
	//getPlayerLevel(uid)
	lua_register(m_luaState, "getPlayerLevel", LuaScriptInterface::luaGetPlayerLevel);
	//getPlayerMagLevel(uid)
	lua_register(m_luaState, "getPlayerMagLevel", LuaScriptInterface::luaGetPlayerMagLevel);
	//getPlayerName(uid)	
	lua_register(m_luaState, "getPlayerName", LuaScriptInterface::luaGetPlayerName);
	//getPlayerAccess(uid)	
	lua_register(m_luaState, "getPlayerAccess", LuaScriptInterface::luaGetPlayerAccess);
	//getPlayerPosition(uid)
	lua_register(m_luaState, "getPlayerPosition", LuaScriptInterface::luaGetPlayerPosition);
	//getPlayerSkill(uid,skillid)
	lua_register(m_luaState, "getPlayerSkill", LuaScriptInterface::luaGetPlayerSkill);
	//getPlayerMasterPos(cid)
	lua_register(m_luaState, "getPlayerMasterPos", LuaScriptInterface::luaGetPlayerMasterPos);
	//getPlayerVocation(cid)
	lua_register(m_luaState, "getPlayerVocation", LuaScriptInterface::luaGetPlayerVocation);
	//getPlayerItemCount(cid,itemid)
	lua_register(m_luaState, "getPlayerItemCount", LuaScriptInterface::luaGetPlayerItemCount);
	//getPlayerSoul(cid)
	lua_register(m_luaState, "getPlayerSoul", LuaScriptInterface::luaGetPlayerSoul);
	//getPlayerFreeCap(cid)
	lua_register(m_luaState, "getPlayerFreeCap", LuaScriptInterface::luaGetPlayerFreeCap);
	//getPlayerLight(cid)
	lua_register(m_luaState, "getPlayerLight", LuaScriptInterface::luaGetPlayerLight);
	//getPlayerSlotItem(cid, slot)
	lua_register(m_luaState, "getPlayerSlotItem", LuaScriptInterface::luaGetPlayerSlotItem);
	//getPlayerDepotItems(uid, depotid)
	lua_register(m_luaState, "getPlayerDepotItems", LuaScriptInterface::luaGetPlayerDepotItems);	
	//getPlayerGuildId(cid)
	lua_register(m_luaState, "getPlayerGuildId", LuaScriptInterface::luaGetPlayerGuildId);
	//getPlayerGuildName(cid)
	lua_register(m_luaState, "getPlayerGuildName", LuaScriptInterface::luaGetPlayerGuildName);
	//getPlayerGuildRank(cid)
	lua_register(m_luaState, "getPlayerGuildRank", LuaScriptInterface::luaGetPlayerGuildRank);
	//getPlayerGuildNick(cid)
	lua_register(m_luaState, "getPlayerGuildNick", LuaScriptInterface::luaGetPlayerGuildNick);
	
	//getPlayerStorageValue(uid,valueid)
	lua_register(m_luaState, "getPlayerStorageValue", LuaScriptInterface::luaGetPlayerStorageValue);
	//setPlayerStorageValue(uid,valueid, newvalue)
	lua_register(m_luaState, "setPlayerStorageValue", LuaScriptInterface::luaSetPlayerStorageValue);
	
	//getGlobalStorageValue(valueid)
	lua_register(m_luaState, "getGlobalStorageValue", LuaScriptInterface::luaGetGlobalStorageValue);
	//setGlobalStorageValue(valueid, newvalue)
	lua_register(m_luaState, "setGlobalStorageValue", LuaScriptInterface::luaSetGlobalStorageValue);
	
	//getTilePzInfo(pos) 1 is pz. 0 no pz.
	lua_register(m_luaState, "getTilePzInfo", LuaScriptInterface::luaGetTilePzInfo);
	//getTileHouseInfo(pos). 0 no house. != 0 house id
	lua_register(m_luaState, "getTileHouseInfo", LuaScriptInterface::luaGetTileHouseInfo);
	
	//getItemRWInfo(uid)
	lua_register(m_luaState, "getItemRWInfo", LuaScriptInterface::luaGetItemRWInfo);
	//getThingfromPos(pos)
	lua_register(m_luaState, "getThingfromPos", LuaScriptInterface::luaGetThingfromPos);
	//getThing(uid)
	lua_register(m_luaState, "getThing", LuaScriptInterface::luaGetThing);
	//getThingPos(uid)
	lua_register(m_luaState, "getThingPos", LuaScriptInterface::luaGetThingPos);
	
	//doRemoveItem(uid,n)
	lua_register(m_luaState, "doRemoveItem", LuaScriptInterface::luaDoRemoveItem);
	//doPlayerFeed(uid,food)
	lua_register(m_luaState, "doPlayerFeed", LuaScriptInterface::luaDoFeedPlayer);	
	//doPlayerSendCancel(uid,text)
	lua_register(m_luaState, "doPlayerSendCancel", LuaScriptInterface::luaDoSendCancel);
	//doTeleportThing(uid,newpos)
	lua_register(m_luaState, "doTeleportThing", LuaScriptInterface::luaDoTeleportThing);
	//doTransformItem(uid,toitemid)	
	lua_register(m_luaState, "doTransformItem", LuaScriptInterface::luaDoTransformItem);
	//doPlayerSay(uid,text,type)
	lua_register(m_luaState, "doPlayerSay", LuaScriptInterface::luaDoPlayerSay);
	//doSendMagicEffect(position,type)
	lua_register(m_luaState, "doSendMagicEffect", LuaScriptInterface::luaDoSendMagicEffect);
	//doChangeTypeItem(uid,new_type)	
	lua_register(m_luaState, "doChangeTypeItem", LuaScriptInterface::luaDoChangeTypeItem);
	//doSetItemActionId(uid,actionid)
	lua_register(m_luaState, "doSetItemActionId", LuaScriptInterface::luaDoSetItemActionId);
	//doSetItemText(uid,text)
	lua_register(m_luaState, "doSetItemText", LuaScriptInterface::luaDoSetItemText);
	//doSetItemSpecialDescription(uid,desc)
	lua_register(m_luaState, "doSetItemSpecialDescription", LuaScriptInterface::luaDoSetItemSpecialDescription);
	//doSendAnimatedText(position,text,color)
	lua_register(m_luaState, "doSendAnimatedText", LuaScriptInterface::luaDoSendAnimatedText);
	//doPlayerAddSkillTry(cid,skillid,n)
	lua_register(m_luaState, "doPlayerAddSkillTry", LuaScriptInterface::luaDoPlayerAddSkillTry);
	//doPlayerAddHealth(cid,health)
	lua_register(m_luaState, "doPlayerAddHealth", LuaScriptInterface::luaDoPlayerAddHealth);
	//doCreatureAddHealth(cid,health)
	lua_register(m_luaState, "doCreatureAddHealth", LuaScriptInterface::luaDoPlayerAddHealth);
	//doPlayerAddMana(cid,mana)
	lua_register(m_luaState, "doPlayerAddMana", LuaScriptInterface::luaDoPlayerAddMana);
	//doPlayerAddSoul(cid,soul)
	lua_register(m_luaState, "doPlayerAddSoul", LuaScriptInterface::luaDoPlayerAddSoul);
	//doPlayerAddItem(cid,itemid,count or type) . returns uid of the created item
	lua_register(m_luaState, "doPlayerAddItem", LuaScriptInterface::luaDoPlayerAddItem);
	//doPlayerSendTextMessage(cid,MessageClasses,message)
	lua_register(m_luaState, "doPlayerSendTextMessage", LuaScriptInterface::luaDoPlayerSendTextMessage);
	//doPlayerRemoveMoney(cid,money)
	lua_register(m_luaState, "doPlayerRemoveMoney", LuaScriptInterface::luaDoPlayerRemoveMoney);
	//doShowTextWindow(cid,maxlen,canWrite)	
	lua_register(m_luaState, "doShowTextWindow", LuaScriptInterface::luaDoShowTextWindow);	
	//doDecayItem(uid)
	lua_register(m_luaState, "doDecayItem", LuaScriptInterface::luaDoDecayItem);
	//doCreateItem(itemid,type or count,position) .only working on ground. returns uid of the created item
	lua_register(m_luaState, "doCreateItem", LuaScriptInterface::luaDoCreateItem);
	//doSummonCreature(name, position)
	lua_register(m_luaState, "doSummonCreature", LuaScriptInterface::luaDoSummonCreature);
	//doMoveCreature(cid, direction)
	lua_register(m_luaState, "doMoveCreature", LuaScriptInterface::luaDoMoveCreature);
	//doPlayerSetMasterPos(cid,pos)
	lua_register(m_luaState, "doPlayerSetMasterPos", LuaScriptInterface::luaDoPlayerSetMasterPos);
	//doPlayerSetVocation(cid,voc)
	lua_register(m_luaState, "doPlayerSetVocation", LuaScriptInterface::luaDoPlayerSetVocation);
	//doPlayerRemoveItem(cid,itemid,count)
	lua_register(m_luaState, "doPlayerRemoveItem", LuaScriptInterface::luaDoPlayerRemoveItem);
	//doPlayerAddExp(cid,exp)
	lua_register(m_luaState, "doPlayerAddExp", LuaScriptInterface::luaDoPlayerAddExp);
	//doPlayerSetGuildId(cid, id)
	//lua_register(m_luaState, "doPlayerSetGuildId", LuaScriptInterface::luaDoPlayerSetGuildId);
	//doPlayerSetGuildRank(cid, rank)
	lua_register(m_luaState, "doPlayerSetGuildRank", LuaScriptInterface::luaDoPlayerSetGuildRank);
	//doPlayerSetGuildNick(cid, nick)
	lua_register(m_luaState, "doPlayerSetGuildNick", LuaScriptInterface::luaDoPlayerSetGuildNick);
	
	//isPlayer(cid)
	lua_register(m_luaState, "isPlayer", LuaScriptInterface::luaIsPlayer);
	//isContainer(uid)
	lua_register(m_luaState, "isContainer", LuaScriptInterface::luaIsContainer);

	//getPlayerByName(name)
	lua_register(m_luaState, "getPlayerByName", LuaScriptInterface::luaGetPlayerByName);
	
	//getContainerSize(uid)
	lua_register(m_luaState, "getContainerSize", LuaScriptInterface::luaGetContainerSize);
	//getContainerCap(uid)
	lua_register(m_luaState, "getContainerCap", LuaScriptInterface::luaGetContainerCap);
	//getContainerItem(uid, slot)
	lua_register(m_luaState, "getContainerItem", LuaScriptInterface::luaGetContainerItem);
	//doAddContainerItem(uid, itemid, count or subtype)
	lua_register(m_luaState, "doAddContainerItem", LuaScriptInterface::luaDoAddContainerItem);
	
	//getHouseOwner(houseid)
	lua_register(m_luaState, "getHouseOwner", LuaScriptInterface::luaGetHouseOwner);
	//setHouseOwner(houseid, owner)
	lua_register(m_luaState, "setHouseOwner", LuaScriptInterface::luaSetHouseOwner);
	
	//getWorldType()
	lua_register(m_luaState, "getWorldType", LuaScriptInterface::luaGetWorldType);
	//getWorldTime()
	lua_register(m_luaState, "getWorldTime", LuaScriptInterface::luaGetWorldTime);
	//getWorldLight()
	lua_register(m_luaState, "getWorldLight", LuaScriptInterface::luaGetWorldLight);
	//getWorldCreatures(type) 0 players, 1 monsters, 2 npcs, 3 all
	lua_register(m_luaState, "getWorldCreatures", LuaScriptInterface::luaGetWorldCreatures);
	//getWorldUpTime()
	lua_register(m_luaState, "getWorldUpTime", LuaScriptInterface::luaGetWorldUpTime);
	//getGuildId(guild_name)
	lua_register(m_luaState, "getGuildId", LuaScriptInterface::luaGetGuildId);
	
	//createCombatArea( { {...}, {...} } )
	lua_register(m_luaState, "createCombatArea", LuaScriptInterface::luaCreateCombatArea);

	//setCombatParam(combat, key, value)
	lua_register(m_luaState, "setCombatParam", LuaScriptInterface::luaSetCombatParam);

	//createCombatHealthObject()
	lua_register(m_luaState, "createCombatHealthObject", LuaScriptInterface::luaCreateCombatHealthObject);

	//doAreaCombatHealth(cid, area, combat, pos, min, max)
	lua_register(m_luaState, "doAreaCombatHealth", LuaScriptInterface::luaDoAreaCombatHealth);

	//doTargetCombatHealth(cid, combat, target, min, max)
	lua_register(m_luaState, "doTargetCombatHealth", LuaScriptInterface::luaDoTargetCombatHealth);

	//createCombatManaObject()
	lua_register(m_luaState, "createCombatManaObject", LuaScriptInterface::luaCreateCombatManaObject);

	//setCombatManaParam(type, value)
	//lua_register(m_luaState, "setCombatHealthParam", LuaScriptInterface::luaSetCombatManaParam);

	//doAreaCombatMana(cid, area, combat, pos, min, max)
	lua_register(m_luaState, "doAreaCombatMana", LuaScriptInterface::luaDoAreaCombatMana);

	//doTargetCombatMana(cid, combat, target, min, max)
	lua_register(m_luaState, "doTargetCombatMana", LuaScriptInterface::luaDoTargetCombatMana);

	//debugPrint(text)
	lua_register(m_luaState, "debugPrint", LuaScriptInterface::luaDebugPrint);
	
}


int LuaScriptInterface::internalGetPlayerInfo(lua_State *L, PlayerInfo_t info)
{
	unsigned int cid = (unsigned int)popNumber(L);
	ScriptEnviroment* env = getScriptEnv();
	int value;
	
	const Player* player = env->getPlayerByUID(cid);
	if(player){
		const Tile *tile;
		Position pos;
		long stackpos;
		switch(info){
		case PlayerInfoAccess:
			value = player->getAccessLevel();
			break;		
		case PlayerInfoLevel:
			value = player->level;
			break;
		case PlayerInfoMagLevel:
			value = player->magLevel;
			break;
		case PlayerInfoMana:
			value = player->mana;
			break;
		case PlayerInfoHealth:
			value = player->health;
			break;
		case PlayerInfoName:
			lua_pushstring(L, player->name.c_str());
			return 1;
			break;
		case PlayerInfoPosition:			
			pos = player->getPosition();
			tile = player->getTile();
			if(tile){
				stackpos = player->getParent()->__getIndexOfThing(player);
			}
			else{
				stackpos = 0;
			}
			pushPosition(L, pos, stackpos);
			return 1;
			break;
		case PlayerInfoMasterPos:
			pos = player->masterPos;
			pushPosition(L, pos, 0);
			return 1;
			break;
		case PlayerInfoFood:
			value = player->food/1000;
			break;
		case PlayerInfoVocation:
			value = player->getVocationId();
			break;
		case PlayerInfoSoul:
			value = player->getPlayerInfo(PLAYERINFO_SOUL);
			break;
		case PlayerInfoFreeCap:
			value = (int)player->getFreeCapacity();
			break;
		case PlayerInfoGuildId:
			value = player->getGuildId();
			break;
		case PlayerInfoGuildName:
			lua_pushstring(L, player->getGuildName().c_str());
			return 1;
			break;
		case PlayerInfoGuildRank:
			lua_pushstring(L, player->getGuildRank().c_str());
			return 1;
			break;
		case PlayerInfoGuildNick:
			lua_pushstring(L, player->getGuildNick().c_str());
			return 1;
			break;
		default:
			std::string error_str = "Unknown player info. info = " + info;
			reportErrorFunc(error_str);
			value = 0;
			break;		
		}
		lua_pushnumber(L,value);
		return 1;
	}
	else{
		lua_pushnumber(L, -1);
		std::stringstream error_str;
		error_str << "Player not found. info = " << info;
		reportErrorFunc(error_str.str());
		return 1;
	}		
	
	lua_pushnumber(L, 0);
	return 1;
}
//getPlayer[Info](uid)
int LuaScriptInterface::luaGetPlayerFood(lua_State *L){	
	return internalGetPlayerInfo(L,PlayerInfoFood);}
	
int LuaScriptInterface::luaGetPlayerAccess(lua_State *L){
	return internalGetPlayerInfo(L,PlayerInfoAccess);}
	
int LuaScriptInterface::luaGetPlayerLevel(lua_State *L){
	return internalGetPlayerInfo(L,PlayerInfoLevel);}
	
int LuaScriptInterface::luaGetPlayerMagLevel(lua_State *L){
	return internalGetPlayerInfo(L,PlayerInfoMagLevel);}
	
int LuaScriptInterface::luaGetPlayerMana(lua_State *L){
	return internalGetPlayerInfo(L,PlayerInfoMana);}

int LuaScriptInterface::luaGetPlayerHealth(lua_State *L){
	return internalGetPlayerInfo(L,PlayerInfoHealth);}
	
int LuaScriptInterface::luaGetPlayerName(lua_State *L){
	return internalGetPlayerInfo(L,PlayerInfoName);}
	
int LuaScriptInterface::luaGetPlayerPosition(lua_State *L){	
	return internalGetPlayerInfo(L,PlayerInfoPosition);}
	
int LuaScriptInterface::luaGetPlayerVocation(lua_State *L){
	return internalGetPlayerInfo(L,PlayerInfoVocation);}

int LuaScriptInterface::luaGetPlayerMasterPos(lua_State *L){
	return internalGetPlayerInfo(L,PlayerInfoMasterPos);}

int LuaScriptInterface::luaGetPlayerSoul(lua_State *L){
	return internalGetPlayerInfo(L,PlayerInfoSoul);}

int LuaScriptInterface::luaGetPlayerFreeCap(lua_State *L){
	return internalGetPlayerInfo(L,PlayerInfoFreeCap);}
	
int LuaScriptInterface::luaGetPlayerGuildId(lua_State *L){
	return internalGetPlayerInfo(L,PlayerInfoGuildId);}
	
int LuaScriptInterface::luaGetPlayerGuildName(lua_State *L){
	return internalGetPlayerInfo(L,PlayerInfoGuildName);}
	
int LuaScriptInterface::luaGetPlayerGuildRank(lua_State *L){
	return internalGetPlayerInfo(L,PlayerInfoGuildRank);}
	
int LuaScriptInterface::luaGetPlayerGuildNick(lua_State *L){
	return internalGetPlayerInfo(L,PlayerInfoGuildNick);}

//


int LuaScriptInterface::luaDoRemoveItem(lua_State *L)
{	
	//doRemoveItem(uid,n)
	char n = (unsigned char)popNumber(L);	
	unsigned short itemid = (unsigned short)popNumber(L);
						
	ScriptEnviroment* env = getScriptEnv();
	
	Item* item = env->getItemByUID(itemid);
	if(item){
		g_game.internalRemoveItem(item, n);
		
		lua_pushnumber(L, 0);
		return 1;
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushnumber(L, -1);
		return 1;
	}
}

int LuaScriptInterface::luaDoPlayerRemoveItem(lua_State *L)
{
	//doPlayerRemoveItem(cid,itemid,count)
	long count = (unsigned char)popNumber(L);
	unsigned short itemId = (unsigned short)popNumber(L);
	unsigned int cid = (unsigned int)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	
	Player* player = env->getPlayerByUID(cid);
	if(player){
		if(g_game.removeItemOfType(player, itemId, count)){
			lua_pushnumber(L, 1);
		}
		else{
			lua_pushnumber(L, 0);
		}
	}
	else{
		lua_pushnumber(L, -1);
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		return 1;
	}	

	return 1;
}

int LuaScriptInterface::luaDoFeedPlayer(lua_State *L)
{	
	//doFeedPlayer(uid,food)
	int food = (int)popNumber(L);
	unsigned int cid = (unsigned int)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	
	Player* player = env->getPlayerByUID(cid);
	if(player){
		player->food += food*1000;
	}
	else{
		lua_pushnumber(L, -1);
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		return 1;
	}
	
	lua_pushnumber(L, 0);
	return 1;
}

int LuaScriptInterface::luaDoSendCancel(lua_State *L)
{	
	//doSendCancel(uid,text)
	const char * text = popString(L);
	unsigned int cid = (unsigned int)popNumber(L);	
	
	ScriptEnviroment* env = getScriptEnv();
	
	const Player* player = env->getPlayerByUID(cid);
	if(player){
		player->sendCancel(text);
	}
	else{
		lua_pushnumber(L, -1);
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		return 1;
	}		
	lua_pushnumber(L, 0);
	return 1;
}


int LuaScriptInterface::luaDoTeleportThing(lua_State *L)
{
	//doTeleportThing(uid,newpos)
	Position pos;
	long stackpos;
	popPosition(L, pos, stackpos);
	unsigned int id = (unsigned int)popNumber(L);	
	
	ScriptEnviroment* env = getScriptEnv();
	
	Thing* tmp = env->getThingByUID(id);
	if(!tmp){
		lua_pushnumber(L, -1);
		reportErrorFunc(getErrorDesc(LUA_ERROR_THING_NOT_FOUND));
		return 1;
	}
	
	if(g_game.internalTeleport(tmp,(Position&)pos) == RET_NOERROR){
		lua_pushnumber(L, 0);
		return 1;
	}
	
	reportErrorFunc("Item is not moveable");
	lua_pushnumber(L, -1);
	return 1;
}

	
int LuaScriptInterface::luaDoTransformItem(lua_State *L)
{
	//doTransformItem(uid,toitemid)	
	unsigned int toId = (unsigned int)popNumber(L);	
	unsigned int itemid = (unsigned int)popNumber(L);	
	
	ScriptEnviroment* env = getScriptEnv();
	
	Item* item = env->getItemByUID(itemid);
	if(item){
		g_game.transformItem(item, toId);
		
		lua_pushnumber(L, 0);
		return 1;
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushnumber(L, -1);
		return 1;
	}
}

int LuaScriptInterface::luaDoPlayerSay(lua_State *L)
{
	//doPlayerSay(uid,text,type)
	int type = (int)popNumber(L);	
	const char * text = popString(L);
	unsigned int cid = (unsigned int)popNumber(L);
					
	ScriptEnviroment* env = getScriptEnv();
	
	Player* player = env->getPlayerByUID(cid);
	if(player){
		g_game.internalCreatureSay(player,(SpeakClasses)type,std::string(text));
	}
	else{
		lua_pushnumber(L, -1);
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		return 1;
	}		
		
	lua_pushnumber(L, 0);
	return 1;
}

int LuaScriptInterface::luaDoSendMagicEffect(lua_State *L)
{
	//doSendMagicEffect(position,type)
	int type = (int)popNumber(L);	
	Position pos;
	long stackpos;
	popPosition(L, pos, stackpos);
	
	ScriptEnviroment* env = getScriptEnv();
	
	SpectatorVec list;
	SpectatorVec::iterator it;

	if(pos.x == 0xFFFF){
		pos = env->getRealPos();
	}
	
	g_game.getSpectators(Range(pos, true), list);

	for(it = list.begin(); it != list.end(); ++it) {
		Player *p = (*it)->getPlayer();
		if(p)
			p->sendMagicEffect(pos, type);
	}	
	
	lua_pushnumber(L, 0);
	return 1;
}

int LuaScriptInterface::luaDoChangeTypeItem(lua_State *L)
{
	//doChangeTypeItem(uid,new_type)
	unsigned int subtype = (unsigned int)popNumber(L);	
	unsigned int itemid = (unsigned int)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	
	Item* item = env->getItemByUID(itemid);
	if(item){
		g_game.transformItem(item, item->getID(), subtype);
		
		lua_pushnumber(L, 0);
		return 1;		
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushnumber(L, -1);
		return 1;
	}
}


int LuaScriptInterface::luaDoPlayerAddSkillTry(lua_State *L)
{
	//doPlayerAddSkillTry(uid,skillid,n)
	int n = (int)popNumber(L);
	int skillid = (int)popNumber(L);
	unsigned int cid = (unsigned int)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	
	Player* player = env->getPlayerByUID(cid);
	if(player){
		player->addSkillTryInternal(n,skillid);
	}
	else{
		lua_pushnumber(L, -1);
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		return 1;
	}		
		
	lua_pushnumber(L, 0);
	return 1;
}


int LuaScriptInterface::luaDoPlayerAddHealth(lua_State *L)
{
	//doPlayerAddHealth(uid,health)
	//doCreatureAddHealth(uid,health)
	int addhealth = (int)popNumber(L);
	unsigned int cid = (unsigned int)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	
	Creature* creature = env->getCreatureByUID(cid);
	if(creature){
		int tmp = creature->health + addhealth;
		if(tmp <= 0){
			creature->health = 1;
		}
		else if(tmp > creature->healthMax){
			creature->health = creature->healthMax;
		}
		else{
			creature->health = tmp;
		}
		Player* player = creature->getPlayer();
		
		if(player){
			player->sendStats();
		}
		
		SpectatorVec list;
		SpectatorVec::iterator it;

		g_game.getSpectators(Range(creature->getPosition(), true), list);
		for(it = list.begin(); it != list.end(); ++it) {
			Player* p = (*it)->getPlayer();
			if(p)
				p->sendCreatureHealth(creature);
		}
	}
	else{
		lua_pushnumber(L, -1);
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		return 1;
	}		
		
	lua_pushnumber(L, 0);
	return 1;
}

int LuaScriptInterface::luaDoPlayerAddMana(lua_State *L)
{
	//doPlayerAddMana(uid,mana)
	int addmana = (int)popNumber(L);
	unsigned int cid = (unsigned int)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	
	Player* player = env->getPlayerByUID(cid);
	if(player){
		player->mana = std::min(player->manaMax,player->mana+addmana);
		player->sendStats();
	}
	else{
		lua_pushnumber(L, -1);
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		return 1;
	}		
		
	lua_pushnumber(L, 0);
	return 1;
}

int LuaScriptInterface::luaDoPlayerAddItem(lua_State *L)
{
	//doPlayerAddItem(uid,itemid,count or type)
	int type = (int)popNumber(L);
	int itemid = (int)popNumber(L);
	unsigned int cid = (unsigned int)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	unsigned int uid;
	Player* player = env->getPlayerByUID(cid);
	if(player){
		Item* newItem = Item::CreateItem(itemid, type);

		ReturnValue ret = g_game.internalAddItem(player, newItem);
		if(ret != RET_NOERROR){
			Tile* tile = player->getTile();
			ret = g_game.internalAddItem(tile, newItem);
			if(ret != RET_NOERROR){
				delete newItem;

				lua_pushnumber(L, -1);
				reportErrorFunc("Could not add item");
				return 1;
			}
		}
		
		if(newItem->getParent()){
			uid = env->addThing((Thing*)newItem);
		}
		else{
			//stackable item stacked with existing object, newItem will be released
			lua_pushnumber(L, -1);
			return 1;
		}
	}
	else{
		lua_pushnumber(L, -1);
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		return 1;
	}		

	lua_pushnumber(L, uid);
	return 1;
}


int LuaScriptInterface::luaDoPlayerSendTextMessage(lua_State *L)
{	
	//doPlayerSendTextMessage(uid,MessageClasses,message)
	const char * text = popString(L);
	unsigned char messageClass = (unsigned char)popNumber(L);
	unsigned int cid = (unsigned int)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	
	Player* player = env->getPlayerByUID(cid);
	if(player){
		player->sendTextMessage((MessageClasses)messageClass,text);
	}
	else{
		lua_pushnumber(L, -1);
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		return 1;
	}		
	lua_pushnumber(L, 0);
	return 1;
}

int LuaScriptInterface::luaDoSendAnimatedText(lua_State *L)
{	
	//doSendAnimatedText(position,text,color)
	int color = (int)popNumber(L);
	const char * text = popString(L);
	Position pos;
	long stackpos;
	popPosition(L, pos, stackpos);
	
	ScriptEnviroment* env = getScriptEnv();
	
	SpectatorVec list;
	SpectatorVec::iterator it;

	if(pos.x == 0xFFFF){
		pos = env->getRealPos();
	}
	
	g_game.getSpectators(Range(pos, true), list);

	for(it = list.begin(); it != list.end(); ++it) {
		Player *p = (*it)->getPlayer();
		if(p)
			p->sendAnimatedText(pos, color, text);
	}
	
	lua_pushnumber(L, 0);
	return 1;
}

int LuaScriptInterface::luaGetPlayerSkill(lua_State *L)
{
	//getPlayerSkill(uid,skillid)
	unsigned char skillid = (unsigned int)popNumber(L);
	unsigned int cid = (unsigned int)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	
	Player* player = env->getPlayerByUID(cid);
	if(player){
		if(skillid > 6){
			lua_pushnumber(L, -1);
			reportErrorFunc("No valid skillId");
			return 1;
		}

		int value = player->skills[skillid][SKILL_LEVEL];
		lua_pushnumber(L,value);
		return 1;
	}
	else{
		lua_pushnumber(L, -1);
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		return 1;
	}
}

int LuaScriptInterface::luaDoShowTextWindow(lua_State *L)
{
	//doShowTextWindow(uid,maxlen,canWrite)
	//TODO
	/*bool canWrite = */(popNumber(L) != 0);
	/*unsigned short maxlen = */(unsigned short)popNumber(L);
	/*unsigned int uid = */(unsigned int)popNumber(L);
	/*
	ScriptEnviroment* env = getScriptEnv();
	
	Item* item = env->getItemByUID(uid);
	if(item){
		env->_player->sendTextWindow(item, maxlen, canWrite);
		
		lua_pushnumber(L, 0);
		return 1;
	}
	else{
#ifdef __DEBUG__
		std::cout << "luadoShowTextWindow: Item not found" << std::endl;
#endif

		lua_pushnumber(L, -1);
		return 1;
	}
	*/
	lua_pushnumber(L, -1);
	return 1;
}

int LuaScriptInterface::luaGetItemRWInfo(lua_State *L)
{
	//getItemRWInfo(uid)
	unsigned int uid = (unsigned int)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	
	Item* item = env->getItemByUID(uid);
	if(item){
		int maxlen;
		lua_pushnumber(L, (int)(item->getRWInfo(maxlen)));
		return 1;
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushnumber(L, -1);
		return 1;
	}
}

int LuaScriptInterface::luaDoDecayItem(lua_State *L)
{
	//doDecayItem(uid)
	//Note: to stop decay set decayTo = 0 in items.xml
	unsigned int uid = (unsigned int)popNumber(L);	
	
	ScriptEnviroment* env = getScriptEnv();
	
	Item* item = env->getItemByUID(uid);
	if(item){
		g_game.startDecay(item);
		
		lua_pushnumber(L, 0);
		return 1;
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushnumber(L, -1);
		return 1;
	}
}

int LuaScriptInterface::luaGetThingfromPos(lua_State *L)
{
	//getThingfromPos(pos)
	//Note: 
	//	stackpos = 255. Get the top thing(item moveable or creature).
	//	stackpos = 254. Get MagicFieldtItem
	//	stackpos = 253. Get Creature
	
	Position pos;
	long stackpos;
	popPosition(L, pos, stackpos);
	
	ScriptEnviroment* env = getScriptEnv();
	
	Tile* tile = g_game.map->getTile(pos);	
	Thing *thing = NULL;
	
	if(tile){
		if(stackpos == 255){
			thing = tile->getTopCreature();
			
			if(thing == NULL){
				Item* item = tile->getTopDownItem();
				if(item && !item->isNotMoveable())
					thing = item;
			}
		}
		else if(stackpos == 254){
			//TODO
			//thing = tile->getFieldItem();
		}
		else if(stackpos == 253){
			thing = tile->getTopCreature();
		}
		else{
			thing = tile->__getThing(stackpos);
		}
		
		if(thing){
			unsigned int thingid = env->addThing(thing);
			pushThing(L, thing, thingid);
		}
		else{
			pushThing(L, NULL, 0);
		}
		return 1;
		
	}//if(tile)
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_TILE_NOT_FOUND));
		pushThing(L,NULL,0);
		return 1;
	}
}

int LuaScriptInterface::luaDoCreateItem(lua_State *L)
{
	//doCreateItem(itemid,type or count,position) only working on ground. returns uid of the created item
	Position pos;
	long stackpos;
	popPosition(L, pos, stackpos);
	int type = (int)popNumber(L);
	int itemid = (int)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	
	Tile* tile = g_game.map->getTile(pos);
	if(!tile){
		lua_pushnumber(L, 0);
		return 0;
	}
	
	Item* newItem = Item::CreateItem(itemid, type);
	
	ReturnValue ret = g_game.internalAddItem(tile, newItem);
	if(ret != RET_NOERROR){
		delete newItem;
		lua_pushnumber(L, 0);
		return 0;
	}
	
	if(newItem->getParent()){
		unsigned int uid = env->addThing((Thing*)newItem);
		lua_pushnumber(L, uid);
		return 1;	
	}
	else{
		//stackable item stacked with existing object, newItem will be released
		lua_pushnumber(L, -1);
		return 1;
	}
}

int LuaScriptInterface::luaGetPlayerStorageValue(lua_State *L)
{
	//getPlayerStorageValue(cid,valueid)
	unsigned long key = (unsigned int)popNumber(L);
	unsigned int cid = (unsigned int)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	
	Player* player = env->getPlayerByUID(cid);
	if(player){
		long value;
		if(player->getStorageValue(key,value)){
			lua_pushnumber(L,value);
		}
		else{
			lua_pushnumber(L,-1);
		}
		return 1;
	}
	else{
		lua_pushnumber(L, -1);
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		return 1;
	}
}

int LuaScriptInterface::luaSetPlayerStorageValue(lua_State *L)
{
	//setPlayerStorageValue(cid,valueid, newvalue)
	long value = (unsigned int)popNumber(L);
	unsigned long key = (unsigned int)popNumber(L);
	unsigned int cid = (unsigned int)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	
	Player* player = env->getPlayerByUID(cid);
	if(player){
		player->addStorageValue(key,value);
	}
	else{
		lua_pushnumber(L, -1);
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));

		return 1;
	}
	lua_pushnumber(L,0);
	return 1;
}

int LuaScriptInterface::luaDoSetItemActionId(lua_State *L)
{
	//doSetItemActionId(uid,actionid)
	unsigned int actionid = (unsigned int)popNumber(L);	
	unsigned int itemid = (unsigned int)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	
	Item* item = env->getItemByUID(itemid);	
	if(item){
		item->setActionId(actionid);
		
		lua_pushnumber(L, 0);
		return 1;
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushnumber(L, -1);
		return 1;
	}
}

int LuaScriptInterface::luaDoSetItemText(lua_State *L)
{
	//doSetItemText(uid,text)
	const char *text = popString(L);
	unsigned int itemid = (unsigned int)popNumber(L);	
	
	ScriptEnviroment* env = getScriptEnv();
	
	Item* item = env->getItemByUID(itemid);
	if(item){
		std::string str(text);
		item->setText(str);
		
		lua_pushnumber(L, 0);
		return 1;
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushnumber(L, -1);
		return 1;
	}
}

int LuaScriptInterface::luaDoSetItemSpecialDescription(lua_State *L)
{
	//doSetItemSpecialDescription(uid,desc)
	const char *desc = popString(L);
	unsigned int itemid = (unsigned int)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	
	Item* item = env->getItemByUID(itemid);
	if(item){
		std::string str(desc);
		item->setSpecialDescription(str);
		
		lua_pushnumber(L, 0);
		return 1;
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_ITEM_NOT_FOUND));
		lua_pushnumber(L, -1);
		return 1;
	}
}

int LuaScriptInterface::luaGetTilePzInfo(lua_State *L)
{
	//getTilePzInfo(pos)
	Position pos;
	long stackpos;
	popPosition(L, pos, stackpos);
	
	//ScriptEnviroment* env = getScriptEnv();
	
	Tile *tile = g_game.map->getTile(pos);
	
	if(tile){
		if(tile->isPz()){
			lua_pushnumber(L, 1);
		}
		else{
			lua_pushnumber(L, 0);
		}
	}//if(tile)
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_TILE_NOT_FOUND));
		lua_pushnumber(L, -1);
	}
	return 1;
}

int LuaScriptInterface::luaGetTileHouseInfo(lua_State *L)
{
	//getTileHouseInfo(pos)
	Position pos;
	long stackpos;
	popPosition(L, pos, stackpos);
	
	//ScriptEnviroment* env = getScriptEnv();
	
	Tile *tile = g_game.map->getTile(pos);
	if(tile){
		if(HouseTile* houseTile = dynamic_cast<HouseTile*>(tile)){
			House* house = houseTile->getHouse();
			if(house){
				lua_pushnumber(L, house->getHouseId());
			}
			else{
				lua_pushnumber(L, 0);
			}
		}
		else{
			lua_pushnumber(L, 0);
		}
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_TILE_NOT_FOUND));
		lua_pushnumber(L, -1);
	}
	return 1;
}


int LuaScriptInterface::luaDoSummonCreature(lua_State *L){
	//doSummonCreature(name, position)
	Position pos;
	long stackpos;
	popPosition(L, pos, stackpos);
	const char *name = popString(L);
	
	ScriptEnviroment* env = getScriptEnv();
	
	Monster* monster = Monster::createMonster(name);

	if(!monster){
		lua_pushnumber(L, 0);
		std::string error_str = (std::string)"Monster name(" + name + (std::string)") not found";
		reportErrorFunc(error_str);
		return 1;
	}
	
	if(!g_game.placeCreature((Position&)pos, monster)){
		delete monster;
		lua_pushnumber(L, 0);
		std::string error_str = (std::string)"Can not summon monster: " + name;
		reportErrorFunc(error_str);
		return 1;
	}
	
	unsigned int cid = env->addThing((Thing*)monster);
	
	lua_pushnumber(L, cid);
	return 1;	
}


int LuaScriptInterface::luaDoPlayerRemoveMoney(lua_State *L)
{
	//doPlayerRemoveMoney(uid,money)
	int money = (int)popNumber(L);
	unsigned int cid = (unsigned int)popNumber(L);	
					
	ScriptEnviroment* env = getScriptEnv();
	
	Player* player = env->getPlayerByUID(cid);
	if(player){
		if(g_game.removeMoney(player, money)){
			lua_pushnumber(L, 1);
		}
		else{
			lua_pushnumber(L, 0);
		}
	}
	else{
		lua_pushnumber(L, -1);
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		return 1;
	}

	return 1;
}

int LuaScriptInterface::luaDoPlayerSetMasterPos(lua_State *L)
{
	//doPlayerSetMasterPos(cid,pos)
	Position pos;
	long stackpos;
	popPosition(L, pos, stackpos);
	unsigned int cid = (unsigned int)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	
	Player* player = env->getPlayerByUID(cid);
	if(player){
		player->masterPos = pos;
	}
	else{
		lua_pushnumber(L, -1);
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		return 1;
	}
	lua_pushnumber(L, 0);
	return 1;
}

int LuaScriptInterface::luaDoPlayerSetVocation(lua_State *L)
{
	//doPlayerSetVocation(cid,voc)
	int voc = (int)popNumber(L);
	unsigned int cid = (unsigned int)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	
	Player* player = env->getPlayerByUID(cid);
	if(player){
		player->setVocation(voc);
	}
	else{
		lua_pushnumber(L, -1);
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		return 1;
	}
	
	lua_pushnumber(L, 0);
	return 1;
}


int LuaScriptInterface::luaDebugPrint(lua_State *L)
{
	//debugPrint(text)
	const char * text = popString(L);
	reportErrorFunc(std::string(text));
	return 0;
}

int LuaScriptInterface::luaDoPlayerAddSoul(lua_State *L)
{
	//doPlayerAddSoul(cid,soul)
	/*int addsoul = */(int)popNumber(L);
	unsigned int cid = (unsigned int)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	
	Player* player = env->getPlayerByUID(cid);
	if(player){
		//TODO: add soul;
		player->sendStats();
	}
	else{
		lua_pushnumber(L, -1);
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		return 1;
	}		
		
	lua_pushnumber(L, 0);
	return 1;
}

int LuaScriptInterface::luaGetPlayerItemCount(lua_State *L)
{
	//getPlayerItemCount(cid,itemid)
	unsigned short itemId = (unsigned short)popNumber(L);
	unsigned int cid = (unsigned int)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	
	Player* player = env->getPlayerByUID(cid);
	if(player){
		long n = player->__getItemTypeCount(itemId);
		lua_pushnumber(L, n);
	}
	else{
		lua_pushnumber(L, -1);
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		return 1;
	}

	return 1;
}


int LuaScriptInterface::luaGetHouseOwner(lua_State *L)
{
	//getHouseOwner(houseid)
	int houseid = (int)popNumber(L);
	
	House* house = Houses::getInstance().getHouse(houseid);
	if(house){
		long owner = house->getHouseOwner();
		lua_pushnumber(L, owner);
	}
	else{
		lua_pushnumber(L, -1);
		reportErrorFunc(getErrorDesc(LUA_ERROR_HOUSE_NOT_FOUND));
	}
	
	return 1;
}
	
int LuaScriptInterface::luaSetHouseOwner(lua_State *L)
{
	//setHouseOwner(houseid, owner)
	int owner = (int)popNumber(L);
	int houseid = (int)popNumber(L);
	
	House* house = Houses::getInstance().getHouse(houseid);
	if(house){
		house->setHouseOwner(owner);
		lua_pushnumber(L, 1);
	}
	else{
		lua_pushnumber(L, -1);
		reportErrorFunc(getErrorDesc(LUA_ERROR_HOUSE_NOT_FOUND));
	}
	
	return 1;
}


int LuaScriptInterface::luaGetWorldType(lua_State *L)
{
	//getWorldType()
	switch(g_game.getWorldType()){
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
		lua_pushnumber(L, -1);
		break;
	}
	return 1;
}

int LuaScriptInterface::luaGetWorldTime(lua_State *L)
{
	//getWorldTime()
	long time = g_game.light_hour;
	lua_pushnumber(L, time);
	return 1;
}

int LuaScriptInterface::luaGetWorldLight(lua_State *L)
{
	//getWorldLight()
	long level = g_game.lightlevel;
	lua_pushnumber(L, level);
	lua_pushnumber(L, 0xD7);//color
	return 2;
}

int LuaScriptInterface::luaGetWorldCreatures(lua_State *L)
{
	//getWorldCreatures(type) 0 players, 1 monsters, 2 npcs, 3 all
	int type = (int)popNumber(L);
	int value;
	switch(type){
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
		lua_pushnumber(L, -1);
		reportErrorFunc("Wrong creature type.");
		return 1;
		break;
	}
	lua_pushnumber(L, value);
	return 1;
}

int LuaScriptInterface::luaGetWorldUpTime(lua_State *L)
{
	//getWorldUpTime()
	long uptime = 0;

	Status* status = Status::instance();
	if(status){
		uptime = (OTSYS_TIME() - status->start)/1000;
	}

	lua_pushnumber(L, uptime);
	return 1;
}

int LuaScriptInterface::luaGetPlayerLight(lua_State *L)
{
	//getPlayerLight(cid)
	unsigned int cid = (unsigned int)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player){
		LightInfo lightInfo;
		player->getCreatureLight(lightInfo);
		lua_pushnumber(L, lightInfo.level);
		lua_pushnumber(L, lightInfo.color);//color
	}
	else{
		lua_pushnumber(L, -1);
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
		return 1;
	}
	return 2;
}

int LuaScriptInterface::luaDoPlayerAddExp(lua_State *L)
{
	//doPlayerAddExp(cid,exp)
	int exp = (int)popNumber(L);
	unsigned int cid = (unsigned int)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player){
		if(exp > 0){
			player->addExperience(exp);
			lua_pushnumber(L, 1);
		}
		else{
			lua_pushnumber(L, 0);
		}
	}
	else{
		lua_pushnumber(L, -1);
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
	}
	return 1;
}

int LuaScriptInterface::luaGetPlayerSlotItem(lua_State *L)
{
	//getPlayerSlotItem(cid, slot)
	long slot = popNumber(L);
	unsigned long cid = (unsigned long)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player){
		Thing* thing = player->__getThing(slot);
		if(thing){
			long uid = env->addThing(thing);
			pushThing(L, thing, uid);
		}
		else{
			pushThing(L, NULL, 0);
		}
	}
	else{
		pushThing(L, NULL, 0);
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
	}
	return 1;
}

int LuaScriptInterface::luaGetThing(lua_State *L)
{
	//getThing(uid)
	unsigned long uid = (unsigned long)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	
	Thing* thing = env->getThingByUID(uid);
	if(thing){
		pushThing(L, thing, uid);
	}
	else{
		pushThing(L, NULL, 0);
		reportErrorFunc(getErrorDesc(LUA_ERROR_THING_NOT_FOUND));
	}
	
	return 1;
}

int LuaScriptInterface::luaGetThingPos(lua_State *L)
{
	//getThingPos(uid)
	unsigned long uid = (unsigned long)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	
	Thing* thing = env->getThingByUID(uid);
	Position pos(0,0,0);
	if(thing){
		pos = thing->getPosition();
	}
	else{
		reportErrorFunc(getErrorDesc(LUA_ERROR_THING_NOT_FOUND));
	}
	pushPosition(L, pos, 0);

	return 1;
}

int LuaScriptInterface::luaCreateCombatArea(lua_State *L)
{
	//createCombatArea( { {...}, {...} } )

	std::vector<uint8_t> row;
	uint32_t col = 0;

	AreaCombat* area = new AreaCombat;

	int i = 0, j = 0;
	lua_pushnil(L);  // first key //

	while(lua_next(L, -2) != 0){
		lua_pushnil(L);
		row.clear();
		while(lua_next(L, -2) != 0){
			row.push_back((unsigned char)lua_tonumber(L, -1));
			
			lua_pop(L, 1);  // removes `value'; keeps `key' for next iteration //
			j++;
		}

		area->setRow(col, row);
		++col;
		
		j = 0;
		lua_pop(L, 1);  // removes `value'; keeps `key' for next iteration //
		i++;
	}
	
	lua_pop(L, 1);

	ScriptEnviroment* env = getScriptEnv();
	uint32_t newAreaId = env->addCombatArea(area);

	lua_pushnumber(L, newAreaId);
	return 1;
}

int LuaScriptInterface::luaSetCombatParam(lua_State *L)
{
	//setCombatHealthParam(combat, key, value)
	return 0;
}

int LuaScriptInterface::luaCreateCombatHealthObject(lua_State *L)
{
	//createCombatHealthObject()

	ScriptEnviroment* env = getScriptEnv();

	CombatHealth* combat = new CombatHealth(DAMAGE_NONE, NM_ME_NONE);
	uint32_t newCombatId = env->addCombatObject(combat);

	lua_pushnumber(L, newCombatId);
	return 1;
}

int LuaScriptInterface::luaDoAreaCombatHealth(lua_State *L)
{
	//doAreaCombatHealth(cid, area, combat, pos, min, max)

	int32_t maxChange = (int32_t)popNumber(L);
	int32_t minChange = (int32_t)popNumber(L);

	Position pos;
	long stackpos;
	popPosition(L, pos, stackpos);

	uint32_t combatId = (int)popNumber(L);
	uint32_t areaId = (int)popNumber(L);
	uint32_t cid = (uint32_t)popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Creature* creature = NULL;
	
	if(cid != 0){
		creature = env->getCreatureByUID(cid);

		if(!creature){
			reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushnumber(L, -1);
			return 1;
		}
	}

	const Combat* combat = env->getCombatObject(combatId);

	if(!combat || !combat->getCombatHealth()){
		reportErrorFunc(getErrorDesc(LUA_ERROR_COMBAT_NOT_FOUND));
		lua_pushnumber(L, -1);
		return 1;
	}

	const CombatHealth* combatHealth = combat->getCombatHealth();

	const AreaCombat* area = env->getCombatArea(areaId);
	std::list<Tile*> list;
	area->getList(pos, creature->getDirection(), list);

	//TODO: filter out invalid tiles depending on access level/blocking projectile

	for(std::list<Tile*>::iterator it = list.begin(); it != list.end(); ++it){
		combatHealth->doCombat(creature, pos, minChange, maxChange);
	}

	lua_pushnumber(L, 0);
	return 1;
}

int LuaScriptInterface::luaDoTargetCombatHealth(lua_State *L)
{
	//doTargetCombatHealth(cid, combat, target, min, max)

	int32_t maxChange = (int32_t)popNumber(L);
	int32_t minChange = (int32_t)popNumber(L);

	uint32_t targetCid = (int)popNumber(L);

	uint32_t combatId = (int)popNumber(L);
	uint32_t cid = (uint32_t)popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Creature* creature = NULL;
	
	if(cid != 0){
		creature = env->getCreatureByUID(cid);

		if(!creature){
			reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushnumber(L, -1);
			return 1;
		}
	}

	Creature* target = env->getCreatureByUID(targetCid);
	if(!target){
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushnumber(L, -1);
		return 1;
	}

	const Combat* combat = env->getCombatObject(combatId);

	if(!combat || !combat->getCombatHealth()){
		reportErrorFunc(getErrorDesc(LUA_ERROR_COMBAT_NOT_FOUND));
		lua_pushnumber(L, -1);
		return 1;
	}

	const CombatHealth* combatHealth = combat->getCombatHealth();
	combatHealth->doCombat(creature, target, minChange, maxChange);

	lua_pushnumber(L, 0);
	return 1;
}


int LuaScriptInterface::luaCreateCombatManaObject(lua_State *L)
{
	//createCombatManaObject()

	ScriptEnviroment* env = getScriptEnv();

	CombatMana* combat = new CombatMana(NM_ME_NONE);
	uint32_t newCombatId = env->addCombatObject(combat);

	lua_pushnumber(L, newCombatId);
	return 1;
}

int LuaScriptInterface::luaDoAreaCombatMana(lua_State *L)
{
	//doAreaCombatMana(cid, area, combat, pos, min, max)

	int32_t maxChange = (int32_t)popNumber(L);
	int32_t minChange = (int32_t)popNumber(L);

	Position pos;
	long stackpos;
	popPosition(L, pos, stackpos);

	uint32_t combatId = (int)popNumber(L);
	uint32_t areaId = (int)popNumber(L);
	uint32_t cid = (uint32_t)popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Creature* creature = NULL;
	
	if(cid != 0){
		creature = env->getCreatureByUID(cid);

		if(!creature){
			reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushnumber(L, -1);
			return 1;
		}
	}

	const Combat* combat = env->getCombatObject(combatId);

	if(!combat || !combat->getCombatHealth()){
		reportErrorFunc(getErrorDesc(LUA_ERROR_COMBAT_NOT_FOUND));
		lua_pushnumber(L, -1);
		return 1;
	}

	const CombatMana* combatMana = combat->getCombatMana();

	const AreaCombat* area = env->getCombatArea(areaId);
	std::list<Tile*> list;
	area->getList(pos, creature->getDirection(), list);

	//TODO: filter out invalid tiles depending on access level/blocking projectile

	for(std::list<Tile*>::iterator it = list.begin(); it != list.end(); ++it){
		combatMana->doCombat(creature, pos, minChange, maxChange);
	}

	lua_pushnumber(L, 0);
	return 1;
}

int LuaScriptInterface::luaDoTargetCombatMana(lua_State *L)
{
	//doTargetCombatMana(cid, combat, target, min, max)

	int32_t maxChange = (int32_t)popNumber(L);
	int32_t minChange = (int32_t)popNumber(L);

	uint32_t targetCid = (int)popNumber(L);

	uint32_t combatId = (int)popNumber(L);
	uint32_t cid = (uint32_t)popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Creature* creature = NULL;
	
	if(cid != 0){
		creature = env->getCreatureByUID(cid);

		if(!creature){
			reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushnumber(L, -1);
			return 1;
		}
	}

	Creature* target = env->getCreatureByUID(targetCid);
	if(!target){
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushnumber(L, -1);
		return 1;
	}

	const Combat* combat = env->getCombatObject(combatId);

	if(!combat || !combat->getCombatMana()){
		reportErrorFunc(getErrorDesc(LUA_ERROR_COMBAT_NOT_FOUND));
		lua_pushnumber(L, -1);
		return 1;
	}

	const CombatMana* combatMana = combat->getCombatMana();
	combatMana->doCombat(creature, target, minChange, maxChange);

	lua_pushnumber(L, 0);
	return 1;
}

int LuaScriptInterface::luaDoAreaCombatCondition(lua_State *L)
{
	//doAreaCombatCondition(cid, combat, pos)

	Position pos;
	long stackpos;
	popPosition(L, pos, stackpos);

	uint32_t combatId = (int)popNumber(L);
	uint32_t cid = (uint32_t)popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Creature* creature = NULL;
	
	if(cid != 0){
		creature = env->getCreatureByUID(cid);

		if(!creature){
			reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushnumber(L, -1);
			return 1;
		}
	}

	const Combat* combat = env->getCombatObject(combatId);

	if(!combat || !combat->getCombatCondition()){
		reportErrorFunc(getErrorDesc(LUA_ERROR_COMBAT_NOT_FOUND));
		lua_pushnumber(L, -1);
		return 1;
	}

	//const CombatCondition* combatCondition = combat->getCombatCondition();
	//combatCondition->doCombat(creature, pos);

	lua_pushnumber(L, 0);
	return 1;
}

int LuaScriptInterface::luaGetGlobalStorageValue(lua_State *L)
{
	//getGlobalStorageValue(valueid)
	unsigned long key = (unsigned int)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	
	long value;
	if(env->getGlobalStorageValue(key, value)){
		lua_pushnumber(L, value);
	}
	else{
		lua_pushnumber(L, -1);
	}
	return 1;
}

int LuaScriptInterface::luaSetGlobalStorageValue(lua_State *L)
{
	//setGlobalStorageValue(valueid, newvalue)
	long value = (unsigned int)popNumber(L);
	unsigned long key = (unsigned int)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	env->addGlobalStorageValue(key,value);
	lua_pushnumber(L,0);
	return 1;
}

int LuaScriptInterface::luaGetPlayerDepotItems(lua_State *L)
{
	//getPlayerDepotItems(cid, depotid)	
	long depotid = popNumber(L);
	unsigned long cid = (unsigned long)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player){
		Depot* depot = player->getDepot(depotid, false);
		if(depot){
			lua_pushnumber(L, depot->getItemHoldingCount());
		}
		else{
			lua_pushnumber(L, -1);
			reportErrorFunc("Depot not found");
		}
	}
	else{
		lua_pushnumber(L, -1);
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
	}
	return 1;
}

int LuaScriptInterface::luaDoPlayerSetGuildRank(lua_State *L)
{
	//doPlayerSetGuildRank(cid, rank)
	const char* rank = popString(L);
	unsigned long cid = (unsigned long)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player){
		player->setGuildRank(std::string(rank));
		lua_pushnumber(L, 1);
	}
	else{
		lua_pushnumber(L, -1);
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
	}
	return 1;
}

int LuaScriptInterface::luaDoPlayerSetGuildNick(lua_State *L)
{
	//doPlayerSetGuildNick(cid, nick)
	const char* nick = popString(L);
	unsigned long cid = (unsigned long)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	Player* player = env->getPlayerByUID(cid);
	if(player){
		player->setGuildNick(std::string(nick));
		lua_pushnumber(L, 1);
	}
	else{
		lua_pushnumber(L, -1);
		reportErrorFunc(getErrorDesc(LUA_ERROR_PLAYER_NOT_FOUND));
	}
	return 1;
}

int LuaScriptInterface::luaGetGuildId(lua_State *L)
{
	//getGuildId(guild_name)
	const char* name = popString(L);
		
	unsigned long guildId;
	if(IOPlayer::instance()->getGuildIdByName(guildId, std::string(name))){
		lua_pushnumber(L, guildId);
	}
	else{
		lua_pushnumber(L, -1);
		reportErrorFunc("Guild not found");
	}
	return 1;
}

int LuaScriptInterface::luaDoMoveCreature(lua_State *L)
{
	//doMoveCreature(cid, direction)
	long direction = (long)popNumber(L);
	unsigned long cid = (unsigned long)popNumber(L);
	
	switch(direction){
	case NORTH:
	case SOUTH:
	case WEST:
	case EAST:
	case SOUTHWEST:
	case NORTHWEST:
	case NORTHEAST:
	case SOUTHEAST:
		break;
	default:
		lua_pushnumber(L, -1);
		reportErrorFunc("No valid direction");
		return 1;
	}
	
	ScriptEnviroment* env = getScriptEnv();
	
	Creature* creature = env->getCreatureByUID(cid);
	if(creature){
		ReturnValue ret = g_game.internalMoveCreature(creature, (Direction)direction);
		lua_pushnumber(L, ret);
	}
	else{
		lua_pushnumber(L, -1);
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
	}
	return 1;
}

int LuaScriptInterface::luaIsPlayer(lua_State *L)
{
	//isPlayer(cid)
	unsigned long cid = (unsigned long)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	
	if(env->getPlayerByUID(cid)){
		lua_pushnumber(L, 1);
	}
	else{
		lua_pushnumber(L, 0);
	}
	return 1;
}

int LuaScriptInterface::luaIsContainer(lua_State *L)
{
	//isContainer(uid)
	unsigned long uid = (unsigned long)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	
	if(env->getContainerByUID(uid)){
		lua_pushnumber(L, 1);
	}
	else{
		lua_pushnumber(L, 0);
	}
	return 1;
}
	
int LuaScriptInterface::luaGetPlayerByName(lua_State *L)
{
	//getPlayerByName(name)
	const char* name = popString(L);
	
	ScriptEnviroment* env = getScriptEnv();
	
	if(Player* player = g_game.getPlayerByName(name)){
		unsigned long cid = env->addThing(player);
		lua_pushnumber(L, cid);
	}
	else{
		lua_pushnumber(L, 0);
	}
	return 1;
	
}

int LuaScriptInterface::luaGetContainerSize(lua_State *L)
{
	//getContainerSize(uid)
	unsigned long uid = (unsigned long)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	
	if(Container* container = env->getContainerByUID(uid)){
		lua_pushnumber(L, container->size());
	}
	else{
		lua_pushnumber(L, -1);
		reportErrorFunc(getErrorDesc(LUA_ERROR_CONTAINER_NOT_FOUND));
	}
	return 1;
}

int LuaScriptInterface::luaGetContainerCap(lua_State *L)
{
	//getContainerCap(uid)
	unsigned long uid = (unsigned long)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	
	if(Container* container = env->getContainerByUID(uid)){
		lua_pushnumber(L, container->capacity());
	}
	else{
		lua_pushnumber(L, -1);
		reportErrorFunc(getErrorDesc(LUA_ERROR_CONTAINER_NOT_FOUND));
	}
	return 1;
}

int LuaScriptInterface::luaGetContainerItem(lua_State *L)
{
	//getContainerItem(uid, slot)
	unsigned long slot = (unsigned long)popNumber(L);
	unsigned long uid = (unsigned long)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	
	if(Container* container = env->getContainerByUID(uid)){
		Item* item = container->getItem(slot);
		if(item){
			long uid = env->addThing(item);
			pushThing(L, item, uid);
		}
		else{
			pushThing(L, NULL, 0);
		}
	}
	else{
		pushThing(L, NULL, 0);
		reportErrorFunc(getErrorDesc(LUA_ERROR_CONTAINER_NOT_FOUND));
	}
	return 1;
	
}

int LuaScriptInterface::luaDoAddContainerItem(lua_State *L)
{
	//doAddContainerItem(uid, itemid, count or subtype)
	int type = (int)popNumber(L);
	int itemid = (int)popNumber(L);
	unsigned int uid = (unsigned int)popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();
	unsigned long new_uid;
	Container* container = env->getContainerByUID(uid);
	if(container){
		Item* newItem = Item::CreateItem(itemid, type);

		ReturnValue ret = g_game.internalAddItem(container, newItem);
		if(ret != RET_NOERROR){
			delete newItem;
			lua_pushnumber(L, -1);
			reportErrorFunc("Could not add item");
			return 1;
		}
		
		if(newItem->getParent()){
			new_uid = env->addThing((Thing*)newItem);
			lua_pushnumber(L, new_uid);
		}
		else{
			//stackable item stacked with existing object, newItem will be released
			lua_pushnumber(L, -1);
		}
	}
	else{
		lua_pushnumber(L, -1);
		reportErrorFunc(getErrorDesc(LUA_ERROR_CONTAINER_NOT_FOUND));
	}
	return 1;
}
