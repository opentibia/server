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
#include "configmanager.h"
#include "housetile.h"
#include "player.h"
#include "ioplayer.h"
#include "game.h"
#include "actor.h"
#include "town.h"
#include "chat.h"
#include "house.h"
#include "spawn.h"
#include "vocation.h"
#include "status.h"

extern ConfigManager g_config;
extern Game g_game;
extern Vocations g_vocations;

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
	// Enums
	registerClass("Enum");
	registerEnum<RaceType>();
	registerEnum<Direction>();
	registerEnum<CombatType>();
	registerEnum<CombatParam>();
	registerEnum<CallBackParam>();
	registerEnum<ConditionParam>();
	registerEnum<BlockType>();
	registerEnum<ViolationAction>();
	registerEnum<SkillType>();
	registerEnum<LevelType>();
	registerEnum<PlayerStatType>();
	registerEnum<LossType>();
	registerEnum<FormulaType>();
	registerEnum<ConditionID>();
	registerEnum<PlayerSex>();
	registerEnum<ChaseMode>();
	registerEnum<FightMode>();
	registerEnum<TradeState>();
	registerEnum<SlotType>();
	registerEnum<SlotPosition>();
	registerEnum<TileProp>();
	registerEnum<ZoneType>();
	registerEnum<WorldType>();
	registerEnum<Script::ListenerType>();
	
	registerEnum<ConditionType>();
	registerEnum<ConditionEnd>();
	registerEnum<ConditionAttribute>();

	// Classes...
	registerClass("Event");
	registerClass("OnSayEvent", "Event");
	registerClass("OnUseItemEvent", "Event");
	registerClass("OnJoinChannelEvent", "Event");
	registerClass("OnLeaveChannelEvent", "Event");
	registerClass("OnLoginEvent", "Event");
	registerClass("OnLogoutEvent", "Event");
	registerClass("OnLookEvent", "Event");
	registerClass("OnTurnEvent", "Event");
	registerClass("OnServerLoadEvent", "Event");
	registerClass("OnServerUnloadEvent", "Event");
	registerClass("OnSpotCreatureEvent", "Event");
	registerClass("OnLoseCreatureEvent", "Event");
	registerClass("OnSpawnEvent", "Event");
	registerClass("OnThinkEvent", "Event");
	registerClass("OnAdvanceEvent", "Event");
	registerClass("OnKillEvent", "Event");
	registerClass("OnDeathEvent", "Event");

	registerClass("Thing");
	registerClass("Creature", "Thing");
	registerClass("Actor", "Creature");
	registerClass("Player", "Creature");
	registerClass("Item", "Thing");
	registerClass("Teleport", "Item");
	registerClass("Container", "Item");

	registerClass("Tile");

	registerClass("Town");
	registerClass("House");
	registerClass("Waypoint");

	registerClass("Channel");

	// Event classes
	registerMemberFunction("Event", "skip()", &Manager::lua_Event_skip);
	registerMemberFunction("Event", "propagate()", &Manager::lua_Event_propagate);

	// Game classes
	registerMemberFunction("Thing", "getPosition()", &Manager::lua_Thing_getPosition);
	registerMemberFunction("Thing", "getX()", &Manager::lua_Thing_getX);
	registerMemberFunction("Thing", "getY()", &Manager::lua_Thing_getY);
	registerMemberFunction("Thing", "getZ()", &Manager::lua_Thing_getZ);
	registerMemberFunction("Thing", "getParentTile()", &Manager::lua_Thing_getParentTile);
	registerMemberFunction("Thing", "getParent()", &Manager::lua_Thing_getParent);
	registerMemberFunction("Thing", "isMoveable()", &Manager::lua_Thing_isMoveable);
	registerMemberFunction("Thing", "getPosition()", &Manager::lua_Thing_getPosition);
	registerMemberFunction("Thing", "getName()", &Manager::lua_Thing_getName);
	registerMemberFunction("Thing", "getDescription([int lookdistance])", &Manager::lua_Thing_getDescription);
	registerMemberFunction("Thing", "moveTo(table pos)", &Manager::lua_Thing_moveToPosition);
	registerMemberFunction("Thing", "destroy()", &Manager::lua_Thing_destroy);
	
	registerGlobalFunction("getThingByID(int id)", &Manager::lua_getThingByID);

	// Creature
	registerMemberFunction("Creature", "setRawCustomValue(string key, mixed value)", &Manager::lua_Creature_setRawCustomValue);
	registerMemberFunction("Creature", "getRawCustomValue(string key)", &Manager::lua_Creature_getRawCustomValue);

	registerMemberFunction("Creature", "getDirection()", &Manager::lua_Creature_getOrientation);
	registerMemberFunction("Creature", "getHealth()", &Manager::lua_Creature_getHealth);
	registerMemberFunction("Creature", "getHealthMax()", &Manager::lua_Creature_getHealthMax);
	registerMemberFunction("Creature", "setHealth(integer newval)", &Manager::lua_Creature_setHealth);
	registerMemberFunction("Creature", "getID()", &Manager::lua_Creature_getID);
	registerMemberFunction("Creature", "getOrientation()", &Manager::lua_Creature_getOrientation);
	registerMemberFunction("Creature", "getOutfit()", &Manager::lua_Creature_getOutfit);
	registerMemberFunction("Creature", "say(string msg)", &Manager::lua_Creature_say);
	registerMemberFunction("Creature", "setOutfit(table outfit)", &Manager::lua_Creature_setOutfit);
	registerMemberFunction("Creature", "walk(Direction direction)", &Manager::lua_Creature_walk);
	registerMemberFunction("Creature", "getSpeed()", &Manager::lua_Creature_getSpeed);
	registerMemberFunction("Creature", "getArmor()", &Manager::lua_Creature_getArmor);
	registerMemberFunction("Creature", "getDefense()", &Manager::lua_Creature_getDefense);
	registerMemberFunction("Creature", "getBaseSpeed()", &Manager::lua_Creature_getBaseSpeed);
	registerMemberFunction("Creature", "isPushable()", &Manager::lua_Creature_isPushable);
	registerMemberFunction("Creature", "getTarget()", &Manager::lua_Creature_getTarget);
	registerMemberFunction("Creature", "isImmuneToCondition(ConditionType conditiontype)", &Manager::lua_Creature_isImmuneToCondition);
	registerMemberFunction("Creature", "isImmuneToCombat(CombatType combattype)", &Manager::lua_Creature_isImmuneToCombat);
	registerMemberFunction("Creature", "getConditionImmunities()", &Manager::lua_Creature_getConditionImmunities);
	registerMemberFunction("Creature", "getDamageImmunities()", &Manager::lua_Creature_getDamageImmunities);

	registerGlobalFunction("getCreatureByName(string name)", &Manager::lua_getCreatureByName);
	registerGlobalFunction("getCreaturesByName(string name)", &Manager::lua_getCreaturesByName);

	// Actor
	registerMemberFunction("Actor", "setShouldReload(boolean shouldreload)", &Manager::lua_Actor_setShouldReload);
	registerMemberFunction("Actor", "getShouldReload()", &Manager::lua_Actor_getShouldReload);
	registerMemberFunction("Actor", "setAlwaysThink(boolean shouldreload)", &Manager::lua_Actor_setAlwaysThink);
	registerMemberFunction("Actor", "getAlwaysThink()", &Manager::lua_Actor_getAlwaysThink);

	registerMemberFunction("Actor", "setArmor(int newarmor)", &Manager::lua_Actor_setArmor);
	registerMemberFunction("Actor", "setDefense(int newdefense)", &Manager::lua_Actor_setDefense);
	registerMemberFunction("Actor", "setExperienceWorth(int newexp)", &Manager::lua_Actor_setExperienceWorth);
	registerMemberFunction("Actor", "getExperienceWorth()", &Manager::lua_Actor_getExperienceWorth);
	registerMemberFunction("Actor", "setCanPushItems(boolean canpushitems)", &Manager::lua_Actor_setCanPushItems);
	registerMemberFunction("Actor", "getCanPushItems()", &Manager::lua_Actor_getCanPushItems);
	registerMemberFunction("Actor", "setCanPushCreatures(boolean canpushcreatures)", &Manager::lua_Actor_setCanPushCreatures);
	registerMemberFunction("Actor", "getCanPushCreatures()", &Manager::lua_Actor_getCanPushCreatures);
	registerMemberFunction("Actor", "setSpeed(int newspeed)", &Manager::lua_Actor_setSpeed);
	registerMemberFunction("Actor", "setTargetDistance(int newtargetdistance)", &Manager::lua_Actor_setTargetDistance);
	registerMemberFunction("Actor", "getTargetDistance()", &Manager::lua_Actor_getTargetDistance);
	registerMemberFunction("Actor", "setMaxSummons(int newsummons)", &Manager::lua_Actor_setMaxSummons);
	registerMemberFunction("Actor", "getMaxSummons()", &Manager::lua_Actor_getMaxSummons);
	registerMemberFunction("Actor", "setName(string newname)", &Manager::lua_Actor_setName);
	registerMemberFunction("Actor", "setNameDescription(string newname)", &Manager::lua_Actor_setNameDescription);
	registerMemberFunction("Actor", "setStaticAttackChance(int chance)", &Manager::lua_Actor_setStaticAttackChance);
	registerMemberFunction("Actor", "getStaticAttackChance()", &Manager::lua_Actor_getStaticAttackChance);
	registerMemberFunction("Actor", "setFleeHealth(int health)", &Manager::lua_Actor_setFleeHealth);
	registerMemberFunction("Actor", "getFleeHealth()", &Manager::lua_Actor_getFleeHealth);
	registerMemberFunction("Actor", "setPushable(bool pushable)", &Manager::lua_Actor_setPushable);
	registerMemberFunction("Actor", "setBaseSpeed(int newbasespeed)", &Manager::lua_Actor_setBaseSpeed);
	registerMemberFunction("Actor", "setMaxHealth(int newmaxhealth)", &Manager::lua_Actor_setMaxHealth);
	registerMemberFunction("Actor", "getCorpseId()", &Manager::lua_Actor_getCorpseId);
	registerMemberFunction("Actor", "setRace(int racetype)", &Manager::lua_Actor_setRace);
	registerMemberFunction("Actor", "getRace()", &Manager::lua_Actor_getRace);
	registerMemberFunction("Actor", "isSummonable()", &Manager::lua_Actor_isSummonable);
	registerMemberFunction("Actor", "isConvinceable()", &Manager::lua_Actor_isConvinceable);
	registerMemberFunction("Actor", "isIllusionable()", &Manager::lua_Actor_isIllusionable);
	registerMemberFunction("Actor", "setCanBeAttacked(boolean isAttackable)", &Manager::lua_Actor_setCanBeAttacked);
	registerMemberFunction("Actor", "getCanBeAttacked()", &Manager::lua_Actor_getCanBeAttacked);
	registerMemberFunction("Actor", "setCanBeLured(boolean isLureable)", &Manager::lua_Actor_setCanBeLured);
	registerMemberFunction("Actor", "getCanBeLured()", &Manager::lua_Actor_getCanBeLured);
	registerMemberFunction("Actor", "setLightLevel(int newlightlevel)", &Manager::lua_Actor_setLightLevel);
	registerMemberFunction("Actor", "getLightLevel()", &Manager::lua_Actor_getLightLevel);
	registerMemberFunction("Actor", "setLightColor(int newlightcolor)", &Manager::lua_Actor_setLightColor);
	registerMemberFunction("Actor", "getLightColor()", &Manager::lua_Actor_getLightColor);
	registerMemberFunction("Actor", "getManaCost()", &Manager::lua_Actor_getManaCost);
	registerMemberFunction("Actor", "setTarget(Creature target)", &Manager::lua_Actor_setTarget);
	registerMemberFunction("Actor", "setConditionImmunities(table immunities)", &Manager::lua_Actor_setConditionImmunities);
	registerMemberFunction("Actor", "setDamageImmunities(table immunities)", &Manager::lua_Actor_setDamageImmunities);

	registerGlobalFunction("createMonster(string monstertypename, table pos)", &Manager::lua_createMonster);
	registerGlobalFunction("createActor(string name, table pos)", &Manager::lua_createActor);

	// Player
	registerMemberFunction("Player", "getFood()", &Manager::lua_Player_getFood);
	registerMemberFunction("Player", "getMana()", &Manager::lua_Player_getMana);
	registerMemberFunction("Player", "getLevel()", &Manager::lua_Player_getLevel);
	registerMemberFunction("Player", "getMagicLevel()", &Manager::lua_Player_getMagicLevel);
	registerMemberFunction("Player", "isPremium()", &Manager::lua_Player_isPremium);
	registerMemberFunction("Player", "getManaMax()", &Manager::lua_Player_getManaMax);
	registerMemberFunction("Player", "setMana(integer newval)", &Manager::lua_Player_setMana);
	registerMemberFunction("Player", "addManaSpent(integer howmuch)", &Manager::lua_Player_addManaSpent);
	registerMemberFunction("Player", "getSoulPoints()", &Manager::lua_Player_getSoulPoints);
	registerMemberFunction("Player", "setSoulPoints()", &Manager::lua_Player_setSoulPoints);
	registerMemberFunction("Player", "getFreeCap()", &Manager::lua_Player_getFreeCap);
	registerMemberFunction("Player", "getMaximumCap()", &Manager::lua_Player_getMaximumCap);
	registerMemberFunction("Player", "getSex()", &Manager::lua_Player_getSex);
	registerMemberFunction("Player", "getAccess()", &Manager::lua_Player_getAccess);
	registerMemberFunction("Player", "getVocationID()", &Manager::lua_Player_getVocationID);
	registerMemberFunction("Player", "getTownID()", &Manager::lua_Player_getTownID);
	registerMemberFunction("Player", "getGUID()", &Manager::lua_Player_getGUID);
	registerMemberFunction("Player", "getAccessGroup()", &Manager::lua_Player_getGroup);
	registerMemberFunction("Player", "getPremiumDays()", &Manager::lua_Player_getPremiumDays);
	registerMemberFunction("Player", "getSkull()", &Manager::lua_Player_getSkullType);
	registerMemberFunction("Player", "getLastLogin()", &Manager::lua_Player_getLastLogin);
	registerMemberFunction("Player", "getGuildID()", &Manager::lua_Player_getGuildID);
	registerMemberFunction("Player", "getGuildName()", &Manager::lua_Player_getGuildName);
	registerMemberFunction("Player", "getGuildRank()", &Manager::lua_Player_getGuildRank);
	registerMemberFunction("Player", "getGuildNick()", &Manager::lua_Player_getGuildNick);

	registerMemberFunction("Player", "addItem(Item item)", &Manager::lua_Player_addItem);
	registerMemberFunction("Player", "removeItem(int id [, int type [,int count]])", &Manager::lua_Player_removeItem);
	registerMemberFunction("Player", "getInventoryItem(SlotType slot)", &Manager::lua_Player_getInventoryItem);
	registerMemberFunction("Player", "addExperience(int experience)", &Manager::lua_Player_addExperience);
	registerMemberFunction("Player", "setTown(Town town)", &Manager::lua_Player_setTown);
	registerMemberFunction("Player", "setVocation(int vocationid)", &Manager::lua_Player_setVocation);
	registerMemberFunction("Player", "hasGroupFlag(integer flag)", &Manager::lua_Player_hasGroupFlag);

	registerMemberFunction("Player", "countMoney()", &Manager::lua_Player_countMoney);
	registerMemberFunction("Player", "addMoney(int amount)", &Manager::lua_Player_addMoney);
	registerMemberFunction("Player", "removeMoney(int amount)", &Manager::lua_Player_removeMoney);

	registerMemberFunction("Player", "sendMessage(int type, string msg)", &Manager::lua_Player_sendMessage);

	registerMemberFunction("Player", "canUseSpell(string spellname)", &Manager::lua_Player_canUseSpell);
	registerMemberFunction("Player", "learnSpell(string spellname)", &Manager::lua_Player_learnSpell);
	registerMemberFunction("Player", "unlearnSpell(string spellname)", &Manager::lua_Player_unlearnSpell);

	registerGlobalFunction("getOnlinePlayers()", &Manager::lua_getOnlinePlayers);
	registerGlobalFunction("getPlayerByName(string name)", &Manager::lua_getPlayerByName);
	registerGlobalFunction("getPlayerByNameWildcard(string wild)", &Manager::lua_getPlayerByNameWildcard);
	registerGlobalFunction("getPlayersByName(string name)", &Manager::lua_getPlayersByName);
	registerGlobalFunction("getPlayersByNameWildcard(string wild)", &Manager::lua_getPlayersByNameWildcard);

	// Item
	registerGlobalFunction("createItem(int newid[, int count = nil])", &Manager::lua_createItem);
	registerMemberFunction("Item", "getItemID()", &Manager::lua_Item_getItemID);
	registerMemberFunction("Item", "getLongName()", &Manager::lua_Item_getLongName);
	registerMemberFunction("Item", "getCount()", &Manager::lua_Item_getCount);
	registerMemberFunction("Item", "getWeight()", &Manager::lua_Item_getWeight);
	registerMemberFunction("Item", "isPickupable()", &Manager::lua_Item_isPickupable);
	registerMemberFunction("Item", "getSubtype()", &Manager::lua_Item_getSubtype);

	registerMemberFunction("Item", "setItemID(int newid [, int newtype])", &Manager::lua_Item_setItemID);
	registerMemberFunction("Item", "setCount(int newcount)", &Manager::lua_Item_setCount);
	registerMemberFunction("Item", "startDecaying()", &Manager::lua_Item_startDecaying);
	registerMemberFunction("Item", "setSubtype(int newtype)", &Manager::lua_Item_setSubtype);

	registerMemberFunction("Item", "getRawAttribute(string key)", &Manager::lua_Item_getRawAttribute);
	registerMemberFunction("Item", "eraseAttribute(string key)", &Manager::lua_Item_eraseAttribute);
	// We expose type-safe versions as type is important for internal state
	// The generic setAttribute is defined in item.lua
	registerMemberFunction("Item", "setStringAttribute(string key, string value)", &Manager::lua_Item_setStringAttribute);
	registerMemberFunction("Item", "setIntegerAttribute(string key, int value)", &Manager::lua_Item_setIntegerAttribute);
	registerMemberFunction("Item", "setFloatAttribute(string key, float value)", &Manager::lua_Item_setFloatAttribute);
	registerMemberFunction("Item", "setBooleanAttribute(string key, boolean value)", &Manager::lua_Item_setBooleanAttribute);
	
	// Container
	registerMemberFunction("Container", "addItem(Item item)", &Manager::lua_Container_addItem);
	registerMemberFunction("Container", "getItem(int index)", &Manager::lua_Container_getItem);
	registerMemberFunction("Container", "size()", &Manager::lua_Container_getSize);
	registerMemberFunction("Container", "capacity()", &Manager::lua_Container_getCapacity);
	registerMemberFunction("Container", "getItems()", &Manager::lua_Container_getItems);

	registerGlobalFunction("getItemType(int itemid)", &Manager::lua_getItemType);
	registerGlobalFunction("getItemIDByName(string name)", &Manager::lua_getItemIDByName);
	registerGlobalFunction("isValidItemID(int id)", &Manager::lua_isValidItemID);

	// Tile
	registerMemberFunction("Tile", "getThing(int index)", &Manager::lua_Tile_getThing);
	registerMemberFunction("Tile", "getCreatures()", &Manager::lua_Tile_getCreatures);
	registerMemberFunction("Tile", "getMoveableItems()", &Manager::lua_Tile_getMoveableItems);
	registerMemberFunction("Tile", "getItems()", &Manager::lua_Tile_getItems);
	registerMemberFunction("Tile", "queryAdd()", &Manager::lua_Tile_queryAdd);
	registerMemberFunction("Tile", "hasProperty(TileProp prop)", &Manager::lua_Tile_hasProperty);

	registerMemberFunction("Tile", "getItemTypeCount(int itemid)", &Manager::lua_Tile_getItemTypeCount);

	registerMemberFunction("Tile", "addItem(Item item)", &Manager::lua_Tile_addItem);

	// Town
	registerMemberFunction("Town", "getTemplePosition()", &Manager::lua_Town_getTemplePosition);
	registerMemberFunction("Town", "getName()", &Manager::lua_Town_getName);
	registerMemberFunction("Town", "getID()", &Manager::lua_Town_getID);
	
	registerGlobalFunction("getAllTowns()", &Manager::lua_getTowns);
	registerGlobalFunction("sendMailTo(Item item, string playername [, Town town])", &Manager::lua_sendMailTo);

	// Waypoint
	registerMemberFunction("Waypoint", "getPosition()", &Manager::lua_Waypoint_getPosition);
	registerMemberFunction("Waypoint", "getName()", &Manager::lua_Waypoint_getName);

	registerGlobalFunction("getWaypointByName(string name)", &Manager::lua_getWaypointByName);

	// House
	registerMemberFunction("House", "cleanHouse()", &Manager::lua_House_cleanHouse);
	registerMemberFunction("House", "getDoors()", &Manager::lua_House_getDoors);
	registerMemberFunction("House", "getExitPosition()", &Manager::lua_House_getExitPosition);
	registerMemberFunction("House", "getID()", &Manager::lua_House_getID);
	registerMemberFunction("House", "getInvitedList()", &Manager::lua_House_getInvitedList);
	registerMemberFunction("House", "getName()", &Manager::lua_House_getName);
	registerMemberFunction("House", "getPaidUntil()", &Manager::lua_House_getPaidUntil);
	registerMemberFunction("House", "getRent()", &Manager::lua_House_getRent);
	registerMemberFunction("House", "getSubownerList()", &Manager::lua_House_getSubownerList);
	registerMemberFunction("House", "getTiles()", &Manager::lua_House_getTiles);
	registerMemberFunction("House", "getTown()", &Manager::lua_House_getTown);
	registerMemberFunction("House", "kickPlayer(Player who)", &Manager::lua_House_kickPlayer);
	registerMemberFunction("House", "isInvited(Player who)", &Manager::lua_House_isInvited);
	registerMemberFunction("House", "setInviteList(table list)", &Manager::lua_House_setInviteList);
	registerMemberFunction("House", "setOwner(int guid)", &Manager::lua_House_setOwner);
	registerMemberFunction("House", "setSubownerList(table list)", &Manager::lua_House_setSubownerList);
	registerMemberFunction("House", "setPaidUntil(int until)", &Manager::lua_House_setPaidUntil);
	
	registerGlobalFunction("getAllHouses()", &Manager::lua_getHouses);

	// Channel
	registerMemberFunction("Channel", "getID()", &Manager::lua_Channel_getID);
	registerMemberFunction("Channel", "getName()", &Manager::lua_Channel_getName);
	registerMemberFunction("Channel", "getUsers()", &Manager::lua_Channel_getUsers);
	registerMemberFunction("Channel", "addUser(Player player)", &Manager::lua_Channel_addUser);
	registerMemberFunction("Channel", "removeUser(Player player)", &Manager::lua_Channel_removeUser);
	registerMemberFunction("Channel", "talk(Player speaker, int type, string msg)", &Manager::lua_Channel_talk);
}

