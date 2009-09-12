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

#include "lua_manager.h"
#include "script_event.h"
#include "script_environment.h"
#include "script_listener.h"
#include "script_manager.h"
#include "tools.h"

#include "tile.h"
#include "player.h"
#include "creature.h"
#include "actor.h"
#include "item.h"

uint32_t Script::Event::eventID_counter = 0;

using namespace Script;

///////////////////////////////////////////////////////////////////////////////
// General Event class

Event::Event() : eventID(++eventID_counter), propagate_by_default(false)
{
}

Event::~Event() {
}

bool Event::call(Manager& state, Environment& environment, Listener_ptr listener)
{
	LuaThread_ptr thread = state.newThread(this->getName());

	// Stack is empty
	// Push callback
	thread->pushCallback(listener);

	if(thread->isNil()) {
		thread->HandleError("Attempt to call destroyed '" + getName() + "' listener.");
		thread->pop();
		return false;
	}

	std::ostringstream lua_tag_stream;
	lua_tag_stream << "EI_" << eventID; // Event instance tag
	std::string lua_tag = lua_tag_stream.str();

	// Push event
	thread->pushEvent(*this);
	thread->duplicate();
	thread->setRegistryItem(lua_tag);


	// Run thread
	int ms = thread->run(1);
	if(ms > 0)
		state.scheduleThread(ms, thread);

	if(thread->ok() == false) {
		state.pushNil();
		state.setRegistryItem(lua_tag);
		return false;
	}

	// Retrieve event info
	thread->getRegistryItem(lua_tag);

	// Update this instance with values from lua
	update_instance(state, environment, thread);

	// Find out if the event should propagate
	bool propagate = propagate_by_default;
	thread->getField(-1, "skipped");

	if(thread->rawtypeOf() == LUA_TNIL){
		// use default
		thread->pop();
	} else if(thread->isBoolean(-1)) {
		propagate = thread->popBoolean();
	} else {
		thread->HandleError("Invalid type of value 'skipped' of event table from '" + getName() + "' listener.");
		thread->pop();
	}

	// clean up
	thread->pushNil();
	thread->setRegistryItem(lua_tag);
	thread->pop(); // pop event table, will be garbage collected as we cleared the registry from it

	return !propagate;
}


///////////////////////////////////////////////////////////////////////////////
// OnServerLoad Event
///////////////////////////////////////////////////////////////////////////////
// Triggered just after the server loads

OnServerLoad::Event::Event(bool real_startup) :
	real_startup(real_startup)
{
	propagate_by_default = true;
}

OnServerLoad::Event::~Event()
{
}

bool OnServerLoad::Event::dispatch(Manager& state, Environment& environment)
{
	return dispatchEvent<OnServerLoad::Event>
		(this, state, environment, environment.Generic.OnLoad);
}

void OnServerLoad::Event::push_instance(LuaState& state, Environment& environment)
{
	state.pushClassTableInstance("OnServerLoad");
	state.setField(-1, "reload", !real_startup);
	state.setField(-1, "startup", real_startup);
}

void OnServerLoad::Event::update_instance(Manager& state, Environment& environment, LuaThread_ptr thread)
{
}


///////////////////////////////////////////////////////////////////////////////
// OnServerUnload Event
///////////////////////////////////////////////////////////////////////////////
// Triggered just after the server loads

OnServerUnload::Event::Event(bool real_shutdown) :
	real_shutdown(real_shutdown)
{
	propagate_by_default = true;
}

OnServerUnload::Event::~Event()
{
}

bool OnServerUnload::Event::dispatch(Manager& state, Environment& environment)
{
	return dispatchEvent<OnServerUnload::Event>
		(this, state, environment, environment.Generic.OnUnload);
}

void OnServerUnload::Event::push_instance(LuaState& state, Environment& environment)
{
	state.pushClassTableInstance("OnServerUnload");
	state.setField(-1, "reload", !real_shutdown);
	state.setField(-1, "shutdown", real_shutdown);
}

