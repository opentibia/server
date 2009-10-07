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

#include "script_event.h"
#include "lua_manager.h"
#include "script_environment.h"
#include "script_listener.h"
#include "script_manager.h"
#include "tools.h"
#include "tile.h"
#include "player.h"
#include "creature.h"
#include "actor.h"
#include "combat.h"
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
	int32_t ms = thread->run(1);
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

	if(thread->rawtype() == LUA_TNIL){
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
		thread->HandleError(ERROR_WARN, "Event 'OnSay' invalid value of 'class'");
		thread->pop();
	}

	thread->getField(-1, "text");
	if(thread->isString()) {
		text = thread->popString();
	}
	else {
		thread->HandleError(ERROR_WARN, "Event 'OnSay' invalid value of 'text'");
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
		thread->HandleError(ERROR_WARN, "Event 'OnUseItem' invalid value of 'retval'");
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
	else if(toTile){
		moveType = TYPE_STEPIN;
	}
	else if(fromTile){
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

	if(info.moveType == TYPE_STEPIN){
		return isMatch(info, toTile);
	}

	if(info.moveType == TYPE_STEPOUT){
		return isMatch(info, fromTile);
	}

	return false;
}

bool OnMoveCreature::Event::isMatch(const ScriptInformation& info, Tile* tile)
{
	if(tile){
		switch(info.method){
			case FILTER_ITEMID:
			{
				return (tile->items_getItemWithItemId(info.id) != NULL);
				break;
			}
			case FILTER_ACTIONID:
			{
				return (tile->items_getItemWithActionId(info.id) != NULL);
				break;
			}

			default:
				break;
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

	state.pushTile(fromTile);
	state.setField(-2, "fromTile");
	state.pushTile(toTile);
	state.setField(-2, "toTile");

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
	state.pushEnum(direction);
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
		switch(info.method){
			case FILTER_ITEMID:
			{
				return (tile->items_getItemWithItemId(info.id) != NULL);
				break;
			}
			case FILTER_ACTIONID:
			{
				return (tile->items_getItemWithActionId(info.id) != NULL);
				break;
			}

			default:
				break;
		}
	}
	else{
		switch(info.method) {
			case FILTER_ITEMID:
				return item->getID() == info.id;
			case FILTER_ACTIONID:
				return item->getActionId() == info.id;
			default: break;
		}
	}


	return false;
}

bool OnMoveItem::Event::dispatch(Manager& state, Environment& environment)
{
	Script::ListenerMap::iterator list_iter;

	if(item->getActionId() != 0){
		list_iter = environment.Generic.OnMoveItem.ActionId.find(item->getActionId());
		if(list_iter != environment.Generic.OnMoveItem.ActionId.end()){
			if(dispatchEvent<OnMoveItem::Event, ScriptInformation>
					(this, state, environment, list_iter->second)){
				return true;
			}
		}
	}

	list_iter = environment.Generic.OnMoveItem.ItemId.find(item->getID());
	if(list_iter != environment.Generic.OnMoveItem.ItemId.end()){
		if(dispatchEvent<OnMoveItem::Event, ScriptInformation>
				(this, state, environment, list_iter->second)){
			return true;
		}
	}

	if(dispatchEvent<OnMoveItem::Event, ScriptInformation>
			(this, state, environment, environment.Generic.OnMoveItemOnItem)){
		return true;
	}

	return false;
}

void OnMoveItem::Event::push_instance(LuaState& state, Environment& environment)
{
	state.pushClassTableInstance("OnMoveItemEvent");
	state.pushThing(actor);
	state.setField(-2, "creature");
	state.pushTile(tile);
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
// OnChangeOutfit Event
///////////////////////////////////////////////////////////////////////////////
// Triggered when a player request the outfit dialog

OnChangeOutfit::Event::Event(Player* player, std::list<Outfit>& outfitList) :
	player(player),
	outfitList(outfitList)
{
	propagate_by_default = true;
}

OnChangeOutfit::Event::~Event()
{
}

bool OnChangeOutfit::Event::dispatch(Manager& state, Environment& environment)
{
	ListenerList list = player->getListeners(ON_CHANGE_OUTFIT_LISTENER);
	if(dispatchEvent<OnChangeOutfit::Event>
			(this, state, environment, list)
		)
		return true;
	return dispatchEvent<OnChangeOutfit::Event>
		(this, state, environment, environment.Generic.OnChangeOutfit);
}

void OnChangeOutfit::Event::push_instance(LuaState& state, Environment& environment)
{
	state.pushClassTableInstance("OnChangeOutfit");
	state.pushThing(player);
	state.setField(-2, "player");
	state.newTable();
	int n = 1;
	for(std::list<Outfit>::iterator iter = outfitList.begin(); iter != outfitList.end(); ++iter, ++n){
		state.newTable();
		state.setField(-1, "type", iter->lookType);
		state.newTable();
		for(int i = 1; i < 3; ++i){
			state.setField(-1, i, (iter->addons & (1 << i)) != 0);
		}
		state.setField(-2, "addons");
	}
	state.setField(-2, "outfits");
}

void OnChangeOutfit::Event::update_instance(Manager& state, Environment& environment, LuaThread_ptr thread)
{
	/* structure like follows:
		outfits = {
			{type = 130, addons={[1]=true, [2]=false, [3]=true}},
			{type = 130, addons=3}
		}
	*/
	try{
		thread->getField(-1, "outfits");
		if(!thread->isTable())
			thread->HandleError("'outfits' must be a table.");
		// iterate over the table
		thread->pushNil();
		while(thread->iterateTable(-2)){
			Outfit o;

			thread->getField(-1, "type");
			o.lookType = thread->popInteger();

			thread->getField(-1, "addons");
			if(thread->isTable()){
				// Iterate over addons
				thread->pushNil();
				while(thread->iterateTable(-1)){
					bool hasit = thread->popBoolean();
					thread->duplicate();
					int addon = thread->popInteger();
					o.addons |= (1 << addon);
				}
			}
			else{
				// Integer value
				o.addons = thread->popInteger();
			}

			// Only 2 addons for outfits so far...
			o.addons &= 3;

			outfitList.push_back(o);

			thread->pop(); // pop value
		}
		// Pop the 'outfits' table
		thread->pop();
	} catch(Error& e) {
		thread->HandleError(ERROR_WARN, std::string("Event 'OnChangeOutfit' invalid structure of 'outfits'") + e.what());
	}
	thread->pop();
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
			case FILTER_CREATUREID:
				return creature && (int32_t)creature->getID() == info.id;
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
		thread->HandleError(ERROR_WARN, "Event 'OnLook' invalid value of 'description'");
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

OnThink::Event::Event(Creature* creature, int32_t interval) :
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
	state.pushEnum(skill);
	state.setField(-2, "skill");
	state.setField(-1, "oldLevel", oldSkillLevel);
	state.setField(-1, "newLevel", newSkillLevel);
}

void OnAdvance::Event::update_instance(Manager& state, Environment& environment, LuaThread_ptr thread)
{
	;
}

///////////////////////////////////////////////////////////////////////////////
// OnCondition Event
///////////////////////////////////////////////////////////////////////////////
// Triggered when a condition is added/removed or ticks


OnCondition::Event::Event(Creature* creature, Condition* condition) :
	creature(creature),
	condition(condition),
	eventType(EVENT_BEGIN)
{
	propagate_by_default = true;
}

OnCondition::Event::Event(Creature* creature, Condition* condition, ConditionEnd reason) :
	creature(creature),
	condition(condition),
	reason(reason),
	eventType(EVENT_END)
{
	propagate_by_default = true;
}

OnCondition::Event::Event(Creature* creature, Condition* condition, uint32_t ticks) :
	creature(creature),
	condition(condition),
	reason(reason),
	eventType(EVENT_TICK)
{
	propagate_by_default = true;
}

OnCondition::Event::~Event()
{
}

bool OnCondition::Event::check_match(const ScriptInformation& info)
{
	switch(info.method) {
		case FILTER_BEGIN:
			return eventType == EVENT_BEGIN && (condition->getName() == info.name);
		case FILTER_END:
			return eventType == EVENT_END && (condition->getName() == info.name);
		case FILTER_TICK:
			return eventType == EVENT_TICK && (condition->getName() == info.name);
		default: break;
	}

	return false;
}

bool OnCondition::Event::dispatch(Manager& state, Environment& environment)
{
	if(creature){
		ListenerList list = creature->getListeners(ON_CONDITION_LISTENER);
		if(dispatchEvent<OnCondition::Event, ScriptInformation>
				(this, state, environment, list))
			return true;
	}

	if(dispatchEvent<OnCondition::Event, ScriptInformation>
			(this, state, environment, environment.Generic.OnCondition))
		return true;

	return false;
}

void OnCondition::Event::push_instance(LuaState& state, Environment& environment)
{
	state.pushClassTableInstance("OnCondition");
	state.pushThing(creature);
	state.setField(-2, "creature");
	//TODO:
	//state.pushCondition(condition);
	//state.setField(-2, "condition");

	if(eventType == EVENT_BEGIN){
		//
	}
	else if(eventType == EVENT_END){
		state.pushEnum(reason);
		state.setField(-2, "reason");
	}
	else if(eventType == EVENT_TICK){
		state.pushInteger(ticks);
		state.setField(-2, "ticks");
	}
}

void OnCondition::Event::update_instance(Manager& state, Environment& environment, LuaThread_ptr thread)
{
	;
}

///////////////////////////////////////////////////////////////////////////////
// OnAttack Event
///////////////////////////////////////////////////////////////////////////////
// Triggered when a creature attacks another creature (usually every 2 seconds)


OnAttack::Event::Event(Creature* creature, Creature* attacked) :
	creature(creature),
	attacked(attacked)
{
	propagate_by_default = true;
}

OnAttack::Event::~Event()
{
}

bool OnAttack::Event::check_match(const ScriptInformation& info)
{
	// Summons are actors like all actors
	switch(info.method) {
		case FILTER_ALL:
			return true;
		case FILTER_NAME:
			return creature->getPlayer() == NULL && creature->getName() == info.name;
		case FILTER_PLAYER:
			return creature->getPlayer() != NULL;
		case FILTER_ATTACKED_NAME:
			return attacked != NULL && attacked->getPlayer() == NULL && attacked->getName() == info.name;
		case FILTER_ATTACKED_PLAYER:
			return attacked != NULL && attacked->getPlayer() != NULL;
		case FILTER_ATTACKED_ACTOR:
			return attacked != NULL && attacked->getActor() != NULL;
		case FILTER_PLAYER_ATTACK_PLAYER:
			return attacked != NULL && creature->getPlayer() != NULL && attacked->getPlayer() != NULL;
		case FILTER_PLAYER_ATTACK_ACTOR:
			return attacked != NULL && creature->getPlayer() != NULL && attacked->getActor() != NULL;
		case FILTER_ACTOR_ATTACK_PLAYER:
			return attacked != NULL && creature->getActor() != NULL && attacked->getPlayer() != NULL;
		case FILTER_ACTOR_ATTACK_ACTOR:
			return attacked != NULL && creature->getActor() != NULL && attacked->getActor() != NULL;
		default: break;
	}
	return false;
}

bool OnAttack::Event::dispatch(Manager& state, Environment& environment)
{
	if(creature){
		ListenerList list = creature->getListeners(ON_ATTACK_LISTENER);
		if(dispatchEvent<OnAttack::Event, ScriptInformation>
				(this, state, environment, list))
			return true;
	}

	if(dispatchEvent<OnAttack::Event, ScriptInformation>
			(this, state, environment, environment.Generic.OnAttack))
		return true;

	return false;
}

void OnAttack::Event::push_instance(LuaState& state, Environment& environment)
{
	state.pushClassTableInstance("OnAttackEvent");
	state.pushThing(creature);
	state.setField(-2, "creature");
	state.pushThing(attacked);
	state.setField(-2, "attacked");
}

void OnAttack::Event::update_instance(Manager& state, Environment& environment, LuaThread_ptr thread)
{
	;
}

///////////////////////////////////////////////////////////////////////////////
// OnDamage Event
///////////////////////////////////////////////////////////////////////////////
// Triggered when a creature is taking damage (or heals)


OnDamage::Event::Event(CombatType& combatType, CombatSource& combatSource, Creature* creature, int32_t& value) :
	combatType(combatType),
	combatSource(combatSource),
	creature(creature),
	value(value)
{
	propagate_by_default = true;
}

OnDamage::Event::~Event()
{
}

bool OnDamage::Event::check_match(const ScriptInformation& info)
{
	// Player summons are actors
	Creature* attacker = combatSource.getSourceCreature();
	switch(info.method) {
		case FILTER_ALL:
			return true;
		case FILTER_NAME:
			return creature->getPlayer() == NULL && creature->getName() == info.name;
		case FILTER_PLAYER:
			return creature->getPlayer() != NULL;
		case FILTER_ATTACKER_NAME:
			return attacker != NULL && attacker->getName() == info.name;
		case FILTER_ATTACKER_PLAYER:
			return attacker != NULL && attacker->getPlayer() != NULL;
		case FILTER_ATTACKER_ACTOR:
			return attacker != NULL && attacker->getActor() != NULL;
		case FILTER_PLAYER_DAMAGE_PLAYER:
			return attacker != NULL && attacker->getPlayer() != NULL && creature->getPlayer() != NULL;
		case FILTER_PLAYER_DAMAGE_ACTOR:
			return attacker != NULL && attacker->getPlayer() != NULL && creature->getActor() != NULL;
		case FILTER_ACTOR_DAMAGE_ACTOR:
			return attacker != NULL && attacker->getActor() != NULL && creature->getActor() != NULL;
		case FILTER_ACTOR_DAMAGE_PLAYER:
			return attacker != NULL && attacker->getActor() != NULL && creature->getPlayer() != NULL;
		case FILTER_TYPE:
			return combatType == info.combatType;
		default: break;
	}
	return false;
}

bool OnDamage::Event::dispatch(Manager& state, Environment& environment)
{
	if(creature){
		ListenerList list = creature->getListeners(ON_DAMAGE_LISTENER);
		if(dispatchEvent<OnDamage::Event, ScriptInformation>
				(this, state, environment, list))
			return true;
	}

	if(dispatchEvent<OnDamage::Event, ScriptInformation>
			(this, state, environment, environment.Generic.OnDamage))
		return true;

	return false;
}

void OnDamage::Event::push_instance(LuaState& state, Environment& environment)
{
	state.pushClassTableInstance("OnDamageEvent");
	state.pushThing(creature);
	state.setField(-2, "creature");
	state.pushThing(combatSource.getSourceCreature());
	state.setField(-2, "attacker");
	state.pushEnum(combatType);
	state.setField(-2, "combatType");
	state.pushThing(combatSource.getSourceItem());
	state.setField(-2, "item");
	state.pushBoolean(combatSource.isSourceCondition());
	state.setField(-2, "isCondition");
	state.pushInteger(value);
	state.setField(-2, "value");
}

void OnDamage::Event::update_instance(Manager& state, Environment& environment, LuaThread_ptr thread)
{
	thread->getField(-1, "combatType");
	if(thread->isTable()) {
		combatType = thread->popEnum<CombatType>();
	}
	else {
		thread->HandleError(ERROR_WARN, "Event 'OnDamage' invalid value of 'combatType'");
		thread->pop();
	}

	thread->getField(-1, "value");
	if(thread->isNumber()) {
		value = thread->popInteger();
	}
	else {
		thread->HandleError(ERROR_WARN, "Event 'OnDamage' invalid value of 'value'");
		thread->pop();
	}
}

///////////////////////////////////////////////////////////////////////////////
// OnKill Event
///////////////////////////////////////////////////////////////////////////////
// Triggered when a creature reaches 0 health


OnKill::Event::Event(Creature* creature, CombatSource& combatSource) :
	creature(creature),
	combatSource(combatSource)
{
	propagate_by_default = true;
}

OnKill::Event::~Event()
{
}

bool OnKill::Event::check_match(const ScriptInformation& info)
{
	// Player summons are actors
	Creature* killer = combatSource.getSourceCreature();
	switch(info.method) {
		case FILTER_ALL:
			return true;
		case FILTER_NAME:
			return creature->getPlayer() == NULL && creature->getName() == info.name;
		case FILTER_PLAYER:
			return creature->getPlayer() != NULL;
		case FILTER_KILLER_NAME:
			return killer != NULL && killer->getPlayer() == NULL && killer->getName() == info.name;
		case FILTER_KILLER_PLAYER:
			return killer != NULL && killer->getPlayer() != NULL;
		case FILTER_KILLER_ACTOR:
			return killer != NULL && killer->getActor() != NULL;
		case FILTER_PLAYER_KILL_PLAYER:
			return killer != NULL && creature->getPlayer() != NULL && killer->getPlayer() != NULL;
		case FILTER_PLAYER_KILL_ACTOR:
			return killer != NULL && creature->getPlayer() != NULL && killer->getActor() != NULL;
		case FILTER_ACTOR_KILL_ACTOR:
			return killer != NULL && creature->getActor() == NULL && killer->getActor() != NULL;
		case FILTER_ACTOR_KILL_PLAYER:
			return killer != NULL && creature->getActor() != NULL && killer->getPlayer() != NULL;
		default: break;
	}
	return false;
}

bool OnKill::Event::dispatch(Manager& state, Environment& environment)
{
	ListenerList list;
	
	// Tied to killer
	Creature* killer = combatSource.getSourceCreature();
	if(killer){
		list = killer->getListeners(ON_KILL_LISTENER);
		if(dispatchEvent<OnKill::Event, ScriptInformation>
				(this, state, environment, list))
			return true;
	}

	if(dispatchEvent<OnKill::Event, ScriptInformation>
			(this, state, environment, environment.Generic.OnKill))
		return true;
	
	// Tied to dying creature
	list = creature->getListeners(ON_KILLED_LISTENER);
	if(dispatchEvent<OnKill::Event, ScriptInformation>
			(this, state, environment, list))
		return true;

	if(dispatchEvent<OnKill::Event, ScriptInformation>
			(this, state, environment, environment.Generic.OnKilled))
		return true;

	return false;
}

void OnKill::Event::push_instance(LuaState& state, Environment& environment)
{
	state.pushClassTableInstance("OnKillEvent");
	state.pushThing(creature);
	state.setField(-2, "creature");
	state.pushThing(combatSource.getSourceCreature());
	state.setField(-2, "killer");
	state.pushThing(combatSource.getSourceItem());
	state.setField(-2, "item");
	state.pushBoolean(combatSource.isSourceCondition());
	state.setField(-2, "isCondition");
}

void OnKill::Event::update_instance(Manager& state, Environment& environment, LuaThread_ptr thread)
{
	;
}

///////////////////////////////////////////////////////////////////////////////
// OnDeath Event
///////////////////////////////////////////////////////////////////////////////
// Triggered when a creature dies (after death-delay)


OnDeath::Event::Event(Creature* creature, Item* corpse, Creature* killer) :
	creature(creature),
	corpse(corpse),
	killer(killer)
{
	propagate_by_default = true;
}

OnDeath::Event::~Event()
{
}

bool OnDeath::Event::check_match(const ScriptInformation& info)
{
	// Player summons are actors
	switch(info.method) {
		case FILTER_ALL:
			return true;
		case FILTER_NAME:
			return creature->getPlayer() == NULL && creature->getName() == info.name;
		case FILTER_PLAYER:
			return creature->getPlayer() != NULL;
		case FILTER_KILLER_NAME:
			return killer != NULL && killer->getPlayer() == NULL && killer->getName() == info.name;
		case FILTER_KILLER_PLAYER:
			return killer != NULL && killer->getPlayer() != NULL;
		case FILTER_PLAYER_DEATH_BY_PLAYER:
			return killer != NULL && creature->getPlayer() != NULL && killer->getPlayer() != NULL;
		case FILTER_PLAYER_DEATH_BY_ACTOR:
			return killer != NULL && creature->getPlayer() != NULL && killer->getActor() != NULL;
		case FILTER_ACTOR_DEATH_BY_ACTOR:
			return killer != NULL && creature->getActor() == NULL && killer->getActor() != NULL;
		case FILTER_ACTOR_DEATH_BY_PLAYER:
			return killer != NULL && creature->getActor() != NULL && killer->getPlayer() != NULL;
		default: break;
	}
	return false;
}

bool OnDeath::Event::dispatch(Manager& state, Environment& environment)
{
	ListenerList list;
	
	// Tied to killer
	if(killer){
		list = killer->getListeners(ON_DEATH_BY_LISTENER);
		if(dispatchEvent<OnDeath::Event, ScriptInformation>
				(this, state, environment, list))
			return true;
	}

	if(dispatchEvent<OnDeath::Event, ScriptInformation>
			(this, state, environment, environment.Generic.OnDeathBy))
		return true;
	
	// Tied to dying creature
	list = creature->getListeners(ON_DEATH_LISTENER);
	if(dispatchEvent<OnDeath::Event, ScriptInformation>
			(this, state, environment, list))
		return true;

	if(dispatchEvent<OnDeath::Event, ScriptInformation>
			(this, state, environment, environment.Generic.OnDeath))
		return true;

	return false;
}

void OnDeath::Event::push_instance(LuaState& state, Environment& environment)
{
	state.pushClassTableInstance("OnDeathEvent");
	state.pushThing(creature);
	state.setField(-2, "creature");
	state.pushThing(corpse);
	state.setField(-2, "corpse");
	state.pushThing(killer);
	state.setField(-2, "killer");
}

void OnDeath::Event::update_instance(Manager& state, Environment& environment, LuaThread_ptr thread)
{
	;
}
