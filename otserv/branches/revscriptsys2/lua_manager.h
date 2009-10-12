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

#include "classes.h"
#include "enums.h"
#include "outfit.h"
#include <queue>
#include <stdexcept>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

typedef shared_ptr<Waypoint> Waypoint_ptr;

namespace Script {
	typedef uint64_t ObjectID;
	class Environment;
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
	LuaState(Script::Manager* manager);
	virtual ~LuaState();

	// Stack manipulation
	// Returns the size of the stack
	int32_t getStackSize();
	// Checks if the size of the stack is between low and high
	bool checkStackSize(int32_t low, int32_t high = -1);

	// Removes all elements from the stack
	void clearStack();

	// Move value to the given index
	void insert(int32_t idx);
	// Swap top element with a value on the stack
	void swap(int32_t idx);
	// Get the type of the value as a string
	std::string typeName(int32_t idx = -1);
	// Get the type as a lua string
	int32_t rawtype(int32_t idx = -1);
	// Duplicates the top value at index
	void duplicate(int32_t idx = -1);

	// Table manipulation
	void newTable();

	// Top value as value, index is position of the target table on the stack
	void setField(int32_t index, const std::string& field_name);
	void setField(int32_t index, int32_t field_index);
	template<typename T>
	void setField(int32_t index, const std::string& field_name, const T& t) {
		push(t);
		setField((index <= -10000? index : (index < 0? index - 1 : index)), field_name);
	}
	template<typename T>
	void setField(int32_t index, int32_t field_index, const T& t) {
		push(t);
		setField((index <= -10000? index : (index < 0? index - 1 : index)), field_index);
	}
	// Pushes value onto the stack
	void getField(int32_t index, const std::string& field_name);
	void getField(int32_t index, int field_index);
	
	// Iterator over table
	// Make sure to push nil before calling
	bool iterateTable(int32_t index);


	// Top value as value
	void setGlobal(const std::string& gname) {setField(LUA_GLOBALSINDEX, gname);}
	void setRegistryItem(const std::string& rname) {setField(LUA_REGISTRYINDEX, rname);}

	// Pushes value onto the stack
	void getGlobal(const std::string& gname) {getField(LUA_GLOBALSINDEX, gname);}
	void getRegistryItem(const std::string& rname) {getField(LUA_REGISTRYINDEX, rname);}

	// Check
	bool isNil(int32_t index = -1);
	bool isBoolean(int32_t index = -1);
	bool isNumber(int32_t index = -1); // No way to decide if it's double or int
	bool isString(int32_t index = -1);
	bool isUserdata(int32_t index = -1);
	bool isLuaFunction(int32_t index = -1);
	bool isCFunction(int32_t index = -1);
	bool isFunction(int32_t index = -1);
	bool isThread(int32_t index = -1);
	bool isTable(int32_t index = -1);
	// Pop
	void pop(int n = 1);
	bool popBoolean();
	int32_t popInteger();
	uint32_t popUnsignedInteger();
	double popFloat();
	std::string popString();
	Position popPosition(Script::ErrorMode mode = Script::ERROR_THROW);
	void* getUserdata();

	template <typename T> T popValue();
#ifndef __GNUC__
	template <> bool popValue<bool>();
	template <> uint32_t popValue<uint32_t>();
	template <> int32_t popValue<int32_t>();
	template <> float popValue<float>();
	template <> double popValue<double>();
	template <> std::string popValue<std::string>();
	template <> uint64_t popValue<uint64_t>();
#endif

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
	Actor* popActor(Script::ErrorMode mode = Script::ERROR_THROW);
	Item* popItem(Script::ErrorMode mode = Script::ERROR_THROW);
	Container* popContainer(Script::ErrorMode mode = Script::ERROR_THROW);
	Tile* popTile(Script::ErrorMode mode = Script::ERROR_THROW);
	Town* popTown(Script::ErrorMode mode = Script::ERROR_THROW);
	House* popHouse(Script::ErrorMode mode = Script::ERROR_THROW);
	ChatChannel* popChannel(Script::ErrorMode mode = Script::ERROR_THROW);
	Waypoint_ptr popWaypoint(Script::ErrorMode mode = Script::ERROR_THROW);
	OutfitType popOutfit(Script::ErrorMode mode = Script::ERROR_THROW);
	template <class ET> ET popEnum(Script::ErrorMode mode = Script::ERROR_THROW);

