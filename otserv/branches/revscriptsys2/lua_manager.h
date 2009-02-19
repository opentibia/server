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

#ifndef __OTSERV_LUA_MANAGER_H__
#define __OTSERV_LUA_MANAGER_H__

#include <string>
#include <queue>
#include <stdexcept>
#include <map>
#include "boost_common.h"

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

class Player;
class Creature;
class Thing;
class Position;
class PositionEx;
class Town;
class Item;
class Tile;
class House;
class ChatChannel;
class Waypoint;
typedef shared_ptr<Waypoint> Waypoint_ptr;

namespace Script {
	typedef uint64_t ObjectID;
	class Enviroment;
	class Manager;
	class Listener;
	typedef shared_ptr<Listener> Listener_ptr;
	typedef weak_ptr<Listener> Listener_wptr;
	class Event;

	enum ErrorMode {
		ERROR_PASS,
		ERROR_WARN,
		ERROR_THROW,
	};

	class Error : public std::runtime_error {
	public:
		Error(const std::string& msg) : std::runtime_error(msg) {}
	};
}

class LuaStateManager;
class LuaThread;

class LuaState /*abstract*/ {
public:
	LuaState(Script::Enviroment& enviroment);
	virtual ~LuaState();

	// Stack manipulation
	// Returns the size of the stack
	int getStackTop();
	// Checks if the size of the stack is between low and high
	bool checkStackSize(int low, int high = -1);

	// Removes all elements from the stack
	void clearStack();

	// Move value to the given index
	void insert(int idx);
	// Swap top element with a value on the stack
	void swap(int idx);
	// Get the type of the value as a string
	std::string typeOf(int idx = -1);
	// Get the type as a lua string
	int rawtypeOf(int idx = -1);
	// Duplicates the top value at index
	void duplicate(int idx = -1);

	// Table manipulation
	void newTable();

	// Top value as value, index is position of the target table on the stack
	void setField(int index, const std::string& field_name);
	void setField(int index, int field_index);
	template<typename T>
	void setField(int index, const std::string& field_name, const T& t) {
		push(t);
		setField((index <= -10000? index : (index < 0? index - 1 : index)), field_name);
	}
	template<typename T>
	void setField(int index, int field_index, const T& t) {
		push(t);
		setField((index <= -10000? index : (index < 0? index - 1 : index)), field_index);
	}
	// Pushes value onto the stack
	void getField(int index, const std::string& field_name);

	// Top value as value
	void setGlobal(const std::string& gname) {setField(LUA_GLOBALSINDEX, gname);}
	void setRegistryItem(const std::string& rname) {setField(LUA_REGISTRYINDEX, rname);}

	// Pushes value onto the stack
	void getGlobal(const std::string& gname) {getField(LUA_GLOBALSINDEX, gname);}
	void getRegistryItem(const std::string& rname) {getField(LUA_REGISTRYINDEX, rname);}

	// Check
	bool isNil(int index = -1);
	bool isBoolean(int index = -1);
	bool isNumber(int index = -1); // No way to decide if it's double or int
	bool isString(int index = -1);
	bool isUserdata(int index = -1);
	bool isLuaFunction(int index = -1);
	bool isCFunction(int index = -1);
	bool isFunction(int index = -1);
	bool isThread(int index = -1);
	bool isTable(int index = -1);
	// Pop
	void pop(int n = 1);
	bool popBoolean();
	int32_t popInteger();
	uint32_t popUnsignedInteger();
	double popFloat();
	std::string popString();
	Position popPosition(Script::ErrorMode mode = Script::ERROR_THROW);
	void* getUserdata();
	// Push
	void pushNil();
	void pushBoolean(bool b);
	void pushInteger(int32_t i);
	void pushUnsignedInteger(uint32_t ui);
	void pushFloat(double d);
	void pushString(const std::string& str);
	void pushUserdata(void* ptr);
	void pushPosition(const Position& pos);
	void pushPosition(const PositionEx& pos);

	// Events
	void pushEvent(Script::Event& event);
	void pushCallback(Script::Listener_ptr listener);

	// Advanced types
	// Pop
	Thing* popThing(Script::ErrorMode mode = Script::ERROR_THROW);
	Creature* popCreature(Script::ErrorMode mode = Script::ERROR_THROW);
	Player* popPlayer(Script::ErrorMode mode = Script::ERROR_THROW);
	Item* popItem(Script::ErrorMode mode = Script::ERROR_THROW);
	Tile* popTile(Script::ErrorMode mode = Script::ERROR_THROW);
	Town* popTown(Script::ErrorMode mode = Script::ERROR_THROW);
	House* popHouse(Script::ErrorMode mode = Script::ERROR_THROW);
	ChatChannel* popChannel(Script::ErrorMode mode = Script::ERROR_THROW);
	Waypoint_ptr popWaypoint(Script::ErrorMode mode = Script::ERROR_THROW);

