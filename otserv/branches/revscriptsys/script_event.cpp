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
#include "script_enviroment.h"
#include "script_listener.h"
#include "script_manager.h"
#include "tools.h"

#include "tile.h"
#include "player.h"
#include "creature.h"
#include "item.h"

uint32_t Script::Event::eventID_counter = 0;

using namespace Script;

///////////////////////////////////////////////////////////////////////////////
// General Event class

Event::Event() : eventID(++eventID_counter), propagate_by_default(false) {
	std::ostringstream os;
	os << "EI_" << eventID; // Event instance tag
	lua_tag = os.str();
}

Event::~Event() {
}

bool Event::call(Manager& state, Enviroment& enviroment, Listener_ptr listener) {
	LuaThread_ptr thread = state.newThread("OnSay");

	// Stack is empty
	// Push callback
	thread->pushCallback(listener);
	
	if(thread->isNil()) {
		thread->HandleError("Attempt to call destroyed '" + getName() + "' listener.");
		thread->pop();
		return false;
	}

	// Push event
	thread->pushEvent(*this);
	thread->duplicate();
	thread->setRegistryItem(lua_tag);
	

	// Run thread
	thread->run(1);

	if(thread->ok() == false) {
		state.pushNil();
		state.setRegistryItem(lua_tag);
		return false;
	}

	// Retrieve event info
	thread->getRegistryItem(lua_tag);
	
	// Update this instance with values from lua
	update_instance(state, enviroment, thread);

	// Find out if the event should propagate
	bool propagate = propagate_by_default;
	thread->getField(-1, "skipped");
	if(thread->isNil(-1)) {
		thread->pop();
	} else {
		if(thread->isBoolean(-1)) {
			propagate = thread->popBoolean();
		} else {
			thread->HandleError("Invalid type of value 'skipped' of event table from '" + getName() + "' listener.");
			thread->pop();
		}
	}

	// clean up
	thread->pushNil();
	thread->setRegistryItem(lua_tag);
	thread->pop(); // pop event table, will be garbage collected as we cleared the registry from it

	return !propagate;
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

OnSay::Event::~Event() {
}

bool OnSay::Event::check_match(const ScriptInformation& info) {
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

bool OnSay::Event::dispatch(Manager& state, Enviroment& enviroment) {
	ListenerList list = speaker->getListeners(ON_SAY_LISTENER);
	if(dispatchEvent<OnSay::Event, ScriptInformation>
			(this, state, enviroment, list)
		)
		return true;

	return dispatchEvent<OnSay::Event, ScriptInformation>
		(this, state, enviroment, enviroment.Generic.OnSay);
}

void OnSay::Event::push_instance(LuaState& state, Enviroment& enviroment) {
	//std::cout << "pushing instance" << std::endl;
	state.pushClassTableInstance("OnSayEvent");
	state.pushThing(speaker);
	state.setField(-2, "creature");
	state.pushChannel(channel);
	state.setField(-2, "channel");
	state.setField(-1, "class", int32_t(speak_class));
	state.setField(-1, "text", text);
	//std::cout << state.typeOf() << ":" << state.getStackTop() << std::endl;
	//std::cout << "endof" << std::endl;
}

void OnSay::Event::update_instance(Manager& state, Enviroment& enviroment, LuaThread_ptr thread) {
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

OnUseItem::Event::~Event() {
}

bool OnUseItem::Event::check_match(const ScriptInformation& info) {
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

bool OnUseItem::Event::dispatch(Manager& state, Enviroment& enviroment) {
	// Extremely naive solution
	// Should be a map with id:callback instead.
	return dispatchEvent<OnUseItem::Event, ScriptInformation>
		(this, state, enviroment, enviroment.Generic.OnUseItem);
}

void OnUseItem::Event::push_instance(LuaState& state, Enviroment& enviroment) {
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

void OnUseItem::Event::update_instance(Manager& state, Enviroment& enviroment, LuaThread_ptr thread) {
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

OnEquipItem::Event::Event(Player* user, Item* item, slots_t slot, bool equip) :
	user(user),
	item(item),
	equip(equip)
{
	switch(slot){
		case SLOT_HEAD: equipslot = SLOTP_HEAD;
		case SLOT_NECKLACE: equipslot = SLOTP_NECKLACE;
		case SLOT_BACKPACK: equipslot = SLOTP_BACKPACK; break;
		case SLOT_ARMOR: equipslot = SLOTP_ARMOR; break;
		case SLOT_RIGHT: equipslot = SLOTP_RIGHT; break;
		case SLOT_LEFT: equipslot = SLOTP_LEFT; break;
		case SLOT_LEGS: equipslot = SLOTP_LEGS; break;
		case SLOT_FEET: equipslot = SLOTP_FEET; break;
		case SLOT_RING: equipslot = SLOTP_RING; break; 
		case SLOT_AMMO: equipslot = SLOTP_AMMO; break;

		default:
			equipslot = 0;
			break;
	}
}

OnEquipItem::Event::~Event() {
}

bool OnEquipItem::Event::check_match(const ScriptInformation& info) {

	if(((info.slot & equipslot) != equipslot) ||  info.equip != equip){
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

bool OnEquipItem::Event::dispatch(Manager& state, Enviroment& enviroment) {
	// Extremely naive solution
	// Should be a map with id:callback instead.
	return dispatchEvent<OnEquipItem::Event, ScriptInformation>
		(this, state, enviroment, enviroment.Generic.OnEquipItem);
}

void OnEquipItem::Event::push_instance(LuaState& state, Enviroment& enviroment) {
	state.pushClassTableInstance("OnEquipItemEvent");
	state.pushThing(user); 
	state.setField(-2, "player");
	state.pushThing(item);
	state.setField(-2, "item");
}

void OnEquipItem::Event::update_instance(Manager& state, Enviroment& enviroment, LuaThread_ptr thread) {
}

///////////////////////////////////////////////////////////////////////////////
// OnMoveCreature Event
///////////////////////////////////////////////////////////////////////////////
// Triggered when a creature moves

OnMoveCreature::Event::Event(Creature* actor, Creature* creature, Tile* tile, bool stepIn) :
	actor(actor),
	creature(creature),
	tile(tile),
	stepIn(stepIn)
{
}

OnMoveCreature::Event::~Event() {
}

bool OnMoveCreature::Event::check_match(const ScriptInformation& info) {

	if(info.stepIn != stepIn){
		return false;
	}

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

	return false;
}

bool OnMoveCreature::Event::dispatch(Manager& state, Enviroment& enviroment) {
	// Extremely naive solution
	// Should be a map with id:callback instead.
	return dispatchEvent<OnMoveCreature::Event, ScriptInformation>
		(this, state, enviroment, enviroment.Generic.OnMoveCreature);
}

void OnMoveCreature::Event::push_instance(LuaState& state, Enviroment& enviroment) {
	state.pushClassTableInstance("OnMoveCreatureEvent");
	state.pushThing(actor);
	state.setField(-2, "actor");
	state.pushThing(tile);
	state.setField(-2, "tile");
	state.pushThing(creature);
	state.setField(-2, "creature");
}

void OnMoveCreature::Event::update_instance(Manager& state, Enviroment& enviroment, LuaThread_ptr thread) {
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

OnMoveItem::Event::~Event() {
}

bool OnMoveItem::Event::check_match(const ScriptInformation& info) {

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
						return itemOnTile->getID() == info.id;
					case FILTER_ACTIONID:
						return itemOnTile->getActionId() == info.id;
					case FILTER_UNIQUEID:
						return itemOnTile->getUniqueId() == info.id;
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

bool OnMoveItem::Event::dispatch(Manager& state, Enviroment& enviroment) {
	// Extremely naive solution
	// Should be a map with id:callback instead.
	return dispatchEvent<OnMoveItem::Event, ScriptInformation>
		(this, state, enviroment, enviroment.Generic.OnMoveItem);
}

void OnMoveItem::Event::push_instance(LuaState& state, Enviroment& enviroment) {
	state.pushClassTableInstance("OnMoveItemEvent");
	state.pushThing(actor);
	state.setField(-2, "actor");
	state.pushThing(tile);
	state.setField(-2, "tile");
	state.pushThing(item);
	state.setField(-2, "item");
}

void OnMoveItem::Event::update_instance(Manager& state, Enviroment& enviroment, LuaThread_ptr thread) {
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

OnJoinChannel::Event::~Event() {
}

bool OnJoinChannel::Event::dispatch(Manager& state, Enviroment& enviroment) {
	ListenerList list = chatter->getListeners(ON_OPEN_CHANNEL_LISTENER);
	if(dispatchEvent<OnJoinChannel::Event>
			(this, state, enviroment, list)
		)
		return true;
	return dispatchEvent<OnJoinChannel::Event>
		(this, state, enviroment, enviroment.Generic.OnJoinChannel);
}

void OnJoinChannel::Event::push_instance(LuaState& state, Enviroment& enviroment) {
	state.pushClassTableInstance("OnJoinChannelEvent");
	state.pushThing(chatter);
	state.setField(-2, "player");
	state.pushChannel(channel);
	state.setField(-2, "channel");
}

void OnJoinChannel::Event::update_instance(Manager& state, Enviroment& enviroment, LuaThread_ptr thread) {
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

bool OnLeaveChannel::Event::dispatch(Manager& state, Enviroment& enviroment) {
	ListenerList list = chatter->getListeners(ON_CLOSE_CHANNEL_LISTENER);
	if(dispatchEvent<OnLeaveChannel::Event>
			(this, state, enviroment, list)
		)
		return true;
	return dispatchEvent<OnLeaveChannel::Event>
		(this, state, enviroment, enviroment.Generic.OnLeaveChannel);
}

void OnLeaveChannel::Event::push_instance(LuaState& state, Enviroment& enviroment) {
	state.pushClassTableInstance("OnLeaveChannelEvent");
	state.pushThing(chatter);
	state.setField(-2, "player");
	state.pushChannel(channel);
	state.setField(-2, "channel");
}

void OnLeaveChannel::Event::update_instance(Manager& state, Enviroment& enviroment, LuaThread_ptr thread) {
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

bool OnLogin::Event::dispatch(Manager& state, Enviroment& enviroment) {
	return dispatchEvent<OnLogin::Event>
		(this, state, enviroment, enviroment.Generic.OnLogin);
}

void OnLogin::Event::push_instance(LuaState& state, Enviroment& enviroment) {
	state.pushClassTableInstance("OnLogin");
	state.pushThing(player);
	state.setField(-2, "player");
}

void OnLogin::Event::update_instance(Manager& state, Enviroment& enviroment, LuaThread_ptr thread) {
	// Nothing can change...
}


///////////////////////////////////////////////////////////////////////////////
// OnLogout Event
///////////////////////////////////////////////////////////////////////////////
// Triggered when a player enters the server

OnLogout::Event::Event(Player* player, bool forced, bool timeout) :
	player(player),
	forced(forced),
	timeout(timeout)
{
	propagate_by_default = true;
}

OnLogout::Event::~Event() {
}

bool OnLogout::Event::dispatch(Manager& state, Enviroment& enviroment) {
	ListenerList list = player->getListeners(ON_LOGOUT_LISTENER);
	if(dispatchEvent<OnLogout::Event>
			(this, state, enviroment, list)
		)
		return true;
	return dispatchEvent<OnLogout::Event>
		(this, state, enviroment, enviroment.Generic.OnLogout);
}

void OnLogout::Event::push_instance(LuaState& state, Enviroment& enviroment) {
	state.pushClassTableInstance("OnLogout");
	state.pushThing(player);
	state.setField(-2, "player");
	state.setField(-1, "forced", forced);
	state.setField(-1, "timeout", timeout);
}

void OnLogout::Event::update_instance(Manager& state, Enviroment& enviroment, LuaThread_ptr thread) {
	// Nothing can change...
}


///////////////////////////////////////////////////////////////////////////////
// OnUseItem Event
///////////////////////////////////////////////////////////////////////////////
// Triggered when a creature speaks

OnLook::Event::Event(Player* player, std::string& desc, Thing* object) :
	player(player),
	desc(desc),
	object(object)
{
	propagate_by_default = true;
}

OnLook::Event::~Event() {
}

bool OnLook::Event::check_match(const ScriptInformation& info) {
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

bool OnLook::Event::dispatch(Manager& state, Enviroment& enviroment) {
	ListenerList list = player->getListeners(ON_LOOK_LISTENER);
	if(dispatchEvent<OnLook::Event>
			(this, state, enviroment, list)
		)
		return true;

	// Extremely naive solution
	// Should be a map with id:callback instead.
	return dispatchEvent<OnLook::Event, ScriptInformation>
		(this, state, enviroment, enviroment.Generic.OnLook);
}

void OnLook::Event::push_instance(LuaState& state, Enviroment& enviroment) {
	state.pushClassTableInstance("OnLookEvent");
	state.pushThing(player);
	state.setField(-2, "player");
	state.setField(-1, "description", desc);
	state.pushThing(object);
	state.setField(-2, "object");
}

void OnLook::Event::update_instance(Manager& state, Enviroment& enviroment, LuaThread_ptr thread) {
	thread->getField(-1, "description");
	if(thread->isString()) {
		desc = thread->popString();
	} 
	else {
		thread->HandleError("Event 'OnLook' invalid value of 'description'");
		thread->pop();
	}
}

