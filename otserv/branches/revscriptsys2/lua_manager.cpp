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

#ifdef WIN32
#include <winsock2.h>
#endif

#include <boost/filesystem.hpp>

#include "configmanager.h"

#include "lua_manager.h"
#include "script_manager.h"
#include "script_environment.h"
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

LuaState::LuaState(Script::Manager* man) : state(NULL), manager(man)
{
	if(manager)
		environment = manager->environment;
}

LuaState::~LuaState()
{
	;
}

void LuaState::HandleError(Script::ErrorMode mode, const std::string& error)
{
	if(mode == Script::ERROR_THROW) {
		throw Script::Error(error);
	} else if(mode == Script::ERROR_WARN) {
		std::cout << "Lua warning:" << error << std::endl;
	}
	// pass
}

void LuaState::HandleError(const std::string& error)
{
	HandleError(Script::ERROR_WARN, error);
}

// Stack manipulation
int32_t LuaState::getStackSize()
{
	return lua_gettop(state);
}

bool LuaState::checkStackSize(int32_t low, int32_t high)
{
	int32_t t = getStackSize();
	if(t < low) return false;
	if(high != -1 && t > high) return false;
	return true;
}

void LuaState::duplicate(int32_t idx /* = -1 */)
{
	lua_pushvalue(state, idx);
}

// Table manipulation

void LuaState::newTable()
{
	lua_newtable(state);
}

bool LuaState::iterateTable(int32_t index)
{
	return lua_next(state, index) != 0;
}

void LuaState::getField(int32_t index, const std::string& field_name)
{
	if(!isTable(index) && !isUserdata(index))
		throw Script::Error("Attempt to index non-table value.");
	lua_getfield(state, index, field_name.c_str());
}

void LuaState::getField(int32_t index, int field_index)
{
	lua_pushnumber(state, field_index);
	lua_gettable(state, (index < 0? index - 1 : index));
}

void LuaState::setField(int32_t index, const std::string& field_name)
{
	lua_setfield(state, index, field_name.c_str());
}

