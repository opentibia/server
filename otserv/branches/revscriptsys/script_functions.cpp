/////////////////////////////////////////////////////////////////////
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

#include "lua_manager.h"
#include "script_manager.h"

#include "script_event.h"
#include "script_listener.h"

#include "game.h"
#include "creature.h"

extern Game g_game;

using namespace Script;

///////////////////////////////////////////////////////////////////////////////
// Bit lib declarations

int lua_BitNot(lua_State* L);
int lua_BitAnd(lua_State* L);
int lua_BitOr(lua_State* L);
int lua_BitXor(lua_State* L);
int lua_BitLeftShift(lua_State* L);
int lua_BitRightShift(lua_State* L);
// Unsigned
int lua_BitUNot(lua_State* L);
int lua_BitUAnd(lua_State* L);
int lua_BitUOr(lua_State* L);
int lua_BitUXor(lua_State* L);
int lua_BitULeftShift(lua_State* L);
int lua_BitURightShift(lua_State* L);

const luaL_Reg lua_BitReg[] =
{
	{"bnot", lua_BitNot},
	{"band", lua_BitAnd},
	{"bor", lua_BitOr},
	{"bxor", lua_BitXor},
	{"lshift", lua_BitLeftShift},
	{"rshift", lua_BitRightShift},
	// Unsigned
	{"ubnot", lua_BitUNot},
	{"uband", lua_BitUAnd},
	{"ubor", lua_BitUOr},
	{"ubxor", lua_BitUXor},
	{"ulshift", lua_BitULeftShift},
	{"urshift", lua_BitURightShift},
	{NULL,NULL}
};

///////////////////////////////////////////////////////////////////////////////
// Function list

void Manager::registerClasses() {
	// Event classes
	registerClass("Event");
	registerMemberFunction("Event", "skip()", &Manager::lua_Event_skip);
	registerMemberFunction("Event", "propagate()", &Manager::lua_Event_propagate);
	registerClass("OnSayEvent");

	// Game classes
	registerClass("Thing");
	registerMemberFunction("Thing", "getPosition()", &Manager::lua_Thing_getPosition);
	registerMemberFunction("Thing", "moveTo(table pos)", &Manager::lua_Thing_moveToPosition);

	registerClass("Creature", "Thing");
	registerMemberFunction("Creature", "getHealth()", &Manager::lua_Creature_getHealth);
	registerMemberFunction("Creature", "getHealthMax()", &Manager::lua_Creature_getHealthMax);

	registerMemberFunction("Creature", "getOrientation()", &Manager::lua_Creature_getOrientation);
	registerMemberFunction("Creature", "getDirection()", &Manager::lua_Creature_getOrientation);

	registerClass("Monster", "Creature");

	registerClass("Player", "Creature");
	registerMemberFunction("Player", "setStorageValue(string key, string value)", &Manager::lua_Player_setStorageValue);
	registerMemberFunction("Player", "getStorageValue(string key)", &Manager::lua_Player_getStorageValue);

	registerClass("Item", "Thing");

	registerClass("Teleport", "Item");

	registerClass("Container", "Item");

	registerClass("Tile");
}

void Manager::registerFunctions() {
	registerGlobalFunction("wait(int delay)", &Manager::lua_wait);

	registerGlobalFunction("registerGenericOnSayListener(string method, boolean case_sensitive, string filter, function callback)", &Manager::lua_registerGenericEvent_OnSay);
	registerGlobalFunction("registerSpecificOnSayListener(userdata who, string method, boolean case_sensitive, string filter, function callback)", &Manager::lua_registerSpecificEvent_OnSay);

	registerGlobalFunction("stopListener(string listener_id)", &Manager::lua_stopListener);

	registerGlobalFunction("sendMagicEffect(position where, int type)", &Manager::lua_sendMagicEffect);

	luaL_register(state, "bit", lua_BitReg);
}

///////////////////////////////////////////////////////////////////////////////
// Utility functions

int LuaState::lua_wait() {
	// integer delay is ontop of stack
	return lua_yield(state, 1);
}

///////////////////////////////////////////////////////////////////////////////
// Register Events

