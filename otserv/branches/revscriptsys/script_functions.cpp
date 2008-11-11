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
	// Classes...
	registerClass("Event");
	registerClass("OnSayEvent");
	registerClass("OnUseItemEvent");

	registerClass("Thing");
	registerClass("Creature", "Thing");
	registerClass("Monster", "Creature");
	registerClass("Player", "Creature");
	registerClass("Item", "Thing");
	registerClass("Teleport", "Item");
	registerClass("Container", "Item");

	registerClass("Tile");

	// Event classes
	registerMemberFunction("Event", "skip()", &Manager::lua_Event_skip);
	registerMemberFunction("Event", "propagate()", &Manager::lua_Event_propagate);

	// Game classes
	registerMemberFunction("Thing", "getPosition()", &Manager::lua_Thing_getPosition);
	registerMemberFunction("Thing", "getParentTile()", &Manager::lua_Thing_getParentTile);
	registerMemberFunction("Thing", "moveTo(table pos)", &Manager::lua_Thing_moveToPosition);

	// Creature
	registerMemberFunction("Creature", "getHealth()", &Manager::lua_Creature_getHealth);
	registerMemberFunction("Creature", "getHealthMax()", &Manager::lua_Creature_getHealthMax);

	registerMemberFunction("Creature", "getOrientation()", &Manager::lua_Creature_getOrientation);
	registerMemberFunction("Creature", "getDirection()", &Manager::lua_Creature_getOrientation);
	registerMemberFunction("Creature", "getName()", &Manager::lua_Creature_getName);
	
	registerMemberFunction("Creature", "walk(int direction)", &Manager::lua_Creature_walk);

	// Player
	registerMemberFunction("Player", "setStorageValue(string key, string value)", &Manager::lua_Player_setStorageValue);
	registerMemberFunction("Player", "getStorageValue(string key)", &Manager::lua_Player_getStorageValue);
	registerMemberFunction("Player", "getFood()", &Manager::lua_Player_getFood);
	registerMemberFunction("Player", "getMana()", &Manager::lua_Player_getMana);
	registerMemberFunction("Player", "getManaMax()", &Manager::lua_Player_getManaMax);
	registerMemberFunction("Player", "getSoulPoints()", &Manager::lua_Player_getSoulPoints);
	registerMemberFunction("Player", "getFreeCap()", &Manager::lua_Player_getFreeCap);
	registerMemberFunction("Player", "getMaximumCap()", &Manager::lua_Player_getMaximumCap);
	registerMemberFunction("Player", "getSex()", &Manager::lua_Player_getSex);
	registerMemberFunction("Player", "getVocationID()", &Manager::lua_Player_getVocationID);
	registerMemberFunction("Player", "getTownID()", &Manager::lua_Player_getTownID);
	registerMemberFunction("Player", "getGUID()", &Manager::lua_Player_getGUID);
	registerMemberFunction("Player", "getAccessGroup()", &Manager::lua_Player_getGroup);
	registerMemberFunction("Player", "getPremiumDays()", &Manager::lua_Player_getPremiumDays);
	registerMemberFunction("Player", "getSkull()", &Manager::lua_Player_getSkullType);
	registerMemberFunction("Player", "getGuildID()", &Manager::lua_Player_getGuildID);
	registerMemberFunction("Player", "getGuildName()", &Manager::lua_Player_getGuildName);
	registerMemberFunction("Player", "getGuildRank()", &Manager::lua_Player_getGuildRank);
	registerMemberFunction("Player", "getGuildNick()", &Manager::lua_Player_getGuildNick);
	
	registerMemberFunction("Player", "addItem(Item item)", &Manager::lua_Player_addItem);

	// Item
	registerGlobalFunction("createItem(int newid[, int count])", &Manager::lua_createItem);
	registerMemberFunction("Item", "getItemID()", &Manager::lua_Item_getItemID);

	registerMemberFunction("Item", "setItemID(int newid)", &Manager::lua_Item_setItemID);

	registerGlobalFunction("getItemIDByName(string name)", &Manager::lua_getItemIDByName);

	// Tile
	registerMemberFunction("Tile", "getThing(int index)", &Manager::lua_Tile_getThing);
	registerMemberFunction("Tile", "getCreatures()", &Manager::lua_Tile_getCreatures);
	registerMemberFunction("Tile", "getMoveableItems()", &Manager::lua_Tile_getMoveableItems);
	registerMemberFunction("Tile", "getItems()", &Manager::lua_Tile_getItems);
	registerMemberFunction("Tile", "addItem(Item item)", &Manager::lua_Tile_addItem);

}

void Manager::registerFunctions() {
	registerGlobalFunction("wait(int delay)", &Manager::lua_wait);

	registerGlobalFunction("registerGenericOnSayListener(string method, boolean case_sensitive, string filter, function callback)", &Manager::lua_registerGenericEvent_OnSay);
	registerGlobalFunction("registerSpecificOnSayListener(Creature who, string method, boolean case_sensitive, string filter, function callback)", &Manager::lua_registerSpecificEvent_OnSay);

	registerGlobalFunction("registerGenericOnUseItemListener(string method, int filter, function callback)", &Manager::lua_registerGenericEvent_OnUseItem);

	registerGlobalFunction("stopListener(string listener_id)", &Manager::lua_stopListener);


	registerGlobalFunction("wait(int delay)", &Manager::lua_wait);

	registerGlobalFunction("getTile(int x, int y, int z)", &Manager::lua_getTile);
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

	// This here explains why boost is so awesome, thanks to our custom
	// delete function, the listener is cleanly stopped when all references
	// to it is removed. :)
	boost::any p(si_onsay);
	Listener_ptr listener(
		new Listener(ONSAY_LISTENER, p, *this->getManager()),
		boost::bind(&Listener::deactivate, _1));

	enviroment.registerSpecificListener(listener);
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

	int id = popInteger();
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
		throw Error("Invalid argument (2) 'method'");
	}
	si_onuse.id = id;

	boost::any p(si_onuse);
	Listener_ptr listener(new Listener(ONUSEITEM_LISTENER, p, *this->getManager()));

	enviroment.Generic.OnUseItem.push_back(listener);

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
	is.str(listener_id.substr(_2pos+1));
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
	// Event table is ontop of stack
	setField(-2, "skipped", true);
	return 1;
}