	// Push
	void pushThing(Thing* thing);
	void pushTile(Tile* tile);
	void pushTown(Town* town);
	void pushHouse(House* house);
	void pushChannel(ChatChannel* channel);
	void pushWaypoint(Waypoint_ptr pos);


	// Generic
	void push(bool b) {pushBoolean(b);}
	void push(int i) {pushInteger(i);}
	void push(uint32_t ui) {pushUnsignedInteger(ui);}
	void push(double d) {pushFloat(d);}
	void push(const std::string& str) {pushString(str);}
	void push(Thing* thing) {pushThing(thing);}

	// Don't use pushTable on a userdata class and vice-versa (events are table classes, everything else userdata)
	Script::ObjectID* pushClassInstance(const std::string& classname);
	void pushClassTableInstance(const std::string& classname);

	// Might throw depending on the first parameter, second is equivalent to WARN
	void HandleError(Script::ErrorMode mode, const std::string& error);
	void HandleError(const std::string& error);

	//////////////////////////////////////////////////////////////////////////////
	// Classes
	// As C++ does not support partial class definitions, we have to put all lua
	// functions declarations here.
	// *********************************************************************
	// PLEASE document the functions on the otfans.net when adding new ones!
	// *********************************************************************

	// - Utility
	int lua_wait();
	// - Register Events
	int lua_registerGenericEvent_OnSay();
	int lua_registerSpecificEvent_OnSay();
	int lua_registerGenericEvent_OnUseItem();
	int lua_registerGenericEvent_OnJoinChannel();
	int lua_registerSpecificEvent_OnJoinChannel();
	int lua_registerGenericEvent_OnLeaveChannel();
	int lua_registerSpecificEvent_OnLeaveChannel();
	int lua_registerGenericEvent_OnLogin();
	int lua_registerGenericEvent_OnLogout();
	int lua_registerSpecificEvent_OnLogout();
	int lua_registerGenericEvent_OnLookAtItem();
	int lua_registerGenericEvent_OnLookAtCreature();
	int lua_registerSpecificEvent_OnLook();
	int lua_registerGenericEvent_OnEquipItem();
	int lua_registerGenericEvent_OnDeEquipItem();
	int lua_registerGenericEvent_OnStepInCreature();
	int lua_registerGenericEvent_OnStepOutCreature();
	//int lua_registerGenericEvent_OnMoveCreature();
	int lua_registerSpecificEvent_OnMoveCreature();
	int lua_registerGenericEvent_OnMoveItem();
	int lua_registerSpecificEvent_OnCreatureTurn();
	int lua_registerSpecificEvent_OnSpotCreature();
	int lua_registerSpecificEvent_OnLoseCreature();
	int lua_registerGenericEvent_OnCreatureTurn();
	int lua_registerGenericEvent_OnServerLoad();

	int lua_stopListener();


	// - Event
	int lua_Event_skip();
	int lua_Event_propagate();


	// - Thing
	int lua_Thing_getPosition();
	int lua_Thing_getParent();
	int lua_Thing_getParentTile();

	int lua_Thing_isMoveable();
	int lua_Thing_getName();
	int lua_Thing_getDescription();

	int lua_Thing_moveToPosition();
	int lua_Thing_destroy();


	// - - Creature
	int lua_Creature_getID();
	int lua_Creature_getOrientation();
	int lua_Creature_getHealth();
	int lua_Creature_getHealthMax();
	int lua_Creature_setHealth();
	int lua_Creature_getName();
	int lua_Creature_say();

	int lua_Creature_walk();

	int lua_getCreatureByName();
	int lua_getCreaturesByName();

	// - - - Player
	int lua_Player_getFood();
	int lua_Player_getMana();
	int lua_Player_getManaMax();
	int lua_Player_setMana();
	int lua_Player_addManaSpent();
	int lua_Player_getSoulPoints();
	int lua_Player_setSoulPoints();
	int lua_Player_getFreeCap();
	int lua_Player_getMaximumCap();
	int lua_Player_getLevel();
	int lua_Player_getMagicLevel();

	int lua_Player_getSex();
	int lua_Player_getAccess();
	int lua_Player_getGroup();
	int lua_Player_getVocationID();
	int lua_Player_getTownID();
	int lua_Player_getGUID();
	int lua_Player_getPremiumDays();
	int lua_Player_getSkullType();
	int lua_Player_getLastLogin();
	int lua_Player_hasGroupFlag();

	int lua_Player_setStorageValue();
	int lua_Player_getStorageValue();