void OnServerUnload::Event::update_instance(Manager& state, Environment& environment, LuaThread_ptr thread)
{
}


///////////////////////////////////////////////////////////////////////////////
// OnSay Event
///////////////////////////////////////////////////////////////////////////////
// Triggered when a creature speaks

OnSay::Event::Event(Creature* _speaker, SpeakClass& _class, ChatChannel* channel, std::string& _text) :
	speaker(_speaker),
	speak_class(_class),
	channel(channel),
	text(_text)
{
}

OnSay::Event::~Event()
{
}

bool OnSay::Event::check_match(const ScriptInformation& info)
{
	std::string match_text = text;
	if(!info.case_sensitive) {
		toLowerCaseString(match_text);
	}

	switch(info.method) {
		case FILTER_ALL:
			return true;
		case FILTER_SUBSTRING:
			return match_text.find(info.filter) != std::string::npos;
		case FILTER_MATCH_BEGINNING:
			return match_text.find(info.filter) == 0;
		case FILTER_EXACT:
			return match_text == info.filter;
		default: break;
	}
	return false;
}

bool OnSay::Event::dispatch(Manager& state, Environment& environment)
{
	ListenerList list = speaker->getListeners(ON_SAY_LISTENER);
	if(dispatchEvent<OnSay::Event, ScriptInformation>
			(this, state, environment, list)
		)
		return true;

	return dispatchEvent<OnSay::Event, ScriptInformation>
		(this, state, environment, environment.Generic.OnSay);
}

void OnSay::Event::push_instance(LuaState& state, Environment& environment)
{
	//std::cout << "pushing instance" << std::endl;
	state.pushClassTableInstance("OnSayEvent");
	state.pushThing(speaker);
	state.setField(-2, "creature");
	state.pushChannel(channel);
	state.setField(-2, "channel");
	state.setField(-1, "class", int32_t(speak_class));
	state.setField(-1, "text", text);
	//std::cout << state.typeOf() << ":" << state.getStackSize() << std::endl;
	//std::cout << "endof" << std::endl;
}

void OnSay::Event::update_instance(Manager& state, Environment& environment, LuaThread_ptr thread)
{
	thread->getField(-1, "class");
	if(thread->isNumber()) {
		speak_class = (SpeakClass)thread->popInteger();
	}
	else {
		thread->HandleError("Event 'OnSay' invalid value of 'class'");
		thread->pop();
	}

	thread->getField(-1, "text");
	if(thread->isString()) {
		text = thread->popString();
	}
	else {
		thread->HandleError("Event 'OnSay' invalid value of 'text'");
		thread->pop();
	}
}


///////////////////////////////////////////////////////////////////////////////
// OnHear Event
///////////////////////////////////////////////////////////////////////////////
// Triggered when a creature hears another creature speak

OnHear::Event::Event(Creature* creature, Creature* talking_creature, const std::string& message, const SpeakClass& speak_class) :
	creature(creature),
	talking_creature(talking_creature),
	message(message),
	speak_class(speak_class)
{
}

OnHear::Event::~Event()
{
}

bool OnHear::Event::dispatch(Manager& state, Environment& environment)
{
	ListenerList list = creature->getListeners(ON_HEAR_LISTENER);
	if(dispatchEvent<OnHear::Event>(this, state, environment, list))
		return true;
	return false;
}

void OnHear::Event::push_instance(LuaState& state, Environment& environment)
{
	state.pushClassTableInstance("OnHearEvent");
	state.pushThing(creature);
	state.setField(-2, "creature");
	state.pushThing(talking_creature);
	state.setField(-2, "talking_creature");
	state.setField(-1, "class", int32_t(speak_class));
	state.setField(-1, "text", message);
}

void OnHear::Event::update_instance(Manager& state, Environment& environment, LuaThread_ptr thread)
{
}


///////////////////////////////////////////////////////////////////////////////
// OnUseItem Event
///////////////////////////////////////////////////////////////////////////////
// Triggered when a player use an item

