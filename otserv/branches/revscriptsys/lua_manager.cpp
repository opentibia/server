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

#include "configmanager.h"

#include "lua_manager.h"
#include "script_manager.h"
#include "script_enviroment.h"
#include "script_listener.h"
#include "script_event.h"

#include "game.h"
#include "tile.h"
#include "container.h"
#include "player.h"
#include "town.h"
#include "chat.h"

extern Game g_game;
extern ConfigManager g_config;
extern Chat g_chat;

LuaState::LuaState(Script::Enviroment& env) : enviroment(env) {
	;
}

LuaState::~LuaState() {
	;
}

void LuaState::HandleError(Script::ErrorMode mode, const std::string& error) {
	if(mode == Script::ERROR_THROW) {
		throw Script::Error(error);
	} else if(mode == Script::ERROR_WARN) {
		std::cout << "Lua warning:" << error << std::endl;
	}
	// pass
}

void LuaState::HandleError(const std::string& error) {
	HandleError(Script::ERROR_WARN, error);
}

// Stack manipulation
int LuaState::getStackTop() {
	return lua_gettop(state);
}

bool LuaState::checkStackSize(int low, int high) {
	int t = getStackTop();
	if(t < low) return false;
	if(high != -1 && t > high) return false;
	return true;
}

void LuaState::duplicate(int idx /* = -1 */) {
	lua_pushvalue(state, idx);
}

// Table manipulation

void LuaState::newTable() {
	lua_newtable(state);
}

void LuaState::getField(int index, const std::string& field_name) {
	lua_getfield(state, index, field_name.c_str());
}

void LuaState::setField(int index, const std::string& field_name) {
	lua_setfield(state, index, field_name.c_str());
}

void LuaState::setField(int index, int field_index) {
	push(field_index);
	lua_insert(state, -2);
	if(index <= -10000)
		lua_settable(state, index);
	else
		if(index < 0)
			lua_settable(state, index-1);
		else
			lua_settable(state, index);
}

void LuaState::clearStack() {
	lua_settop(state, 0);
}

void LuaState::insert(int idx) {
	lua_insert(state, idx);
}

void LuaState::swap(int idx) {
	lua_insert(state, idx);
	lua_pushvalue(state, idx+1);
	lua_remove(state, idx+1);
}

std::string LuaState::typeOf(int idx) {
	return lua_typename(state, lua_type(state, idx));
}

// Check
bool LuaState::isNil(int index) {
	return lua_isnil(state, index);
}

bool LuaState::isBoolean(int index) {
	return lua_isboolean(state, index) || lua_isnil(state, index);
}

bool LuaState::isNumber(int index) {
	return lua_isnumber(state, index) != 0;
}

bool LuaState::isString(int index) {
	return lua_isstring(state, index) != 0;
}

bool LuaState::isUserdata(int index) {
	return lua_isuserdata(state, index) != 0;
}

bool LuaState::isLuaFunction(int index) {
	return lua_isfunction(state, index) != 0;
}

bool LuaState::isCFunction(int index) {
	return lua_iscfunction(state, index) != 0;
}

bool LuaState::isThread(int index) {
	return lua_isthread(state, index) != 0;
}

bool LuaState::isTable(int index) {
	return lua_istable(state, index) != 0;
}

bool LuaState::isFunction(int index) {
	return lua_isfunction(state, index) != 0 || lua_iscfunction(state, index) != 0;
}

// Pop
void LuaState::pop(int n) {
	lua_pop(state, n);
}

bool LuaState::popBoolean() {
	bool b = (lua_toboolean(state, -1) != 0);
	pop();
	return b;
}

int32_t LuaState::popInteger() {
	int32_t i = lua_tointeger(state, -1);
	pop();
	return i;
}

uint32_t LuaState::popUnsignedInteger() {
	double d = lua_tonumber(state, -1);
	pop();
	return uint32_t(d);
}

double LuaState::popFloat() {
	double d = lua_tonumber(state, -1);
	pop();
	return d;
}