void LuaState::setField(int32_t index, int32_t field_index)
{
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

void LuaState::clearStack()
{
	lua_settop(state, 0);
}

void LuaState::insert(int32_t idx)
{
	if(idx < 0 && idx < -lua_gettop(state))
		throw Script::Error("Insertion to position outside stack (under).");
	if(idx > 0 && idx > lua_gettop(state))
		throw Script::Error("Insertion to position outside stack (over).");
	lua_insert(state, idx);
}

void LuaState::swap(int32_t idx)
{
	lua_insert(state, idx);
	lua_pushvalue(state, idx+1);
	lua_remove(state, idx+1);
}

std::string LuaState::typeName(int32_t idx)
{
	return lua_typename(state, lua_type(state, idx));
}

int32_t LuaState::rawtype(int32_t idx)
{
	return lua_type(state, idx);
}

// Check
bool LuaState::isNil(int32_t index)
{
	return lua_isnil(state, index);
}

bool LuaState::isBoolean(int32_t index)
{
	return lua_isboolean(state, index) || lua_isnil(state, index);
}

bool LuaState::isNumber(int32_t index)
{
	if(lua_isnumber(state, index) != 0)
		return true;
	if(lua_istable(state, index) != 0){
		lua_getfield(state, index, "__intValue");
		if(lua_isnil(state, -1)){
			lua_pop(state, 1);
			return false;
		}
		else{
			lua_pop(state, 1);
			return true;
		}
	}
	return false;
}

bool LuaState::isString(int32_t index)
{
	return lua_isstring(state, index) != 0;
}

bool LuaState::isUserdata(int32_t index)
{
	return lua_isuserdata(state, index) != 0;
}

bool LuaState::isLuaFunction(int32_t index)
{
	return lua_isfunction(state, index) != 0;
}

bool LuaState::isCFunction(int32_t index)
{
	return lua_iscfunction(state, index) != 0;
}

bool LuaState::isThread(int32_t index)
{
	return lua_isthread(state, index) != 0;
}

bool LuaState::isTable(int32_t index)
{
	return lua_istable(state, index) != 0;
}

bool LuaState::isFunction(int32_t index)
{
	return lua_isfunction(state, index) != 0 || lua_iscfunction(state, index) != 0;
}

// Pop
void LuaState::pop(int n)
{
	lua_pop(state, n);
}

bool LuaState::popBoolean()
{
	bool b = (lua_toboolean(state, -1) != 0);
	pop();
	return b;
}

int32_t LuaState::popInteger()
{
	return int32_t(popFloat());
}

int32_t LuaState::popInteger(int def)
{
	if(lua_isnumber(state, -1) || lua_istable(state, -1))
		def = popInteger();
	return def;
}

uint32_t LuaState::popUnsignedInteger()
{
	double d = popFloat();
	if(d < 0)
		throw Script::Error("Expected unsigned number, got negative number.");
	return uint32_t(d);
}

double LuaState::popFloat()
{
	double d;
	if(lua_istable(state, -1)) {
		lua_getfield(state, -1, "__intValue");
		d = lua_tonumber(state, -1);
		pop(2);
	}
	else{
		d = lua_tonumber(state, -1);
		pop(1);
	}
	return d;
}

std::string LuaState::popString()
{
	size_t len;
	const char* cstr = lua_tolstring(state, -1, &len);
	std::string str(cstr, len);
	pop();
	return str;
}

void* LuaState::getUserdata()
{
	void* p = lua_touserdata(state, -1);
	pop();
	return p;
}

// Push
void LuaState::pushNil()
{
	lua_pushnil(state);
}

void LuaState::pushBoolean(bool b)
{
	lua_pushboolean(state, b);
}

void LuaState::pushInteger(int32_t i)
{
	lua_pushnumber(state, i);
}

void LuaState::pushUnsignedInteger(uint32_t ui)
{
	lua_pushnumber(state, ui);
}

void LuaState::pushFloat(double d)
{
	lua_pushnumber(state, d);
}

void LuaState::pushString(const std::string& str)
{
	lua_pushstring(state, str.c_str());
}

void LuaState::pushUserdata(void* ptr)
{
	lua_pushlightuserdata(state, ptr);
}

///////////////////////////////////////////////////////////////////////////////
// Push an empty instance of a class

Script::ObjectID* LuaState::pushClassInstance(const std::string& cname)
{
	Script::ObjectID* p = (Script::ObjectID*)lua_newuserdata(state, sizeof(Script::ObjectID));
	lua_getfield(state, LUA_REGISTRYINDEX, ("OTClass_" + cname).c_str());
	lua_setmetatable(state, -2);
	return p;
}

void LuaState::pushClassTableInstance(const std::string& cname)
{
	newTable();
	lua_getfield(state, LUA_REGISTRYINDEX, ("OTClass_" + cname).c_str());
	lua_setmetatable(state, -2);
}

void LuaState::getMetaObject()
{
	if(lua_getmetatable(state, -1)){
		lua_getfield(state, -1, "__object"); // Get __object
		if(lua_isnil(state, -1)){
			// No __object member, use it directly
			pop(2); // Pop the nil & metatable
		}
		else{
			// It had a __object member, use it!

			swap(-2); // Swap __object with metatable on stack
			pop(); // Pop metatable, now __object is ontop of stack
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// Event pushing/popping

void LuaState::pushCallback(Script::Listener_ptr listener)
{
	lua_getfield(state, LUA_REGISTRYINDEX, listener->getLuaTag().c_str());
}

///////////////////////////////////////////////////////////////////////////////
// Advanced type pushing/popping

void LuaState::pushEvent(Script::Event& event)
{
	event.push_instance(*this, *environment);
}

void LuaState::pushPosition(const Position& pos)
{
	newTable();
	pushInteger(pos.x);
	setField(-2, "x");
	pushInteger(pos.y);
	setField(-2, "y");
	pushInteger(pos.z);
	setField(-2, "z");
}

void LuaState::pushPosition(const PositionEx& pos)
{
	newTable();
	pushInteger(pos.x);
	setField(-2, "x");
	pushInteger(pos.y);
	setField(-2, "y");
	pushInteger(pos.z);
	setField(-2, "z");
	pushInteger(pos.stackpos);
	setField(-2, "stackpos");
}

void LuaState::pushOutfit(const OutfitType& outfit)
{
	newTable();
	pushInteger(outfit.lookType);
	setField(-2, "type");
	pushInteger(outfit.lookHead);
	setField(-2, "head");
	pushInteger(outfit.lookBody);
	setField(-2, "body");
	pushInteger(outfit.lookLegs);
	setField(-2, "legs");
	pushInteger(outfit.lookFeet);
	setField(-2, "feet");
	pushInteger(outfit.lookTypeEx);
	setField(-2, "item");
	pushInteger(outfit.lookAddons);
	setField(-2, "addons");
}

void LuaState::pushThing(Thing* thing)
{
	if(thing && thing->getItem()){
		Item* item = thing->getItem();
		Script::ObjectID* objid = NULL;

		if(item->getContainer()) {
			objid = pushClassInstance("Container");
		}
		else if(item->getTeleport()) {
			objid = pushClassInstance("Teleport");
		}
		else {
			objid = pushClassInstance("Item");
		}
		*objid = environment->addThing(item);
	}
	else if(thing && thing->getCreature()) {
		Creature* creature = thing->getCreature();
		Script::ObjectID* objid;

		if(creature->getPlayer()) {
			objid = pushClassInstance("Player");
		}
		else if(creature->getActor()) {
			objid = pushClassInstance("Actor");
		}
		else {
			pushNil();
			return;
		}
		*objid = environment->addThing(creature);
	} else if(thing && thing->getTile()) {
		pushTile(thing->getTile());
	} else if(thing) {
		Script::ObjectID* objid;
		objid = pushClassInstance("Thing");
		*objid = environment->addThing(thing);
	} else {
		pushNil();
	}
}

void LuaState::pushTile(Tile* tile)
{
	if(tile) {
		pushClassTableInstance("Tile");
		setField(-1, "x", tile->getPosition().x);
		setField(-1, "y", tile->getPosition().y);
		setField(-1, "z", tile->getPosition().z);
	} else {
		pushNil();
	}
}

void LuaState::pushTown(Town* town)
{
	if(town) {
		Script::ObjectID* objid = pushClassInstance("Town");
		*objid = town->getTownID();
	} else {
		pushNil();
	}
}

void LuaState::pushHouse(House* house)
{
	if(house) {
		Script::ObjectID* objid = pushClassInstance("House");
		*objid = environment->addObject(house);
	} else {
		pushNil();
	}
}

void LuaState::pushChannel(ChatChannel* channel)
{
	if(channel) {
		Script::ObjectID* objid = pushClassInstance("Channel");
		*objid = environment->addObject(channel);
	} else {
		pushNil();
	}
}

void LuaState::pushCondition(Condition* condition)
{
	if(condition) {
		Script::ObjectID* objid = pushClassInstance("Condition");
		*objid = environment->addObject(condition);
	} else {
		pushNil();
	}
}

void LuaState::pushWaypoint(Waypoint_ptr wp)
{
	if(wp) {
		Script::ObjectID* objid = pushClassInstance("Waypoint");
		*objid = environment->addObject(wp.get());
	} else {
		pushNil();
	}
}

Position LuaState::popPosition(Script::ErrorMode mode /* = Script::ERROR_THROW */)
{
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

OutfitType LuaState::popOutfit(Script::ErrorMode mode /* = Script::ERROR_THROW */)
{
	OutfitType outfit;
	if(!isTable(-1)) {
		HandleError(mode, "Attempt to treat non-table value as an outfit.");
		pop();
		return outfit;
	}
	getField(-1, "type");
	outfit.lookType = popInteger();
	getField(-1, "head");
	outfit.lookHead = popInteger();
	
	getField(-1, "body");
	outfit.lookBody = popInteger(0);
	getField(-1, "legs");
	outfit.lookLegs = popInteger(0);
	getField(-1, "feet");
	outfit.lookFeet = popInteger(0);
	getField(-1, "item");
	outfit.lookTypeEx = popInteger(0);
	getField(-1, "addons");
	outfit.lookAddons = popInteger(0);

	pop();
	return outfit;
}

Tile* LuaState::popTile(Script::ErrorMode mode /* = Script::ERROR_THROW */)
{
	getMetaObject();

	if(!isTable(-1)) {
		HandleError(mode, std::string("Couldn't pop tile, top object is not of valid type (") + luaL_typename(state, -1) + ")");
		pop();
		return NULL;
	}

	getField(-1, "x");
	int32_t x = popInteger();
	getField(-1, "y");
	int32_t y = popInteger();
	getField(-1, "z");
	int32_t z = popInteger();

	Tile* tile = g_game.getTile(x, y, z);
	if(!tile)
		HandleError(mode, "Tile position is invalid.");
	return tile;
}

Town* LuaState::popTown(Script::ErrorMode mode /* = Script::ERROR_THROW */)
{
	getMetaObject();

	if(!isUserdata(-1)) {
		HandleError(mode, std::string("Couldn't pop town, top object is not of valid type (") + luaL_typename(state, -1) + ")");
		pop();
		return NULL;
	}

	Script::ObjectID* objid = (Script::ObjectID*)lua_touserdata(state, -1);
	pop();

	return Towns::getInstance().getTown((uint32_t)*objid);
}

ChatChannel* LuaState::popChannel(Script::ErrorMode mode /* = Script::ERROR_THROW */)
{
	getMetaObject();

	if(!isUserdata(-1)) {
		HandleError(mode, std::string("Couldn't pop channel, top object is not of valid type (") + luaL_typename(state, -1) + ")");
		pop();
		return NULL;
	}

	Script::ObjectID* objid = (Script::ObjectID*)lua_touserdata(state, -1);
	pop();

	return (ChatChannel*)environment->getObject(*objid);
}

House* LuaState::popHouse(Script::ErrorMode mode /* = Script::ERROR_THROW */)
{
	getMetaObject();

	if(!isUserdata(-1)) {
		HandleError(mode, std::string("Couldn't pop house, top object is not of valid type (") + luaL_typename(state, -1) + ")");
		pop();
		return NULL;
	}

	Script::ObjectID* objid = (Script::ObjectID*)lua_touserdata(state, -1);
	pop();

	return (House*)environment->getObject(*objid);
}

Waypoint_ptr LuaState::popWaypoint(Script::ErrorMode mode /* = Script::ERROR_THROW */)
{
	getMetaObject();

	if(!isUserdata(-1)) {
		HandleError(mode, std::string("Couldn't pop waypoint, top object is not of valid type (") + luaL_typename(state, -1) + ")");
		pop();
		return Waypoint_ptr();
	}

	Script::ObjectID* objid = (Script::ObjectID*)lua_touserdata(state, -1);
	pop();

	return ((Waypoint*)environment->getObject(*objid))->shared_from_this();
}

Thing* LuaState::popThing(Script::ErrorMode mode /* = Script::ERROR_THROW */)
{
	getMetaObject();

	if(!isUserdata(-1)) {
		HandleError(mode, std::string("Couldn't pop thing, top object is not of valid type (") + luaL_typename(state, -1) + ")");
		pop();
		return NULL;
	}

	Script::ObjectID* objid = (Script::ObjectID*)lua_touserdata(state, -1);
	pop();
	Thing* thing = environment->getThing(*objid);
	if(!thing) HandleError(mode, "Object does not exist in object list.");

	return thing;
}

Creature* LuaState::popCreature(Script::ErrorMode mode /* = Script::ERROR_THROW */)
{
	Thing* t = popThing(mode);
	if(t) {
		Creature* c = t->getCreature();
		if(!c) HandleError(mode, "Object is not a creature.");
		return c;
	}
	return NULL;
}

Player* LuaState::popPlayer(Script::ErrorMode mode /* = Script::ERROR_THROW */)
{
	Creature* c = popCreature(mode);
	if(c) {
		Player* p = c->getPlayer();
		if(!p) HandleError(mode, "Object is not a player.");
		return p;
	}
	return NULL;
}

Actor* LuaState::popActor(Script::ErrorMode mode /* = Script::ERROR_THROW */)
{
	Creature* c = popCreature(mode);
	if(c) {
		Actor* a = c->getActor();
		if(!a) HandleError(mode, "Object is not an actor.");
		return a;
	}
	return NULL;
}

Item* LuaState::popItem(Script::ErrorMode mode /* = Script::ERROR_THROW */)
{
	Thing* t = popThing(mode);
	if(t) {
		Item* i = t->getItem();
		if(!i) HandleError(mode, "Object is not an item.");
		return i;
	}
	return NULL;
}

Container* LuaState::popContainer(Script::ErrorMode mode /* = Script::ERROR_THROW */)
{
	Item* t = popItem(mode);
	if(t) {
		Container* i = t->getContainer();
		if(!i) HandleError(mode, "Object is not a container.");
		return i;
	}
	return NULL;
}

Condition* LuaState::popCondition(Script::ErrorMode mode /* = Script::ERROR_THROW */)
{
	getMetaObject();

	if(!isUserdata(-1)) {
		HandleError(mode, std::string("Couldn't pop condition, top object is not of valid type (") + luaL_typename(state, -1) + ")");
		pop();
		return NULL;
	}

	Script::ObjectID* objid = (Script::ObjectID*)lua_touserdata(state, -1);
	pop();

	return (Condition*)environment->getObject(*objid);
}

///////////////////////////////////////////////////////////////////////////////
// Lua State Thread

LuaStateManager::LuaStateManager(Script::Manager* man) : LuaState(man)
{
	state = luaL_newstate();
	if(!state){
		throw std::runtime_error("Could not create lua context, fatal error");
	}

	// Load all standard libraries
	luaL_openlibs(state);

	setupLuaStandardLibrary();
}

LuaStateManager::~LuaStateManager() {
	lua_close(state);
}

void LuaStateManager::setupLuaStandardLibrary() {
	//getGlobal("package");
	lua_getfield(state, LUA_GLOBALSINDEX, "package");
	assert(lua_istable(state, 1));
	//pushString(g_config.getString(ConfigManager::DATA_DIRECTORY) + "scripts/?.lua");
	lua_pushstring(state, (g_config.getString(ConfigManager::DATA_DIRECTORY) + "scripts/?.lua").c_str());
	//setField(2, "path");
	lua_setfield(state, -2, "path");

	lua_pop(state, 1);
}

bool LuaStateManager::loadFile(std::string file)
{
	//std::cout << "Loaded file " << file << std::endl;
	//loads file as a chunk at stack top
	int32_t ret = luaL_loadfile(state, file.c_str());

	if(ret != 0){
		std::cout << "Lua Error: " << popString() << std::endl;
		return false;
	}
	//check that it is loaded as a function
	if(lua_isfunction(state, -1) == 0){
		return false;
	}

	//execute it
	// REVSCRIPT TODO a better error handler here
	ret = lua_pcall(state, 0, 0, 0);
	if(ret != 0) {
		std::cout << "Lua Error: Failed to load file " << file << " - " << popString() << std::endl;
		return false;
	}
	return true;
}

bool LuaStateManager::loadDirectory(std::string dir_path)
{
	using namespace boost::filesystem;
	// default construction yields past-the-end
	recursive_directory_iterator end_itr;

	for(recursive_directory_iterator itr(dir_path); itr != end_itr; ++itr){
		std::string s = itr->string();
		s = (s.size() >= 4? s.substr(s.size() - 4) : "");
		if(s == ".lua")
			if(!loadFile(itr->string())) // default construction yields past-the-endath()))
				return false;
	}
	return true;
}

LuaThread_ptr LuaStateManager::newThread(const std::string& name)
{
	LuaThread_ptr p(new LuaThread(manager, name));
	threads[p->state] = p;
	return p;
}

void LuaStateManager::scheduleThread(int32_t schedule, LuaThread_ptr thread)
{
	ThreadSchedule s;
	s.scheduled_time = OTSYS_TIME() + schedule;
	s.thread = thread;
	queued_threads.push(s);
}

void LuaStateManager::runScheduledThreads()
{
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

LuaThread::LuaThread(Script::Manager* manager, const std::string& name) :
	LuaState(manager),
	name(name),
	thread_state(0)
{
	state = lua_newthread(manager->state);
	lua_pop(manager->state, 1); // Remove the thread from the main stack
}

LuaThread::LuaThread(Script::Manager* manager, lua_State* L) :
	LuaState(manager),
	name("Lua generated coroutine"),
	thread_state(0)
{
	state = L;
}

LuaThread::~LuaThread()
{
}

bool LuaThread::ok() const
{
	return thread_state == 0 || thread_state == LUA_YIELD;
}

std::string LuaThread::report(const std::string& extramessage)
{
	std::ostringstream os;
	if(extramessage.empty()){
		// Fetch message from stack
		std::string errmsg = popString();
		os << "Lua Error: " << errmsg << "\n";
	}
	else{
		// Custom message
		os << "Lua Error: " << extramessage << "\n";
	}
	os << "Stack trace:\n";
	os << "Line\tFunction\t\tSource\n";

	lua_Debug ar;

	int32_t level = 0;
	while(lua_getstack(state, level++, &ar) != 0) {
		lua_getinfo(state, "nSl", &ar);

		if(ar.currentline != -1)
			os << ar.currentline;

		os << "\t";

		int32_t tabcount = 20;
		if(!ar.name)
			ar.name = "<unknown>";
		os << ar.name;
		tabcount -= 20 - strlen(ar.name);

		while(tabcount-- > 0)
			os << " ";

		os << ar.short_src;
		os << "\n";
	}

	return os.str();
}

int32_t LuaThread::run(int32_t args)
{
	int32_t ret = lua_resume(state, args);
	thread_state = ret;
	if(ret == LUA_YIELD) {
		// Thread yielded, add us to the manager
		if(!isString() || !(popString() == "WAIT")){
			std::cout << report("Tried to yield from thread with an invalid code.");
			while(getStackSize() > 0)
				pop();
			return 0;
		}

		if(!isNumber()){
			report("Tried to wait a non-number duration.");
			while(getStackSize() > 0)
				pop();
			return 0;
		}

		int32_t schedule = popInteger();
		if(schedule < 1){
			report("Minimum wait duration is 1 milliseconds.");
			while(getStackSize() > 0)
				pop();
			return 0;
		}
		return schedule;
	} else if(ret == 0) {
		// Thread exited normally, do nothing, it will be garbage collected
	} else if(ret == LUA_ERRRUN) {
		std::cout << report();
	} else if(ret == LUA_ERRERR) {
		// Can't handle, just print error message
		std::cout << "Lua Error when recovering from error (thread " << name << ")\n";
	} else if(ret == LUA_ERRMEM) {
		std::cout << "Lua Error: Memory Allocation Failed!\n";
	} else {
		std::cout << "Lua Error: An unknown error occured!\n";
	}
	return 0;
}