OnUseItem::Event::Event(Player* user, Item* item, const PositionEx* toPos, ReturnValue& retval) :
	user(user),
	item(item),
	targetPos(toPos),
	retval(retval)
{
}

OnUseItem::Event::~Event()
{
}

bool OnUseItem::Event::check_match(const ScriptInformation& info)
{
	switch(info.method) {
		case FILTER_ITEMID:
			return item->getID() == info.id;
		case FILTER_ACTIONID:
			return item->getActionId() == info.id;
		case FILTER_UNIQUEID:
			return item->getUniqueId() == info.id;
		default: break;
	}
	return false;
}

bool OnUseItem::Event::dispatch(Manager& state, Environment& environment)
{
	// Extremely naive solution
	// Should be a map with id:callback instead.
	return dispatchEvent<OnUseItem::Event, ScriptInformation>
		(this, state, environment, environment.Generic.OnUseItem);
}

void OnUseItem::Event::push_instance(LuaState& state, Environment& environment)
{
	state.pushClassTableInstance("OnUseItemEvent");
	state.pushThing(user);
	state.setField(-2, "player");
	state.pushThing(item);
	state.setField(-2, "item");
	state.setField(-1, "retval", retval);
	if(targetPos) {
		state.pushPosition(*targetPos);
	} else {
		state.pushNil();
	}
	state.setField(-2, "target");
}

void OnUseItem::Event::update_instance(Manager& state, Environment& environment, LuaThread_ptr thread)
{
	thread->getField(-1, "retval");
	if(thread->isNumber()) {
		retval = (ReturnValue)thread->popInteger();
	}
	else {
		thread->HandleError("Event 'OnUseItem' invalid value of 'retval'");
		thread->pop();
	}
}


///////////////////////////////////////////////////////////////////////////////
// OnEquipItem Event
///////////////////////////////////////////////////////////////////////////////
// Triggered when a player equip/dequip an item

OnEquipItem::Event::Event(Player* user, Item* item, SlotType slot, bool equip) :
	user(user),
	item(item),
	equip(equip)
{
	switch(slot.value()){
		case ::enums::SLOT_HEAD:     equipslot = SLOTPOSITION_HEAD;     break;
		case ::enums::SLOT_NECKLACE: equipslot = SLOTPOSITION_NECKLACE; break;
		case ::enums::SLOT_BACKPACK: equipslot = SLOTPOSITION_BACKPACK; break;
		case ::enums::SLOT_ARMOR:    equipslot = SLOTPOSITION_ARMOR;    break;
		case ::enums::SLOT_RIGHT:    equipslot = SLOTPOSITION_RIGHT;    break;
		case ::enums::SLOT_LEFT:     equipslot = SLOTPOSITION_LEFT;     break;
		case ::enums::SLOT_LEGS:     equipslot = SLOTPOSITION_LEGS;     break;
		case ::enums::SLOT_FEET:     equipslot = SLOTPOSITION_FEET;     break;
		case ::enums::SLOT_RING:     equipslot = SLOTPOSITION_RING;     break;
		case ::enums::SLOT_AMMO:     equipslot = SLOTPOSITION_AMMO;     break;

		default:                     equipslot = SLOTPOSITION_NONE;     break;
	}
}

OnEquipItem::Event::~Event()
{
}

bool OnEquipItem::Event::check_match(const ScriptInformation& info)
{

	if((info.slot & equipslot) || info.equip != equip){
		return false;
	}

	switch(info.method) {
		case FILTER_ITEMID:
			return item->getID() == info.id;
		case FILTER_ACTIONID:
			return item->getActionId() == info.id;
		case FILTER_UNIQUEID:
			return item->getUniqueId() == info.id;
		default: break;
	}
	return false;
}

bool OnEquipItem::Event::dispatch(Manager& state, Environment& environment)
{
	// Extremely naive solution
	// Should be a map with id:callback instead.
	return dispatchEvent<OnEquipItem::Event, ScriptInformation>
		(this, state, environment, environment.Generic.OnEquipItem);
}