std::string LuaState::popString() {
	size_t len;
	const char* cstr = lua_tolstring(state, -1, &len);
	std::string str(cstr, len);
	pop();
	return str;
}

void* LuaState::getUserdata() {
	void* p = lua_touserdata(state, -1);
	pop();
	return p;
}

// Push
void LuaState::pushNil() {
	lua_pushnil(state);
}

void LuaState::pushBoolean(bool b) {
	lua_pushboolean(state, b);
}

void LuaState::pushInteger(int32_t i) {
	lua_pushnumber(state, i);
}

void LuaState::pushUnsignedInteger(uint32_t ui) {
	lua_pushnumber(state, ui);
}

void LuaState::pushFloat(double d) {
	lua_pushnumber(state, d);
}

void LuaState::pushString(const std::string& str) {
	lua_pushstring(state, str.c_str());
}

void LuaState::pushUserdata(void* ptr) {
	lua_pushlightuserdata(state, ptr);
}

///////////////////////////////////////////////////////////////////////////////
// Push an empty instance of a class

Script::ObjectID* LuaState::pushClassInstance(const std::string& cname) {
	Script::ObjectID* p = (Script::ObjectID*)lua_newuserdata(state, sizeof(Script::ObjectID));
	lua_getfield(state, LUA_REGISTRYINDEX, ("OTClass_" + cname).c_str());
	lua_setmetatable(state, -2);
	return p;
}

void LuaState::pushClassTableInstance(const std::string& cname) {
	newTable();
	lua_getfield(state, LUA_REGISTRYINDEX, ("OTClass_" + cname).c_str());
	lua_setmetatable(state, -2);
}

///////////////////////////////////////////////////////////////////////////////
// Event pushing/popping

void LuaState::pushCallback(Script::Listener_ptr listener) {
	lua_getfield(state, LUA_REGISTRYINDEX, listener->getLuaTag().c_str());
}

///////////////////////////////////////////////////////////////////////////////
// Advanced type pushing/popping

void LuaState::pushEvent(Script::Event& event) {
	event.push_instance(*this, enviroment);
}

void LuaState::pushPosition(const Position& pos) {
	newTable();
	pushInteger(pos.x);
	setField(-2, "x");
	pushInteger(pos.y);
	setField(-2, "y");
	pushInteger(pos.z);
	setField(-2, "z");
}

void LuaState::pushThing(Thing* thing) {
	if(thing && thing->getItem()){
		Item* item = thing->getItem();
		Script::ObjectID* objid;

		if(const Container* container = item->getContainer()) {
			objid = pushClassInstance("Container");
		}
		else if(const Teleport* teleport = item->getTeleport()) {
			objid = pushClassInstance("Teleport");
		}
		else {
			objid = pushClassInstance("Item");
		}
		*objid = enviroment.addThing(item);
	}
	else if(thing && thing->getCreature()) {
		Creature* creature = thing->getCreature();
		Script::ObjectID* objid;

		if(creature->getPlayer()) {
			objid = pushClassInstance("Player");
		}
		else if(creature->getMonster()) {
			objid = pushClassInstance("Monster");
		}
		else if(creature->getNpc()) {
			objid = pushClassInstance("NPC");
		}
		*objid = enviroment.addThing(creature);
	} else if(thing && thing->getTile()) {
		pushTile(thing->getTile());
	} else if(thing) {
		Script::ObjectID* objid;
		objid = pushClassInstance("Thing");
		*objid = enviroment.addThing(thing);
	} else {
		pushNil();
	}
}

void LuaState::pushTile(Tile* tile) {
	if(tile) {
		pushClassTableInstance("Tile");
		setField(-1, "__x", tile->getPosition().x);
		setField(-1, "__y", tile->getPosition().y);
		setField(-1, "__z", tile->getPosition().z);
	} else {
		pushNil();
	}
}

void LuaState::pushTown(Town* town) {
	if(town) {
		Script::ObjectID* objid = pushClassInstance("Town");
		*objid = town->getTownID();
	} else {
		pushNil();
	}
}

