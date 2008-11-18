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

#include "player.h"
#include "creature.h"
#include "item.h"

uint32_t Script::Event::eventID_counter = 0;

using namespace Script;

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

OnSay::Event::Event(Creature* _speaker, SpeakClass& _class, ChatChannel* channel, std::string& _receiver, std::string& _text) :
	speaker(_speaker),
	speak_class(_class),
	channel(channel),
	receiver(_receiver),
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
	if(dispatchEvent<OnSay::Event, ScriptInformation>
			(this, state, enviroment, speaker->getListeners(ON_SAY_LISTENER))
		)
		return true;

	return dispatchEvent<OnSay::Event, ScriptInformation>
		(this, state, enviroment, enviroment.Generic.OnSay);
}

void OnSay::Event::push_instance(LuaState& state, Enviroment& enviroment) {
	//std::cout << "pushing instance" << std::endl;
	state.pushClassTableInstance("OnSayEvent");
	state.pushThing(speaker);
	state.setField(-2, "speaker");
	state.pushChannel(channel);
	state.setField(-2, "channel");
	state.setField(-1, "class", int32_t(speak_class));
	state.setField(-1, "receiver", receiver);
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

	thread->getField(-1, "receiver");
	if(thread->isString()) {
		receiver = thread->popString();
	}
	else {
		thread->HandleError("Event 'OnSay' invalid value of 'receiver'");
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
// Triggered when a creature speaks

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
	state.setField(-2, "user");
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
	if(dispatchEvent<OnJoinChannel::Event>
			(this, state, enviroment, chatter->getListeners(ON_OPEN_CHANNEL_LISTENER))
		)
		return true;
	return dispatchEvent<OnJoinChannel::Event>
		(this, state, enviroment, enviroment.Generic.OnJoinChannel);
}

void OnJoinChannel::Event::push_instance(LuaState& state, Enviroment& enviroment) {
	state.pushClassTableInstance("OnJoinChannelEvent");
	state.pushThing(chatter);
	state.setField(-2, "user");
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
	if(dispatchEvent<OnLeaveChannel::Event>
			(this, state, enviroment, chatter->getListeners(ON_CLOSE_CHANNEL_LISTENER))
		)
		return true;
	return dispatchEvent<OnLeaveChannel::Event>
		(this, state, enviroment, enviroment.Generic.OnLeaveChannel);
}

void OnLeaveChannel::Event::push_instance(LuaState& state, Enviroment& enviroment) {
	state.pushClassTableInstance("OnLeaveChannelEvent");
	state.pushThing(chatter);
	state.setField(-2, "user");
	state.pushChannel(channel);
	state.setField(-2, "channel");
}

void OnLeaveChannel::Event::update_instance(Manager& state, Enviroment& enviroment, LuaThread_ptr thread) {
	// Nothing can change...
}