void OnEquipItem::Event::push_instance(LuaState& state, Environment& environment)
{
	state.pushClassTableInstance("OnEquipItemEvent");
	state.pushThing(user);
	state.setField(-2, "player");
	state.pushThing(item);
	state.setField(-2, "item");
}

void OnEquipItem::Event::update_instance(Manager& state, Environment& environment, LuaThread_ptr thread)
{
}

///////////////////////////////////////////////////////////////////////////////
// OnMoveCreature Event
///////////////////////////////////////////////////////////////////////////////
// Triggered when a creature moves

OnMoveCreature::Event::Event(Creature* actor, Creature* moving_creature, Tile* fromTile, Tile* toTile) :
	actor(actor),
	moving_creature(moving_creature),
	fromTile(fromTile),
	toTile(toTile),
	moveType(TYPE_NONE)
{
	if(fromTile && toTile){
		moveType = TYPE_MOVE;
	}
	else if(fromTile){
		moveType = TYPE_STEPIN;
	}
	else if(toTile){
		moveType = TYPE_STEPOUT;
	}
}

OnMoveCreature::Event::~Event()
{
}

bool OnMoveCreature::Event::check_match(const ScriptInformation& info)
{
	if(moveType != TYPE_MOVE && moveType != info.moveType){
		return false;
	}

	if(info.method == FILTER_NONE){
		return true;
	}

	return (isMatch(info, fromTile) || isMatch(info, toTile));
}

bool OnMoveCreature::Event::isMatch(const ScriptInformation& info, Tile* tile)
{
	if(tile){
		int32_t j = tile->__getLastIndex();
		Item* item = NULL;
		for(int32_t i = tile->__getFirstIndex(); i < j; ++i){
			Thing* thing = tile->__getThing(i);
			if(thing && (item = thing->getItem())){
				switch(info.method) {
					case FILTER_ITEMID:
						return item->getID() == info.id;
					case FILTER_ACTIONID:
						return item->getActionId() == info.id;
					case FILTER_UNIQUEID:
						return item->getUniqueId() == info.id;
					default: break;
				}
			}
		}
	}

	return false;
}

bool OnMoveCreature::Event::dispatch(Manager& state, Environment& environment)
{
	ListenerList list = moving_creature->getListeners(ON_MOVE_CREATURE_LISTENER);
	if(dispatchEvent<OnMoveCreature::Event, ScriptInformation>
			(this, state, environment, list)
		)
		return true;

	// Extremely naive solution
	// Should be a map with id:callback instead.
	return dispatchEvent<OnMoveCreature::Event, ScriptInformation>
		(this, state, environment, environment.Generic.OnMoveCreature);
}

void OnMoveCreature::Event::push_instance(LuaState& state, Environment& environment)
{
	state.pushClassTableInstance("OnMoveCreatureEvent");
	state.pushThing(actor);
	state.setField(-2, "creature");

	if(moveType == TYPE_MOVE){
		state.pushThing(fromTile);
		state.setField(-2, "fromTile");
		state.pushThing(toTile);
		state.setField(-2, "toTile");
	}
	else if(moveType == TYPE_STEPIN){
		state.pushThing(fromTile);
		state.setField(-2, "toTile");
	}
	else if(moveType == TYPE_STEPOUT){
		state.pushThing(toTile);
		state.setField(-2, "fromTile");
	}

	state.pushThing(moving_creature);
	state.setField(-2, "moving_creature");
}

void OnMoveCreature::Event::update_instance(Manager& state, Environment& environment, LuaThread_ptr thread)
{
}

///////////////////////////////////////////////////////////////////////////////
// OnTurn Event
///////////////////////////////////////////////////////////////////////////////
// Triggered when a creature turns

OnTurn::Event::Event(Creature* creature, Direction dir) :
	creature(creature),
	direction(dir)
{
	propagate_by_default = true;
}