void Manager::registerFunctions() {
	// General functions
	registerGlobalFunction("wait(int delay)", &Manager::lua_wait);
	registerGlobalFunction("stacktrace(thread thread)", &Manager::lua_stacktrace);
	registerGlobalFunction("require_directory(string path)", &Manager::lua_require_directory);
	registerGlobalFunction("get_thread_id(thread t)", &Manager::lua_get_thread_id);

	registerGlobalFunction("getConfigValue(string key)", &Manager::lua_getConfigValue);

	// Register different events

	// OnSay/OnHear
	registerGlobalFunction("registerOnSay(string method, boolean case_sensitive, string filter, function callback)", &Manager::lua_registerGenericEvent_OnSay);
	registerGlobalFunction("registerOnCreatureSay(Creature who, string method, boolean case_sensitive, string filter, function callback)", &Manager::lua_registerSpecificEvent_OnSay);
	registerGlobalFunction("registerOnHear(Creature who, function callback)", &Manager::lua_registerSpecificEvent_OnHear);

	// OnUse
	registerGlobalFunction("registerOnUseItem(string method, int filter, function callback)", &Manager::lua_registerGenericEvent_OnUseItem);

	// OnMove
	registerGlobalFunction("registerOnCreatureMove(Creature who, function callback)", &Manager::lua_registerSpecificEvent_CreatureMove);
	registerGlobalFunction("registerOnCreatureMoveIn(Creature who, string method, int itemid, function callback)", &Manager::lua_registerSpecificEvent_CreatureMoveIn);
	registerGlobalFunction("registerOnCreatureMoveOut(Creature who, string method, int itemid, function callback)", &Manager::lua_registerSpecificEvent_CreatureMoveOut);
	registerGlobalFunction("registerOnAnyCreatureMoveIn(string method, int itemid, function callback)", &Manager::lua_registerGenericEvent_CreatureMoveIn);
	registerGlobalFunction("registerOnAnyCreatureMoveOut(string method, int itemid, function callback)", &Manager::lua_registerGenericEvent_CreatureMoveOut);

	// OnTurn
	registerGlobalFunction("registerOnAnyCreatureTurn(function callback)", &Manager::lua_registerGenericEvent_OnCreatureTurn);
	registerGlobalFunction("registerOnCreatureTurn(Creature who, function callback)", &Manager::lua_registerSpecificEvent_OnCreatureTurn);

	// On(De)Equip
	registerGlobalFunction("registerOnEquipItem(string method, int filter, SlotPosition slot, function callback)", &Manager::lua_registerGenericEvent_OnEquipItem);
	registerGlobalFunction("registerOnDeEquipItem(string method, int filter, SlotPosition slot, function callback)", &Manager::lua_registerGenericEvent_OnDeEquipItem);
	
	// OnServerLoad
	registerGlobalFunction("registerOnServerLoad(function callback)", &Manager::lua_registerGenericEvent_OnServerLoad);

	// OnMoveItem
	// Registering other OnMoveItem events are done through lua
	registerGlobalFunction("registerOnMoveItem(string method, int filter, boolean isadd, boolean isontile, function callback)", &Manager::lua_registerGenericEvent_OnMoveItem);

	// OnSpot / OnLose
	registerGlobalFunction("registerOnSpotCreature(Creature creature, function callback)", &Manager::lua_registerSpecificEvent_OnSpotCreature);
	registerGlobalFunction("registerOnLoseCreature(Creature creature, function callback)", &Manager::lua_registerSpecificEvent_OnLoseCreature);
	
	// OnThink
	registerGlobalFunction("registerOnCreatureThink(Creature creature, function callback)", &Manager::lua_registerSpecificEvent_OnCreatureThink);
	
	// OnSpawn
	registerGlobalFunction("registerOnSpawn(string cname, function callback)", &Manager::lua_registerGenericEvent_OnSpawn);

	// OnJoin / OnLeave (channel)
	registerGlobalFunction("registerOnJoinChannel(function callback)", &Manager::lua_registerGenericEvent_OnJoinChannel);
	registerGlobalFunction("registerOnPlayerJoinChannel(Player player, function callback)", &Manager::lua_registerSpecificEvent_OnJoinChannel);
	registerGlobalFunction("registerOnLeaveChannel(function callback)", &Manager::lua_registerGenericEvent_OnLeaveChannel);
	registerGlobalFunction("registerOnPlayerLeaveChannel(Player player, function callback)", &Manager::lua_registerSpecificEvent_OnLeaveChannel);

	// OnLogin / OnLogout
	registerGlobalFunction("registerOnLogin(function callback)", &Manager::lua_registerGenericEvent_OnLogin);
	registerGlobalFunction("registerOnLogout(function callback)", &Manager::lua_registerGenericEvent_OnLogout);
	registerGlobalFunction("registerOnPlayerLogout(Player player, function callback)", &Manager::lua_registerSpecificEvent_OnLogout);

	// OnLook
	registerGlobalFunction("registerOnLookAtItem(string method, int filter, function callback)", &Manager::lua_registerGenericEvent_OnLookAtItem);
	registerGlobalFunction("registerOnLookAtCreature(Creature creature, function callback)", &Manager::lua_registerGenericEvent_OnLookAtCreature);
	registerGlobalFunction("registerOnPlayerLookAt(Creature creature, function callback)", &Manager::lua_registerSpecificEvent_OnLook);
	
	// OnAdvance
	registerGlobalFunction("registerOnAdvance([LevelType skillid = nil], function callback)", &Manager::lua_registerGenericEvent_OnAdvance);
	registerGlobalFunction("registerOnPlayerAdvance(Player player [, LevelType skillid = nil], function callback)", &Manager::lua_registerSpecificEvent_OnAdvance);

	// OnKill
	registerGlobalFunction("registerOnKill([string what = nil], function callback)", &Manager::lua_registerGenericEvent_OnKill);
	registerGlobalFunction("registerOnCreatureKill(Creature killer, function callback)", &Manager::lua_registerSpecificEvent_OnKill);
	registerGlobalFunction("registerOnKilled([string what = nil], function callback)", &Manager::lua_registerGenericEvent_OnKilled);
	registerGlobalFunction("registerOnCreatureKilled(Creature creature, function callback)", &Manager::lua_registerSpecificEvent_OnKilled);

	registerGlobalFunction("registerOnDeathBy([string what = nil], function callback)", &Manager::lua_registerGenericEvent_OnDeathBy);
	registerGlobalFunction("registerOnCreatureDeathBy(Creature killer, function callback)", &Manager::lua_registerSpecificEvent_OnDeathBy);
	registerGlobalFunction("registerOnDeath([string what = nil], function callback)", &Manager::lua_registerGenericEvent_OnDeath);
	registerGlobalFunction("registerOnCreatureDeath(Creature creature, function callback)", &Manager::lua_registerSpecificEvent_OnDeath);


	registerGlobalFunction("stopListener(string listener_id)", &Manager::lua_stopListener);

	// Game/Map functions
	registerGlobalFunction("getTile(int x, int y, int z)", &Manager::lua_getTile);
	registerGlobalFunction("sendMagicEffect(position where, int type)", &Manager::lua_sendMagicEffect);
	registerGlobalFunction("sendAnimatedText(position where, int color, string text)", &Manager::lua_sendAnimatedText);
	
	registerGlobalFunction("sendMailTo(Item item, string player [, Town town])", &Manager::lua_sendMailTo);

	registerGlobalFunction("getWorldType()", &Manager::lua_getWorldType);
	registerGlobalFunction("getWorldTime()", &Manager::lua_getWorldTime);
	registerGlobalFunction("getWorldUpTime()", &Manager::lua_getWorldUpTime);

	// Register the bitlib
	luaL_register(state, "bit", lua_BitReg);
}

///////////////////////////////////////////////////////////////////////////////
// Utility functions

int LuaState::lua_wait()
{
	// integer delay is ontop of stack
	pushString("WAIT");
	return lua_yield(state, 2);
}

int LuaState::lua_getConfigValue()
{
	std::string key = popString();

	if(key == "sql_user" || key == "sql_pass" || key == "_G"){
		throw Script::Error("Read-protected config value " + key + " accessed.");
	}

	g_config.getConfigValue(key, state);
	return 1;
}

int LuaState::lua_require_directory()
{
	std::string script_dir = g_config.getString(ConfigManager::DATA_DIRECTORY) + "scripts/";
	std::string dir = popString();
	if(!manager->loadDirectory(script_dir + dir)){
		throw Error("Failed to load directory " + dir + ".");
	}
	pushBoolean(true);
	return 1;
}

int LuaState::lua_get_thread_id()
{
	lua_State* L = lua_tothread(state, -1);
	lua_pop(state, 1);
	std::ostringstream os;
	os << L;
	pushString(os.str());
	return 1;
}