void LuaState::pushChannel(ChatChannel* channel) {
	if(channel) {
		Script::ObjectID* objid = pushClassInstance("Channel");
		*objid = enviroment.addObject(channel);
	} else {
		pushNil();
	}
}

Position LuaState::popPosition(Script::ErrorMode mode /* = Script::ERROR_THROW */) {
	Position pos(0, 0, 0);
	if(!isTable(-1)) {
		HandleError(mode, "Attempt to treat non-table value as a position.");
		pop();
		return pos;
	}
	getField(-1, "x");
	pos.x = popInteger();
	getField(-1, "y");
	pos.y = popInteger();
	getField(-1, "z");
	pos.z = popInteger();
	pop();
	return pos;
}

Tile* LuaState::popTile(Script::ErrorMode mode /* = Script::ERROR_THROW */) {
	if(!isTable(-1)) {
		HandleError(mode, std::string("Couldn't pop tile, top object is not of valid type (") + luaL_typename(state, -1) + ")");
		pop();
		return NULL;
	}

	getField(-1, "__x");
	int x = popInteger();
	getField(-1, "__y");
	int y = popInteger();
	getField(-1, "__z");
	int z = popInteger();

	Tile* tile = g_game.getTile(x, y, z);
	if(!tile) HandleError(mode, "Tile position is invalid.");
	return tile;
}

Town* LuaState::popTown(Script::ErrorMode mode /* = Script::ERROR_THROW */) {
	if(!isUserdata(-1)) {
		HandleError(mode, std::string("Couldn't pop town, top object is not of valid type (") + luaL_typename(state, -1) + ")");
		pop();
		return NULL;
	}

	Script::ObjectID* objid = (Script::ObjectID*)lua_touserdata(state, -1);
	pop();

	return Towns::getInstance().getTown((uint32_t)*objid);
}

ChatChannel* LuaState::popChannel(Script::ErrorMode mode /* = Script::ERROR_THROW */) {
	if(!isUserdata(-1)) {
		HandleError(mode, std::string("Couldn't pop channel, top object is not of valid type (") + luaL_typename(state, -1) + ")");
		pop();
		return NULL;
	}

	Script::ObjectID* objid = (Script::ObjectID*)lua_touserdata(state, -1);
	pop();

	return (ChatChannel*)enviroment.getObject(*objid);
}

Thing* LuaState::popThing(Script::ErrorMode mode /* = Script::ERROR_THROW */) {
	if(!isUserdata(-1)) {
		HandleError(mode, std::string("Couldn't pop thing, top object is not of valid type (") + luaL_typename(state, -1) + ")");
		pop();
		return NULL;
	}

	Script::ObjectID* objid = (Script::ObjectID*)lua_touserdata(state, -1);
	pop();
	Thing* thing = enviroment.getThing(*objid);
	if(!thing) HandleError(mode, "Object does not exist in object list.");

	return thing;
}

Creature* LuaState::popCreature(Script::ErrorMode mode /* = Script::ERROR_THROW */) {
	Thing* t = popThing(mode);
	if(t) {
		Creature* c = t->getCreature();
		if(!c) HandleError(mode, "Object is not a creature.");
		return c;
	}
	return NULL;
}

Player* LuaState::popPlayer(Script::ErrorMode mode /* = Script::ERROR_THROW */) {
	Creature* c = popCreature(mode);
	if(c) {
		Player* p = c->getPlayer();
		if(!p) HandleError(mode, "Object is not a player.");
		return p;
	}
	return NULL;
}