OnTurn::Event::~Event()
{
}

bool OnTurn::Event::dispatch(Manager& state, Environment& environment)
{
	ListenerList list = creature->getListeners(ON_TURN_LISTENER);
	if(dispatchEvent<OnTurn::Event>
			(this, state, environment, list)
		)
		return true;

	return dispatchEvent<OnTurn::Event>
		(this, state, environment, environment.Generic.OnTurn);
}

void OnTurn::Event::push_instance(LuaState& state, Environment& environment)
{
	state.pushClassTableInstance("OnTurnEvent");
	state.pushThing(creature);
	state.setField(-2, "creature");
	state.pushInteger(direction);
	state.setField(-2, "direction");
}

void OnTurn::Event::update_instance(Manager& state, Environment& environment, LuaThread_ptr thread)
{
	// ...
}

///////////////////////////////////////////////////////////////////////////////
// OnMoveCreature Event
///////////////////////////////////////////////////////////////////////////////
// Triggered when an item is moved

OnMoveItem::Event::Event(Creature* actor, Item* item, Tile* tile, bool addItem) :
	actor(actor),
	item(item),
	tile(tile),
	addItem(addItem)
{
}

OnMoveItem::Event::~Event()
{
}

bool OnMoveItem::Event::check_match(const ScriptInformation& info)
{

	if(info.addItem != addItem){
		return false;
	}

	if(info.isItemOnTile){
		int32_t j = tile->__getLastIndex();
		Item* itemOnTile = NULL;
		for(int32_t i = tile->__getFirstIndex(); i < j; ++i){
			Thing* thing = tile->__getThing(i);
			if(thing && (itemOnTile = thing->getItem()) && itemOnTile != item){
				switch(info.method) {
					case FILTER_ITEMID:
						if(itemOnTile->getID() == info.id)
							return true;
						break;
					case FILTER_ACTIONID:
						if(itemOnTile->getActionId() == info.id)
							return true;
						break;
					case FILTER_UNIQUEID:
						if(itemOnTile->getUniqueId() == info.id)
							return true;
						break;
					default: break;
				}
			}
		}
	}
	else{
		switch(info.method) {
			case FILTER_ITEMID:
				return item->getID() == info.id;
			case FILTER_ACTIONID:
				return item->getActionId() == info.id;
			case FILTER_UNIQUEID:
				return item->getUniqueId() == info.id;
			default: break;
		}
	}


	return false;
}

bool OnMoveItem::Event::dispatch(Manager& state, Environment& environment)
{
	// Extremely naive solution
	// Should be a map with id:callback instead.
	return dispatchEvent<OnMoveItem::Event, ScriptInformation>
		(this, state, environment, environment.Generic.OnMoveItem);
}

void OnMoveItem::Event::push_instance(LuaState& state, Environment& environment)
{
	state.pushClassTableInstance("OnMoveItemEvent");
	state.pushThing(actor);
	state.setField(-2, "creature");
	state.pushThing(tile);
	state.setField(-2, "tile");
	state.pushThing(item);
	state.setField(-2, "item");
}

void OnMoveItem::Event::update_instance(Manager& state, Environment& environment, LuaThread_ptr thread)
{
}


///////////////////////////////////////////////////////////////////////////////
// OnJoinChannel Event
///////////////////////////////////////////////////////////////////////////////
// Triggered when a player opens a new chat channel

OnJoinChannel::Event::Event(Player* chatter, ChatChannel* channel) :
	chatter(chatter),
	channel(channel)
{
	propagate_by_default = true;
}

OnJoinChannel::Event::~Event()
{
}

bool OnJoinChannel::Event::dispatch(Manager& state, Environment& environment)
{
	ListenerList list = chatter->getListeners(ON_OPEN_CHANNEL_LISTENER);
	if(dispatchEvent<OnJoinChannel::Event>
			(this, state, environment, list)
		)
		return true;
	return dispatchEvent<OnJoinChannel::Event>
		(this, state, environment, environment.Generic.OnJoinChannel);
}