int LuaState::lua_registerGenericEvent_OnSay() {
	// Store callback
	insert(-4);

	std::string filter = popString();
	bool case_sensitive = popBoolean();
	std::string method = popString();

	OnSay::ScriptInformation si_onsay;
	if(method == "all") {
		si_onsay.method = OnSay::FILTER_ALL;
	}
	else if(method == "substring") {
		si_onsay.method = OnSay::FILTER_SUBSTRING;
	}
	else if(method == "beginning") {
		si_onsay.method = OnSay::FILTER_MATCH_BEGINNING;
	}
	else if(method == "exact") {
		si_onsay.method = OnSay::FILTER_EXACT;
	}
	else {
		throw Error("Invalid argument (2) 'method'");
	}
	si_onsay.filter = filter;
	si_onsay.case_sensitive = case_sensitive;

	boost::any p(si_onsay);
	Listener_ptr listener(new Listener(ONSAY_LISTENER, p, *this->getManager()));

	enviroment.Generic.OnSay.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_OnSay() {
	// Store callback
	insert(-5);

	std::string filter = popString();
	bool case_sensitive = popBoolean();
	std::string method = popString();
	Creature* who = popCreature();

	// Callback is no the top of the stack

	OnSay::ScriptInformation si_onsay;
	if(method == "all") {
		si_onsay.method = OnSay::FILTER_ALL;
	}
	else if(method == "substring") {
		si_onsay.method = OnSay::FILTER_SUBSTRING;
	}
	else if(method == "beginning") {
		si_onsay.method = OnSay::FILTER_MATCH_BEGINNING;
	}
	else if(method == "exact") {
		si_onsay.method = OnSay::FILTER_EXACT;
	}
	else {
		throw Error("Invalid argument (2) 'method'");
	}
	si_onsay.filter = filter;
	si_onsay.case_sensitive = case_sensitive;

	boost::any p(si_onsay);
	Listener_ptr listener(new Listener(ONSAY_LISTENER, p, *this->getManager()));

	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}


int LuaState::lua_stopListener() {
	std::string listener_id = popString();

	size_t _pos = listener_id.find("_") + 1;
	if(_pos == std::string::npos) {
		throw Error("Invalid argument to stopListener");
	}
	size_t _2pos = listener_id.find("_", _pos);
	if(_2pos == std::string::npos) {
		throw Error("Invalid argument to stopListener");
	}

	std::istringstream is(listener_id.substr(_pos, _2pos));
	uint32_t id, type;
	is >> type;
	if(!is) {
		pushBoolean(false);
		return 1;
	}
	is.str(listener_id.substr(_2pos));
	is >> id;
	if(!is) {
		pushBoolean(false);
		return 1;
	}
	pushBoolean(enviroment.stopListener((ListenerType)type, id));
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Member functions

///////////////////////////////////////////////////////////////////////////////
// Class Event

int LuaState::lua_Event_skip() {
	return 1;
}

int LuaState::lua_Event_propagate() {
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Class Thing

int LuaState::lua_Thing_getPosition() {
	Thing* thing = popThing();
	pushPosition(thing->getPosition());
	return 1;
}

int LuaState::lua_Thing_moveToPosition() {
	Position pos = popPosition();
	Thing* thing = popThing();

	pushBoolean(g_game.internalTeleport(thing, pos) == RET_NOERROR);
	
	return 1;
}

int LuaState::lua_Creature_getHealth() {
	Creature* creature = popCreature();
	pushInteger(creature->getHealth());
	return 1;
}

int LuaState::lua_Creature_getHealthMax() {
	Creature* creature = popCreature();
	pushInteger(creature->getMaxHealth());
	return 1;
}

int LuaState::lua_Creature_getOrientation() {
	Creature* creature = popCreature();
	pushInteger(creature->getDirection());
	return 1;
}

int LuaState::lua_Player_setStorageValue() {
	bool remove = false;
	std::string value;
	if(isNil(-1)) {
		pop();
		remove = true;
	} else {
		value = popString();
	}
	std::string key = popString();
	Player* player = popPlayer();
	
	if(remove)
		player->eraseStorageValue(key);
	else
		player->addStorageValue(key, value);

	pushBoolean(true);
	return 1;
}

int LuaState::lua_Player_getStorageValue() {
	std::string key = popString();
	Player* player = popPlayer();
	std::string value;
	if(player->getStorageValue(key, value)) {
		pushString(value);
	} else {
		pushNil();
	}
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
int LuaState::lua_sendMagicEffect()
{
	//doSendMagicEffect(pos, type)
	uint32_t type = popUnsignedInteger();
	Position pos = popPosition();
	
	g_game.addMagicEffect(pos, type);
	pushBoolean(true);
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Bit definitions

int lua_BitNot(lua_State *L)
{
	int32_t number = (int32_t)lua_tonumber(L, -1);
	lua_pop(L, 1);
	lua_pushnumber(L, ~number);
	return 1;
}

int lua_BitUNot(lua_State *L)
{
	uint32_t number = (uint32_t)lua_tonumber(L, -1);
	lua_pop(L, 1);
	lua_pushnumber(L, ~number);
	return 1;
}

#define MULTIOP(type, name, op) \
	int lua_Bit##name(lua_State *L) { \
	int n = lua_gettop(L); \
	type i, w = (type)lua_tonumber(L, -1); \
	lua_pop(L, 1); \
	for(i = 2; i <= n; ++i){ \
	w op (type)lua_tonumber(L, -1); \
	lua_pop(L, 1); \
	} \
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
	int lua_Bit##name(lua_State *L) { \
	type n2 = (type)lua_tonumber(L, -1), n1 = (type)lua_tonumber(L, -2); \
	lua_pop(L, 2); \
	lua_pushnumber(L, (n1 op n2)); \
	return 1; \
}

SHIFTOP(int32_t, LeftShift, <<)
SHIFTOP(int32_t, RightShift, >>)
SHIFTOP(uint32_t, ULeftShift, <<)
SHIFTOP(uint32_t, URightShift, >>)