	// Push
	void pushThing(Thing* thing);
	void pushTile(Tile* tile);
	void pushTown(Town* town);
	void pushHouse(House* house);
	void pushChannel(ChatChannel* channel);
	void pushWaypoint(Waypoint_ptr pos);
	void pushOutfit(const OutfitType& outfit);
	template<class E, int size_>
	void pushEnum(const Enum<E, size_>&);


	// Generic
	void push(bool b) {pushBoolean(b);}
	void push(int32_t i) {pushInteger(i);}
	void push(uint32_t ui) {pushUnsignedInteger(ui);}
	void push(uint64_t ui) {pushFloat((double)ui);}
	void push(double d) {pushFloat(d);}
	void push(const std::string& str) {pushString(str);}
	void push(Thing* thing) {pushThing(thing);}
	void push(const Position& pos) {pushPosition(pos);}
	#ifndef __GNUC__
	void push(int i) {pushInteger(i);}
	#endif

	// Don't use pushTable on a userdata class and vice-versa (events are table classes, everything else userdata)
	Script::ObjectID* pushClassInstance(const std::string& classname);
	void pushClassTableInstance(const std::string& classname);

	// This fixes the __object metamethod
	// If top object is table, and has __object member, pop table and push the __object member
	void getMetaObject();

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
	int lua_require_directory();
	int lua_get_thread_id();
	int lua_stacktrace();

	int lua_getConfigValue();
	// - Register Events
	int lua_registerGenericEvent_OnSay();
	int lua_registerSpecificEvent_OnSay();
	int lua_registerSpecificEvent_OnHear();
	int lua_registerGenericEvent_OnUseItem();
	int lua_registerGenericEvent_OnJoinChannel();
	int lua_registerSpecificEvent_OnJoinChannel();
	int lua_registerGenericEvent_OnLeaveChannel();
	int lua_registerSpecificEvent_OnLeaveChannel();
	int lua_registerGenericEvent_OnLogin();
	int lua_registerGenericEvent_OnLogout();
	int lua_registerSpecificEvent_OnLogout();

	int lua_registerGenericEvent_OnChangeOutfit();
	int lua_registerSpecificEvent_OnChangeOutfit();

	int lua_registerGenericEvent_OnLookAtItem();
	int lua_registerGenericEvent_OnLookAtCreature();
	int lua_registerSpecificEvent_OnLook();
	int lua_registerGenericEvent_OnEquipItem();
	int lua_registerGenericEvent_OnDeEquipItem();
	int lua_registerSpecificEvent_CreatureMove();
	int lua_registerSpecificEvent_CreatureMoveIn();
	int lua_registerSpecificEvent_CreatureMoveOut();
	int lua_registerGenericEvent_CreatureMoveIn();
	int lua_registerGenericEvent_CreatureMoveOut();
	int lua_registerGenericEvent_OnAdvance();
	int lua_registerSpecificEvent_OnAdvance();
	int lua_registerGenericEvent_OnMoveItem();
	int lua_registerGenericEvent_OnSpawn();
	int lua_registerSpecificEvent_OnCreatureTurn();
	int lua_registerSpecificEvent_OnSpotCreature();
	int lua_registerSpecificEvent_OnLoseCreature();
	int lua_registerSpecificEvent_OnCreatureThink();
	int lua_registerGenericEvent_OnCreatureTurn();
	int lua_registerGenericEvent_OnServerLoad();
	int lua_registerGenericEvent_OnCondition();
	int lua_registerSpecificEvent_OnCondition();
	int lua_registerGenericEvent_OnAttack();
	int lua_registerSpecificEvent_OnAttack();
	int lua_registerGenericEvent_OnDamage();
	int lua_registerSpecificEvent_OnDamage();
	int lua_registerGenericEvent_OnKill();
	int lua_registerSpecificEvent_OnKill();
	int lua_registerGenericEvent_OnKilled();
	int lua_registerSpecificEvent_OnKilled();
	int lua_registerGenericEvent_OnDeathBy();
	int lua_registerSpecificEvent_OnDeathBy();
	int lua_registerGenericEvent_OnDeath();
	int lua_registerSpecificEvent_OnDeath();

	int lua_stopListener();


	// - Event
	int lua_Event_skip();
	int lua_Event_propagate();