int LuaState::lua_stacktrace()
{
	lua_State* L = lua_tothread(state, -1);
	lua_pop(state, 1);

	std::string report;
	{
		// Local thread is OK here
		LuaThread lt(manager, L);
		report = lt.report();
	}
	pushString(report);
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Register Events

int LuaState::lua_registerGenericEvent_OnServerLoad()
{
	Listener_ptr listener(new Listener(ON_LOAD_LISTENER, boost::any(), *manager));

	environment->Generic.OnLoad.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

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
	Listener_ptr listener(new Listener(ON_SAY_LISTENER, p, *manager));

	// OnSay event list is sorted by length, from longest to shortest
	if(si_onsay.method == OnSay::FILTER_EXACT){
		// Insert at beginning
		environment->Generic.OnSay.insert(environment->Generic.OnSay.begin(), listener);
	}
	else if(si_onsay.method == OnSay::FILTER_ALL){
		// All comes very last
		environment->Generic.OnSay.push_back(listener);
	}
	else{
		if(environment->Generic.OnSay.empty()){
			environment->Generic.OnSay.push_back(listener);
		}
		else{
			bool registered = false;
			for(ListenerList::iterator listener_iter = environment->Generic.OnSay.begin(),
				end = environment->Generic.OnSay.end();
				listener_iter != end; ++listener_iter)
			{
				OnSay::ScriptInformation info = boost::any_cast<OnSay::ScriptInformation>((*listener_iter)->getData());

				if(si_onsay.method == OnSay::FILTER_MATCH_BEGINNING){
					// We should be inserted before substrings...
					if(info.method == OnSay::FILTER_SUBSTRING || info.method == OnSay::FILTER_ALL){
						environment->Generic.OnSay.insert(listener_iter, listener);
						registered = true; 
						break;
					}

					if(info.filter.length() < si_onsay.filter.length()){
						environment->Generic.OnSay.insert(listener_iter, listener);
						registered = true; 
						break;
					}
				}
				else{
					assert(si_onsay.method == OnSay::FILTER_SUBSTRING);
					// We should be inserted before generic...
					if(info.method == OnSay::FILTER_ALL){
						environment->Generic.OnSay.insert(listener_iter, listener);
						registered = true; 
						break;
					}

					if(info.filter.length() < si_onsay.filter.length()){
						environment->Generic.OnSay.insert(listener_iter, listener);
						registered = true; 
						break;
					}
				}
			}
			if(!registered)
				environment->Generic.OnSay.push_back(listener);
		}
	}
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

	// This here explains why boost is so awesome, thanks to our custom
	// delete function, the listener is cleanly stopped when all references
	// to it is removed. :)
	boost::any p(si_onsay);
	Listener_ptr listener(
		new Listener(ON_SAY_LISTENER, p, *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_OnHear() {
	// Store callback
	insert(-2);

	Creature* who = popCreature();

	// Callback is no the top of the stack

	OnHear::ScriptInformation si_onhear;

	// This here explains why boost is so awesome, thanks to our custom
	// delete function, the listener is cleanly stopped when all references
	// to it is removed. :)
	boost::any p(si_onhear);
	Listener_ptr listener(
		new Listener(ON_HEAR_LISTENER, p, *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnUseItem() {
	// Store callback
	insert(-3);

	int32_t id = popInteger();
	std::string method = popString();

	OnUseItem::ScriptInformation si_onuse;
	if(method == "itemid") {
		si_onuse.method = OnUseItem::FILTER_ITEMID;
	}
	else if(method == "actionid") {
		si_onuse.method = OnUseItem::FILTER_ACTIONID;
	}
	else if(method == "uniqueid") {
		si_onuse.method = OnUseItem::FILTER_UNIQUEID;
	}
	else {
		throw Error("Invalid argument (1) 'method'");
	}
	si_onuse.id = id;

	boost::any p(si_onuse);
	Listener_ptr listener(new Listener(ON_USE_ITEM_LISTENER, p, *manager));

	environment->Generic.OnUseItem.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnJoinChannel() {
	boost::any p(0);
	Listener_ptr listener(new Listener(ON_OPEN_CHANNEL_LISTENER, p, *manager));

	environment->Generic.OnJoinChannel.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_OnJoinChannel() {
	// Store callback
	insert(-2);

	Player* who = popPlayer();

	boost::any p(0);
	Listener_ptr listener(
		new Listener(ON_OPEN_CHANNEL_LISTENER, p, *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnLeaveChannel() {
	boost::any p(0);
	Listener_ptr listener(new Listener(ON_CLOSE_CHANNEL_LISTENER, p, *manager));

	environment->Generic.OnLeaveChannel.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_OnLeaveChannel() {
	// Store callback
	insert(-2);

	Player* who = popPlayer();

	boost::any p(0);
	Listener_ptr listener(
		new Listener(ON_CLOSE_CHANNEL_LISTENER, p, *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnLogin() {
	boost::any p(0);
	Listener_ptr listener(new Listener(ON_LOGIN_LISTENER, p, *manager));

	environment->Generic.OnLogin.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnLogout() {
	boost::any p(0);
	Listener_ptr listener(new Listener(ON_LOGOUT_LISTENER, p, *manager));

	environment->Generic.OnLogout.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_OnLogout() {
	// Store callback
	insert(-2);

	Player* who = popPlayer();

	boost::any p(0);
	Listener_ptr listener(
		new Listener(ON_LOGOUT_LISTENER, p, *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnLookAtItem() {
	// Store callback
	insert(-3);

	uint32_t id = popInteger();
	std::string method = popString();

	OnLook::ScriptInformation si_onlook;
	if(method == "itemid") {
		si_onlook.method = OnLook::FILTER_ITEMID;
	}
	else if(method == "actionid") {
		si_onlook.method = OnLook::FILTER_ACTIONID;
	}
	else if(method == "uniqueid") {
		si_onlook.method = OnLook::FILTER_UNIQUEID;
	}
	else {
		throw Error("Invalid argument (1) 'method'");
	}
	si_onlook.id = id;

	boost::any p(si_onlook);
	Listener_ptr listener(new Listener(ON_LOOK_LISTENER, p, *manager));

	environment->Generic.OnLook.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnLookAtCreature() {
	// Store callback
	insert(-3);

	Creature* at = popCreature();

	OnLook::ScriptInformation si_onlook;
	si_onlook.method = OnLook::FILTER_CREATUREID;
	si_onlook.id = at->getID();

	boost::any p(si_onlook);
	Listener_ptr listener(new Listener(ON_LOOK_LISTENER, p, *manager));

	environment->Generic.OnLook.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_OnLook() {
	// Store callback
	insert(-2);

	Creature* who = popCreature();

	OnLook::ScriptInformation si_onlook;
	si_onlook.method = OnLook::FILTER_NONE;
	si_onlook.id = 0;

	// This here explains why boost is so awesome, thanks to our custom
	// delete function, the listener is cleanly stopped when all references
	// to it is removed. :)
	boost::any p(si_onlook);
	Listener_ptr listener(
		new Listener(ON_LOOK_LISTENER, p, *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnEquipItem() {
	// Store callback
	insert(-4);

	SlotPosition slot = popEnum<SlotPosition>();
	int32_t id = popInteger();
	std::string method = popString();

	OnEquipItem::ScriptInformation si_onequip;
	
	si_onequip.slot = slot;
	
	if(method == "itemid") {
		si_onequip.method = OnEquipItem::FILTER_ITEMID;
	}
	else if(method == "actionid") {
		si_onequip.method = OnEquipItem::FILTER_ACTIONID;
	}
	else if(method == "uniqueid") {
		si_onequip.method = OnEquipItem::FILTER_UNIQUEID;
	}
	else {
		throw Error("Invalid argument (1) 'method'");
	}
	si_onequip.id = id;
	si_onequip.equip = true;

	boost::any p(si_onequip);
	Listener_ptr listener(new Listener(ON_EQUIP_ITEM_LISTENER, p, *manager));

	environment->Generic.OnEquipItem.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnDeEquipItem() {
	// Store callback
	insert(-4);

	SlotPosition slot = popEnum<SlotPosition>();
	int32_t id = popInteger();
	std::string method = popString();

	OnEquipItem::ScriptInformation si_ondeequip;
	
	si_ondeequip.slot = slot;

	if(method == "itemid") {
		si_ondeequip.method = OnEquipItem::FILTER_ITEMID;
	}
	else if(method == "actionid") {
		si_ondeequip.method = OnEquipItem::FILTER_ACTIONID;
	}
	else if(method == "uniqueid") {
		si_ondeequip.method = OnEquipItem::FILTER_UNIQUEID;
	}
	else {
		throw Error("Invalid argument (1) 'method'");
	}
	si_ondeequip.id = id;
	si_ondeequip.equip = false;

	boost::any p(si_ondeequip);
	Listener_ptr listener(new Listener(ON_EQUIP_ITEM_LISTENER, p, *manager));

	environment->Generic.OnEquipItem.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_CreatureMove() {
	// Store callback
	insert(-2);

	Creature* who = popCreature();

	OnMoveCreature::ScriptInformation si_onmovecreature;
	si_onmovecreature.method = OnMoveCreature::FILTER_NONE;
	si_onmovecreature.id = 0;
	si_onmovecreature.slot = 0;
	si_onmovecreature.moveType = OnMoveCreature::TYPE_MOVE;

	boost::any p(si_onmovecreature);
	Listener_ptr listener(
		new Listener(ON_MOVE_CREATURE_LISTENER, p, *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_CreatureMoveIn() {
	// Store callback
	insert(-4);

	int32_t id = popInteger();
	std::string method = popString();
	Creature* who = popCreature();

	OnMoveCreature::ScriptInformation si_onmovecreature;
	if(method == "itemid") {
		si_onmovecreature.method = OnMoveCreature::FILTER_ITEMID;
	}
	else if(method == "actionid") {
		si_onmovecreature.method = OnMoveCreature::FILTER_ACTIONID;
	}
	else if(method == "uniqueid") {
		si_onmovecreature.method = OnMoveCreature::FILTER_UNIQUEID;
	}
	else {
		throw Error("Invalid argument (1) 'method'");
	}
	si_onmovecreature.slot = 0;
	si_onmovecreature.id = id;
	si_onmovecreature.moveType = OnMoveCreature::TYPE_STEPIN;

	boost::any p(si_onmovecreature);
	Listener_ptr listener(
		new Listener(ON_MOVE_CREATURE_LISTENER, p, *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_CreatureMoveOut() {
	// Store callback
	insert(-4);

	int32_t id = popInteger();
	std::string method = popString();
	Creature* who = popCreature();

	OnMoveCreature::ScriptInformation si_onmovecreature;
	if(method == "itemid") {
		si_onmovecreature.method = OnMoveCreature::FILTER_ITEMID;
	}
	else if(method == "actionid") {
		si_onmovecreature.method = OnMoveCreature::FILTER_ACTIONID;
	}
	else if(method == "uniqueid") {
		si_onmovecreature.method = OnMoveCreature::FILTER_UNIQUEID;
	}
	else {
		throw Error("Invalid argument (1) 'method'");
	}
	si_onmovecreature.slot = 0;
	si_onmovecreature.id = id;
	si_onmovecreature.moveType = OnMoveCreature::TYPE_STEPOUT;

	boost::any p(si_onmovecreature);
	Listener_ptr listener(
		new Listener(ON_MOVE_CREATURE_LISTENER, p, *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_CreatureMoveIn() {
	// Store callback
	insert(-3);

	int32_t id = popInteger();
	std::string method = popString();

	OnMoveCreature::ScriptInformation si_onmovecreature;
	if(method == "itemid") {
		si_onmovecreature.method = OnMoveCreature::FILTER_ITEMID;
	}
	else if(method == "actionid") {
		si_onmovecreature.method = OnMoveCreature::FILTER_ACTIONID;
	}
	else if(method == "uniqueid") {
		si_onmovecreature.method = OnMoveCreature::FILTER_UNIQUEID;
	}
	else {
		throw Error("Invalid argument (1) 'method'");
	}
	si_onmovecreature.slot = 0;
	si_onmovecreature.id = id;
	si_onmovecreature.moveType = OnMoveCreature::TYPE_STEPIN;

	boost::any p(si_onmovecreature);
	Listener_ptr listener(new Listener(ON_MOVE_CREATURE_LISTENER, p, *manager));

	environment->Generic.OnMoveCreature.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_CreatureMoveOut() {
	// Store callback
	insert(-3);

	int32_t id = popInteger();
	std::string method = popString();

	OnMoveCreature::ScriptInformation si_onmovecreature;
	if(method == "itemid") {
		si_onmovecreature.method = OnMoveCreature::FILTER_ITEMID;
	}
	else if(method == "actionid") {
		si_onmovecreature.method = OnMoveCreature::FILTER_ACTIONID;
	}
	else if(method == "uniqueid") {
		si_onmovecreature.method = OnMoveCreature::FILTER_UNIQUEID;
	}
	else {
		throw Error("Invalid argument (1) 'method'");
	}
	si_onmovecreature.slot = 0;
	si_onmovecreature.id = id;
	si_onmovecreature.moveType = OnMoveCreature::TYPE_STEPOUT;

	boost::any p(si_onmovecreature);
	Listener_ptr listener(new Listener(ON_MOVE_CREATURE_LISTENER, p, *manager));

	environment->Generic.OnMoveCreature.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnSpawn() {
	// Store callback
	insert(-2);

	std::string cname = popString();
	toLowerCaseString(cname);


	boost::any p(0);
	Listener_ptr listener(
		new Listener(ON_SPAWN_LISTENER, p, *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->Generic.OnSpawn[cname].push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnCreatureTurn() {
	Listener_ptr listener(new Listener(ON_LOOK_LISTENER, boost::any(), *manager));

	environment->Generic.OnTurn.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_OnCreatureTurn() {
	// Store callback
	insert(-2);

	Creature* who = popCreature();

	Listener_ptr listener(
		new Listener(ON_TURN_LISTENER, boost::any(), *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnMoveItem() {
	// Store callback
	insert(-5);

	bool isAddItem = popBoolean();
	bool isItemOnTile = popBoolean();
	int32_t id = popInteger();
	std::string method = popString();

	OnMoveItem::ScriptInformation si_onmoveitem;
	if(method == "itemid") {
		si_onmoveitem.method = OnMoveItem::FILTER_ITEMID;
	}
	else if(method == "actionid") {
		si_onmoveitem.method = OnMoveItem::FILTER_ACTIONID;
	}
	else if(method == "uniqueid") {
		si_onmoveitem.method = OnMoveItem::FILTER_UNIQUEID;
	}
	else {
		throw Error("Invalid argument (1) 'method'");
	}
	si_onmoveitem.id = id;
	si_onmoveitem.addItem = isAddItem;
	si_onmoveitem.isItemOnTile = isItemOnTile;

	boost::any p(si_onmoveitem);
	Listener_ptr listener(new Listener(ON_MOVE_ITEM_LISTENER, p, *manager));

	switch(si_onmoveitem.method){
		case OnMoveItem::FILTER_ITEMID: environment->Generic.OnMoveItem.ItemId[id].push_back(listener); break;
		case OnMoveItem::FILTER_ACTIONID: environment->Generic.OnMoveItem.ActionId[id].push_back(listener); break;
		case OnMoveItem::FILTER_UNIQUEID: environment->Generic.OnMoveItem.UniqueId[id].push_back(listener); break;

		default:
			break;
	}

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_OnSpotCreature() {
	// Store callback
	insert(-2);

	Creature* who = popCreature();

	Listener_ptr listener(
		new Listener(ON_SPOT_CREATURE_LISTENER, boost::any(), *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_OnLoseCreature() {
	// Store callback
	insert(-2);

	Creature* who = popCreature();

	Listener_ptr listener(
		new Listener(ON_LOSE_CREATURE_LISTENER, boost::any(), *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_OnCreatureThink() {
	// Store callback
	insert(-2);

	Creature* who = popCreature();

	Listener_ptr listener(
		new Listener(ON_THINK_LISTENER, boost::any(), *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnAdvance() {
	OnAdvance::ScriptInformation si_onadvance;

	// We always receive to arguments
	// store callback
	insert(-2);

	// nil as first means no skill
	if(isNil()){
		si_onadvance.method = OnAdvance::FILTER_ALL;
		si_onadvance.skill = LEVEL_EXPERIENCE; // Unused, just don't leave it hanging
	}
	else{
		si_onadvance.method = OnAdvance::FILTER_SKILL;
		si_onadvance.skill = popEnum<LevelType>();
	}
	// Pop the nil
	pop();

	boost::any p(si_onadvance);
	Listener_ptr listener(new Listener(ON_ADVANCE_LISTENER, p, *manager));

	// Add it to the listener list
	environment->Generic.OnAdvance.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener tag
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_OnAdvance() {
	Creature* who = NULL;
	OnAdvance::ScriptInformation si_onadvance;

	// store callback
	insert(-3);

	// Then comes the skill, nil means all
	if(isNil()){
		si_onadvance.method = OnAdvance::FILTER_ALL;
		si_onadvance.skill = LEVEL_EXPERIENCE; // Unused, just don't leave it hanging
	}
	else{
		si_onadvance.method = OnAdvance::FILTER_SKILL;
		si_onadvance.skill = popEnum<LevelType>();
	}
	// Pop the nil
	pop();

	// Finally player
	who = popPlayer();

	// Callback is now the top of the stack
	boost::any p(si_onadvance);
	Listener_ptr listener(
		new Listener(ON_ADVANCE_LISTENER, p, *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnKill()
{
	// Tied to killing creature, either by name or to all kills
	// Store callback
	insert(-2);

	OnKill::ScriptInformation si_onkill;

	if(isNil(-1)){
		si_onkill.method = OnKill::FILTER_ALL;
		pop();
	}
	else{
		std::string name = popString();
		if(name == "Player"){
			si_onkill.method = OnKill::FILTER_KILLER_PLAYER;
		}
		else{
			si_onkill.method = OnKill::FILTER_KILLER_NAME;
			si_onkill.name = name;
		}
	}

	boost::any p(si_onkill);
	Listener_ptr listener(new Listener(ON_KILL_LISTENER, p, *manager));

	environment->Generic.OnKill.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_OnKill()
{
	// Tied to a specific creature, and run each time that creature kills something
	// Store callback
	insert(-2);

	Creature* who = popCreature();

	OnKill::ScriptInformation si_onkill;
	si_onkill.method = OnKill::FILTER_ALL;

	boost::any p(si_onkill);
	Listener_ptr listener(
		new Listener(ON_KILL_LISTENER, p, *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnKilled()
{
	// Tied to any dying creature, by name (or all dying creatures)
	// Store callback
	insert(-2);

	OnKill::ScriptInformation si_onkill;

	if(isNil(-1)){
		si_onkill.method = OnKill::FILTER_ALL;
		pop();
	}
	else{
		std::string name = popString();
		if(name == "Player"){
			si_onkill.method = OnKill::FILTER_PLAYER;
		}
		else{
			si_onkill.method = OnKill::FILTER_NAME;
			si_onkill.name = name;
		}
	}

	boost::any p(si_onkill);
	Listener_ptr listener(new Listener(ON_KILLED_LISTENER, p, *manager));

	environment->Generic.OnKilled.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_OnKilled()
{
	// Tied to a specific dying creature
	// Store callback
	insert(-2);

	Creature* who = popCreature();

	OnKill::ScriptInformation si_onkill;
	si_onkill.method = OnKill::FILTER_ALL;

	boost::any p(si_onkill);
	Listener_ptr listener(
		new Listener(ON_KILLED_LISTENER, p, *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnDeathBy()
{
	// Store callback
	insert(-2);

	OnDeath::ScriptInformation si_ondeath;
	
	if(isNil(-1)){
		si_ondeath.method = OnDeath::FILTER_ALL;
		pop();
	}
	else{
		std::string name = popString();
		if(name == "Player"){
			si_ondeath.method = OnDeath::FILTER_KILLER_PLAYER;
		}
		else{
			si_ondeath.method = OnDeath::FILTER_KILLER_NAME;
			si_ondeath.name = name;
		}
	}

	boost::any p(si_ondeath);
	Listener_ptr listener(new Listener(ON_DEATH_BY_LISTENER, p, *manager));

	environment->Generic.OnDeathBy.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_OnDeathBy()
{
	// Store callback
	insert(-2);

	Creature* who = popCreature();

	OnDeath::ScriptInformation si_ondeath;
	si_ondeath.method = OnDeath::FILTER_ALL;

	boost::any p(si_ondeath);
	Listener_ptr listener(
		new Listener(ON_DEATH_BY_LISTENER, p, *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnDeath()
{
	// Store callback
	insert(-2);

	OnDeath::ScriptInformation si_ondeath;

	if(isNil(-1)){
		si_ondeath.method = OnDeath::FILTER_ALL;
		pop();
	}
	else{
		std::string name = popString();
		if(name == "Player"){
			si_ondeath.method = OnDeath::FILTER_PLAYER;
		}
		else{
			si_ondeath.method = OnDeath::FILTER_NAME;
			si_ondeath.name = name;
		}
	}

	boost::any p(si_ondeath);
	Listener_ptr listener(new Listener(ON_DEATH_LISTENER, p, *manager));

	environment->Generic.OnDeath.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_OnDeath()
{
	// Store callback
	insert(-2);

	Creature* who = popCreature();

	OnDeath::ScriptInformation si_ondeath;
	si_ondeath.method = OnDeath::FILTER_ALL;

	boost::any p(si_ondeath);
	Listener_ptr listener(
		new Listener(ON_DEATH_LISTENER, p, *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Stop listener function

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
	is.str(listener_id.substr(_2pos+1));
	is >> id;
	if(!is) {
		pushBoolean(false);
		return 1;
	}
	pushBoolean(environment->stopListener((ListenerType)type, id));
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Member functions
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Class Event

int LuaState::lua_Event_skip() {
	// Event table is ontop of stack
	setField(-1, "skipped", false);
	return 1;
}

int LuaState::lua_Event_propagate() {
	// Event table is ontop of stack
	setField(-1, "skipped", true);
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Class Thing

int LuaState::lua_Thing_getPosition()
{
	Thing* thing = popThing();
	pushPosition(thing->getPosition());
	return 1;
}

int LuaState::lua_Thing_getX()
{
	Thing* thing = popThing();
	pushInteger(thing->getPosition().x);
	return 1;
}

int LuaState::lua_Thing_getY()
{
	Thing* thing = popThing();
	pushInteger(thing->getPosition().y);
	return 1;
}

int LuaState::lua_Thing_getZ()
{
	Thing* thing = popThing();
	pushInteger(thing->getPosition().z);
	return 1;
}

int LuaState::lua_Thing_getParentTile()
{
	Thing* thing = popThing();
	pushTile(thing->getTile());
	return 1;
}

int LuaState::lua_Thing_getParent()
{
	Thing* thing = popThing();
	Cylinder* parent = thing->getParent();
	if(parent->getTile()) {
		pushTile(static_cast<Tile*>(parent));
	} else if(parent->getItem()){
		pushThing(parent->getItem());
	} else if(parent->getCreature()){
		pushThing(parent->getCreature());
	} else {
		pushNil();
	}
	return 1;
}

int LuaState::lua_Thing_moveToPosition()
{
	Position pos = popPosition();
	Thing* thing = popThing();

	pushBoolean(g_game.internalTeleport(NULL, thing, pos) == RET_NOERROR);

	return 1;
}

int LuaState::lua_Thing_isMoveable()
{
	Thing* thing = popThing();
	pushBoolean(thing->isPushable());
	return 1;
}

int LuaState::lua_Thing_getName() {
	Thing* t = popThing();
	if(Creature* c = t->getCreature()) {
		pushString(c->getName());
	} else if(Item* i = t->getItem()) {
		pushString(i->getName());
	} else {
		pushString("");
	}
	return 1;
}

int LuaState::lua_Thing_getDescription()
{
	int32_t lookdistance = 0;
	if(getStackSize() > 1) {
		lookdistance = popInteger();
	}
	Thing* thing = popThing();
	pushString(thing->getDescription(lookdistance));
	return 1;
}

int LuaState::lua_Thing_destroy()
{
	Thing* thing = popThing();
	if(Item* item = thing->getItem()) {
		pushBoolean(g_game.internalRemoveItem(NULL, item) == RET_NOERROR);
	} else if(Creature* creature = thing->getCreature()) {
		if(Player* player = creature->getPlayer()) {
			player->kickPlayer();
			pushBoolean(true);
		} else {
			pushBoolean(g_game.removeCreature(creature));
		}
	} else {
		pushBoolean(false);
	}
	return 1;
}

int LuaState::lua_getThingByID()
{
	Script::ObjectID objid = (Script::ObjectID)popFloat();
	Thing* thing = environment->getThing(objid);
	pushThing(thing);
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Class Tile

int LuaState::lua_Tile_getThing()
{
	int32_t index = popInteger(); // lua indices start at 1, but we don't give a crap!
	Tile* tile = popTile();

	// -1 is top item
	if(index < 0) {
		index = tile->__getLastIndex() + index;
	}
	if(index > tile->__getLastIndex()) {
		throw Error("Tile:getThing : Index out of range!");
	}

	pushThing(tile->__getThing(index));
	return 1;
}

int LuaState::lua_Tile_getCreatures()
{
	Tile* tile = popTile();

	newTable();
	int n = 1;
	for(CreatureIterator it = tile->creatures_begin(), end_iter = tile->creatures_end(); it != end_iter; ++it, ++n){
		pushThing(*it);
		setField(-2, n);
	}
	return 1;
}

int LuaState::lua_Tile_getMoveableItems()
{
	Tile* tile = popTile();

	newTable();
	int n = 1;
	for(TileItemIterator iter = tile->items_begin(), end_iter = tile->items_end(); iter != end_iter; ++iter){
		if((*iter)->isMoveable()) {
			pushThing(*iter);
			setField(-2, n++);
		}
	}
	return 1;
}

int LuaState::lua_Tile_getItems()
{
	Tile* tile = popTile();

	newTable();
	int n = 1;
	if(tile->ground) {
		pushThing(tile->ground);
		setField(-2, n++);
	}

	for(TileItemIterator iter = tile->items_begin(), end_iter = tile->items_end(); iter != end_iter; ++iter, ++n){
		pushThing(*iter);
		setField(-2, n);
	}

	return 1;
}

int LuaState::lua_Tile_getItemTypeCount() {
	int32_t type = popInteger();
	Tile* tile = popTile();
	push(tile->__getItemTypeCount(type));
	return 1;
}

int LuaState::lua_Tile_queryAdd()
{
	int32_t flags = 0;
	if(getStackSize() > 2) {
		flags = popInteger();
	}
	Thing* thing = popThing();
	Tile* tile = popTile();

	pushInteger(tile->__queryAdd(0, thing, 1, flags));
	return 1;
}

int LuaState::lua_Tile_addItem()
{
	Item* item = popItem(Script::ERROR_PASS);
	if(item == NULL) {
		pushInteger(RET_NOTPOSSIBLE);
		pushBoolean(false);
		return 2;
	}
	Tile* tile = popTile();

	if(item->getParent() != VirtualCylinder::virtualCylinder){
		// Must remove from previous parent...
		g_game.internalRemoveItem(NULL, item);
	}

	ReturnValue ret = g_game.internalAddItem(NULL, tile, item);
	pushInteger(ret);
	pushBoolean(ret == RET_NOERROR);
	return 2;
}

int LuaState::lua_Tile_hasProperty()
{
	TileProp prop = popEnum<TileProp>();
	Tile* tile = popTile();
	pushBoolean(tile && tile->hasFlag(prop));
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Class Creature

int LuaState::lua_Creature_getID()
{
	Creature* creature = popCreature();
	pushUnsignedInteger(creature->getID());
	return 1;
}

int LuaState::lua_Creature_getHealth()
{
	Creature* creature = popCreature();
	pushInteger(creature->getHealth());
	return 1;
}

int LuaState::lua_Creature_getHealthMax()
{
	Creature* creature = popCreature();
	pushInteger(creature->getMaxHealth());
	return 1;
}

int LuaState::lua_Creature_setHealth()
{
	int32_t newval = popInteger();
	Creature* c = popCreature();

	if(newval >= c->getHealth()){
		g_game.combatChangeHealth(COMBAT_HEALING, NULL, c, newval - c->getHealth(), false);
	}
	else{
		g_game.combatChangeHealth(COMBAT_UNDEFINEDDAMAGE, NULL, c, newval - c->getHealth(), false);
	}

	push(true);
	return 1;
}

int LuaState::lua_Creature_getOrientation()
{
	Creature* creature = popCreature();
	pushEnum(creature->getDirection());
	return 1;
}

int LuaState::lua_Creature_getNameDescription()
{
	Creature* creature = popCreature();
	pushString(creature->getNameDescription());
	return 1;
}

int LuaState::lua_Creature_walk()
{
	Direction ndir = popEnum<Direction>();
	Creature* creature = popCreature();

	switch(ndir.value()){
		case ::enums::NORTH:
		case ::enums::SOUTH:
		case ::enums::WEST:
		case ::enums::EAST:
		case ::enums::SOUTHWEST:
		case ::enums::NORTHWEST:
		case ::enums::NORTHEAST:
		case ::enums::SOUTHEAST:
			break;
		default:
			throw Error("Creature:walk : Invalid direction");
	}

	ReturnValue ret = g_game.internalMoveCreature(NULL, creature, (Direction)ndir, FLAG_NOLIMIT);
	pushBoolean(ret == RET_NOERROR);
	return 1;
}

int LuaState::lua_Creature_getSpeed()
{
	Creature* creature = popCreature();
	pushInteger(creature->getSpeed());
	return 1;
}

int LuaState::lua_Creature_getArmor()
{
	Creature* creature = popCreature();
	pushInteger(creature->getArmor());
	return 1;
}

int LuaState::lua_Creature_getDefense()
{
	Creature* creature = popCreature();
	pushInteger(creature->getDefense());
	return 1;
}

int LuaState::lua_Creature_getBaseSpeed()
{
	Creature* creature = popCreature();
	pushInteger(creature->getBaseSpeed());
	return 1;
}

int LuaState::lua_Creature_isPushable()
{
	Creature* creature = popCreature();
	pushInteger(creature->isPushable());
	return 1;
}

int LuaState::lua_Creature_getTarget()
{
	Creature* creature = popCreature();
	pushThing(creature->getAttackedCreature());
	return 1;
}

int LuaState::lua_Creature_isImmuneToCondition()
{
	Creature* creature = popCreature();
	ConditionType conditionType = popEnum<ConditionType>();

	pushBoolean(creature->isImmune(conditionType));
	return 1;
}

int LuaState::lua_Creature_isImmuneToCombat()
{
	Creature* creature = popCreature();
	CombatType combatType = popEnum<CombatType>();

	pushBoolean(creature->isImmune(combatType));
	return 1;
}

int LuaState::lua_Creature_getConditionImmunities()
{
	Creature* creature = popCreature();

	newTable();
	for(ConditionType::iterator i = CONDITION_POISON; i != ConditionType::end(); ++i){
		pushBoolean(creature->isImmune(*i));
		setField(-2, i->toString());
	}
	return 1;
}

int LuaState::lua_Creature_getDamageImmunities()
{
	Creature* creature = popCreature();

	newTable();
	for(CombatType::iterator i = COMBAT_PHYSICALDAMAGE; i != CombatType::end(); ++i){
		pushBoolean(creature->isImmune(*i));
		setField(-2, i->toString());
	}
	return 1;
}

int LuaState::lua_Creature_setRawCustomValue()
{
	bool remove = false;
	std::string value;
	if(isNil(-1)) {
		pop();
		remove = true;
	}
	else if(isString(-1) || isNumber(-1)){
		value = popString();
	}
	else{
		throw Error("Raw creature custom value must be either string or number (was " + typeName() + ").");
	}
	std::string key = popString();
	Creature* creature = popCreature();

	if(remove)
		creature->eraseCustomValue(key);
	else
		creature->setCustomValue(key, value);

	pushBoolean(true);
	return 1;
}

int LuaState::lua_Creature_getRawCustomValue()
{
	std::string key = popString();
	Creature* creature = popCreature();
	std::string value;
	if(creature->getCustomValue(key, value)) {
		pushString(value);
	} else {
		pushNil();
	}
	return 1;
}

int LuaState::lua_Creature_say()
{
	std::string msg = popString();
	Creature* creature = popCreature();

	bool ret = g_game.internalCreatureSay(creature, SPEAK_SAY, msg);
	pushBoolean(ret);
	return 1;
}

int LuaState::lua_Creature_getOutfit()
{
	Creature* creature = popCreature();
	pushOutfit(creature->getCurrentOutfit());
	return 1;
}

int LuaState::lua_Creature_setOutfit()
{
	OutfitType outfit = popOutfit();
	Creature* creature = popCreature();
	g_game.internalCreatureChangeOutfit(creature, outfit);
	pushBoolean(true);
	return 1;
}

int LuaState::lua_getCreatureByName()
{
	std::string name = popString();
	pushThing(g_game.getCreatureByName(name));
	return 1 ;
}

int LuaState::lua_getCreaturesByName()
{
	std::string name = popString();
	std::vector<Creature*> creatures = g_game.getCreaturesByName(name);

	int n = 1;
	newTable();
	for(std::vector<Creature*>::iterator i = creatures.begin(); i != creatures.end(); ++i){
		pushThing(*i);
		setField(-2, n++);
	}
	return 1 ;
}

///////////////////////////////////////////////////////////////////////////////
// Class Actor

int LuaState::lua_createActor()
{
	Position p = popPosition();
	std::string name = popString();

	// Create an empty actor, with some default values
	CreatureType ct;
	OutfitType ot;
	ot.lookType = 130;
	ct.outfit(ot);

	Actor* a = Actor::create(ct);

	// Set some default attributes, so the actor can be spawned with issues
	a->getType().name(name);

	g_game.placeCreature(a, p);
	pushThing(a);
	return 1;
}

int LuaState::lua_createMonster()
{
	Position p = popPosition();

	// Create a monster from a monster type
	Actor* a = Actor::create(popString());
	if(a)
		g_game.placeCreature(a, p);
	pushThing(a);
	return 1;
}

int LuaState::lua_Actor_setShouldReload()
{
	bool b = popBoolean();
	popActor()->shouldReload(b);
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Actor_getShouldReload()
{
	pushBoolean(popActor()->shouldReload());
	return 1;
}

int LuaState::lua_Actor_setAlwaysThink()
{
	bool b = popBoolean();
	popActor()->alwaysThink(b);
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Actor_getAlwaysThink()
{
	pushBoolean(popActor()->alwaysThink());
	return 1;
}

template <class T>
int Actor_modAttribute(LuaState* l, void (CreatureType::*mfp)(const T&)){
	T value = l->popValue<T>();
	Actor* actor = l->popActor();
	((actor->getType()).*(mfp))(value);
	l->pushBoolean(true);
	return 1;
}

template <class E, int32_t size_>
int Actor_modAttribute(LuaState* l, void (CreatureType::*mfp)(const Enum<E, size_>&)){
	Enum<E, size_> value = l->popEnum<Enum<E, size_> >();
	Actor* actor = l->popActor();
	((actor->getType()).*(mfp))(value);
	l->pushBoolean(true);
	return 1;
}

#define Actor_getAttribute(mfp) \
	push(popActor()->getType().mfp()), 1


int LuaState::lua_Actor_setArmor()
{
	return Actor_modAttribute(this, &CreatureType::armor);
}

int LuaState::lua_Actor_setDefense()
{
	return Actor_modAttribute(this, &CreatureType::defense);
}

int LuaState::lua_Actor_getExperienceWorth()
{
	return Actor_getAttribute(experience);
}

int LuaState::lua_Actor_setExperienceWorth()
{
	return Actor_modAttribute(this, &CreatureType::experience);
}

int LuaState::lua_Actor_getCanPushItems()
{
	return Actor_getAttribute(canPushItems);
}

int LuaState::lua_Actor_setCanPushItems()
{
	return Actor_modAttribute(this, &CreatureType::canPushItems);
}

int LuaState::lua_Actor_getCanPushCreatures()
{
	return Actor_getAttribute(canPushCreatures);
}

int LuaState::lua_Actor_setCanPushCreatures()
{
	return Actor_modAttribute(this, &CreatureType::canPushCreatures);
}

int LuaState::lua_Actor_setSpeed()
{
	int32_t value = popInteger();
	Actor* actor = popActor();
	g_game.changeSpeed(actor, value - actor->getSpeed());

	pushBoolean(true);
	return 1;
}

int LuaState::lua_Actor_getTargetDistance()
{
	return Actor_getAttribute(targetDistance);
}

int LuaState::lua_Actor_setTargetDistance()
{
	return Actor_modAttribute(this, &CreatureType::targetDistance);
}

int LuaState::lua_Actor_setConditionImmunities()
{
	if(!isTable(-1)) {
		HandleError(Script::ERROR_THROW, "Attempt to treat non-table value as an immunity-table.");
		pop();
		pushBoolean(false);
		return 1;
	}

	ConditionType immunities = CONDITION_NONE;
	for(ConditionType::iterator i = CONDITION_POISON; i != ConditionType::end(); ++i){
		getField(-1, i->toString());
		immunities |= (ConditionType)popInteger();		
	}

	pop();

	Actor* actor = popActor();
	actor->getType().conditionImmunities(immunities);
	//actor->updateMapCache();

	pushBoolean(true);
	return 1;
}

int LuaState::lua_Actor_setDamageImmunities()
{
	if(!isTable(-1)) {
		HandleError(Script::ERROR_THROW, "Attempt to treat non-table value as an immunity-table.");
		pop();
		pushBoolean(false);
		return 1;
	}

	CombatType immunities = COMBAT_NONE;
	for(CombatType::iterator i = COMBAT_PHYSICALDAMAGE; i != CombatType::end(); ++i){
		getField(-1, i->toString());
		immunities |= (CombatType)popInteger();		
	}

	pop();

	Actor* actor = popActor();
	actor->getType().damageImmunities(immunities);
	//actor->updateMapCache();

	pushBoolean(true);
	return 1;
}

int LuaState::lua_Actor_getMaxSummons()
{
	return Actor_getAttribute(maxSummons);
}

int LuaState::lua_Actor_setMaxSummons()
{
	return Actor_modAttribute(this, &CreatureType::maxSummons);
}

int LuaState::lua_Actor_setName()
{
	std::string value = popString();
	Actor* actor = popActor();
	actor->getType().name(value);
	//TODO: notify all clients about the name change

	pushBoolean(true);
	return 1;
}

int LuaState::lua_Actor_setNameDescription()
{
	std::string value = popString();
	Actor* actor = popActor();
	actor->getType().nameDescription(value);
	actor->updateNameDescription();
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Actor_setStaticAttackChance()
{
	return Actor_modAttribute(this, &CreatureType::staticAttackChance);
}

int LuaState::lua_Actor_getStaticAttackChance()
{
	return Actor_getAttribute(staticAttackChance);
}

int LuaState::lua_Actor_setFleeHealth()
{
	return Actor_modAttribute(this, &CreatureType::fleeHealth);
}

int LuaState::lua_Actor_getFleeHealth()
{
	return Actor_getAttribute(fleeHealth);
}

int LuaState::lua_Actor_setPushable()
{
	return Actor_modAttribute(this, &CreatureType::pushable);
}

int LuaState::lua_Actor_setBaseSpeed()
{
	int32_t value = popInteger();
	Actor* actor = popActor();
	actor->getType().base_speed(value);
	actor->updateBaseSpeed();
	g_game.changeSpeed(actor, 0);

	pushBoolean(true);
	return 1;
}

int LuaState::lua_Actor_setMaxHealth()
{
	int32_t value = popInteger();
	Actor* actor = popActor();
	actor->getType().health_max(value);
	actor->updateMaxHealth();
	g_game.addCreatureHealth(actor);

	pushBoolean(true);
	return 1;
}

int LuaState::lua_Actor_getCorpseId()
{
	return Actor_getAttribute(corpseId);
}

int LuaState::lua_Actor_setRace()
{
	return Actor_modAttribute(this, &CreatureType::race);
}

int LuaState::lua_Actor_getRace()
{
	return Actor_getAttribute(race);
}

int LuaState::lua_Actor_isSummonable()
{
	return Actor_getAttribute(isSummonable);
}

int LuaState::lua_Actor_isConvinceable()
{
	return Actor_getAttribute(isConvinceable);
}

int LuaState::lua_Actor_isIllusionable()
{
	return Actor_getAttribute(isIllusionable);
}

int LuaState::lua_Actor_setCanBeAttacked()
{
	return Actor_modAttribute(this, &CreatureType::isAttackable);
}

int LuaState::lua_Actor_getCanBeAttacked()
{
	return Actor_getAttribute(isAttackable);
}

int LuaState::lua_Actor_setCanBeLured()
{
	return Actor_modAttribute(this, &CreatureType::isLureable);
}

int LuaState::lua_Actor_getCanBeLured()
{
	return Actor_getAttribute(isLureable);
}

int LuaState::lua_Actor_setLightLevel()
{
	int32_t value = popInteger();
	Actor* actor = popActor();
	actor->getType().lightLevel(value);
	actor->updateLightLevel();
	g_game.changeLight(actor);

	pushBoolean(true);
	return 1;
}

int LuaState::lua_Actor_getLightLevel()
{
	return Actor_getAttribute(lightLevel);
}

int LuaState::lua_Actor_setLightColor()
{
	int32_t value = popInteger();
	Actor* actor = popActor();
	actor->getType().lightColor(value);
	actor->updateLightColor();
	g_game.changeLight(actor);

	pushBoolean(true);
	return 1;
}

int LuaState::lua_Actor_getLightColor()
{
	return Actor_getAttribute(lightColor);
}

int LuaState::lua_Actor_getManaCost()
{
	return Actor_getAttribute(manaCost);
}

int LuaState::lua_Actor_setTarget()
{
	Creature* target = popCreature(ERROR_PASS);
	Actor* actor = popActor();
	pushBoolean(actor->setAttackedCreature(target));
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Class Player

int LuaState::lua_Player_getFood()
{
	Player* p = popPlayer();
	Condition* condition = p->getCondition(CONDITION_REGENERATION, CONDITIONID_DEFAULT, 0);
	if(condition){
		push(condition->getTicks() / 1000);
	}
	else{
		pushInteger(0);
	}
	return 1;
}

int LuaState::lua_Player_getLevel()
{
	Player* p = popPlayer();
	push(p->getLevel());
	return 1;
}

int LuaState::lua_Player_getMagicLevel()
{
	Player* p = popPlayer();
	push(p->getMagicLevel());
	return 1;
}

int LuaState::lua_Player_isPremium()
{
	Player* player = popPlayer();
	push(player->isPremium());
	return 1;
}

int LuaState::lua_Player_getAccess()
{
	Player* p = popPlayer();
	push(p->getAccessLevel());
	return 1;
}

int LuaState::lua_Player_getMana()
{
	Player* p = popPlayer();
	push(p->getMana());
	return 1;
}

int LuaState::lua_Player_getManaMax()
{
	Player* p = popPlayer();
	push(p->getMaxMana());
	return 1;
}

int LuaState::lua_Player_setMana()
{
	int32_t newval = popInteger();
	Player* p = popPlayer();
	g_game.combatChangeMana(NULL, p, newval - p->getMana(), false);
	push(true);
	return 1;
}

int LuaState::lua_Player_addManaSpent()
{
	int32_t mana = popInteger();
	Player* p = popPlayer();
	p->addManaSpent(mana);
	push(true);
	return 1;
}

int LuaState::lua_Player_getVocationID()
{
	Player* p = popPlayer();
	push(p->getVocationId());
	return 1;
}

int LuaState::lua_Player_getSoulPoints()
{
	Player* p = popPlayer();
	push(p->getPlayerInfo(PLAYERINFO_SOUL));
	return 1;
}

int LuaState::lua_Player_setSoulPoints()
{
	int32_t soul = popInteger();
	Player* p = popPlayer();
	p->changeSoul(soul - p->getPlayerInfo(PLAYERINFO_SOUL));
	push(true);
	return 1;
}

int LuaState::lua_Player_getFreeCap()
{
	Player* p = popPlayer();
	push(p->getFreeCapacity());
	return 1;
}

int LuaState::lua_Player_getMaximumCap()
{
	Player* p = popPlayer();
	push(p->getCapacity());
	return 1;
}

int LuaState::lua_Player_getGuildID()
{
	Player* p = popPlayer();
	push(p->getGuildId());
	return 1;
}

int LuaState::lua_Player_getGuildName()
{
	Player* p = popPlayer();
	push(p->getGuildName());
	return 1;
}

int LuaState::lua_Player_getGuildRank()
{
	Player* p = popPlayer();
	push(p->getGuildRank());
	return 1;
}

int LuaState::lua_Player_getGuildNick()
{
	Player* p = popPlayer();
	push(p->getGuildNick());
	return 1;
}

int LuaState::lua_Player_getSex()
{
	Player* p = popPlayer();
	push(p->getSex());
	return 1;
}

int LuaState::lua_Player_getTownID()
{
	Player* p = popPlayer();
	push(p->getTown());
	return 1;
}

int LuaState::lua_Player_getGUID()
{
	Player* p = popPlayer();
	push(p->getGUID());
	return 1;
}

int LuaState::lua_Player_getGroup()
{
	Player* p = popPlayer();
	push(p->getAccessGroup());
	return 1;
}

int LuaState::lua_Player_getPremiumDays()
{
	Player* p = popPlayer();
	push(p->getPremiumDays());
	return 1;
}

int LuaState::lua_Player_getLastLogin()
{
	Player* p = popPlayer();
	pushUnsignedInteger(p->getLastLoginSaved());
	return 1;
}

int LuaState::lua_Player_getSkullType()
{
	Player* p = popPlayer();
#ifdef __SKULLSYSTEM__
	push(p->getSkull());
#else
	pushInteger(0);
#endif
	return 1;
}
int LuaState::lua_Player_hasGroupFlag()
{
	int32_t f = popInteger();
	Player* player = popPlayer();
	if(f < 0 || f >= PlayerFlag_LastFlag)
		throw Error("Invalid player flag passed to function Player.hasGroupFlag!");
	pushBoolean(player->hasFlag((PlayerFlags)f));
	return 1;
}

int LuaState::lua_Player_addExperience()
{
	int64_t exp = (int64_t)popFloat();
	Player* p = popPlayer();
	p->addExperience(exp);
	return 1;
}

int LuaState::lua_Player_getInventoryItem()
{
	SlotType slot(popEnum<SlotType>());
	Player* player = popPlayer();

	Item* i = player->getInventoryItem(slot);
	if(i)
		pushThing(i);
	else
		pushNil();

	return 1;
}

int LuaState::lua_Player_getItemTypeCount()
{
	int32_t type = popInteger();
	Player* player = popPlayer();
	push(player->__getItemTypeCount(type));
	return 1;
}

int LuaState::lua_Player_countMoney()
{
	Player* player = popPlayer();
	pushInteger(g_game.getMoney(player));
	return 1;
}

int LuaState::lua_Player_addMoney()
{
	int32_t amount = popInteger();
	Player* player = popPlayer();
	pushBoolean(g_game.addMoney(NULL, player, amount));
	return 1;
}

int LuaState::lua_Player_removeMoney()
{
	int32_t amount = popInteger();
	Player* player = popPlayer();
	pushBoolean(g_game.removeMoney(NULL, player, amount));
	return 1;
}

int LuaState::lua_Player_setVocation()
{
	int32_t vocationID = popInteger();
	Player* player = popPlayer();

	if(g_vocations.getVocation(vocationID)) {
		player->setVocation(vocationID);
		pushBoolean(true);
	} else {
		pushBoolean(false);
	}
	return 1;
}

int LuaState::lua_Player_setTown()
{
	Town* town = popTown();
	Player* player = popPlayer();

	player->setMasterPos(town->getTemplePosition());
	player->setTown(town->getTownID());
	pushBoolean(true);

	return 1;
}

int LuaState::lua_Player_sendMessage()
{
	std::string text = popString();
	uint32_t messageClass = popUnsignedInteger();
	Player* player = popPlayer();

	player->sendTextMessage((MessageClasses)messageClass, text);
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Player_canUseSpell()
{
	std::string spellName = popString();
	Player* player = popPlayer();
	//TODO: player->canUseSpell()
	pushBoolean(false);
	return 1;
}

int LuaState::lua_Player_learnSpell()
{
	std::string spellName = popString();
	Player* player = popPlayer();
	//TODO: player->learnSpell()
	pushBoolean(false);
	return 1;
}

int LuaState::lua_Player_unlearnSpell()
{
	std::string spellName = popString();
	Player* player = popPlayer();
	//TODO: player->learnSpell()
	pushBoolean(false);
	return 1;
}

int LuaState::lua_Player_addItem()
{
	bool canDropOnMap = true;

	Item* item = popItem(ERROR_PASS);
	if(item == NULL) {
		pushInteger(RET_NOTPOSSIBLE);
		pushBoolean(false);
		return 2;
	}
	Player* player = popPlayer();

	if(item->getParent() != VirtualCylinder::virtualCylinder){
		// Must remove from previous parent...
		g_game.internalRemoveItem(NULL, item);
	}

	ReturnValue ret = RET_NOERROR;
	if(canDropOnMap){
		ret = g_game.internalPlayerAddItem(player, item);
	}
	else{
		ret = g_game.internalAddItem(NULL, player, item);
	}
	pushInteger(ret);
	pushBoolean(ret == RET_NOERROR);
	return 2;
}

int LuaState::lua_Player_removeItem()
{
	int32_t subtype = -1;
	int32_t count = 1;
	int32_t itemid = 0;
	if(getStackSize() > 3)
		count = popInteger();
	if(getStackSize() > 2)
		subtype = popInteger();
	itemid = popInteger();
	Player* player = popPlayer();
	
	const ItemType& it = Item::items[itemid];
	if(it.id == 0){
		pushBoolean(false);
		return 1;
	}
	if(it.stackable && subtype != -1 && count == 1)
		count = subtype;

	pushBoolean(g_game.removeItemOfType(NULL, player, itemid, count, subtype));
	return 1;
}

int LuaState::lua_getOnlinePlayers()
{
	newTable();
	int n = 1;
	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it){
		if(!it->second->isRemoved()){
			pushThing(it->second);
			setField(-2, n++);
		}
	}
	return 1 ;
}

int LuaState::lua_getPlayerByName()
{
	std::string name = popString();
	pushThing(g_game.getPlayerByName(name));
	return 1 ;
}

int LuaState::lua_getPlayersByName()
{
	std::string name = popString();
	std::vector<Player*> players = g_game.getPlayersByName(name);

	int n = 1;
	newTable();
	for(std::vector<Player*>::iterator i = players.begin(); i != players.end(); ++i){
		pushThing(*i);
		setField(-2, n++);
	}
	return 1 ;
}

int LuaState::lua_getPlayerByNameWildcard()
{
	std::string name = popString();

	Player* p = NULL;
	ReturnValue ret = g_game.getPlayerByNameWildcard(name, p);

	pushInteger(ret);
	pushThing(p);
	return 2;
}

int LuaState::lua_getPlayersByNameWildcard()
{
	std::string name = popString();
	std::vector<Player*> players = g_game.getPlayersByNameWildcard(name);

	int n = 1;
	newTable();
	for(std::vector<Player*>::iterator i = players.begin(); i != players.end(); ++i){
		pushThing(*i);
		setField(-2, n++);
	}
	return 1 ;
}

///////////////////////////////////////////////////////////////////////////////
// Class Item

int LuaState::lua_createItem()
{
	int32_t count = -1;
	if(getStackSize() > 1) {
		if(!isNil(-1))
			count = popInteger();
		else
			pop();
	}
	int32_t id = popUnsignedInteger();

	Item* item = Item::CreateItem((uint16_t)id, (count < 0? 0 : count));
	item->addRef();
	pushThing(item);
	// It will be freed if not assigned to any parent
	g_game.FreeThing(item);
	return 1;
}

int LuaState::lua_Item_getItemID()
{
	Item* item = popItem();
	pushInteger(item->getID());
	return 1;
}

int LuaState::lua_Item_getLongName()
{
	Item* item = popItem();
	pushString(item->getLongName());
	return 1;
}

int LuaState::lua_Item_getCount()
{
	Item* item = popItem();
	if(item->isStackable())
		pushInteger(item->getItemCount());
	else if(item->isRune())
		pushInteger(item->getCharges());
	else
		pushInteger(1);
	return 1;
}

int LuaState::lua_Item_getSubtype()
{
	Item* item = popItem();
	if(item->isRune() || item->isFluidContainer() || item->isSplash())
		pushInteger(item->getSubType());
	else
		pushNil();
	return 1;
}

int LuaState::lua_Item_getWeight()
{
	Item* item = popItem();
	pushFloat(item->getWeight());
	return 1;
}

int LuaState::lua_Item_isPickupable()
{
	Item* item = popItem();
	pushBoolean(item->isPickupable());
	return 1;
}

// Attributes!

// Template function to prevent code duplication
template<typename T>
int setItemAttribute(LuaState* state)
{
	T value = state->popValue<T>();
	std::string key = state->popString();
	Item* item = state->popItem();

	item->setAttribute(key, value);

	state->pushBoolean(true);
	return 1;
}

#define exposeItemAttribute(Name, Type) \
int LuaState::lua_Item_set ## Name ## Attribute()                                   \
{                                                                                   \
	return setItemAttribute<Type>(this);                                            \
}

exposeItemAttribute(String, std::string)
exposeItemAttribute(Integer, int32_t)
exposeItemAttribute(Float, float)
exposeItemAttribute(Boolean, bool)


int LuaState::lua_Item_getRawAttribute()
{
	std::string key = popString();
	Item* item = popItem();

	boost::any value = item->getAttribute(key);

	if(value.empty())
		pushNil();
	if(value.type() == typeid(std::string))
		push(boost::any_cast<std::string>(value));
	if(value.type() == typeid(int32_t))
		push(boost::any_cast<int32_t>(value));
	if(value.type() == typeid(float))
		push(boost::any_cast<float>(value));
	if(value.type() == typeid(bool))
		push(boost::any_cast<bool>(value));
	else
		pushNil();
	return 1;
}

int LuaState::lua_Item_eraseAttribute()
{
	std::string key = popString();
	Item* item = popItem();

	item->eraseAttribute(key);

	pushBoolean(true);
	return 1;
}


int LuaState::lua_Item_setItemID()
{
	int32_t newcount = -1;
	if(getStackSize() > 2) {
		newcount = popInteger();
	}

	int32_t newid = popInteger();
	Item* item = popItem();

	if(newid < 0 || newid > 65535) {
		throw Error("Item.setItemID : item ID provided");
	}

	const ItemType& it = Item::items[newid];
	if(it.stackable && newcount > 100 || newcount < -1){
		throw Error("Item.setItemID : Stack count is out of range.");
	}

	g_game.transformItem(NULL, item, newid, newcount);

	pushBoolean(true);
	return 1;
}

int LuaState::lua_Item_startDecaying()
{
	Item* item = popItem();
	g_game.startDecay(item);
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Item_setCount()
{
	int32_t newcount = popInteger();
	Item* item = popItem();
	if(!item->isStackable() && !item->isRune()) {
		throw Error("Item.setCount: Item is not stackable!");
	}
	if(newcount < 1 || newcount > 100) {
		throw Error("Item.setCount: New count out of range!");
	}

	pushBoolean(g_game.transformItem(NULL, item, item->getID(), newcount) != NULL);
	return 1;
}

int LuaState::lua_Item_setSubtype()
{
	int32_t newtype = popInteger();
	Item* item = popItem();
	if(!item->isRune() && !item->isFluidContainer() && !item->isSplash()) {
		throw Error("Item.setSubtype: Item does not have a subtype!");
	}
	if(newtype < 0 || newtype > 100) {
		throw Error("Item.setSubtype: New subtype out of range!");
	}

	pushBoolean(g_game.transformItem(NULL, item, item->getID(), newtype) != NULL);
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Class Container
int LuaState::lua_Container_addItem()
{
	Item* item = popItem(ERROR_PASS);
	if(item == NULL) {
		pushInteger(RET_NOTPOSSIBLE);
		pushBoolean(false);
		return 2;
	}
	Container* container = popContainer();

	if(item->getParent() != VirtualCylinder::virtualCylinder){
		// Must remove from previous parent...
		g_game.internalRemoveItem(NULL, item);
	}

	ReturnValue ret = g_game.internalAddItem(NULL, container, item);
	pushInteger(ret);
	pushBoolean(ret == RET_NOERROR);
	return 2;
}

int LuaState::lua_Container_getItem()
{
	int32_t index = popInteger();
	Container* container = popContainer();
	push(container->getItem(index));
	return 1;
}

int LuaState::lua_Container_getSize()
{
	Container* container = popContainer();
	pushInteger(container->size());
	return 1;
}

int LuaState::lua_Container_getCapacity()
{
	Container* container = popContainer();
	pushInteger(container->capacity());
	return 1;
}

int LuaState::lua_Container_getItems()
{
	Container* container = popContainer();

	newTable();
	int n = 1;
	for(ItemList::const_iterator it = container->getItems(); it != container->getEnd(); ++it){
		pushThing(*it);
		setField(-2, n++);
	}
	return 1;
}

int LuaState::lua_getItemIDByName()
{
	std::string name = popString();

	int32_t itemid = Item::items.getItemIdByName(name);
	if(itemid == -1){
		pushNil();
	} else {
		pushUnsignedInteger(itemid);
	}
	return 1;
}

int LuaState::lua_getItemType()
{
	int32_t itemid = popInteger();

	newTable();

	const ItemType& it = Item::items[itemid];

	if(it.id == 0){
		pushNil();
		return 1;
	}

	setField(-1, "id", it.id);
	setField(-1, "clientID", it.clientId);

	setField(-1, "isGround", it.isGroundTile());
	setField(-1, "isContainer", it.isContainer());
	setField(-1, "isSplash", it.isSplash());
	setField(-1, "isFluidContainer", it.isFluidContainer());
	setField(-1, "isDoor", it.isDoor());
	setField(-1, "isMagicField", it.isMagicField());
	setField(-1, "isTeleport", it.isTeleport());
	setField(-1, "isKey", it.isKey());
	setField(-1, "isDepot", it.isDepot());
	setField(-1, "isMailbox", it.isMailbox());
	setField(-1, "isTrashHolder", it.isTrashHolder());
	setField(-1, "isRune", it.isRune());
	setField(-1, "isBed", it.isBed());
	setField(-1, "hasSpecialType", it.isRune() || it.isFluidContainer() || it.isSplash());

	setField(-1, "name", it.name);
	setField(-1, "article", it.article);
	setField(-1, "pluralName", it.pluralName);
	setField(-1, "description", it.description);
	setField(-1, "maxItems", it.maxItems);
	setField(-1, "weight", it.weight);
	setField(-1, "showCount", it.showCount);
	setField(-1, "weaponType", it.weaponType);
	setField(-1, "ammoType", it.ammoType);
	setField(-1, "shootType", it.shootType);
	setField(-1, "magicEffect", it.magicEffect);
	setField(-1, "attack", it.attack);
	setField(-1, "defense", it.defense);
	setField(-1, "extraDefense", it.extraDefense);
	setField(-1, "armor", it.armor);
	setField(-1, "slotPosition", it.slotPosition);
	setField(-1, "isVertical", it.isVertical);
	setField(-1, "isHorizontal", it.isHorizontal);
	setField(-1, "isHangable", it.isHangable);
	setField(-1, "allowDistRead", it.allowDistRead);
	setField(-1, "clientCharges", it.clientCharges);
	setField(-1, "speed", it.speed);
	setField(-1, "decayTo", it.decayTo);
	setField(-1, "decayTime", it.decayTime);
	setField(-1, "stopTime", it.stopTime);
	setField(-1, "corpseType", it.corpseType);

	setField(-1, "canReadText", it.canReadText);
	setField(-1, "canWriteText", it.canWriteText);
	setField(-1, "maxTextLen", it.maxTextLen);
	setField(-1, "writeOnceItemID", it.writeOnceItemId);

	setField(-1, "stackable", it.stackable);
	setField(-1, "useable", it.useable);
	setField(-1, "moveable", it.moveable);
	setField(-1, "alwaysOnTop", it.alwaysOnTop);
	setField(-1, "alwaysOnTopOrder", it.alwaysOnTopOrder);
	setField(-1, "pickupable", it.pickupable);
	setField(-1, "rotateable", it.rotateable);
	setField(-1, "rotateTo", it.rotateTo);

	setField(-1, "runeMagicLevel", it.runeMagicLevel);
	setField(-1, "runeLevel", it.runeLevel);
	setField(-1, "runeSpellName", it.runeSpellName);

	setField(-1, "wieldInfo", it.wieldInfo);
	setField(-1, "vocationString", it.vocationString);
	setField(-1, "minRequiredLevel", it.minRequiredLevel);
	setField(-1, "minRequiredMagicLevel", it.minRequiredMagicLevel);

	setField(-1, "lightLevel", it.lightLevel);
	setField(-1, "lightColor", it.lightColor);

	setField(-1, "floorChangeDown", it.floorChangeDown);
	setField(-1, "floorChangeNorth", it.floorChangeNorth);
	setField(-1, "floorChangeSouth", it.floorChangeSouth);
	setField(-1, "floorChangeEast", it.floorChangeEast);
	setField(-1, "floorChangeWest", it.floorChangeWest);
	setField(-1, "hasHeight", it.hasHeight);

	setField(-1, "blockSolid", it.blockSolid);
	setField(-1, "blockProjectile", it.blockProjectile);
	setField(-1, "blockPathFind", it.blockPathFind);
	setField(-1, "allowPickupable", it.allowPickupable);

	setField(-1, "bedPartnerDirection", it.bedPartnerDirection);
	setField(-1, "maleSleeperID", it.maleSleeperID);
	setField(-1, "femaleSleeperID", it.femaleSleeperID);
	setField(-1, "noSleeperID", it.noSleeperID);

	setField(-1, "transformEquipTo", it.transformEquipTo);
	setField(-1, "transformDeEquipTo", it.transformDeEquipTo);
	setField(-1, "showDuration", it.showDuration);
	setField(-1, "showCharges", it.showCharges);
	setField(-1, "charges", it.charges);
	setField(-1, "breakChance", it.breakChance);
	setField(-1, "hitChance", it.hitChance);
	setField(-1, "maxHitChance", it.maxHitChance);
	setField(-1, "shootRange", it.shootRange);
	setField(-1, "ammoAction", it.ammoAction);
	setField(-1, "fluidSource", it.fluidSource);

	return 1;
}

int LuaState::lua_isValidItemID()
{
	int32_t id = popInteger();

	const ItemType& it = Item::items[(uint16_t)id];
	pushBoolean(id >= 100 && it.id != 0);
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Combat functions

#if 0

int LuaState::lua_Combat_create()
{
	pushCombat(new Combat);
	return 1;
}

bool LuaState::getArea(std::list<uint32_t>& list, uint32_t& rows)
{
	rows = 0;
	uint32_t i = 0, j = 0;
	pushNil();  // first key //

	while(lua_next(L, -2) != 0){
		pushNil();
		while(lua_next(L, -2) != 0){
			list.push_back(popNumber());
			++j;
		}

		++rows;

		j = 0;
		pop();
		++i;
	}

	pop();
	return (rows != 0);
}

int LuaState::lua_CombatArea_create()
{
	//createCombatArea( {area}, <optional> {extArea} )
	int32_t parameters = lua_gettop(m_luaState);

	AreaCombat* area = new AreaCombat;

	if(parameters > 1)
	{
		//has extra parameter with diagonal area information

		uint32_t rowsExtArea;
		std::list<uint32_t> listExtArea;
		getArea(listExtArea, rowsExtArea);

		// setup all possible rotations
		area->setupExtArea(listExtArea, rowsExtArea);
	}

	uint32_t rowsArea = 0;
	std::list<uint32_t> listArea;
	getArea(listArea, rowsArea);

	// setup all possible rotations
	area->setupArea(listArea, rowsArea);

	pushCombatArea(area);
	return 1;
}

int LuaState::lua_Condition_create()
{
	ConditionType type = (ConditionType)popNumber();

	Condition* condition = Condition::createCondition(CONDITIONID_COMBAT, type, 0, 0);
	if(!condition)
		throw Error("createCondition: Invalid argument 'type', unknown condition type!");

	pushCondition(condition);
	return 1;
}

int LuaState::lua_Combat_setArea()
{
	AreaCombat* area = popCombatArea();
	Combat* combat = popCombat();

	combat->setArea(new AreaCombat(*area));

	pushBoolean(true);
	return 1;
}

int LuaState::lua_Combat_setCondition()
{
	Condition* condition = popCondition();
	Combat* combat = popCombat();

	combat->setCondition(condition);
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Combat_setParameter()
{
	uint32_t value = popUnsignedInteger();
	CombatParam key = (CombatParam)popInteger();
	Combat* combat = popCombat();

	combat->setParam(key, value);
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Condition_setParameter()
{
	//setConditionParam(condition, key, value)
	int32_t value = popInteger();
	ConditionParam key = (ConditionParam)popInteger();
	Condition* condition = popCondition();

	condition->setParam(key, value);
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Condition_addDamage()
{
	//addDamageCondition(condition, rounds, time, value)
	int32_t value = popInteger();
	int32_t time = popInteger();
	int32_t rounds  = popInteger();
	ConditionDamage* condition = dynamic_cast<ConditionDamage*>(popCondition());

	if(!condition)
		throw Error("Condition.addDamage: Condition is not a Damage Condition!");
	condition->addDamage(rounds, time, value);
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Condition_addOutfit()
{
	OutfitType outfit = popOutfit();
	ConditionOutfit* condition = dynamic_cast<ConditionOutfit*>(popCondition());

	if(!condition)
		throw Error("Condition.addOutfit: Condition is not an Outfit Condition!");
	condition->addOutfit(outfit);
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Combat_setCallback()
{
	//setCombatCallBack(combat, key, function)
	std::string function = putAwayFunction(function_str); // Save function
	CallBackParam_t key = (CallBackParam_t)popNumber();
	Combat* combat = popCombat();
	
	combat->setCallback(
	//setCombatCallBack(combat, key, function_name)
	const char* function = popString(L);
	std::string function_str(function);
	CallBackParam_t key = (CallBackParam_t)popNumber(L);
	uint32_t combatId = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	if(env->getScriptId() != EVENT_ID_LOADING){
		reportError(__FUNCTION__, "This function can only be used while loading the script.");
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	Combat* combat = env->getCombatObject(combatId);

	if(!combat){
		throwLuaException(getErrorDesc(LUA_ERROR_COMBAT_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	LuaScriptInterface* scriptInterface = env->getScriptInterface();

	combat->setCallback(key);
	CallBack* callback = combat->getCallback(key);
	if(!callback){
		std::stringstream ss;
		ss << (uint32_t)key << " is not a valid callback key";
		reportError(__FUNCTION__, ss.str());
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	if(!callback->loadCallBack(scriptInterface, function_str)){
		reportError(__FUNCTION__, "Can not load callback");
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	lua_pushnumber(L, LUA_NO_ERROR);
	return 1;
}

int LuaState::luaSetCombatFormula(lua_State *L)
{
	//setCombatFormula(combat, type, mina, minb, maxa, maxb)
	ScriptEnvironment* env = getScriptEnv();

	if(env->getScriptId() != EVENT_ID_LOADING){
		reportError(__FUNCTION__, "This function can only be used while loading the script.");
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	double maxb = popFloatNumber(L);
	double maxa = popFloatNumber(L);
	double minb = popFloatNumber(L);
	double mina = popFloatNumber(L);

	FormulaType type = (FormulaType)popNumber(L);
	uint32_t combatId = popNumber(L);

	Combat* combat = env->getCombatObject(combatId);

	if(combat){
		combat->setPlayerCombatValues(type, mina, minb, maxa, maxb);

		lua_pushnumber(L, LUA_NO_ERROR);
	}
	else{
		throwLuaException(getErrorDesc(LUA_ERROR_COMBAT_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
	}
	return 1;
}

int LuaState::luaSetConditionFormula(lua_State *L)
{
	//setConditionFormula(condition, mina, minb, maxa, maxb)
	ScriptEnvironment* env = getScriptEnv();

	if(env->getScriptId() != EVENT_ID_LOADING){
		reportError(__FUNCTION__, "This function can only be used while loading the script.");
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	double maxb = popFloatNumber(L);
	double maxa = popFloatNumber(L);
	double minb = popFloatNumber(L);
	double mina = popFloatNumber(L);

	uint32_t conditionId = popNumber(L);

	ConditionSpeed* condition = dynamic_cast<ConditionSpeed*>(env->getConditionObject(conditionId));

	if(condition){
		condition->setFormulaVars(mina, minb, maxa, maxb);

		lua_pushnumber(L, LUA_NO_ERROR);
	}
	else{
		throwLuaException(getErrorDesc(LUA_ERROR_CONDITION_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
	}
	return 1;
}

int LuaState::luaDoCombat(lua_State *L)
{
	//doCombat(cid, combat, param)
	ScriptEnvironment* env = getScriptEnv();

	LuaVariant var = popVariant(L);
	uint32_t combatId = (uint32_t)popNumber(L);
	uint32_t cid = (uint32_t)popNumber(L);

	Creature* creature = NULL;

	if(cid != 0){
		creature = env->getCreatureByUID(cid);

		if(!creature){
			throwLuaException(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushnumber(L, LUA_ERROR);
			return 1;
		}
	}

	const Combat* combat = env->getCombatObject(combatId);

	if(!combat){
		throwLuaException(getErrorDesc(LUA_ERROR_COMBAT_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	if(var.type == VARIANT_NONE){
		throwLuaException(getErrorDesc(LUA_ERROR_VARIANT_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	switch(var.type){
		case VARIANT_NUMBER:
		{
			Creature* target = g_game.getCreatureByID(var.number);

			if(!target){
				lua_pushnumber(L, LUA_ERROR);
				return 1;
			}

			if(combat->hasArea()){
				combat->doCombat(creature, target->getPosition());
				//std::cout << "Combat->hasArea()" << std::endl;
			}
			else{
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
			if(combat->hasArea()){
				combat->doCombat(creature, var.pos);
			}
			else{
				combat->postCombatEffects(creature, var.pos);
				g_game.addMagicEffect(var.pos, NM_ME_PUFF);
			}
			break;
		}

		case VARIANT_STRING:
		{
			Player* target = g_game.getPlayerByName(var.text);
			if(!target){
				lua_pushnumber(L, LUA_ERROR);
				return 1;
			}

			combat->doCombat(creature, target);
			break;
		}

		default:
		{
			throwLuaException(getErrorDesc(LUA_ERROR_VARIANT_UNKNOWN));
			lua_pushnumber(L, LUA_ERROR);
			return 1;
			break;
		}
	}

	lua_pushnumber(L, LUA_NO_ERROR);
	return 1;
}

int LuaState::luaDoAreaCombatHealth(lua_State *L)
{
	//doAreaCombatHealth(cid, type, pos, area, min, max, effect)
	uint8_t effect = (uint8_t)popNumber(L);
	int32_t maxChange = (int32_t)popNumber(L);
	int32_t minChange = (int32_t)popNumber(L);
	uint32_t areaId = popNumber(L);

	PositionEx pos;
	popPosition(L, pos);

	CombatType combatType = (CombatType)popNumber(L);
	uint32_t cid = (uint32_t)popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = NULL;

	if(cid != 0){
		creature = env->getCreatureByUID(cid);

		if(!creature){
			throwLuaException(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushnumber(L, LUA_ERROR);
			return 1;
		}
	}

	const AreaCombat* area = env->getCombatArea(areaId);
	if(area || areaId == 0){
		CombatParams params;
		params.combatType = combatType;
		params.impactEffect = effect;
		Combat::doCombatHealth(creature, pos, area, minChange, maxChange, params);

		lua_pushnumber(L, LUA_NO_ERROR);
	}
	else{
		throwLuaException(getErrorDesc(LUA_ERROR_AREA_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
	}

	return 1;
}

int LuaState::luaDoTargetCombatHealth(lua_State *L)
{
	//doTargetCombatHealth(cid, target, type, min, max, effect)
	uint8_t effect = (uint8_t)popNumber(L);
	int32_t maxChange = (int32_t)popNumber(L);
	int32_t minChange = (int32_t)popNumber(L);
	CombatType combatType = (CombatType)popNumber(L);
	uint32_t targetCid = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = NULL;

	if(cid != 0){
		creature = env->getCreatureByUID(cid);

		if(!creature){
			throwLuaException(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushnumber(L, LUA_ERROR);
			return 1;
		}
	}

	Creature* target = env->getCreatureByUID(targetCid);
	if(target){
		CombatParams params;
		params.combatType = combatType;
		params.impactEffect = effect;
		Combat::doCombatHealth(creature, target, minChange, maxChange, params);

		lua_pushnumber(L, LUA_NO_ERROR);
	}
	else{
		throwLuaException(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
	}
	return 1;
}

int LuaState::luaDoAreaCombatMana(lua_State *L)
{
	//doAreaCombatMana(cid, pos, area, min, max, effect)
	uint8_t effect = (uint8_t)popNumber(L);
	int32_t maxChange = (int32_t)popNumber(L);
	int32_t minChange = (int32_t)popNumber(L);
	uint32_t areaId = popNumber(L);

	PositionEx pos;
	popPosition(L, pos);

	uint32_t cid = (uint32_t)popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = NULL;

	if(cid != 0){
		creature = env->getCreatureByUID(cid);

		if(!creature){
			throwLuaException(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushnumber(L, LUA_ERROR);
			return 1;
		}
	}

	const AreaCombat* area = env->getCombatArea(areaId);
	if(area || areaId == 0){
		CombatParams params;
		params.impactEffect = effect;
		Combat::doCombatMana(creature, pos, area, minChange, maxChange, params);

		lua_pushnumber(L, LUA_NO_ERROR);
	}
	else{
		throwLuaException(getErrorDesc(LUA_ERROR_AREA_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
	}
	return 1;
}

int LuaState::luaDoTargetCombatMana(lua_State *L)
{
	//doTargetCombatMana(cid, target, min, max, effect)
	uint8_t effect = (uint8_t)popNumber(L);
	int32_t maxChange = (int32_t)popNumber(L);
	int32_t minChange = (int32_t)popNumber(L);
	uint32_t targetCid = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = NULL;

	if(cid != 0){
		creature = env->getCreatureByUID(cid);

		if(!creature){
			throwLuaException(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushnumber(L, LUA_ERROR);
			return 1;
		}
	}

	Creature* target = env->getCreatureByUID(targetCid);
	if(target){
		CombatParams params;
		params.impactEffect = effect;
		Combat::doCombatMana(creature, target, minChange, maxChange, params);

		lua_pushnumber(L, LUA_NO_ERROR);
	}
	else{
		throwLuaException(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
	}
	return 1;
}

int LuaState::luaDoAreaCombatCondition(lua_State *L)
{
	//doAreaCombatCondition(cid, pos, area, condition, effect)
	uint8_t effect = (uint8_t)popNumber(L);
	uint32_t conditionId = popNumber(L);
	uint32_t areaId = popNumber(L);
	PositionEx pos;
	popPosition(L, pos);
	uint32_t cid = (uint32_t)popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = NULL;

	if(cid != 0){
		creature = env->getCreatureByUID(cid);
		if(!creature){
			throwLuaException(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushnumber(L, LUA_ERROR);
			return 1;
		}
	}

	const Condition* condition = env->getConditionObject(conditionId);
	if(condition){

		const AreaCombat* area = env->getCombatArea(areaId);

		if(area || areaId == 0){
			CombatParams params;
			params.impactEffect = effect;
			params.condition = condition;
			Combat::doCombatCondition(creature, pos, area, params);

			lua_pushnumber(L, LUA_NO_ERROR);
		}
		else{
			throwLuaException(getErrorDesc(LUA_ERROR_AREA_NOT_FOUND));
			lua_pushnumber(L, LUA_ERROR);
		}
	}
	else{
		throwLuaException(getErrorDesc(LUA_ERROR_CONDITION_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
	}
	return 1;
}

int LuaState::luaDoTargetCombatCondition(lua_State *L)
{
	//doTargetCombatCondition(cid, target, condition, effect)
	uint8_t effect = (uint8_t)popNumber(L);
	uint32_t conditionId = popNumber(L);
	uint32_t targetCid = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = NULL;

	if(cid != 0){
		creature = env->getCreatureByUID(cid);

		if(!creature){
			throwLuaException(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushnumber(L, LUA_ERROR);
			return 1;
		}
	}

	Creature* target = env->getCreatureByUID(targetCid);
	if(target){
		const Condition* condition = env->getConditionObject(conditionId);
		if(condition){
			CombatParams params;
			params.impactEffect = effect;
			params.condition = condition;
			Combat::doCombatCondition(creature, target, params);

			lua_pushnumber(L, LUA_NO_ERROR);
		}
		else{
			throwLuaException(getErrorDesc(LUA_ERROR_CONDITION_NOT_FOUND));
			lua_pushnumber(L, LUA_ERROR);
		}
	}
	else{
		throwLuaException(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
	}
	return 1;
}

int LuaState::luaDoAreaCombatDispel(lua_State *L)
{
	//doAreaCombatDispel(cid, pos, area, type, effect)
	uint8_t effect = (uint8_t)popNumber(L);
	ConditionType dispelType = (ConditionType)popNumber(L);
	uint32_t areaId = popNumber(L);
	PositionEx pos;
	popPosition(L, pos);
	uint32_t cid = (uint32_t)popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = NULL;

	if(cid != 0){
		creature = env->getCreatureByUID(cid);

		if(!creature){
			throwLuaException(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushnumber(L, LUA_ERROR);
			return 1;
		}
	}

	const AreaCombat* area = env->getCombatArea(areaId);
	if(area || areaId == 0){
		CombatParams params;
		params.impactEffect = effect;
		params.dispelType = dispelType;
		Combat::doCombatDispel(creature, pos, area, params);

		lua_pushnumber(L, LUA_NO_ERROR);
	}
	else{
		throwLuaException(getErrorDesc(LUA_ERROR_AREA_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
	}
	return 1;
}

int LuaState::luaDoTargetCombatDispel(lua_State *L)
{
	//doTargetCombatDispel(cid, target, type, effect)
	uint8_t effect = (uint8_t)popNumber(L);
	ConditionType dispelType = (ConditionType)popNumber(L);
	uint32_t targetCid = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = NULL;

	if(cid != 0){
		creature = env->getCreatureByUID(cid);

		if(!creature){
			throwLuaException(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
			lua_pushnumber(L, LUA_ERROR);
			return 1;
		}
	}

	Creature* target = env->getCreatureByUID(targetCid);
	if(target){
		CombatParams params;
		params.impactEffect = effect;
		params.dispelType = dispelType;
		Combat::doCombatDispel(creature, target, params);

		lua_pushnumber(L, LUA_NO_ERROR);
	}
	else{
		throwLuaException(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
	}

	return 1;
}

int LuaState::luaDoChallengeCreature(lua_State *L)
{
	//doChallengeCreature(cid, target)
	uint32_t targetCid = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);

	if(!creature){
		throwLuaException(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	Creature* target = env->getCreatureByUID(targetCid);
	if(!target){
		throwLuaException(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	target->challengeCreature(creature);
	lua_pushnumber(L, LUA_NO_ERROR);
	return 1;
}

int LuaState::luaDoConvinceCreature(lua_State *L)
{
	//doConvinceCreature(cid, target)
	uint32_t targetCid = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);

	if(!creature){
		throwLuaException(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	Creature* target = env->getCreatureByUID(targetCid);
	if(!target){
		throwLuaException(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	target->convinceCreature(creature);
	lua_pushnumber(L, LUA_NO_ERROR);
	return 1;
}

int LuaState::luaGetMonsterTargetList(lua_State *L)
{
	//getMonsterTargetList(cid)

	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(!creature){
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	Actor* monster = creature->getActor();
	if(!monster){
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	lua_newtable(L);
	uint32_t i = 1;
	const CreatureList& targetList = monster->getTargetList();
	for(CreatureList::const_iterator it = targetList.begin(); it != targetList.end(); ++it){
		if(monster->isTarget(*it)){
			uint32_t targetCid = env->addThing(*it);
			lua_pushnumber(L, i);
			lua_pushnumber(L, targetCid);
			lua_settable(L, -3);
			++i;
		}
	}

	return 1;
}

int LuaState::luaGetMonsterFriendList(lua_State *L)
{
	//getMonsterFriendList(cid)

	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(!creature){
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	Actor* monster = creature->getActor();
	if(!monster){
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	lua_newtable(L);
	uint32_t i = 1;
	Creature* friendCreature;
	const CreatureList& friendList = monster->getFriendList();
	for(CreatureList::const_iterator it = friendList.begin(); it != friendList.end(); ++it){
		friendCreature = *it;
		if(!friendCreature->isRemoved() && friendCreature->getPosition().z == monster->getPosition().z){
			uint32_t friendCid = env->addThing(*it);
			lua_pushnumber(L, i);
			lua_pushnumber(L, friendCid);
			lua_settable(L, -3);
			++i;
		}
	}

	return 1;
}

int LuaState::luaDoSetMonsterTarget(lua_State *L)
{
	//doSetMonsterTarget(cid, target)
	uint32_t targetCid = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(!creature){
		throwLuaException(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	Actor* monster = creature->getActor();
	if(!monster){
		throwLuaException(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	Creature* target = env->getCreatureByUID(targetCid);
	if(!target){
		throwLuaException(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	if(!monster->isSummon()){
		monster->selectTarget(target);
	}

	lua_pushnumber(L, LUA_NO_ERROR);
	return 1;
}

int LuaState::luaDoMonsterChangeTarget(lua_State *L)
{
	//doMonsterChangeTarget(cid)
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(!creature){
		throwLuaException(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	Actor* monster = creature->getActor();
	if(!monster){
		throwLuaException(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	if(!monster->isSummon()){
		monster->searchTarget(true);
	}

	lua_pushnumber(L, LUA_NO_ERROR);
	return 1;
}

int LuaState::luaDoAddCondition(lua_State *L)
{
	//doAddCondition(cid, condition)

	uint32_t conditionId = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(!creature){
		throwLuaException(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	Condition* condition = env->getConditionObject(conditionId);
	if(!condition){
		throwLuaException(getErrorDesc(LUA_ERROR_CONDITION_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	creature->addCondition(condition->clone());
	lua_pushnumber(L, LUA_NO_ERROR);
	return 1;
}

int LuaState::luaDoRemoveCondition(lua_State *L)
{
	//doRemoveCondition(cid, type)

	ConditionType conditionType = (ConditionType)popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnvironment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(!creature){
		throwLuaException(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	Condition* condition = creature->getCondition(conditionType, CONDITIONID_COMBAT);
	if(!condition){
		condition = creature->getCondition(conditionType, CONDITIONID_DEFAULT);
	}

	if(condition){
		creature->removeCondition(condition);
	}

	lua_pushnumber(L, LUA_NO_ERROR);
	return 1;
}
#endif

///////////////////////////////////////////////////////////////////////////////
// Town

int LuaState::lua_Town_getTemplePosition()
{
	Town* town = popTown();
	pushPosition(town->getTemplePosition());
	return 1;
}

int LuaState::lua_Town_getID()
{
	Town* town = popTown();
	pushInteger(town->getTownID());
	return 1;
}

int LuaState::lua_Town_getName()
{
	Town* town = popTown();
	pushString(town->getName());
	return 1;
}

int LuaState::lua_Town_getHouse()
{
	Houses& houses = Houses::getInstance();

	uint32_t houseid = 0;
	std::string name;

	if(isNumber()){
		houseid = popInteger();
		std::ostringstream os;
		os << houseid;
		name = os.str();
	}
	else{
		name = popString();

		for(HouseMap::iterator it = houses.getHouseBegin(); it != houses.getHouseEnd(); ++it){
			if(it->second->getName() == name)
				houseid = it->second->getHouseId();
		}
		if(houseid == 0)
			throw Script::Error("Town.getHouse : No house by the name '" + name + "' exists.");
	}
	Town* town = popTown();

	House* house = houses.getHouse(houseid);

	if(!house)
		throw Script::Error("Town.getHouse : No house by the name '" + name + "' exists.");
	if(house->getTownId() != town->getTownID())
		throw Script::Error("Town.getHouse : No house by the name '" + name + "' exists belongs to the town " + town->getName());

	pushHouse(house);
	return 1;
}

int LuaState::lua_Town_getHouses()
{
	Town* town = popTown();

	Houses& houses = Houses::getInstance();

	newTable();
	int n = 1;
	for(HouseMap::iterator it = houses.getHouseBegin(); it != houses.getHouseEnd(); ++it){
		if(it->second->getTownId() == town->getTownID()){
			pushHouse(it->second);
			setField(-2, n++);
		}
	}
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// House

int LuaState::lua_House_cleanHouse()
{
	House* house = popHouse();
	house->cleanHouse();
	pushBoolean(true);
	return 1;
}

int LuaState::lua_House_getDoors()
{
	House* house = popHouse();

	newTable();
	int n = 1;
	for(HouseDoorList::iterator hit = house->getDoorBegin(), end = house->getDoorEnd();
		hit != end; ++hit)
	{
		pushThing(*hit);
		setField(-2, n++);
	}

	return 1;
}

int LuaState::lua_House_getExitPosition()
{
	House* house = popHouse();
	pushPosition(house->getEntryPosition());
	return 1;
}

int LuaState::lua_House_getID()
{
	House* house = popHouse();
	pushUnsignedInteger(house->getHouseId());
	return 1;
}

int LuaState::lua_House_getInvitedList()
{
	House* house = popHouse();
	std::string list;
	if(!house->getAccessList(GUEST_LIST, list))
		pushNil();
	// Function is overloaded in lua to return table
	pushString(list);
	return 1;
}

int LuaState::lua_House_getName()
{
	House* house = popHouse();
	pushString(house->getName());
	return 1;
}

int LuaState::lua_House_getPaidUntil()
{
	House* house = popHouse();
	pushUnsignedInteger(house->getPaidUntil());
	return 1;
}

int LuaState::lua_House_getRent()
{
	House* house = popHouse();
	pushInteger(house->getRent());
	return 1;
}

int LuaState::lua_House_getSubownerList()
{
	House* house = popHouse();
	std::string list;
	if(!house->getAccessList(SUBOWNER_LIST, list))
		pushNil();
	// Function is overloaded in lua to return table
	pushString(list);
	return 1;
}

int LuaState::lua_House_getTiles()
{
	House* house = popHouse();
	
	newTable();
	int n = 1;
	for(HouseTileList::iterator hit = house->getTileBegin(), end = house->getTileEnd();
		hit != end; ++hit)
	{
		pushTile(*hit);
		setField(-2, n++);
	}

	return 1;
}

int LuaState::lua_House_getTown()
{
	House* house = popHouse();
	
	pushTown(Towns::getInstance().getTown(house->getTownId()));
	return 1;
}
int LuaState::lua_House_kickPlayer()
{
	Player* who = popPlayer();
	House* house = popHouse();
	pushBoolean(house->kickPlayer(NULL, who->getName()));
	return 1;
}

int LuaState::lua_House_isInvited()
{
	Player* player = popPlayer();
	House* house = popHouse();
	pushBoolean(house->isInvited(player));
	return 1;
}

int LuaState::lua_House_setOwner()
{
	uint32_t guid = popUnsignedInteger();
	House* house = popHouse();

	house->setHouseOwner(guid);

	return 1;
}

int LuaState::lua_House_setInviteList()
{
	std::ostringstream list;

	// Flatten list
	pushNil();
	while(lua_next(state, -2) != 0) {
		list << popString() << "\n";
	}
	pop();

	House* house = popHouse();

	house->setAccessList(GUEST_LIST, list.str());

	return 1;
}

int LuaState::lua_House_setSubownerList()
{
	std::ostringstream list;

	// Flatten list
	pushNil();
	while(lua_next(state, -2) != 0) {
		list << popString() << "\n";
	}
	pop();

	House* house = popHouse();

	house->setAccessList(SUBOWNER_LIST, list.str());

	return 1;
}

int LuaState::lua_House_setPaidUntil()
{
	uint32_t until = popUnsignedInteger();
	House* house = popHouse();
	house->setPaidUntil(until);
	pushBoolean(true);
	return 1;
}


///////////////////////////////////////////////////////////////////////////////
// Channel

int LuaState::lua_Channel_getID()
{
	ChatChannel* channel = popChannel();
	push(channel->getId());
	return 1;
}

int LuaState::lua_Channel_getName()
{
	ChatChannel* channel = popChannel();
	push(channel->getName());
	return 1;
}

int LuaState::lua_Channel_getUsers()
{
	ChatChannel* channel = popChannel();
	newTable();
	int n = 1;
	for(UsersMap::const_iterator iter = channel->getUsers().begin(); iter != channel->getUsers().end(); ++iter)
	{
		pushThing(iter->second);
		setField(-2, n++);
	}
	return 1;
}

int LuaState::lua_Channel_addUser()
{
	Player* user = popPlayer();
	ChatChannel* channel = popChannel();
	g_game.playerOpenChannel(user->getID(), channel->getId());
	return 1;
}

int LuaState::lua_Channel_removeUser()
{
	Player* user = popPlayer();
	ChatChannel* channel = popChannel();
	g_game.playerCloseChannel(user->getID(), channel->getId());
	user->sendClosePrivate(channel->getId());
	return 1;
}

int LuaState::lua_Channel_talk()
{
	std::string text = popString();
	int32_t t = popInteger();
	/*Player* user = */popPlayer(ERROR_PASS);
	ChatChannel* channel = popChannel();
	channel->talk(NULL, (SpeakClass)t, text);
	pushBoolean(true);
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// (Class) Waypoint

int LuaState::lua_getWaypointByName()
{
	std::string name = popString();

	Waypoint_ptr wp = g_game.getMap()->waypoints.getWaypointByName(name);
	if(wp){
		pushWaypoint(wp);
	}
	else{
		pushNil();
	}
	return 1;
}

int LuaState::lua_Waypoint_getPosition()
{
	Waypoint_ptr wp = popWaypoint();

	pushPosition(wp->pos);
	return 1;
}

int LuaState::lua_Waypoint_getName()
{
	Waypoint_ptr wp = popWaypoint();

	pushString(wp->name);
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// (Class) Game

int LuaState::lua_sendMagicEffect()
{
	uint32_t type = popUnsignedInteger();
	Position pos = popPosition();
	
	g_game.addMagicEffect(pos, type);
	pushBoolean(true);
	return 1;
}

int LuaState::lua_sendAnimatedText()
{
	std::string text = popString();
	uint32_t color = popUnsignedInteger();
	Position pos = popPosition();

	if(text.length() > 8)
		throw Error("Invalid value for parameter 'text': Animated text must be less than 8 characters long.");
	if(color > 255)
		throw Error("Invalid value for parameter 'color': Color is an unsigned integer between 0 and 255.");

	g_game.addAnimatedText(pos, color, text);
}

int LuaState::lua_getTile()
{
	int32_t z = popInteger();
	int32_t y = popInteger();
	int32_t x = popInteger();
	pushTile(g_game.getTile(x, y, z));
	return 1;
}

int LuaState::lua_getTowns()
{
	Towns& towns = Towns::getInstance();

	newTable();
	int n = 1;
	for(TownMap::const_iterator i = towns.getTownBegin(); i != towns.getTownEnd(); ++i){
		pushTown(const_cast<Town*>((*i).second));
		setField(-2, n++);
	}
	return 1;
}

int LuaState::lua_sendMailTo()
{
	Town* town = NULL;
	if(getStackSize() > 2) {
		town = popTown();
	}

	uint32_t townId = 0;
	std::string name = popString();

	if(town == NULL){
		if(!IOPlayer::instance()->getDefaultTown(name, townId)){
			pushBoolean(false);
			return 1;
		}
	}
	else{
		townId = town->getTownID();
	}

	Item* item = popItem();
	if(item == NULL) {
		pushBoolean(false);
		return 1;
	}

	bool result = IOPlayer::instance()->sendMail(NULL, name, townId, item);
	pushBoolean(result);
	return 1;
}

int LuaState::lua_getHouses()
{
	Houses& houses = Houses::getInstance();

	newTable();
	int n = 1;
	for(HouseMap::iterator it = houses.getHouseBegin(); it != houses.getHouseEnd(); ++it){
		pushHouse(it->second);
		setField(-2, n++);
	}

	return 1;
}

int LuaState::lua_getWorldType()
{
	pushEnum(g_game.getWorldType());
	return 1;
}

int LuaState::lua_getWorldTime()
{
	push(g_game.getLightHour());
	return 1;
}

int LuaState::lua_getWorldUpTime()
{
	push(Status::instance()->getUpTime());
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
	int32_t i, n = lua_gettop(L); \
	type w = (type)lua_tonumber(L, -1); \
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


#if 0
/* Unconverted lua functions...
 * Anyone feels up to the task of converting?
 */

int LuaScriptInterface::luaDoFeedPlayer()
{
	//doFeedPlayer(uid, food)
	int32_t food = (int32_t)popInteger();
	Player* player = popPlayer();

	player->addDefaultRegeneration(food * 1000);
	pushBoolean(true);
	return 1;
}

int LuaScriptInterface::luaDoPlayerAddSkillTry()
{
	//doPlayerAddSkillTry(cid, skillid, n)
	uint32_t n = popUnsignedInteger();
	uint32_t skillid = popUnsignedInteger();
	Player* player = popPlayer();

	if(skillid < SKILL_FIRST || skillid > SKILL_LAST) {
		throwLuaException("Invalid skill index!");
	}
	player->addSkillAdvance((SkillType)skillid, n);
	pushBoolean(true);
	return 1;
}

int LuaScriptInterface::luaDoRelocate()
{
	//doRelocate(pos, posTo)
	//Moves all moveable objects from pos to posTo

	PositionEx toPos = popPositionEx();
	PositionEx fromPos = popPositionEx();

	Tile* fromTile = g_game.getTile(fromPos.x, fromPos.y, fromPos.z);
	if(!fromTile){
		throwLuaException(getErrorDesc(LUA_ERROR_TILE_NOT_FOUND));
	}

	Tile* toTile = g_game.getTile(toPos.x, toPos.y, toPos.z);
	if(!toTile){
		throwLuaException(getErrorDesc(LUA_ERROR_TILE_NOT_FOUND));
	}

	int32_t thingCount = fromTile->getThingCount();
	for(int32_t i = thingCount - 1; i >= 0; --i){
		Thing* thing = fromTile->__getThing(i);
		if(thing){
			if(Item* item = thing->getItem()){
				if(item->isPushable()){
					g_game.internalTeleport(NULL, item, toPos);
				}
			}
			else if(Creature* creature = thing->getCreature()){
				g_game.internalTeleport(NULL, creature, toPos);
			}
		}
	}

	pushBoolean(true);
	return 1;
}

int LuaScriptInterface::luaDoSendAnimatedText()
{
	//doSendAnimatedText(pos, text, color)
	uint32_t color = popUnsignedInteger();
	std::string text = popString();
	PositionEx pos = popPosition();

	ScriptEnvironment* env = getScriptEnv();

	SpectatorVec list;
	SpectatorVec::iterator it;

	if(pos.x == 0xFFFF){
		pos = env->getRealPos();
	}

	g_game.addAnimatedText(pos, color, text);
	return 1;
}

int LuaScriptInterface::luaGetPlayerSkill()
{
	//getPlayerSkill(cid, skillid)
	uint32_t skillid = popUnsignedInteger();
	const Player* player = popPlayer();

	if(skillid < SKILL_FIRST || skillid > SKILL_LAST){
		throwLuaException("No valid skillId");
	}
	uint32_t value = player->skills[skillid][SKILL_LEVEL];
	pushNumber(value);
	return 1;
}

int LuaScriptInterface::luaGetPlayerLossPercent()
{
	//getPlayerLossPercent(cid, lossType)
	uint8_t lossType = (uint8_t)popUnsignedInteger();
	const Player* player = popPlayer();

	if(lossType > LOSS_LAST){
		throwLuaException("No valid lossType");
	}
	uint32_t value = player->getLossPercent((LossType)lossType);
	pushNumber(value);
	return 1;
}

int LuaScriptInterface::luaDoPlayerSetLossPercent()
{
	//doPlayerSetLossPercent(cid, lossType, newPercent)
	uint32_t newPercent = popUnsignedInteger();
	uint8_t lossType = (uint8_t)popUnsignedInteger();
	Player* player = popPlayer();

	if(lossType > LOSS_LAST){
		throwLuaException("Invalid valid lossType");
	}
	if(newPercent > 100){
		reportLuaError("lossPercent value higher than 100, truncated to 100");
		newPercent = 100;
	}
	player->setLossPercent((LossType)lossType, newPercent);
	pushBoolean(true);
	return 1;
}

int LuaScriptInterface::luaDoSetCreatureDropLoot()
{
	//doSetCreatureDropLoot(cid, doDrop)
	bool doDrop = popBoolean();
	Creature* creature = popCreature();

	creature->setDropLoot(doDrop);
	return 1;
}

int LuaScriptInterface::luaDoShowTextDialog()
{
	//doShowTextDialog(cid, itemid, text)
	std::string text = popString();
	uint32_t itemId = popUnsignedInteger();
	Player* player = popPlayer();

	player->setWriteItem(NULL, 0);
	player->sendTextWindow(itemId, text);
	pushBoolean(true);
	return 1;
}

int LuaScriptInterface::luaGetTileItemById()
{
	//getTileItemById(pos, itemId, <optional> subType)
	uint32_t parameters = lua_gettop(m_luaState);

	int32_t subType = -1;
	if(parameters > 2){
		subType = popInteger();
	}

	int32_t itemId = popInteger();

	PositionEx pos = popPosition();

	Tile* tile = g_game.getTile(pos.x, pos.y, pos.z);
	if(!tile){
		pushThing(NULL);
		return 1;
	}

	Item* item = g_game.findItemOfType(tile, itemId, false, subType);
	if(!item){
		pushThing(NULL);
		return 1;
	}

	pushThing(item);

	return 1;
}

int LuaScriptInterface::luaGetTileItemByType()
{
	//getTileItemByType(pos, type)
	uint32_t rType = popUnsignedInteger();

	if(rType >= ITEM_TYPE_LAST){
		throwLuaException("Not a valid item type");
		pushThing(NULL);
		return 1;
	}

	PositionEx pos = popPosition();

	Tile* tile = g_game.getTile(pos.x, pos.y, pos.z);
	if(!tile){
		pushThing(NULL);
		return 1;
	}

	for(uint32_t i = 0; i < tile->getThingCount(); ++i){
		if(Item* item = tile->__getThing(i)->getItem()){
			const ItemType& it = Item::items[item->getID()];
			if(it.type == (ItemTypes_t) rType){
				pushThing(item);
				return 1;
			}
		}
	}

	pushThing(NULL);
	return 1;
}

int LuaScriptInterface::luaGetTileThingByPos()
{
	//getTileThingByPos(pos)

	PositionEx pos = popPositionEx();

	ScriptEnvironment* env = getScriptEnv();

	Tile* tile = g_game.getTile(pos.x, pos.y, pos.z);
	if(!tile){
		if(pos.stackpos == -1){
			pushInteger(-1);
			return 1;
		}
		else{
			pushThing(NULL);
			return 1;
		}
	}

	if(pos.stackpos == -1){
		pushNumber(tile->getThingCount());
		return 1;
	}

	Thing* thing = tile->__getThing(pos.stackpos);
	if(!thing){
		pushThing(NULL);
		return 1;
	}

	pushThing(thing);
	return 1;
}

int LuaScriptInterface::luaGetTopCreature()
{
	//getTopCreature(pos)

	PositionEx pos = popPositionEx();

	Tile* tile = g_game.getTile(pos.x, pos.y, pos.z);
	if(!tile){
		pushThing(NULL);
		return 1;
	}

	Thing* thing = tile->getTopCreature();
	if(!thing || !thing->getCreature()){
		pushThing(NULL);
		return 1;
	}

	pushThing(thing);
	return 1;
}

int LuaScriptInterface::luaDoCreateItem()
{
	//doCreateItem(itemid [, subtype/count], pos)
	//Returns uid of the created item, only works on tiles.

	uint32_t parameters = lua_gettop(m_luaState);

	PositionEx pos;
	popPosition(pos);

	uint32_t count = 0;
	if(parameters > 2){
		count = popUnsignedInteger();
	}

	uint32_t itemId = popUnsignedInteger();

	Tile* tile = g_game.getTile(pos);
	if(!tile){
		tile = new Tile(pos);
		g_game.getMap()->setTile(pos, newtile);
	}

	const ItemType& it = Item::items[itemId];
	if(it.stackable && count > 100){
		throwLuaException("Stack count cannot be higher than 100.");
	}

	Item* newItem = Item::CreateItem(itemId, count);

	ReturnValue ret = g_game.internalAddItem(NULL, tile, newItem, INDEX_WHEREEVER, FLAG_NOLIMIT);
	if(ret != RET_NOERROR){
		delete newItem;
		pushThing(NULL);
		return 1;
	}

	// RTODO: MAKE ME RETURN STACKED ITEM!
	if(newItem->getParent()){
		pushThing(newItem);
	}
	else{
		//stackable item stacked with existing object, newItem will be released
		pushThing(NULL);
	}
	return 1;
}

int LuaScriptInterface::luaDoCreateItemEx()
{
	//doCreateItemEx(itemid, <optional> count/subtype)
	//Returns uid of the created item

	int32_t parameters = lua_gettop(m_luaState);

	uint32_t count = 0;
	if(parameters > 1){
		count = popUnsignedInteger();
	}

	uint32_t itemId = popUnsignedInteger();

	ScriptEnvironment* env = getScriptEnv();

	const ItemType& it = Item::items[itemId];
	if(it.stackable && count > 100){
		throwLuaException("Stack count cannot be higher than 100.");
	}

	Item* newItem = Item::CreateItem(itemId, count);
	if(!newItem){
		throwLuaException("Can not create item with id 0.");
	}

	newItem->setParent(VirtualCylinder::virtualCylinder);
	env->addTempItem(newItem);

	pushThing(newItem);
	return 1;
}

int LuaScriptInterface::luaGetTileHouseInfo()
{
	//getTileHouseInfo(pos)
	Position pos = popPosition();

	Tile *tile = g_game.getMap()->getTile(pos);

	if(HouseTile* houseTile = dynamic_cast<HouseTile*>(tile)){
		House* house = houseTile->getHouse();
		if(house){
			pushUnsignedInteger(house->getHouseId());
		}
		else{
			pushUnsignedInteger(0);
		}
	}
	else {
		pushBoolean(false);
	}
	return 1;
}

int LuaScriptInterface::luaGetHouseOwner()
{
	//getHouseOwner(house)
	House* house = popHouse();
	pushUnsignedInteger(house->getHouseOwner());
	return 1;
}

int LuaScriptInterface::luaGetHouseRent()
{
	//getHouseRent(house)
	House* house = popHouse();
	pushUnsignedInteger(house->getRent());
	return 1;
}

int LuaScriptInterface::luaGetHouseByPlayerGUID()
{
	//getHouseByPlayerGUID(playerGUID)
	uint32_t guid = popUnsignedInteger();
	pushHouse(Houses::getInstance().getHouseByPlayerId(guid));
	return 1;
}

int LuaScriptInterface::luaGetHouseTilesSize()
{
	//getHouseTilesSize(house)
	House* house = popHouse();
	pushNumber(house->getHouseTileSize());
	return 1;
}

int LuaScriptInterface::luaGetWorldCreatureCount()
{
	//getWorldCreatureCount(type)
	//0 players, 1 monsters, 2 npcs, 3 all
	uint32_t type = popUnsignedInteger();
	switch(type){
		case 0:
			pushNumber(g_game.getPlayersOnline());
			break;
		case 1:
			pushNumber(g_game.getMonstersOnline());
			break;
		case 2:
			pushNumber(g_game.getNpcsOnline());
			break;
		case 3:
			pushNumber(g_game.getCreaturesOnline());
			break;
		default:
			throwLuaException("Invalid creature type.");
			break;
	}
	return 1;
}

int LuaScriptInterface::luaBroadcastMessage()
{
	//broadcastMessage([messageClass,] message)
	uint32_t parameters = lua_gettop(m_luaState);
	std::string message = popString();
	uint32_t type = MSG_STATUS_WARNING;
	if(parameters == 2) {
		type = popUnsignedInteger();
		if(type >= 0x12 && type <= 0x19) {
		throwLuaException("Not valid messageClass.");
		}
	}
	g_game.anonymousBroadcastMessage(MSG_STATUS_WARNING, message);
	pushBoolean(true);
	return 1;
}

int LuaScriptInterface::luaGetCreatureLight()
{
	//getCreatureLight(creature)
	Creature* creature = popCreature();

	LightInfo lightInfo;
	creature->getCreatureLight(lightInfo);
	pushNumber(lightInfo.level);
	pushNumber(lightInfo.color);//color
	return 2;
}

int LuaScriptInterface::luaGetPlayerItemById()
{
	//getPlayerItemByID(cid, deepSearch, itemID [, subType])
	uint32_t parameters = lua_gettop(m_luaState);

	int32_t subType = -1;
	if(parameters > 3){
		subType = popInteger(L);
	}

	uint32_t itemId = popUnsignedInteger(L);
	bool deepSearch = popBoolean();
	Player* player = popPlayer();

	Item* item = g_game.findItemOfType(player, itemId, deepSearch, subType);
	pushThing(item);
	return 1;
}

int LuaScriptInterface::luaCountPlayerDepotItems()
{
	//countPlayerDepotItems(player, depot)
	uint32_t depotid = 0;
	if(isInteger()) {
		depotid = popUnsignedInteger();
	} else {
		depotid = popTown()->getTownID;
	}
	Player* player = popPlayer();

	const Depot* depot = player->getDepot(depotid, true);
	if(depot){
		pushUnsignedInteger(depot->getItemHoldingCount());
	}
	else{
		pushUnsignedInteger(0);
	}
	return 1;
}

int LuaScriptInterface::luaDoPlayerSetGuildRank()
{
	//doPlayerSetGuildRank(player, rank)
	std::string rank = popString();
	Player* player = popPlayer();

	player->setGuildRank(rank);
	pushBoolean(true);
	return 1;
}

int LuaScriptInterface::luaDoPlayerSetGuildNick()
{
	//doPlayerSetGuildNick(player, nick)
	std::string rank = popString();
	Player* player = popPlayer();

	player->setGuildNick(rank);
	pushBoolean(true);
	return true;
}

int LuaScriptInterface::luaGetGuildId()
{
	//getGuildID(guild_name)
	std::string name = popString();
	uint32_t guildId;
	if(IOPlayer::instance()->getGuildIdByName(guildId, std::string(name))){
		pushUnsignedInteger(guildId);
	}
	else{
		pushUnsignedInteger(0);
	}
	return 1;
}

int LuaScriptInterface::luaGetPlayerGUIDByName()
{
	//getPlayerGUIDByName(name)
	std::string name = popString();

	Player* player = g_game.getPlayerByName(name);
	uint32_t value = 0;

	if(player){
		value = player->getGUID();
	}
	else{
		uint32_t guid;
		if(IOPlayer::instance()->getGuidByName(guid, name)){
			value = guid;
		}
	}

	pushUnsignedInteger(value);
	return 1;
}

int LuaScriptInterface::luaGetContainerSize()
{
	//getContainerSize(container)
	Container* container = popContainer();
	pushInteger(container->size());
	return 1;
}

int LuaScriptInterface::luaGetContainerCap()
{
	//getContainerCap(container)
	Container* container = popContainer();
	pushInteger(container->capacity());
	return 1;
}

int LuaScriptInterface::luaGetContainerItem()
{
	//getContainerItem(container, slot)
	uint32_t slot = popUnsignedInteger();
	Container* container = popContainer();
	pushThing(container->getItem(slot));
	return 1;

}

int LuaScriptInterface::luaDoAddContainerItem()
{
	//doAddContainerItem(container, item)
	Item* item = popItem();
	Container* container = popContainer();

	Cylinder* old_parent = item->getParent();
	if(old_parent != container) {
		ReturnValue ret = g_game.internalMoveItem(NULL, old_parent, container, INDEX_WHEREEVER, item, item->getItemCount());
		if(ret != RET_NOERROR) {
			pushBoolean(false);
		}
	}
	pushBoolean(true);
}

int LuaScriptInterface::luaGetDepotId()
{
	//getDepotID(depot)
	Depot* depot = popDepot();
	pushUnsignedInteger(depot->getDepotId());
	return 1;
}

int LuaScriptInterface::luaDoPlayerAddOutfit()
{
	//doPlayerAddOutfit(player, looktype, addon)
	int addon = (int)popNumber();
	int looktype = (int)popNumber();
	Player* player = popPlayer();

	player->addOutfit(looktype, addon);
	pushBoolean(true);
	return 1;
}

int LuaScriptInterface::luaDoPlayerRemOutfit(lua_State *L)
{
	//doPlayerRemOutfit(player, looktype, addon)
	int addon = (int)popNumber(L);
	int looktype = (int)popNumber(L);
	Player* player = popPlayer();

	player->remOutfit(looktype, addon);
	pushBoolean(true);
	return 1;
}

int LuaScriptInterface::luaDoSetCreatureLight()
{
	//doSetCreatureLight(creature, lightLevel, lightColor, time)
	int32_t time = popInteger();
	uint32_t color = popUnsignedInteger();
	uint32_t level = popUnsignedInteger();
	Creature* creature = popCreature();

	color = std::max(color, 255);
	level = std::max(level, 255);

	Condition* condition = Condition::createCondition(CONDITIONID_COMBAT, CONDITION_LIGHT, time, level | (color << 8));
	creature->addCondition(condition);
	return 1;
}

int LuaScriptInterface::luaIsItemStackable()
{
	//isItemStackable(item/itemid)
	uint32_t itemid;
	if(isNumber()) {
		itemid = popUnsignedInteger();
	} else {
		itemid = popItem()->getID();
	}
	const ItemType& it = Item::items[itemid];
	pushBoolean(it.stackable);
	return 1;
}

int LuaScriptInterface::luaIsItemRune()
{
	//isItemRune(item/itemid)
	uint32_t itemid;
	if(isNumber()) {
		itemid = popUnsignedInteger();
	} else {
		itemid = popItem()->getID();
	}
	const ItemType& it = Item::items[itemid];
	pushBoolean(it.isRune());
	return 1;
}

int LuaScriptInterface::luaIsItemDoor()
{
	//isItemDoor(item/itemid)
	uint32_t itemid;
	if(isNumber()) {
		itemid = popUnsignedInteger();
	} else {
		itemid = popItem()->getID();
	}
	const ItemType& it = Item::items[itemid];
	pushBoolean(it.isDoor());
	return 1;
}

int LuaScriptInterface::luaIsItemContainer()
{
	//isItemContainer(item/itemid)
	uint32_t itemid;
	if(isNumber()) {
		itemid = popUnsignedInteger();
	} else {
		itemid = popItem()->getID();
	}
	const ItemType& it = Item::items[itemid];
	pushBoolean(it.isContainer());
	return 1;
}

int LuaScriptInterface::luaIsItemFluidContainer()
{
	//isItemFluidContainer(item/itemid)
	uint32_t itemid;
	if(isNumber()) {
		itemid = popUnsignedInteger();
	} else {
		itemid = popItem()->getID();
	}
	const ItemType& it = Item::items[itemid];
	pushBoolean(it.isFluidContainer());
	return 1;
}

int LuaScriptInterface::luaGetItemName(lua_State *L)
{
	//getItemName(item/itemid)
	uint32_t itemid;
	if(isNumber()) {
		itemid = popUnsignedInteger();
	} else {
		itemid = popItem()->getID();
	}
	const ItemType& it = Item::items[itemid];
	pushString(it.name);
	return 1;
}

int LuaScriptInterface::luaGetItemDescriptions()
{
	//getItemDescriptions(item/itemid)
	//returns the name, the article and the plural name of the item
	uint32_t itemid;
	if(isNumber()) {
		itemid = popUnsignedInteger();
	} else {
		itemid = popItem()->getID();
	}
	const ItemType& it = Item::items[itemid];

	lua_newtable(m_luaState);
	setField("name", it.name);
	setField("article", it.article);
	setField("plural", it.pluralName);
	return 1;
}

int LuaScriptInterface::luaGetItemWeight()
{
	//getItemWeight(item)
	uint32_t itemid;
	if(isNumber()) {
		itemid = popUnsignedInteger();
		const ItemType& it = Item::items[itemid];
		pushFloat(it.weight);
	} else {
		Item* item = popItem();
		pushFloat(item->getWeight());
	}
	return 1;
}

int LuaScriptInterface::luaGetDataDirectory()
{
	pushString(g_config.getString(ConfigManager::DATA_DIRECTORY));
	return 1;
}

int LuaScriptInterface::luaDoSendDistanceShoot()
{
	//doSendDistanceShoot(frompos, topos, type)
	uint8_t type = std::max(255, popUnsignedInteger());
	PositionEx toPos = popPosition();
	PositionEx fromPos = popPosition();

	ScriptEnvironment* env = getScriptEnv();

	if(fromPos.x == 0xFFFF){
		fromPos = env->getRealPos();
	}
	if(toPos.x == 0xFFFF){
		toPos = env->getRealPos();
	}

	g_game.addDistanceEffect(fromPos, toPos, type);
	pushBoolean(true);
	return 1;
}

int LuaScriptInterface::luaGetCreatureMaster()
{
	//getCreatureMaster(cid)
	//returns the creature's master or itself if the creature isn't a summon
	Creature* slave = popCreature();
	pushThing(creature->getMaster());
	return 1;
}

int LuaScriptInterface::luaGetCreatureSummons()
{
	//getCreatureSummons(creature)
	//returns a table with all the summons of the creature
	Creature* master = popCreature();

	lua_newtable(L);
	const std::list<Creature*>& summons = creature->getSummons();
	std::list<Creature*>::const_iterator it = summons.begin();
	uint32_t i = 0;
	for( ; it != summons.end(); ++it, ++i){
		pushUnsignedInteger(i);
		pushCreature(*it);
		lua_settable(L, -3);
	}

	return 1;
}

#endif
