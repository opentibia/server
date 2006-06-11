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

#include "luascript.h"
#include "player.h"
#include "item.h"

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


ScriptEnviroment::ScriptEnviroment()
{
	resetEnv();
	m_lastUID = 70000;
}

ScriptEnviroment::~ScriptEnviroment()
{
	//
}

void ScriptEnviroment::resetEnv()
{
	m_interface = NULL;
	m_scriptId = 0;
}
	
void ScriptEnviroment::setScriptId(long scriptId, LuaScriptInterface* scriptInterface)
{
	m_scriptId = scriptId;
	m_interface = scriptInterface;
}

void ScriptEnviroment::setEventDesc(const std::string& desc)
{
	m_eventdesc = desc;
}
	
void ScriptEnviroment::getEventInfo(long& scriptId, std::string& desc, LuaScriptInterface* scriptInterface)
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
	//TODO
	return 0;
}
	
Thing* ScriptEnviroment::getThingByUID(long uid)
{
	//TODO
	return NULL;
}

Item* ScriptEnviroment::getItemByUID(long uid)
{
	//TODO
	return NULL;
}

Creature* ScriptEnviroment::getCreatureByUID(long uid)
{
	//TODO
	return NULL;
}

Player* ScriptEnviroment::getPlayerByUID(long uid)
{
	//TODO
	return NULL;
}


LuaScriptInterface::LuaScriptInterface()
{
	m_luaState = NULL;
	if(!initState()){
		//handle error
	}
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
	std::cout <<  "stack side: " << a << std::endl;
	for(int i = 1; i <= a ; i++){
		std::cout << lua_typename(m_luaState, lua_type(m_luaState,-i)) << " " << lua_topointer(m_luaState, -i) << std::endl;
	}
}

long LuaScriptInterface::loadFile(const std::string& file, const std::string& eventName)
{
	int ret = luaL_loadfile(m_luaState, file.c_str());
	if(ret != 0){
		 m_lastLuaError = ret;
		return -1;
	}
	if(lua_isfunction(m_luaState, -1) == 0){
		return -1;
	}
	//
	ret = lua_pcall(m_luaState, 0, 0, 0);
	if(ret != 0){
		return -1;
	}
	//
	lua_getfield(m_luaState, LUA_REGISTRYINDEX, "EVENTS");
	if(lua_istable(m_luaState, -1) == 0){
		return -1;
	}
	//
	lua_getglobal(m_luaState, eventName.c_str());
	if(lua_isfunction(m_luaState, -1) == 0){
		return -1;
	}
	
	lua_pushnumber(m_luaState, m_runningEventId++);
	lua_pushvalue(m_luaState, -2);
	lua_rawset(m_luaState, -4);
	lua_pop(m_luaState, 2);

	return 1;
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

void LuaScriptInterface::reportError(const std::string& error_desc)
{
	ScriptEnviroment* env = getScriptEnv();
	long fileId;
	std::string event_desc;
	LuaScriptInterface* scriptInterface;
	env->getEventInfo(fileId, event_desc, scriptInterface);
	
	std::cout << "Lua Script Error: ";
	if(scriptInterface){
		std::cout << "[" << scriptInterface->getInterfaceName() << "] " <<
			scriptInterface->getFileById(fileId) << std::endl;
	}
	std::cout << event_desc << std::endl;
	std::cout << error_desc << std::endl;
}


std::string LuaScriptInterface::getInterfaceName()
{
	return "Base Lua Interface";
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
	luaopen_math(m_luaState);
	luaopen_string(m_luaState);
	
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
	
void LuaScriptInterface::registerFunctions()
{
	//TODO
	//lua_register(L, "name", C_function);
}