int LuaState::lua_Event_propagate() {
	// Event table is ontop of stack
	setField(-2, "skipped", false);
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

int LuaState::lua_Thing_getParentTile() 
{
	Thing* thing = popThing();
	pushThing(thing->getTile());
	return 1;
}

int LuaState::lua_Thing_moveToPosition() 
{
	Position pos = popPosition();
	Thing* thing = popThing();

	pushBoolean(g_game.internalTeleport(thing, pos) == RET_NOERROR);
	
	return 1;
}

int LuaState::lua_Tile_getThing()
{
	int index = popInteger() - 1; // lua indices start at 1
	Tile* tile = popTile();
	
	// -1 is top item
	if(index > tile->__getLastIndex()) {
		index = tile->__getLastIndex() - index;
	}
	if(index < 0) {
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
	for(CreatureVector::iterator iter = tile->creatures.begin(),
		end_iter = tile->creatures.end();
		iter != end_iter; ++iter, ++n)
	{
		pushThing(*iter);
		setField(-3, n);
	}
	return 1;
}

int LuaState::lua_Tile_getMoveableItems() 
{
	Tile* tile = popTile();
	
	newTable();
	int n = 1;
	for(ItemVector::iterator iter = tile->downItems.begin(),
		end_iter = tile->downItems.end();
		iter != end_iter; ++iter)
	{
		if((*iter)->isNotMoveable() == false) {
			pushThing(*iter);
			setField(-3, n++);
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
		setField(-3, n++);
	}

	for(ItemVector::iterator iter = tile->topItems.begin(),
		end_iter = tile->topItems.end();
		iter != end_iter; ++iter, ++n)
	{
		pushThing(*iter);
		setField(-3, n);
	}

	for(ItemVector::iterator iter = tile->downItems.begin(),
		end_iter = tile->downItems.end();
		iter != end_iter; ++iter, ++n)
	{
		pushThing(*iter);
		setField(-3, n);
	}
	return 1;
}

int LuaState::lua_Tile_addItem()
{
	Item* item = popItem(Script::ERROR_PASS);
	if(item == NULL) {
		pushNil();
		return 1;
	}
	Tile* tile = popTile();

	if(item->getParent() != VirtualCylinder::virtualCylinder){
		// Must remove from previous parent...
		g_game.internalRemoveItem(item);
	}

	ReturnValue ret = g_game.internalAddItem(tile, item);
	pushBoolean(ret == RET_NOERROR);
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Class Creature

int LuaState::lua_Creature_getName() {
	Player* p = popPlayer();
	pushString(p->getName());
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

int LuaState::lua_Creature_walk()
{
	int ndir = popInteger();
	Creature* creature = popCreature();

	switch(ndir){
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
			throw Error("Creature:walk : Invalid direction");
	}

	ReturnValue ret = g_game.internalMoveCreature(creature, (Direction)ndir, FLAG_NOLIMIT);
	pushBoolean(ret == RET_NOERROR);
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Class Player

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

int LuaState::lua_Player_getFood() {
	Player* p = popPlayer();
	Condition* condition = p->getCondition(CONDITION_REGENERATION, CONDITIONID_DEFAULT);
	if(condition){
		push(condition->getTicks() / 1000);
	}
	else{
		pushInteger(0);
	}
	return 1;
}

int LuaState::lua_Player_getAccess() {
	Player* p = popPlayer();
	push(p->getAccessLevel());
	return 1;
}

int LuaState::lua_Player_getMana() {
	Player* p = popPlayer();
	push(p->getMana());
	return 1;
}

int LuaState::lua_Player_getManaMax() {
	Player* p = popPlayer();
	push(p->getMaxMana());
	return 1;
}

int LuaState::lua_Player_getVocationID() {
	Player* p = popPlayer();
	push(p->getVocationId());
	return 1;
}

int LuaState::lua_Player_getSoulPoints() {
	Player* p = popPlayer();
	push(p->getPlayerInfo(PLAYERINFO_SOUL));
	return 1;
}

int LuaState::lua_Player_getFreeCap() {
	Player* p = popPlayer();
	push(p->getFreeCapacity());
	return 1;
}

int LuaState::lua_Player_getMaximumCap() {
	Player* p = popPlayer();
	push(p->getCapacity());
	return 1;
}

int LuaState::lua_Player_getGuildID() {
	Player* p = popPlayer();
	push(p->getGuildId());
	return 1;
}

int LuaState::lua_Player_getGuildName() {
	Player* p = popPlayer();
	push(p->getGuildName());
	return 1;
}

int LuaState::lua_Player_getGuildRank() {
	Player* p = popPlayer();
	push(p->getGuildRank());
	return 1;
}

int LuaState::lua_Player_getGuildNick() {
	Player* p = popPlayer();
	push(p->getGuildNick());
	return 1;
}

int LuaState::lua_Player_getSex() {
	Player* p = popPlayer();
	push(p->getSex() == PLAYERSEX_MALE);
	return 1;
}

int LuaState::lua_Player_getTownID() {
	Player* p = popPlayer();
	push(p->getTown());
	return 1;
}

int LuaState::lua_Player_getGUID() {
	Player* p = popPlayer();
	push(p->getGUID());
	return 1;
}

int LuaState::lua_Player_getGroup() {
	Player* p = popPlayer();
	push(p->getAccessGroup());
	return 1;
}

int LuaState::lua_Player_getPremiumDays() {
	Player* p = popPlayer();
	push(p->getPremiumDays());
	return 1;
}

int LuaState::lua_Player_getSkullType() {
	Player* p = popPlayer();
#ifdef __SKULLSYSTEM__
	push(p->getSkull());
#else
	pushInteger(0);
#endif
	return 1;
}

int LuaState::lua_Player_addItem()
{
	bool canDropOnMap = true;

	Item* item = popItem(ERROR_PASS);
	if(item == NULL) {
		pushNil();
		return 1;
	}
	Player* player = popPlayer();

	if(item->getParent() != VirtualCylinder::virtualCylinder){
		// Must remove from previous parent...
		g_game.internalRemoveItem(item);
	}

	ReturnValue ret = RET_NOERROR;
	if(canDropOnMap){
		ret = g_game.internalPlayerAddItem(player, item);
	}
	else{
		ret = g_game.internalAddItem(player, item);
	}
	pushBoolean(ret == RET_NOERROR);
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Class Item

int LuaState::lua_createItem()
{
	int count = -1;
	if(getStackTop() > 1) {
		count = popInteger();
	}
	int id = popUnsignedInteger();

	Item* item = Item::CreateItem((uint16_t)id, (count < 0? 0 : count));
	item->useThing2();
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

int LuaState::lua_Item_setItemID()
{
	int newcount = -1;
	if(getStackTop() > 2) {
		newcount = popInteger();
	}

	int newid = popInteger();
	Item* item = popItem();
	
	if(newid < 0 || newid > 65535) {
		throw Error("Item:setItemID : item ID provided");
	}

	const ItemType& it = Item::items[newid];
	if(it.stackable && newcount > 100 || newcount < -1){
		throw Error("Item:setItemID : Stack count cannot be higher than 100.");
	}

	Item* newItem = g_game.transformItem(item, newid, newcount);

	pushBoolean(true);
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

int LuaState::lua_getTile()
{
	int z = popInteger();
	int y = popInteger();
	int x = popInteger();
	pushTile(g_game.getTile(x, y, z));
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


#if 0
/* Unconverted lua functions...
 * Anyone feels up to the task of converting?
 */

int LuaScriptInterface::luaGetPlayerFlagValue()
{
	//getPlayerFlagValue(who, flag)
	uint32_t flagindex = popUnsignedInteger();
	Player* player = popPlayer();

	if(flagindex < PlayerFlag_LastFlag){
		pushBoolean(player->hasFlag((PlayerFlags)flagindex));
	}
	else{
		reportLuaError("No valid flag index.");
		pushBoolean(false);
	}
	return 1;
}

int LuaScriptInterface::luaPlayerLearnInstantSpell()
{
	//playerLearnInstantSpell(cid, name)
	std::string spellName = popString();
	Player* player = popPlayer();

	InstantSpell* spell = g_spells->getInstantSpellByName(spellName);
	if(!spell){
		std::string error_str = (std::string)"Spell \"" + spellName +  (std::string)"\" not found";
		throwLuaException(error_str);
	}

	player->learnInstantSpell(spell->getName());
	pushBoolean(true);
	return 1;
}

int LuaScriptInterface::luaCanPlayerLearnInstantSpell()
{
	//canPlayerLearnInstantSpell(who, name)
	std::string spellName = popString();
	Player* player = popPlayer();

	InstantSpell* spell = g_spells->getInstantSpellByName(spellName);
	if(!spell){
		std::string error_str = (std::string)"Spell \"" + spellName +  (std::string)"\" not found";
		throwLuaException(error_str);
	}

	if(!player->hasFlag(PlayerFlag_IgnoreSpellCheck)){
		if(player->getLevel() < spell->getLevel()){
			pushBoolean(false);
			return 1;
		}

		if(player->getMagicLevel() < spell->getMagicLevel()){
			pushBoolean(false);
			return 1;
		}
	}

	pushBoolean(true);
	return 1;
}

int LuaScriptInterface::luaGetPlayerLearnedInstantSpell()
{
	//getPlayerLearnedInstantSpell(who, name)
	std::string spellName = popString();
	Player* player = popPlayer();

	InstantSpell* spell = g_spells->getInstantSpellByName(spellName);
	if(!spell) {
		std::string error_str = (std::string)"Spell \"" + spellName +  (std::string)"\" not found";
		throwLuaException(error_str);
	}

	pushBoolean(player->hasLearnedInstantSpell(spellName));
	return 1;
}

int LuaScriptInterface::luaGetPlayerInstantSpellCount()
{
	//getPlayerInstantSpellCount(cid)
	Player* player = popPlayer();

	pushInteger(g_spells->getInstantSpellCount(player));
	return 1;
}

int LuaScriptInterface::luaGetPlayerInstantSpellInfo()
{
	//getPlayerInstantSpellInfo(cid, index)
	uint32_t index = popUnsignedInteger();
	Player* player = popPlayer();

	InstantSpell* spell = g_spells->getInstantSpellByIndex(player, index);
	if(!spell){
		reportLuaError(LUA_ERROR_SPELL_NOT_FOUND);
	}

	lua_newtable(m_luaState);
	setField("name", spell? spell->getName(): "");
	setField("words", spell? spell->getWords(): "");
	setField("level", spell? spell->getLevel(): 0);
	setField("mlevel", spell? spell->getMagicLevel(): 0);
	setField("mana", spell? spell->getManaCost(player): 0);
	setField("manapercent", spell? spell->getManaPercent(): 0);
	return 1;
}

int LuaScriptInterface::luaGetInstantSpellInfoByName()
{
	//getInstantSpellInfoByName(cid, name)
	std::string spellName = popString();
	Player* player = popPlayer();

	InstantSpell* spell = g_spells->getInstantSpellByName(spellName);
	if(!spell){
		reportLuaError(LUA_ERROR_SPELL_NOT_FOUND);
	}

	lua_newtable(m_luaState);
	setField("name", spell? spell->getName(): "");
	setField("words", spell? spell->getWords(): "");
	setField("level", spell? spell->getLevel(): 0);
	setField("mlevel", spell? spell->getMagicLevel(): 0);
	setField("mana", spell? spell->getManaCost(player): 0);
	setField("manapercent", spell? spell->getManaPercent(): 0);
	return 1;
}

int LuaScriptInterface::luaGetInstantSpellWords()
{
	//getInstantSpellWords(name)
	std::string spellName = popString();
	InstantSpell* spell = g_spells->getInstantSpellByName(spellName);
	if(!spell){
		reportErrorFunc(getErrorDesc(LUA_ERROR_SPELL_NOT_FOUND));
		reportLuaError(LUA_ERROR_SPELL_NOT_FOUND);
	}

	pushString(spell? spell->getWords(): "");
	return 1;
}


int LuaScriptInterface::luaDoRemoveItem()
{
	//doRemoveItem(uid, <optional> count)
	int32_t parameters = lua_gettop(m_luaState);

	int32_t count = -1;
	if(parameters > 1){
		count = popInteger();
	}

	uint32_t uid;
	Item* item = popItem(&uid);

	ReturnValue ret = g_game.internalRemoveItem(item, count);
	if(ret != RET_NOERROR){
		pushBoolean(false);
		return 1;
	}

	ScriptEnviroment* env = getScriptEnv();
	env->removeItemByUID(uid);

	pushBoolean(true);
	return 1;
}

int LuaScriptInterface::luaDoPlayerRemoveItem()
{
	//doPlayerRemoveItem(cid, itemid, count, <optional> subtype)
	int32_t parameters = lua_gettop(m_luaState);

	int32_t subType = -1;
	if(parameters > 3){
		subType = popNumber();
	}

	uint32_t count = popInteger();
	uint16_t itemId = (uint16_t)popUnsignedInteger();
	Player* player = popPlayer();

	pushBoolean(g_game.removeItemOfType(player, itemId, count, subType));

	return 1;
}

int LuaScriptInterface::luaDoFeedPlayer()
{
	//doFeedPlayer(uid, food)
	int32_t food = (int32_t)popInteger();
	Player* player = popPlayer();

	player->addDefaultRegeneration(food * 1000);
	pushBoolean(true);
	return 1;
}

int LuaScriptInterface::luaDoSendCancel()
{
	//doSendCancel(uid, text)
	std::string text = popString();
	Player* player = popPlayer();

	player->sendCancel(text.c_str());
	pushBoolean(true);
	return 1;
}

int LuaScriptInterface::luaDoSendDefaultCancel()
{
	//doPlayerSendDefaultCancel(cid, ReturnValue)
	ReturnValue ret = (ReturnValue)popInteger();
	Player* player = popPlayer();

	player->sendCancelMessage(ret);
	pushBoolean(true);
	return 1;
}

int LuaScriptInterface::luaDoTeleportThing()
{
	//doTeleportThing(uid, newpos)
	PositionEx pos = popPosition();
	Thing* thing = popThing();

	pushBoolean(g_game.internalTeleport(thing, pos) == RET_NOERROR);
	return 1;
}

int LuaScriptInterface::luaDoCreatureSay()
{
	//doPlayerSay(cid, text, type)
	uint32_t type = popUnsignedInteger();
	std::string text = popString();
	Player* player = popPlayer();

	g_game.internalCreatureSay(player,(SpeakClasses)type, text);
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
	player->addSkillAdvance((skills_t)skillid, n);
	pushBoolean(true);
	return 1;
}

int LuaScriptInterface::luaDoPlayerAddManaSpent()
{
	//doPlayerAddManaSpent(cid, mana)
	int32_t mana = popInteger();
	Player* player = popPlayer();

	if(mana < 0) {
		reportLuaError("Can not add less than 0 mana to a player.");
		pushBoolean(false);
		return 1;
	}

	player->addManaSpent(mana);
	pushBoolean(true);
	return 1;
}

int LuaScriptInterface::luaDoPlayerAddMana()
{
	//doPlayerAddMana(cid, mana)
	int32_t manaChange = popInteger();
	Player* player = popPlayer();

	g_game.combatChangeMana(NULL, player, manaChange);
	pushBoolean(true);
	return 1;
}

int LuaScriptInterface::luaDoPlayerAddItem()
{
	//doPlayerAddItem(cid, itemid, <optional> count/subtype)
	int32_t parameters = lua_gettop(m_luaState);

	uint32_t count = 0;
	if(parameters > 2){
		count = popUnsignedInteger();
	}

	uint32_t itemId = popUnsignedInteger();
	Player* player = popPlayer();

	const ItemType& it = Item::items[itemId];
	if(it.stackable && count > 100){
		reportErrorFunc("Stack count cannot be higher than 100.");
		count = 100;
	}

	Item* newItem = Item::CreateItem(itemId, count);

	if(!newItem){
		throwLuaException("Couldn't create item of the specified ID.");
	}

	ReturnValue ret = g_game.internalPlayerAddItem(player, newItem);

	if(ret != RET_NOERROR){
		delete newItem;
		reportErrorFunc("Could not add item");
		pushBoolean(false);
	} else if(newItem->getParent()){
		pushThing(newItem);
	}
	else{
		//stackable item stacked with existing object, newItem will be released
		pushThing(NULL);
	}

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
					g_game.internalTeleport(item, toPos);
				}
			}
			else if(Creature* creature = thing->getCreature()){
				g_game.internalTeleport(creature, toPos);
			}
		}
	}

	pushBoolean(true);
	return 1;
}

int LuaScriptInterface::luaDoPlayerSendTextMessage()
{
	//doPlayerSendTextMessage(cid, MessageClasses, message)
	std::string text = popString();
	uint32_t messageClass = popUnsignedInteger();
	Player* player = popPlayer();

	player->sendTextMessage((MessageClasses)messageClass, text);
	pushBoolean(true);
	return 1;
}

int LuaScriptInterface::luaDoSendAnimatedText()
{
	//doSendAnimatedText(pos, text, color)
	uint32_t color = popUnsignedInteger();
	std::string text = popString();
	PositionEx pos = popPosition();

	ScriptEnviroment* env = getScriptEnv();

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
	uint32_t value = player->getLossPercent((lossTypes_t)lossType);
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
	player->setLossPercent((lossTypes_t)lossType, newPercent);
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

int LuaScriptInterface::luaGetItemRWInfo()
{
	//getItemRWInfo(uid)
	const Item* item = popItem();
	
	uint32_t rwflags = 0;
	if(item->isReadable())
		rwflags |= 1;

	if(item->canWriteText()){
		rwflags |= 2;
	}

	pushUnsignedInteger(rwflags);
	return 1;
}

int LuaScriptInterface::luaDoDecayItem()
{
	//doDecayItem(uid)
	//Note: to stop decay set decayTo = 0 in items.otb
	Item* item = popItem();
	g_game.startDecay(item);
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

	ScriptEnviroment* env = getScriptEnv();

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

	ReturnValue ret = g_game.internalAddItem(tile, newItem, INDEX_WHEREEVER, FLAG_NOLIMIT);
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

	ScriptEnviroment* env = getScriptEnv();

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

int LuaScriptInterface::luaDoCreateTeleport()
{
	//doCreateTeleport(teleportID, positionToGo, createPosition)
	Position createPos = popPosition();
	Position toPos = popPosition();
	uint32_t itemId = popUnsignedInteger();

	Tile* tile = g_game.getMap()->getTile(createPos);
	if(!tile){
		throwLuaException(getErrorDesc(LUA_ERROR_TILE_NOT_FOUND));
	}

	Item* newItem = Item::CreateItem(itemId);
	Teleport* newTp = newItem->getTeleport();

	if(!newTp){
		delete newItem;
		throwLuaException("ID is not a teleport");
	}

	newTp->setDestPos(toPos);

	ReturnValue ret = g_game.internalAddItem(tile, newTp, INDEX_WHEREEVER, FLAG_NOLIMIT);
	if(ret != RET_NOERROR){
		delete newItem;
		pushThing(NULL);
	}

	if(newItem->getParent()){
		pushThing(newItem);
	}
	else{
		// stackable item stacked with existing object, newItem will be released
		// note: this makes no sense, since when are teleports stackable?
		pushThing(NULL);
	}
	return 1;
}

int LuaScriptInterface::luaGetPlayerStorageValue()
{
	//getPlayerStorageValue(cid, valueid)
	uint32_t key = popUnsignedInteger();
	Player* player = popPlayer();
	
	ScriptEnviroment* env = getScriptEnv();
	int32_t value;
	if(player->getStorageValue(key, value)){
		pushInteger(value);
	}
	else{
		pushInteger(-1);
	}
	return 1;
}

int LuaScriptInterface::luaSetPlayerStorageValue()
{
	//setPlayerStorageValue(cid, valueid, newvalue)
	int32_t value = popInteger();
	uint32_t key = popUnsignedInteger();
	Player* player = popPlayer();

	if(IS_IN_KEYRANGE(key, RESERVED_RANGE)){
		std::stringstream error_str;
		error_str << "Accesing to reserverd range: "  << key;
		throwLuaException(error_str.str());
	}

	player->addStorageValue(key,value);
	pushBoolean(true);
	return 1;
}

int LuaScriptInterface::luaDoSetItemActionId()
{
	//doSetItemActionID(uid, actionid)
	uint32_t actionid = popUnsignedInteger();
	Item* item = popItem();

	item->setActionId(actionid);
	pushBoolean(true);
	return 1;
}

int LuaScriptInterface::luaDoSetItemText()
{
	//doSetItemText(uid, text)
	std::string text = popString();
	Item* item = popItem();
	item->setText(text);
	pushBoolean(true);
	return 1;
}

int LuaScriptInterface::luaDoSetItemSpecialDescription()
{
	//doSetItemSpecialDescription(uid, desc)
	std::string text = popString();
	Item* item = popItem();

	if(text != ""){
		item->setSpecialDescription(text);
	}
	else{
		item->resetSpecialDescription();
	}
	pushBoolean(true);
	return 1;
}

int LuaScriptInterface::luaGetTilePzInfo()
{
	//getTilePzInfo(pos)
	Position pos = popPosition();

	Tile *tile = g_game.getMap()->getTile(pos);

	pushBoolean(tile && tile->hasFlag(TILESTATE_PROTECTIONZONE));
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

int LuaScriptInterface::luaDoCreateMonster()
{
	//doCreateMonster(name, pos)
	PositionEx pos = popPosition();
	std::string name = popString();

	Monster* monster = Monster::createMonster(name);

	if(!monster){
		std::string error_str = "Monster name(" + name + ") not found";
		throwLuaException(error_str);
	}

	if(!g_game.placeCreature(monster, (Position&)pos)){
		delete monster;
		std::string error_str = (std::string)"Can not summon monster: " + name;
		throwLuaException(error_str);
	}

	pushThing(monster);
	return 1;
}

int LuaScriptInterface::luaDoSummonCreature()
{
	//doSummonCreature(creature, name, pos)
	PositionEx pos = popPositionEx();
	std::string name = popString();
	Creature* creature = popCreature();

	Monster* monster = Monster::createMonster(name);
	if(!monster){
		std::string error_str = (std::string)"Monster name(" + name + (std::string)") not found";
		throwLuaException(error_str);
	}

	creature->addSummon(monster);
	if(!g_game.placeCreature(monster, (Position&)pos)){
		creature->removeSummon(monster);
		delete monster;
		pushBoolean(false);
		return 1;
	}

	pushThing(monster);

	return 1;
}

int LuaScriptInterface::luaDoRemoveCreature()
{
	//doRemoveCreature(cid)
	Creature* creature = popCreature();

	//Players will get kicked without restrictions
	Player* player = creature->getPlayer();
	if(player){
		player->kickPlayer();
	}
	//Monsters/NPCs will get removed
	else{
		g_game.removeCreature(creature);
	}
	pushBoolean(true);
	return 1;
}

int LuaScriptInterface::luaDoPlayerRemoveMoney()
{
	//doPlayerRemoveMoney(cid, money)
	uint32_t money = popInteger();
	Player* player = popPlayer();
	if(money < 0) {
		pushBoolean(false);
		return 1;
	}
	pushBoolean(g_game.removeMoney(player, money));
	return 1;
}

int LuaScriptInterface::luaDoPlayerSetTown()
{
	//doPlayerSetTown(cid, towind)
	Town* town = popTown();
	Player* player = popPlayer();
	player->masterPos = town->getTemplePosition();
	player->setTown(town->getTownID());
	pushBoolean(true);
	return 1;
}

int LuaScriptInterface::luaDoPlayerSetVocation()
{
	//doPlayerSetVocation(cid, voc)
	uint32_t voc = popInteger();
	Player* player = popPlayer();

	player->setVocation(voc);
	pushBoolean(true);

	return 1;
}

int LuaScriptInterface::luaDoPlayerAddSoul()
{
	//doPlayerAddSoul(cid, soul)
	int32_t addsoul = popInteger();
	Player* player = popPlayer();

	player->changeSoul(addsoul);
	pushBoolean(true);
	return 1;
}

int LuaScriptInterface::luaGetPlayerItemCount()
{
	//getPlayerItemCount(cid, itemid)
	uint32_t itemId = popUnsignedInteger();
	Player* player = popPlayer();

	uint32_t n = player->__getItemTypeCount(itemId);
	pushNumber(n);
	return 1;
}

int LuaScriptInterface::luaGetHouseOwner()
{
	//getHouseOwner(house)
	House* house = popHouse();
	pushUnsignedInteger(house->getHouseOwner());
	return 1;
}

int LuaScriptInterface::luaGetHouseName()
{
	//getHouseName(house)
	House* house = popHouse();
	pushString(house->getName());
	return 1;
}

int LuaScriptInterface::luaGetHouseEntry()
{
	//getHouseEntry(houseid)
	House* house = popHouse();
	pushPosition(house->getEntryPosition());
	return 1;
}

int LuaScriptInterface::luaGetHouseRent()
{
	//getHouseRent(house)
	House* house = popHouse();
	pushUnsignedInteger(house->getRent());
	return 1;
}

int LuaScriptInterface::luaGetHouseTown()
{
	//getHouseTown(house)
	House* house = popHouse();
	Town* town = Towns::getInstance().getTown(house->getTownId());
	pushTown(town);
	return 1;
}

int LuaScriptInterface::luaGetHouseAccessList()
{
	//getHouseAccessList(house, listid)
	uint32_t listid = popUnsignedInteger();
	House* house = popHouse();

	std::string list;
	if(house->getAccessList(listid, list)){
		pushString(list);
	}
	else{
		throwLuaException("Invalid listid.");
	}
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

int LuaScriptInterface::luaSetHouseAccessList()
{
	//setHouseAccessList(house, listid, listtext)
	std::string list = popString();
	uint32_t listid = popUnsignedInteger();
	House* house = popHouse();

	house->setAccessList(listid, list);
	pushBoolean(true);
	return 1;
}

int LuaScriptInterface::luaSetHouseOwner()
{
	//setHouseOwner(house, owner)
	uint32_t guid;
	Player* player = popPlayer(NULL, LUA_PASS);
	if(!player) {
		// It is a GUID?
		guid = popUnsignedInteger();
	} else {
		guid = player->getGUID();
	}
	House* house = popHouse();

	house->setHouseOwner(guid);
	pushBoolean(true);
	return 1;
}

int LuaScriptInterface::luaGetWorldType()
{
	//getWorldType()
	switch(g_game.getWorldType()){
		case WORLD_TYPE_NO_PVP:
			pushNumber(1);
			break;
		case WORLD_TYPE_PVP:
			pushNumber(2);
			break;
		case WORLD_TYPE_PVP_ENFORCED:
			pushNumber(3);
			break;
		default:
			throwLuaException("Unknown world type");
			break;
	}
	return 1;
}

int LuaScriptInterface::luaGetWorldTime()
{
	//getWorldTime()
	uint32_t time = g_game.getLightHour();
	pushNumber(time);
	return 1;
}

int LuaScriptInterface::luaGetWorldLight()
{
	//getWorldLight()
	LightInfo lightInfo;
	g_game.getWorldLightInfo(lightInfo);

	pushNumber(lightInfo.level);
	pushNumber(lightInfo.color);
	return 2;
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

int LuaScriptInterface::luaGetWorldUpTime()
{
	//getWorldUpTime()
	uint32_t uptime = 0;

	Status* status = Status::instance();
	if(status){
		uptime = status->getUptime();
	}

	pushNumber(uptime);
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

int LuaScriptInterface::luaDoPlayerAddExp()
{
	//doPlayerAddExp(player, exp)
	int32_t exp = popInteger();
	Player* player = popPlayer();

	if(exp >= 0){
		player->addExperience(exp);
		pushBoolean(true);
	}
	else{
		pushBoolean(false);
	}
	return 1;
}

int LuaScriptInterface::luaGetPlayerSlotItem()
{
	//getPlayerSlotItem(player, slot)
	uint32_t slot = popUnsignedInteger();
	Player* player = popPlayer();

	Thing* thing = player->__getThing(slot);
	if(thing){
		pushThing(thing);
	}
	else{
		pushThing(NULL);
	}
	return 1;
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

int LuaScriptInterface::luaGetThing()
{
	//getThing(uid)
	uint32_t uid = popUnsignedInteger();

	ScriptEnviroment* env = getScriptEnv();
	Thing* thing = env->getThingByUID(uid);
	if(thing) throwLuaException("Could not find thing");

	pushThing(thing);
	return 1;
}

int LuaScriptInterface::luaQueryTileAddThing()
{
	//queryTileAddThing(uid, pos, <optional> flags)
	int32_t parameters = lua_gettop(m_luaState);

	uint32_t flags = 0;
	if(parameters > 2){
		flags = popUnsignedInteger();
	}

	PositionEx pos = popPositionEx();
	Thing* thing = popThing();

	ScriptEnviroment* env = getScriptEnv();

	Tile* tile = g_game.getTile(pos.x, pos.y, pos.z);
	if(!tile){
		pushUnsignedInteger(RET_NOTPOSSIBLE);
		return 1;
	}

	Thing* thing = env->getThingByUID(uid);
	if(!thing){
		pushUnsignedInteger(RET_NOTPOSSIBLE);
		return 1;
	}

	ReturnValue ret = tile->__queryAdd(0, thing, 1, flags);
	pushUnsignedInteger(ret);
	return 1;
}

int LuaScriptInterface::luaGetThingPos()
{
	//getThingPos(uid)
	Thing* thing = popThing();

	PositionEx pos(0, 0, 0, 0);
	pos = thing->getPosition();
	if(thing->getParent()){
		pos.stackpos = thing->getParent()->__getIndexOfThing(thing);
	}
	pushPosition(pos);
	return 1;
}

int LuaScriptInterface::luaCreateCombatObject()
{
	//createCombatObject()
	ScriptEnviroment* env = getScriptEnv();
	
	if(env->getScriptId() != EVENT_ID_LOADING){
		reportError(__FUNCTION__, "This function can only be used while loading the script.");
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}
	
	Combat* combat = new Combat;

	if(!combat){
		throwLuaException(getErrorDesc(LUA_ERROR_COMBAT_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	uint32_t newCombatId = env->addCombatObject(combat);
	lua_pushnumber(L, newCombatId);
	return 1;
}

bool LuaScriptInterface::getArea(lua_State *L, std::list<uint32_t>& list, uint32_t& rows)
{
	rows = 0;
	uint32_t i = 0, j = 0;
	lua_pushnil(L);  // first key //

	while(lua_next(L, -2) != 0){
		lua_pushnil(L);
		while(lua_next(L, -2) != 0){
			list.push_back((uint32_t)lua_tonumber(L, -1));

			lua_pop(L, 1);  // removes `value'; keeps `key' for next iteration //
			j++;
		}

		++rows;

		j = 0;
		lua_pop(L, 1);  // removes `value'; keeps `key' for next iteration //
		i++;
	}

	lua_pop(L, 1);
	return (rows != 0);
}

int LuaScriptInterface::luaCreateCombatArea(lua_State *L)
{
	//createCombatArea( {area}, <optional> {extArea} )
	ScriptEnviroment* env = getScriptEnv();

	if(env->getScriptId() != EVENT_ID_LOADING){
		reportError(__FUNCTION__, "This function can only be used while loading the script.");
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	int32_t parameters = lua_gettop(m_luaState);

	AreaCombat* area = new AreaCombat;

	if(parameters > 1){
		//has extra parameter with diagonal area information

		uint32_t rowsExtArea;
		std::list<uint32_t> listExtArea;
		getArea(L, listExtArea, rowsExtArea);

		// setup all possible rotations
		area->setupExtArea(listExtArea, rowsExtArea);
	}

	uint32_t rowsArea = 0;
	std::list<uint32_t> listArea;
	getArea(L, listArea, rowsArea);

	// setup all possible rotations
	area->setupArea(listArea, rowsArea);

	uint32_t newAreaId = env->addCombatArea(area);

	lua_pushnumber(L, newAreaId);
	return 1;
}

int LuaScriptInterface::luaCreateConditionObject(lua_State *L)
{
	//createConditionObject(type)
	ScriptEnviroment* env = getScriptEnv();

	if(env->getScriptId() != EVENT_ID_LOADING){
		reportError(__FUNCTION__, "This function can only be used while loading the script.");
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	ConditionType_t type = (ConditionType_t)popNumber(L);

	Condition* condition = Condition::createCondition(CONDITIONID_COMBAT, type, 0, 0);
	if(condition){
		uint32_t newConditionId = env->addConditionObject(condition);
		lua_pushnumber(L, newConditionId);
	}
	else{
		throwLuaException(getErrorDesc(LUA_ERROR_CONDITION_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
	}

	return 1;
}

int LuaScriptInterface::luaSetCombatArea(lua_State *L)
{
	//setCombatArea(combat, area)
	uint32_t areaId = popNumber(L);
	uint32_t combatId = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

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

	const AreaCombat* area = env->getCombatArea(areaId);
	if(!area){
		throwLuaException(getErrorDesc(LUA_ERROR_AREA_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	combat->setArea(new AreaCombat(*area));

	lua_pushnumber(L, LUA_NO_ERROR);
	return 1;
}

int LuaScriptInterface::luaSetCombatCondition(lua_State *L)
{
	//setCombatCondition(combat, condition)
	uint32_t conditionId = popNumber(L);
	uint32_t combatId = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

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

	const Condition* condition = env->getConditionObject(conditionId);
	if(!condition){
		throwLuaException(getErrorDesc(LUA_ERROR_CONDITION_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	combat->setCondition(condition);

	lua_pushnumber(L, LUA_NO_ERROR);
	return 1;
}

int LuaScriptInterface::luaSetCombatParam(lua_State *L)
{
	//setCombatParam(combat, key, value)
	uint32_t value = popNumber(L);
	CombatParam_t key = (CombatParam_t)popNumber(L);
	uint32_t combatId = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	if(env->getScriptId() != EVENT_ID_LOADING){
		reportError(__FUNCTION__, "This function can only be used while loading the script.");
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	Combat* combat = env->getCombatObject(combatId);

	if(combat){
		combat->setParam(key, value);
		lua_pushnumber(L, LUA_NO_ERROR);
	}
	else{
		throwLuaException(getErrorDesc(LUA_ERROR_COMBAT_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
	}
	return 1;
}

int LuaScriptInterface::luaSetConditionParam(lua_State *L)
{
	//setConditionParam(condition, key, value)
	int32_t value = (int32_t)popNumber(L);
	ConditionParam_t key = (ConditionParam_t)popNumber(L);
	uint32_t conditionId = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	if(env->getScriptId() != EVENT_ID_LOADING){
		reportError(__FUNCTION__, "This function can only be used while loading the script.");
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	Condition* condition = env->getConditionObject(conditionId);

	if(condition){
		condition->setParam(key, value);
		lua_pushnumber(L, LUA_NO_ERROR);
	}
	else{
		throwLuaException(getErrorDesc(LUA_ERROR_CONDITION_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
	}
	return 1;
}

int LuaScriptInterface::luaAddDamageCondition(lua_State *L)
{
	//addDamageCondition(condition, rounds, time, value)
	int32_t value = (int32_t)popNumber(L);
	int32_t time = (int32_t)popNumber(L);
	int32_t rounds  = (int32_t)popNumber(L);
	uint32_t conditionId = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	if(env->getScriptId() != EVENT_ID_LOADING){
		reportError(__FUNCTION__, "This function can only be used while loading the script.");
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	ConditionDamage* condition = dynamic_cast<ConditionDamage*>(env->getConditionObject(conditionId));

	if(condition){
		condition->addDamage(rounds, time, value);
		lua_pushnumber(L, LUA_NO_ERROR);
	}
	else{
		throwLuaException(getErrorDesc(LUA_ERROR_CONDITION_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
	}
	return 1;
}

int LuaScriptInterface::luaAddOutfitCondition(lua_State *L)
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

	if(env->getScriptId() != EVENT_ID_LOADING){
		reportError(__FUNCTION__, "This function can only be used while loading the script.");
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	ConditionOutfit* condition = dynamic_cast<ConditionOutfit*>(env->getConditionObject(conditionId));

	if(condition){
		condition->addOutfit(outfit);
		lua_pushnumber(L, LUA_NO_ERROR);
	}
	else{
		throwLuaException(getErrorDesc(LUA_ERROR_CONDITION_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
	}
	return 1;
}

int LuaScriptInterface::luaSetCombatCallBack(lua_State *L)
{
	//setCombatCallBack(combat, key, function_name)
	const char* function = popString(L);
	std::string function_str(function);
	CallBackParam_t key = (CallBackParam_t)popNumber(L);
	uint32_t combatId = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

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

int LuaScriptInterface::luaSetCombatFormula(lua_State *L)
{
	//setCombatFormula(combat, type, mina, minb, maxa, maxb)
	ScriptEnviroment* env = getScriptEnv();

	if(env->getScriptId() != EVENT_ID_LOADING){
		reportError(__FUNCTION__, "This function can only be used while loading the script.");
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	double maxb = popFloatNumber(L);
	double maxa = popFloatNumber(L);
	double minb = popFloatNumber(L);
	double mina = popFloatNumber(L);

	formulaType_t type = (formulaType_t)popNumber(L);
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

int LuaScriptInterface::luaSetConditionFormula(lua_State *L)
{
	//setConditionFormula(condition, mina, minb, maxa, maxb)
	ScriptEnviroment* env = getScriptEnv();

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

int LuaScriptInterface::luaDoCombat(lua_State *L)
{
	//doCombat(cid, combat, param)
	ScriptEnviroment* env = getScriptEnv();

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

int LuaScriptInterface::luaDoAreaCombatHealth(lua_State *L)
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

int LuaScriptInterface::luaDoTargetCombatHealth(lua_State *L)
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

int LuaScriptInterface::luaDoAreaCombatMana(lua_State *L)
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

int LuaScriptInterface::luaDoTargetCombatMana(lua_State *L)
{
	//doTargetCombatMana(cid, target, min, max, effect)
	uint8_t effect = (uint8_t)popNumber(L);
	int32_t maxChange = (int32_t)popNumber(L);
	int32_t minChange = (int32_t)popNumber(L);
	uint32_t targetCid = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

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

int LuaScriptInterface::luaDoAreaCombatCondition(lua_State *L)
{
	//doAreaCombatCondition(cid, pos, area, condition, effect)
	uint8_t effect = (uint8_t)popNumber(L);
	uint32_t conditionId = popNumber(L);
	uint32_t areaId = popNumber(L);
	PositionEx pos;
	popPosition(L, pos);
	uint32_t cid = (uint32_t)popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

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

int LuaScriptInterface::luaDoTargetCombatCondition(lua_State *L)
{
	//doTargetCombatCondition(cid, target, condition, effect)
	uint8_t effect = (uint8_t)popNumber(L);
	uint32_t conditionId = popNumber(L);
	uint32_t targetCid = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

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

int LuaScriptInterface::luaDoAreaCombatDispel(lua_State *L)
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

int LuaScriptInterface::luaDoTargetCombatDispel(lua_State *L)
{
	//doTargetCombatDispel(cid, target, type, effect)
	uint8_t effect = (uint8_t)popNumber(L);
	ConditionType_t dispelType = (ConditionType_t)popNumber(L);
	uint32_t targetCid = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

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

int LuaScriptInterface::luaDoChallengeCreature(lua_State *L)
{
	//doChallengeCreature(cid, target)
	uint32_t targetCid = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

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

int LuaScriptInterface::luaDoConvinceCreature(lua_State *L)
{
	//doConvinceCreature(cid, target)
	uint32_t targetCid = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

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

int LuaScriptInterface::luaGetMonsterTargetList(lua_State *L)
{
	//getMonsterTargetList(cid)

	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(!creature){
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	Monster* monster = creature->getMonster();
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

int LuaScriptInterface::luaGetMonsterFriendList(lua_State *L)
{
	//getMonsterFriendList(cid)

	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(!creature){
		reportErrorFunc(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	Monster* monster = creature->getMonster();
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

int LuaScriptInterface::luaDoSetMonsterTarget(lua_State *L)
{
	//doSetMonsterTarget(cid, target)
	uint32_t targetCid = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(!creature){
		throwLuaException(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	Monster* monster = creature->getMonster();
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

int LuaScriptInterface::luaDoMonsterChangeTarget(lua_State *L)
{
	//doMonsterChangeTarget(cid)
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

	Creature* creature = env->getCreatureByUID(cid);
	if(!creature){
		throwLuaException(getErrorDesc(LUA_ERROR_CREATURE_NOT_FOUND));
		lua_pushnumber(L, LUA_ERROR);
		return 1;
	}

	Monster* monster = creature->getMonster();
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

int LuaScriptInterface::luaDoAddCondition(lua_State *L)
{
	//doAddCondition(cid, condition)

	uint32_t conditionId = popNumber(L);
	uint32_t cid = popNumber(L);

	ScriptEnviroment* env = getScriptEnv();

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

int LuaScriptInterface::luaDoRemoveCondition(lua_State *L)
{
	//doRemoveCondition(cid, type)

	ConditionType_t conditionType = (ConditionType_t)popNumber(L);
	uint32_t cid = popNumber(L);
	
	ScriptEnviroment* env = getScriptEnv();

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

int LuaScriptInterface::luaNumberToVariant(lua_State *L)
{
	//numberToVariant(number)
	LuaVariant var;
	var.type = VARIANT_NUMBER;
	var.number = popNumber(L);

	LuaScriptInterface::pushVariant(L, var);
	return 1;
}

int LuaScriptInterface::luaStringToVariant(lua_State *L)
{
	//stringToVariant(string)
	LuaVariant var;
	var.type = VARIANT_STRING;
	var.text = popString(L);

	LuaScriptInterface::pushVariant(L, var);
	return 1;
}

int LuaScriptInterface::luaPositionToVariant(lua_State *L)
{
	//positionToVariant(pos)
	LuaVariant var;
	var.type = VARIANT_POSITION;
	popPosition(L, var.pos);

	LuaScriptInterface::pushVariant(L, var);
	return 1;
}

int LuaScriptInterface::luaTargetPositionToVariant(lua_State *L)
{
	//targetPositionToVariant(pos)
	LuaVariant var;
	var.type = VARIANT_TARGETPOSITION;
	popPosition(L, var.pos);

	LuaScriptInterface::pushVariant(L, var);
	return 1;
}

int LuaScriptInterface::luaVariantToNumber(lua_State *L)
{
	//variantToNumber(var)
	LuaVariant var = popVariant(L);

	uint32_t number = 0;
	if(var.type == VARIANT_NUMBER){
		number = var.number;
	}

	lua_pushnumber(L, number);
	return 1;
}

int LuaScriptInterface::luaVariantToString(lua_State *L)
{
	//variantToString(var)
	LuaVariant var = popVariant(L);

	std::string text = "";
	if(var.type == VARIANT_STRING){
		text = var.text;
	}

	lua_pushstring(L, text.c_str());
	return 1;
}

int LuaScriptInterface::luaVariantToPosition(lua_State *L)
{
	//luaVariantToPosition(var)
	LuaVariant var = popVariant(L);

	PositionEx pos(0, 0, 0, 0);
	if(var.type == VARIANT_POSITION || var.type == VARIANT_TARGETPOSITION){
		pos = var.pos;
	}

	pushPosition(L, pos, pos.stackpos);
	return 1;
}

int LuaScriptInterface::luaDoChangeSpeed()
{
	//doChangeSpeed(creature, delta)
	int32_t delta = popInteger();
	Creature* creature = popCreature();
	g_game.changeSpeed(creature, delta);
	pushBoolean(true);
	return 1;
}

int LuaScriptInterface::luaSetCreatureOutfit()
{
	//doSetCreatureOutfit(creature, outfit)
	Outfit_t outfit;
	outfit.lookType = getField("type");
	outfit.lookTypeEx = getField("extra");
	outfit.lookHead = getField("head");
	outfit.lookBody = getField("body");
	outfit.lookLegs = getField("legs");
	outfit.lookFeet = getField("feet");
	outfit.lookAddons = getField("addons");
	pop(); // outfit table
	Creature* creature = popCreature();

	creature->setDefaultOutfit(outfit);	
	g_game.internalCreatureChangeOutfit(creature, outfit);
	pushBoolean(true);
	return 1;
}

int LuaScriptInterface::luaGetCreatureOutfit()
{
	//getCreatureOutfit(creature)
	Creature* creature = popCreature();
	const Outfit_t outfit = creature->getCurrentOutfit();
	
	lua_newtable(m_luaState);
	setField("type", outfit.lookType);
	setField("extra", outfit.lookTypeEx);
	setField("head", outfit.lookHead);
	setField("body", outfit.lookBody);
	setField("legs", outfit.lookLegs);
	setField("feet", outfit.lookFeet);
	setField("addons", outfit.lookAddons);
	return 1;
}

int LuaScriptInterface::luaGetGlobalStorageValue()
{
	//getGlobalStorageValue(valueid)
	uint32_t key = popUnsignedInteger();

	ScriptEnviroment* env = getScriptEnv();

	int32_t value;
	if(env->getGlobalStorageValue(key, value)){
		pushInteger(value);
	}
	else{
		pushInteger(-1);
	}
	return 1;
}

int LuaScriptInterface::luaSetGlobalStorageValue()
{
	//setGlobalStorageValue(valueid, newvalue)
	int32_t value = popInteger();
	uint32_t key = popUnsignedInteger();

	ScriptEnviroment* env = getScriptEnv();
	env->addGlobalStorageValue(key, value);
	pushBoolean(true)
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

int LuaScriptInterface::luaIsMoveable()
{
	//isMoveable(uid)
	Thing* thing = popThing();
	pushBoolean(thing->isPushable());
	return 1;
}

int LuaScriptInterface::luaGetPlayerByName()
{
	//getPlayerByName(name)
	std::string name = popString();
	pushThing(g_game.getPlayerByName(name));
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
		ReturnValue ret = g_game.internalMoveItem(old_parent, container, INDEX_WHEREEVER, item, item->getItemCount());
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

int LuaScriptInterface::luaGetCreatureName()
{
	//getCreatureName(creature)
	Creature* creature = popCreature();
	pushString(creature->getName());
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

int LuaScriptInterface::luaGetCreatureSpeed()
{
	//getCreatureSpeed(creature)
	Creature* creature = popCreature();
	pushInteger(creature->getSpeed());
	return 1;
}

int LuaScriptInterface::luaGetCreatureBaseSpeed()
{
	//getCreatureBaseSpeed(creature)
	Creature* creature = popCreature();
	pushInteger(creature->getBaseSpeed());
	return 1;
}

int LuaScriptInterface::luaGetCreatureTarget(lua_State *L)
{
	//getCreatureTarget(creature)
	Creature* creature = popCreature();
	pushThing(creature->getAttackedCreature());
	return 1;
}

int LuaScriptInterface::luaGetCreatureHealth(lua_State *L)
{
	//getCreatureHealth(creature)
	Creature* creature = popCreature();
	pushInteger(creature->getHealth());
	return 1;
}

int LuaScriptInterface::luaGetCreatureMaxHealth(lua_State *L)
{
	//getCreatureMaxHealth(creature)
	Creature* creature = popCreature();
	pushInteger(creature->getMaxHealth());
	return 1;
}

int LuaScriptInterface::luaIsPremium()
{
	//isPremium(player)
	Player* player = popPlayer();
	pushBoolean(player->isPremium());
	return 1;
}

int LuaScriptInterface::luaHasCondition()
{
	//hasCondition(creature, conditionid)
	ConditionType_t conditionType = (ConditionType_t)popUnsignedInteger();
	Creature* creature = popCreature();

	pushBoolean(creature->hasCondition(conditionType));
	return 1;
}

int LuaScriptInterface::luaDoSendDistanceShoot()
{
	//doSendDistanceShoot(frompos, topos, type)
	uint8_t type = std::max(255, popUnsignedInteger());
	PositionEx toPos = popPosition();
	PositionEx fromPos = popPosition();

	ScriptEnviroment* env = getScriptEnv();

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