	// - Thing
	int lua_Thing_getPosition();
	int lua_Thing_getX();
	int lua_Thing_getY();
	int lua_Thing_getZ();
	int lua_Thing_getParent();
	int lua_Thing_getParentTile();

	int lua_Thing_isMoveable();
	int lua_Thing_getName();
	int lua_Thing_getDescription();

	int lua_Thing_moveToPosition();
	int lua_Thing_destroy();

	int lua_getThingByID();

	// - - Creature
	int lua_Creature_getID();
	int lua_Creature_getOrientation();
	int lua_Creature_getHealth();
	int lua_Creature_getHealthMax();
	int lua_Creature_setHealth();
	int lua_Creature_getName();
	int lua_Creature_getNameDescription();
	int lua_Creature_getOutfit();
	int lua_Creature_say();
	int lua_Creature_setOutfit();
	int lua_Creature_walk();
	int lua_Creature_addSummon();
	int lua_Creature_getSpeed();
	int lua_Creature_getArmor();
	int lua_Creature_getDefense();
	int lua_Creature_getBaseSpeed();
	int lua_Creature_isPushable();
	int lua_Creature_getTarget();
	int lua_Creature_isImmuneToCombat();
	int lua_Creature_getDamageImmunities();
	int lua_Creature_isImmuneToMechanic();
	int lua_Creature_getMechanicImmunities();

	int lua_Creature_setRawCustomValue();
	int lua_Creature_getRawCustomValue();

	int lua_getCreatureByName();
	int lua_getCreaturesByName();

	// - - - Actor
	int lua_createMonster();
	int lua_createActor();

	int lua_Actor_setShouldReload();
	int lua_Actor_setAlwaysThink();
	int lua_Actor_getShouldReload();
	int lua_Actor_getAlwaysThink();

	// Actor type changes
	int lua_Actor_setArmor();
	int lua_Actor_setDefense();
	int lua_Actor_setExperienceWorth();
	int lua_Actor_getExperienceWorth();
	int lua_Actor_setCanPushItems();
	int lua_Actor_getCanPushItems();
	int lua_Actor_getCanPushCreatures();
	int lua_Actor_setCanPushCreatures();
	int lua_Actor_setSpeed();
	int lua_Actor_setTargetDistance();
	int lua_Actor_getTargetDistance();
	int lua_Actor_setMaxSummons();
	int lua_Actor_getMaxSummons();
	int lua_Actor_setName();
	int lua_Actor_setNameDescription();
	int lua_Actor_setStaticAttackChance();
	int lua_Actor_getStaticAttackChance();
	int lua_Actor_setFleeHealth();
	int lua_Actor_getFleeHealth();
	int lua_Actor_setPushable();
	int lua_Actor_setBaseSpeed();
	int lua_Actor_setMaxHealth();
	int lua_Actor_getCorpseId();
	int lua_Actor_setRace();
	int lua_Actor_getRace();
	int lua_Actor_isSummonable();
	int lua_Actor_isConvinceable();
	int lua_Actor_isIllusionable();
	int lua_Actor_setCanBeAttacked();
	int lua_Actor_getCanBeAttacked();
	int lua_Actor_setCanBeLured();
	int lua_Actor_getCanBeLured();
	int lua_Actor_setLightLevel();
	int lua_Actor_getLightLevel();
	int lua_Actor_setLightColor();
	int lua_Actor_getLightColor();
	int lua_Actor_getManaCost();
	int lua_Actor_setTarget();
	int lua_Actor_setMechanicImmunities();
	int lua_Actor_setDamageImmunities();

	/*
	TODO:
	SummonList summonList;
	LootItems lootItems;
	ElementMap elementMap;
	SpellList spellAttackList;
	SpellList spellDefenseList;

	uint32_t yellChance;
	uint32_t yellSpeedTicks;
	VoiceVector voiceVector;

	int32_t changeTargetSpeed;
	int32_t changeTargetChance;
	*/

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
	int lua_Player_isPremium();
	int lua_Player_getCorpseId();

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

	int lua_Player_getGuildID();
	int lua_Player_getGuildName();
	int lua_Player_getGuildRank();
	int lua_Player_getGuildNick();

	int lua_Player_getInventoryItem();
	int lua_Player_addItem();
	int lua_Player_removeItem();
	int lua_Player_getItemTypeCount();

	int lua_Player_setVocation();
	int lua_Player_setTown();
	int lua_Player_addExperience();

	int lua_Player_countMoney();
	int lua_Player_addMoney();
	int lua_Player_removeMoney();