void OnJoinChannel::Event::push_instance(LuaState& state, Environment& environment)
{
	state.pushClassTableInstance("OnJoinChannelEvent");
	state.pushThing(chatter);
	state.setField(-2, "player");
	state.pushChannel(channel);
	state.setField(-2, "channel");
}

void OnJoinChannel::Event::update_instance(Manager& state, Environment& environment, LuaThread_ptr thread)
{
	// Nothing can change...
}


///////////////////////////////////////////////////////////////////////////////
// OnLeaveChannel Event
///////////////////////////////////////////////////////////////////////////////
// Triggered when a player closes an existing chat channel

OnLeaveChannel::Event::Event(Player* chatter, ChatChannel* channel) :
	chatter(chatter),
	channel(channel)
{
	propagate_by_default = true;
}

OnLeaveChannel::Event::~Event() {
}

bool OnLeaveChannel::Event::dispatch(Manager& state, Environment& environment)
{
	ListenerList list = chatter->getListeners(ON_CLOSE_CHANNEL_LISTENER);
	if(dispatchEvent<OnLeaveChannel::Event>
			(this, state, environment, list)
		)
		return true;
	return dispatchEvent<OnLeaveChannel::Event>
		(this, state, environment, environment.Generic.OnLeaveChannel);
}

void OnLeaveChannel::Event::push_instance(LuaState& state, Environment& environment)
{
	state.pushClassTableInstance("OnLeaveChannelEvent");
	state.pushThing(chatter);
	state.setField(-2, "player");
	state.pushChannel(channel);
	state.setField(-2, "channel");
}

void OnLeaveChannel::Event::update_instance(Manager& state, Environment& environment, LuaThread_ptr thread)
{
	// Nothing can change...
}


///////////////////////////////////////////////////////////////////////////////
// OnLogin Event
///////////////////////////////////////////////////////////////////////////////
// Triggered when a player enters the server

OnLogin::Event::Event(Player* player) :
	player(player)
{
	propagate_by_default = true;
}

OnLogin::Event::~Event() {
}

bool OnLogin::Event::dispatch(Manager& state, Environment& environment)
{
	return dispatchEvent<OnLogin::Event>
		(this, state, environment, environment.Generic.OnLogin);
}

void OnLogin::Event::push_instance(LuaState& state, Environment& environment)
{
	state.pushClassTableInstance("OnLogin");
	state.pushThing(player);
	state.setField(-2, "player");
}

void OnLogin::Event::update_instance(Manager& state, Environment& environment, LuaThread_ptr thread)
{
	// Nothing can change...
}


///////////////////////////////////////////////////////////////////////////////
// OnLogout Event
///////////////////////////////////////////////////////////////////////////////
// Triggered when a player leaves the server

OnLogout::Event::Event(Player* player, bool forced, bool timeout) :
	player(player),
	forced(forced),
	timeout(timeout)
{
	propagate_by_default = true;
}

OnLogout::Event::~Event()
{
}

bool OnLogout::Event::dispatch(Manager& state, Environment& environment)
{
	ListenerList list = player->getListeners(ON_LOGOUT_LISTENER);
	if(dispatchEvent<OnLogout::Event>
			(this, state, environment, list)
		)
		return true;
	return dispatchEvent<OnLogout::Event>
		(this, state, environment, environment.Generic.OnLogout);
}

void OnLogout::Event::push_instance(LuaState& state, Environment& environment)
{
	state.pushClassTableInstance("OnLogout");
	state.pushThing(player);
	state.setField(-2, "player");
	state.setField(-1, "forced", forced);
	state.setField(-1, "timeout", timeout);
}

void OnLogout::Event::update_instance(Manager& state, Environment& environment, LuaThread_ptr thread)
{
	// Nothing can change...
}


///////////////////////////////////////////////////////////////////////////////
// OnLook Event
///////////////////////////////////////////////////////////////////////////////
// Triggered when a player looks at something