	int lua_Player_getGuildID();
	int lua_Player_getGuildName();
	int lua_Player_getGuildRank();
	int lua_Player_getGuildNick();

	int lua_Player_getInventoryItem();
	int lua_Player_addItem();
	int lua_Player_getItemTypeCount();

	int lua_Player_setVocation();
	int lua_Player_setTown();
	int lua_Player_addExperience();

	int lua_Player_getMoney();
	int lua_Player_addMoney();
	int lua_Player_removeMoney();

	int lua_Player_sendMessage();

	int lua_getOnlinePlayers();
	int lua_getPlayerByName();
	int lua_getPlayersByName();
	int lua_getPlayerByNameWildcard();
	int lua_getPlayersByNameWildcard();

	// - - Item
	int lua_createItem();
	int lua_getItemIDByName();
	int lua_isValidItemID();

	int lua_Item_getItemID();
	int lua_Item_getActionID();
	int lua_Item_getUniqueID();
	int lua_Item_getCount();
	int lua_Item_getWeight();
	int lua_Item_isPickupable();
	int lua_Item_getSpecialDescription();
	int lua_Item_getText();

	int lua_Item_setItemID();
	int lua_Item_setActionID();
	int lua_Item_setCount();
	int lua_Item_setSpecialDescription();
	int lua_Item_setText();
	int lua_Item_startDecaying();


	// - Tile
	int lua_Tile_getThing();
	int lua_Tile_getCreatures();
	int lua_Tile_getMoveableItems();
	int lua_Tile_getItems();
	int lua_Tile_addItem();
	int lua_Tile_getItemTypeCount();
	int lua_Tile_hasProperty();

	int lua_Tile_isPZ();
	int lua_Tile_isPVP();
	int lua_Tile_isNoPVP();
	int lua_Tile_isNoLogout();
	int lua_Tile_doesRefresh();

	int lua_Tile_queryAdd();

	// - House
	int lua_House_cleanHouse();
	int lua_House_getDoors();
	int lua_House_getExitPosition();
	int lua_House_getID();
	int lua_House_getInvitedList();
	int lua_House_getName();
	int lua_House_getPaidUntil();
	int lua_House_getRent();
	int lua_House_getSubownerList();
	int lua_House_getTiles();
	int lua_House_getTown();
	int lua_House_kickPlayer();
	int lua_House_isInvited();
	int lua_House_setInviteList();
	int lua_House_setOwner();
	int lua_House_setSubownerList();
	int lua_House_setPaidUntil();

	// - Town
	int lua_Town_getTemplePosition();
	int lua_Town_getID();
	int lua_Town_getName();
	int lua_Town_getHouse();
	int lua_Town_getHouses();

	// - Channel
	int lua_Channel_getID();
	int lua_Channel_getName();
	int lua_Channel_getUsers();
	int lua_Channel_addUser();
	int lua_Channel_removeUser();
	int lua_Channel_talk();

	// - Waypoints
	int lua_getWaypointByName();
	int lua_Waypoint_getName();
	int lua_Waypoint_getPosition();

	// - Game
	int lua_sendMagicEffect();
	int lua_getTile();
	int lua_getTowns();
	int lua_getHouses();

protected:
	virtual Script::Manager* getManager() = 0;

	// Members
	lua_State* state;
	Script::Enviroment& enviroment;

	friend class LuaThread;
	friend class LuaStateManager;
	friend class Script::Manager;
};

class LuaThread : public LuaState {
public:
	LuaThread(LuaStateManager& manager, const std::string& name);
	virtual ~LuaThread();

	// Returns time to sleep, 0 if execution ended
	int32_t run(int args);

	bool ok() const;
protected:
	// Prints a stack trace
	void report();

	virtual Script::Manager* getManager();

	LuaStateManager& manager;
	std::string name;
	int thread_state;
};

typedef shared_ptr<LuaThread> LuaThread_ptr;
typedef weak_ptr<LuaThread> LuaThread_wptr;

class LuaStateManager : public LuaState {
public:
	LuaStateManager(Script::Enviroment& enviroment);
	virtual ~LuaStateManager();

	bool loadFile(std::string file);

	LuaThread_ptr newThread(const std::string& name);
	void scheduleThread(int32_t schedule, LuaThread_ptr thread);
	void runScheduledThreads();

	struct ThreadSchedule {
		time_t scheduled_time;
		LuaThread_ptr thread;

		bool operator<(const LuaStateManager::ThreadSchedule& rhs) const {
			return scheduled_time < rhs.scheduled_time;
		}
	};
protected:

	typedef std::map<lua_State*, LuaThread_ptr> ThreadMap;
	ThreadMap threads;
	std::priority_queue<ThreadSchedule> queued_threads;
};

#endif