	int lua_Player_sendMessage();

	int lua_getOnlinePlayers();
	int lua_getPlayerByName();
	int lua_getPlayersByName();
	int lua_getPlayerByNameWildcard();
	int lua_getPlayersByNameWildcard();

	int lua_Player_canUseSpell();
	int lua_Player_learnSpell();
	int lua_Player_unlearnSpell();

	// - - Item
	int lua_createItem();
	int lua_getItemIDByName();
	int lua_getItemType();
	int lua_isValidItemID();

	int lua_Item_getItemID();
	int lua_Item_getLongName();
	int lua_Item_getCount();
	int lua_Item_getWeight();
	int lua_Item_isPickupable();
	int lua_Item_getSubtype();

	int lua_Item_setItemID();
	int lua_Item_setCount();
	int lua_Item_setSubtype();
	int lua_Item_startDecaying();

	int lua_Item_setStringAttribute();
	int lua_Item_setIntegerAttribute();
	int lua_Item_setFloatAttribute();
	int lua_Item_setBooleanAttribute();
	int lua_Item_getRawAttribute();
	int lua_Item_eraseAttribute();

	// - Container
	int lua_Container_addItem();
	int lua_Container_getItem();
	int lua_Container_getSize();
	int lua_Container_getCapacity();
	int lua_Container_getItems();
	int lua_Container_getContentDescription();

	// - Tile
	int lua_Tile_getThing();
	int lua_Tile_getCreatures();
	int lua_Tile_getMoveableItems();
	int lua_Tile_getItems();
	int lua_Tile_getItemsWithActionID();
	int lua_Tile_getItemsWithItemID();
	int lua_Tile_getItemWithActionID();
	int lua_Tile_getItemWithItemID();
	int lua_Tile_addItem();
	int lua_Tile_getItemTypeCount();
	int lua_Tile_hasProperty();
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
	int lua_sendDistanceEffect();
	int lua_sendAnimatedText();
	int lua_getTile();
	int lua_getTowns();
	int lua_sendMailTo();
	int lua_setGlobalValue();
	int lua_getGlobalValue();
	int lua_getHouses();
	int lua_getWorldType();
	int lua_getWorldTime();
	int lua_getWorldUpTime();

protected:
	// Members
	lua_State* state;
	Script::Manager* manager;
	Script::Environment* environment;

	friend class LuaThread;
	friend class LuaStateManager;
	friend class Script::Manager;
};

template <> inline bool LuaState::popValue<bool>() {return popBoolean();}
template <> inline uint32_t LuaState::popValue<uint32_t>() {return popUnsignedInteger();}
template <> inline int32_t LuaState::popValue<int32_t>() {return (int32_t)popInteger();}
template <> inline float LuaState::popValue<float>() {return popFloat();}
template <> inline double LuaState::popValue<double>() {return popFloat();}
template <> inline std::string LuaState::popValue<std::string>() {return popString();}
template <> inline uint64_t LuaState::popValue<uint64_t>() {return (uint64_t)popFloat();}

template <class ET> inline ET LuaState::popEnum(Script::ErrorMode mode){
	if(!isTable(-1)){
		HandleError(mode, "Can not construct enum from " + typeName());
	}
	else{
		getField(-1, "__intValue");
		int i = popInteger();
		pop(); // pop enum value
		return ET(i);
	}
	return ET();
}

template<class E, int size_>
inline void LuaState::pushEnum(const Enum<E, size_>& e){
	getGlobal(e.toString());
}


class LuaThread : public LuaState {
public:
	LuaThread(Script::Manager* manager, const std::string& name);
	LuaThread(Script::Manager* manager, lua_State* L);
	virtual ~LuaThread();

	// Returns time to sleep, 0 if execution ended
	int32_t run(int32_t args);

	bool ok() const;

	// Returns a sweetly formatted stack trace
	std::string report(const std::string& extramessage = "");
protected:
	std::string name;
	int32_t thread_state;
};

typedef shared_ptr<LuaThread> LuaThread_ptr;
typedef weak_ptr<LuaThread> LuaThread_wptr;

class LuaStateManager : public LuaState {
public:
	LuaStateManager(Script::Manager* man);
	virtual ~LuaStateManager();

	bool loadFile(std::string file);
	bool loadDirectory(std::string dir);
	void setupLuaStandardLibrary();

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