OnLook::Event::Event(Player* player, std::string& desc, Thing* object) :
	player(player),
	desc(desc),
	object(object)
{
	propagate_by_default = true;
}

OnLook::Event::~Event()
{
}

bool OnLook::Event::check_match(const ScriptInformation& info)
{
	if(info.method == FILTER_NONE)
		return true;

	if(object) {
		Item* item = object->getItem();
		Creature* creature = object->getCreature();

		switch(info.method) {
			case FILTER_ITEMID:
				return item && item->getID() == info.id;
			case FILTER_ACTIONID:
				return item && item->getActionId() == info.id;
			case FILTER_UNIQUEID:
				return item && item->getUniqueId() == info.id;
			case FILTER_CREATUREID:
				return creature && creature->getID() == info.id;
			default: break;
		}
	}
	return false;
}

bool OnLook::Event::dispatch(Manager& state, Environment& environment)
{
	ListenerList list = player->getListeners(ON_LOOK_LISTENER);
	if(dispatchEvent<OnLook::Event>
			(this, state, environment, list)
		)
		return true;

	// Extremely naive solution
	// Should be a map with id:callback instead.
	return dispatchEvent<OnLook::Event, ScriptInformation>
		(this, state, environment, environment.Generic.OnLook);
}

void OnLook::Event::push_instance(LuaState& state, Environment& environment)
{
	state.pushClassTableInstance("OnLookEvent");
	state.pushThing(player);
	state.setField(-2, "player");
	state.setField(-1, "description", desc);
	state.pushThing(object);
	state.setField(-2, "object");
}

void OnLook::Event::update_instance(Manager& state, Environment& environment, LuaThread_ptr thread)
{
	thread->getField(-1, "description");
	if(thread->isString()) {
		desc = thread->popString();
	}
	else {
		thread->HandleError("Event 'OnLook' invalid value of 'description'");
		thread->pop();
	}
}


///////////////////////////////////////////////////////////////////////////////
// OnSpot Event
///////////////////////////////////////////////////////////////////////////////
// Triggered when one creature spots another

OnSpotCreature::Event::Event(Creature* creature, Creature* spotted_creature) :
	creature(creature),
	spotted_creature(spotted_creature)
{
}

OnSpotCreature::Event::~Event()
{
}

bool OnSpotCreature::Event::dispatch(Manager& state, Environment& environment)
{
	ListenerList list = creature->getListeners(ON_SPOT_CREATURE_LISTENER);
	if(dispatchEvent<OnSpotCreature::Event>
			(this, state, environment, list))
		return true;
	return true;
}

void OnSpotCreature::Event::push_instance(LuaState& state, Environment& environment)
{
	state.pushClassTableInstance("OnSpotCreatureEvent");
	state.pushThing(creature);
	state.setField(-2, "creature");
	state.pushThing(spotted_creature);
	state.setField(-2, "spotted_creature");
}

void OnSpotCreature::Event::update_instance(Manager& state, Environment& environment, LuaThread_ptr thread)
{
	;
}


///////////////////////////////////////////////////////////////////////////////
// OnLose Event
///////////////////////////////////////////////////////////////////////////////
// Triggered when one creature spots another

OnLoseCreature::Event::Event(Creature* creature, Creature* lose_creature) :
	creature(creature),
	lose_creature(lose_creature)
{
}

OnLoseCreature::Event::~Event()
{
}

bool OnLoseCreature::Event::dispatch(Manager& state, Environment& environment)
{
	ListenerList list = creature->getListeners(ON_LOSE_CREATURE_LISTENER);
	if(dispatchEvent<OnLoseCreature::Event>
			(this, state, environment, list))
		return true;
	return true;
}

void OnLoseCreature::Event::push_instance(LuaState& state, Environment& environment)
{
	state.pushClassTableInstance("OnLoseCreatureEvent");
	state.pushThing(creature);
	state.setField(-2, "creature");
	state.pushThing(lose_creature);
	state.setField(-2, "lost_creature");
}

