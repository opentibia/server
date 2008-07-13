//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
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

#include "lua manager.h"


LuaState::LuaState(lua_State* state) : state(state) {
}

// Stack manipulation
int LuaState::getStackTop() {
	return lua_gettop(state);
}

bool LuaState::checkStackSize(int low, int high = -1) {
	int t = getStackTop();
	if(t < low) return false;
	if(high != -1 && t > high) return false;
	return true;
}

// Check
bool LuaState::isBoolean(int index = 1) {
	return lua_isboolean(state, index) || lua_isnil(state, index);
}

bool LuaState::isNumber(int index = 1) {
	return lua_isnumber(state, index);
}

bool LuaState::isString(int index = 1) {
	return lua_isstring(state, index);
}

bool LuaState::isUserdata(int index = 1) {
	return lua_isuserdata(state, index);
}

bool LuaState::isLuaFunction(int index = 1) {
	return lua_isfunction(state, index);
}

bool LuaState::isCFunction(int index = 1) {
	return lua_iscfunction(state, index);
}

// Pop
void LuaState::pop(int n = 1) {
	lua_pop(state, n);
}

bool LuaState::popBoolean() {
	bool b = lua_toboolean(state, 1);
	pop();
	return b;
}

int32_t LuaState::popInteger() {
	int32_t i = lua_tointeger(state, 1);
	pop();
	return i;
}

uint32_t LuaState::popUnsignedInteger() {
	double d = lua_tonumber(state, 1);
	pop();
	return uint32_t(d);
}

double LuaState::popFloat() {
	double d = lua_tonumber(state, 1);
	pop();
	return d;
}

std::string LuaState::popString() {
	size_t len;
	const char* cstr = lua_tostring(state, 1, &len);
	std::string str(cstr, len);
	pop();
	return str;
}

voidLuaState::* getUserdata() {
	void* p = lua_touserdata(state, 1);
	pop();
	return p;
}

// Push
void LuaState::pushBoolean(bool b) {
	lua_pushboolean(b);
}

void LuaState::pushInteger(int32_t i) {
	lua_pushnumber(i);
}

void LuaState::pushUnsignedInteger(uint32_t ui) {
	lua_pushnumber(ui);
}

void LuaState::pushFloat(double d) {
	lua_pushnumber(d);
}

void LuaState::pushString(const std::string& str) {
	lua_pushstring(str.c_str());
}

void LuaState::pushUserdata(void* ptr) {
	lua_pushlightuserdata(state, ptr);
}

///////////////////////////////////////////////////////////////////////////////
// Creating Lua Classes etc.

void ScriptManager::registerClass(const std::string& cname) {
	lua_newtable(state); // Metatable
	lua_pushvalue(state, 1); // Another reference to the table
	lua_setfield(state, LUA_REGISTRYINDEX, ("OTClass_" + cname).c_str());
	lua_newtable(state); // Class table
	lua_pushvalue(state, 1); // Another reference to the table
	lua_setfield(state, LUA_GLOBALSINDEX, cname.c_str());

	lua_setfield(state, 2, "__index"); // Set the index metamethod for the class metatable to the class table
	
	lua_pop(l, 1); // Pop the class metatable
}


void ScriptManager::registerClass(const std::string& cname, const std::string& parent_class) {
	lua_newtable(state); // Metatable
	lua_pushvalue(state, 1); // Another reference to the table
	lua_setfield(state, LUA_REGISTRYINDEX, ("OTClass_" + cname).c_str());
	lua_newtable(state); // Class table
	lua_pushvalue(state, 1); // Another reference to the table
	lua_setfield(state, LUA_GLOBALSINDEX, cname.c_str());

	lua_setfield(state, 2, "__index"); // Set the index metamethod for the class metatable to the class table
	
	lua_pop(l, 1); // Pop the class metatable

	lua_getfield(state, LUA_GLOBALSINDEX, cname.c_str()); // Add the derived class table to the top of the stack
	lua_newtable(state); // Create a small redirect table
	lua_getfield(state, LUA_GLOBALSINDEX, parent_class.c_str()); // Get the parent table
	lua_setfield(state, 2, "__index"); // Set the index metamethod for the redirect table to the base class table
	lua_pop(state, 1); // Remove the base class table from the stack
	lua_setmetatable(state, 2); // Set the metatable of the derived class table to the redirect table
	lua_pop(state, 1); // Pop the derived class table
}

ScriptEnviroment::ObjectID* LuaState::pushClassInstance(std::string cname) {
	ScriptEnviroment::ObjectID* p = (ScriptEnviroment::ObjectID*)
		lua_newuserdata(state, sizeof(ScriptEnviroment::ObjectID));
	lua_getfield(state, LUA_REGISTRYINDEX, ("OTClass_" + cname).c_str());
	lua_setmetatable(state, 2);
	return p;
}