Item* LuaState::popItem(Script::ErrorMode mode /* = Script::ERROR_THROW */) {
	Thing* t = popThing(mode);
	if(t) {
		Item* i = t->getItem();
		if(!i) HandleError(mode, "Object is not an item.");
		return i;
	}
	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// Lua State Thread

LuaStateManager::LuaStateManager(Script::Enviroment& enviroment) : LuaState(enviroment) {
	state = luaL_newstate();
	if(!state){
		throw std::runtime_error("Could not create lua context, fatal error");
	}

	// Load all standard libraries
	luaL_openlibs(state);

	//getGlobal("package");
	lua_getfield(state, LUA_GLOBALSINDEX, "package");
	assert(lua_istable(state, 1));
	//pushString(g_config.getString(ConfigManager::DATA_DIRECTORY) + "scripts/?.lua");
	lua_pushstring(state, (g_config.getString(ConfigManager::DATA_DIRECTORY) + "scripts/?.lua").c_str());
	//setField(2, "path");
	lua_setfield(state, -2, "path");
}

LuaStateManager::~LuaStateManager() {
	lua_close(state);
}

bool LuaStateManager::loadFile(std::string file) {
	//loads file as a chunk at stack top
	int ret = luaL_loadfile(state, file.c_str());

	if(ret != 0){
		std::cout << "Lua Error: " << popString() << "\n";
		return false;
	}
	//check that it is loaded as a function
	if(lua_isfunction(state, -1) == 0){
		return false;
	}

	//execute it
	ret = lua_pcall(state, 0, 0, 0);
	if(ret != 0) {
		std::cout << "Lua Error: Failed to load file - " << popString();
		return false;
	}
	return true;
}

LuaThread_ptr LuaStateManager::newThread(const std::string& name) {
	LuaThread_ptr p(new LuaThread(*this, name));
	threads[p->state] = p;
	return p;
}

void LuaStateManager::scheduleThread(int32_t schedule, LuaThread_ptr thread) {
	ThreadSchedule s;
	s.scheduled_time = OTSYS_TIME() + schedule;
	s.thread = thread;
	queued_threads.push(s);
}

void LuaStateManager::runScheduledThreads() {
	int64_t current_time = OTSYS_TIME();
	while(queued_threads.empty() == false) {
		const ThreadSchedule& scheduled = queued_threads.top();
		if(scheduled.scheduled_time < current_time) {
			int32_t t = scheduled.thread->run(0);
			if(t > 0) {
				scheduleThread(t, scheduled.thread);
			} else {
				ThreadMap::iterator iter = threads.find(scheduled.thread->state);
				threads.erase(iter);
			}
			queued_threads.pop();
		} else {
			break;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// Child Thread

LuaThread::LuaThread(LuaStateManager& manager, const std::string& name) :
	LuaState(manager.enviroment),
	manager(manager),
	name(name),
	thread_state(0)
{
	state = lua_newthread(manager.state);
	lua_pop(manager.state, 1); // Remove the thread from the main stack
}

LuaThread::~LuaThread() {
}

bool LuaThread::ok() const {
	return thread_state == 0 || thread_state == LUA_YIELD;
}

Script::Manager* LuaThread::getManager() {
	return static_cast<Script::Manager*>(&manager);
}

int32_t LuaThread::run(int args) {
	int ret = lua_resume(state, args);
	thread_state = ret;
	if(ret == LUA_YIELD) {
		// Thread yielded, add us to the manager
		int32_t schedule = popInteger();
		return schedule;
	} else if(ret == 0) {
		// Thread exited normally, do nothing, it will be garbage collected
	} else if(ret == LUA_ERRRUN) {
		std::string errmsg = popString();
		std::cout << "Lua Error: " << errmsg << "\n";
		std::cout << "Stack trace:\n";
		std::cout << "Line\tFunction\t\tSource\n";

		lua_Debug ar;
		
		int level = 0;
		while(lua_getstack(state, ++level, &ar) != 0) {
			lua_getinfo(state, "nSl", &ar);

			if(ar.currentline != -1) {
				std::cout << ar.currentline;
			}
			std::cout << "\t";
			
			int tabcount = 16;
			if(ar.name) {
				std::cout << ar.name;
				tabcount -= 16 - strlen(ar.name);
			}
			while(tabcount-- > 0) std::cout << " ";

			std::cout << ar.short_src;
			std::cout << "\n";
		}

	} else if(ret == LUA_ERRERR) {
		// Can't handle, just print error message
		std::cout << "Lua Error when recovering from error (thread " << name << ")\n";
	} else if(ret == LUA_ERRMEM) {
		std::cout << "Lua Error: Memory Allocation Failed!\n";
	} else {
		// ??
	}
	return 0;
}