void OnLoseCreature::Event::update_instance(Manager& state, Environment& environment, LuaThread_ptr thread)
{
	;
}


///////////////////////////////////////////////////////////////////////////////
// OnSpawn Event
///////////////////////////////////////////////////////////////////////////////
// Triggered when a creature spawns on map

OnSpawn::Event::Event(Actor* actor, bool reloading) :
	actor(actor),
	reloading(reloading)
{
	propagate_by_default = true;
}

OnSpawn::Event::~Event()
{
}

bool OnSpawn::Event::dispatch(Manager& state, Environment& environment)
{
	Script::ListenerStringMap::iterator eiter = environment.Generic.OnSpawn.find(asLowerCaseString(actor->getName()));

	if(eiter != environment.Generic.OnSpawn.end())
		return dispatchEvent<OnSpawn::Event>
			(this, state, environment, eiter->second);
	return true;
}

void OnSpawn::Event::push_instance(LuaState& state, Environment& environment)
{
	state.pushClassTableInstance("OnSpawnEvent");
	state.pushThing(actor);
	state.setField(-2, "actor");
	state.setField(-1, "reloading", reloading);
}

void OnSpawn::Event::update_instance(Manager& state, Environment& environment, LuaThread_ptr thread)
{
	;
}


///////////////////////////////////////////////////////////////////////////////
// OnThink Event
///////////////////////////////////////////////////////////////////////////////
// Triggered when a creature thinks

OnThink::Event::Event(Creature* creature, int interval) :
	creature(creature),
	interval(interval)
{
	propagate_by_default = true;
}

OnThink::Event::~Event()
{
}

bool OnThink::Event::dispatch(Manager& state, Environment& environment)
{
	ListenerList list = creature->getListeners(ON_THINK_LISTENER);
	if(dispatchEvent<OnThink::Event>
			(this, state, environment, list))
		return true;
	return false;
}

void OnThink::Event::push_instance(LuaState& state, Environment& environment)
{
	state.pushClassTableInstance("OnThinkEvent");
	state.pushThing(creature);
	state.setField(-2, "creature");
	state.setField(-1, "interval", interval);
}

void OnThink::Event::update_instance(Manager& state, Environment& environment, LuaThread_ptr thread)
{
	;
}


///////////////////////////////////////////////////////////////////////////////
// OnAdvance Event
///////////////////////////////////////////////////////////////////////////////
// Triggered when a creature speaks

OnAdvance::Event::Event(Player* _player, LevelType _skill, uint32_t _oldSkillLevel, uint32_t _newSkillLevel) :
	player(_player),
	skill(_skill),
	oldSkillLevel(_oldSkillLevel),
	newSkillLevel(_newSkillLevel)
{
	propagate_by_default = true;
}

OnAdvance::Event::~Event()
{
}

bool OnAdvance::Event::check_match(const ScriptInformation& info)
{
	switch(info.method) {
		case FILTER_ALL:
			return true;
		case FILTER_SKILL:
			return info.skill == skill;
		default: break;
	}
	return false;
}

bool OnAdvance::Event::dispatch(Manager& state, Environment& environment)
{
	ListenerList list = player->getListeners(ON_ADVANCE_LISTENER);
	if(dispatchEvent<OnAdvance::Event, ScriptInformation>
			(this, state, environment, list)
		)
		return true;

	return dispatchEvent<OnAdvance::Event, ScriptInformation>
		(this, state, environment, environment.Generic.OnAdvance);
}

void OnAdvance::Event::push_instance(LuaState& state, Environment& environment)
{
	state.pushClassTableInstance("OnAdvanceEvent");
	state.pushThing(player);
	state.setField(-2, "player");
	state.setField(-1, "skill", skill.value());
	state.setField(-1, "oldLevel", oldSkillLevel);
	state.setField(-1, "newLevel", newSkillLevel);
}

void OnAdvance::Event::update_instance(Manager& state, Environment& environment, LuaThread_ptr thread)
{
	;
}