///////////////////////////////////////////////////////////////////////////////
// Event pushing/popping

void LuaState::pushEventCallback(EventListener_ptr listener) {
	lua_getfield(state, LUA_REGISTRYINDEX, listener->getLuaTag().c_str());
}

///////////////////////////////////////////////////////////////////////////////
// Advanced type pushing/popping

void LuaState::pushThing(Thing* thing) {
	if(thing && thing->getItem()){
		Item* item = thing->getItem();
		ScriptEnviroment::ObjectID* objid;

		if(const Container* container = item->getContainer()) {
			objid = pushClassInstance("Container");
		}
		else if(const Teleport* teleport = item->getTeleport()) {
			objid = pushClassInstance("Teleport");
		}
		else {
			objid = pushClassInstance("Item");
		}
	}
	else if(thing && thing->getCreature()) {
		Creature* creature = thing->getCreature();
		ScriptEnviroment::ObjectID* objid;

		if(creature->getPlayer()) {
			objid = pushClassInstance("Player");
		}
		else if(creature->getMonster()) {
			objid = pushClassInstance("Monster");
		}
		else if(creature->getNpc()) {
			objid = pushClassInstance("NPC");
		}
		*objid = enviroment.addObject<Thing*>(creature);
	} else {
		ScriptEnviroment::ObjectID* objid;
		objid = pushClassInstance("Thing");
		*objid = enviroment.addObject<Thing*>(creature);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Lua State Thread

LuaStateManager::LuaStateManager(ScriptEnviroment& enviroment) : LuaState(), enviroment(enviroment) {
	state = luaL_newstate();
	if(!state){
		throw std::exception("Could not create lua context, fatal error");
	}

	// Load all standard libraries
	luaL_openlibs(state);
}

LuaStateManager::~LuaStateManager() {
}

bool LuaStateManager::loadFile(std::string file) {
	//loads file as a chunk at stack top
	int ret = luaL_loadfile(m_luaState, file.c_str());
	if(ret != 0){
		std::cout << "Lua Error: " << popString(m_luaState) << "\n";
		return false;
	}
	//check that it is loaded as a function
	if(lua_isfunction(m_luaState, -1) == 0){
		return false;
	}

	//execute it
	ret = lua_pcall(m_luaState, 0, 0, 0);
	if(ret != 0) {
		std::cout << "Lua Error: Failed to load file - " << popString();
		return false;
	}
	return true;
}

LuaThread_ptr LuaStateManager::newThread() {
	return new LuaThread(*this);
}

void LuaStateManager::runThread(LuaThread_ptr thread, int args) {
}

void LuaStateManager::scheduleThread(int32_t schedule, LuaThread_ptr thread) {
	LuaThreadSchedule s;
	s.schedule = schedule;
	s.thread = thread;
	threads.push(s);
}

void LuaStateManager::runScheduledThreads() {
	while(true) {
		LuaThreadSchedule& scheduled = threads.top();
		if(scheduled.schedule < current_time) {
			scheduled.thread->run(0);
			threads.pop();
		} else {
			break;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// Child Thread

LuaThread::LuaThread(LuaStateManager& manager, const std::string& name) : LuaState(), manager(manager), name(name) {
	state = lua_newthread(manager.state);
	manager.pop();
}

LuaThread::~LuaThread() {
}

void LuaThread::run(int args) {
	int ret = lua_resume(state, args);
	if(ret == LUA_YIELD) {
		// Thread yielded, add us to the manager
		int32_t schedule = popInteger();
		manager.scheduleThread(schedule, this);
	} else if(ret == 0) {
		// Thread exited normally, do nothing, it will be garbage collected
	} else if(ret == LUA_ERRRUN) {
		std::string errmsg = popString();
		std::cout << "Lua Error: " << errmsg << "\n";

		lua_getfield(L, LUA_GLOBALSINDEX, "debug");
		if (!lua_istable(L, -1)) {
			lua_pop(L, 1);
			return;
		}
		lua_getfield(L, -1, "traceback");
		if (!lua_isfunction(L, -1)) {
			lua_pop(L, 2);
			return;
		}
		lua_pushvalue(L, 1);
		lua_pushinteger(L, 2);
		lua_call(L, 2, 1);
		return;
	} else if(ret == LUA_ERRERR) {
		// Can't handle, just print error message
		std::cout << "Lua Error when recovering from error (thread " << name << ")\n";
	} else if(ret == LUA_ERRMEM) {
		std::cout << "Lua Error: Memory Allocation Failed!\n";
	} else {
		// ??
	}
}
