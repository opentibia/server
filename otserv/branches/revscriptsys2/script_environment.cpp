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

#include "script_environment.h"
#include "script_listener.h"

using namespace Script;

Environment::Environment() {
	objectID_counter = 0;
}

Environment::~Environment() {
}

void Environment::cleanup() {
	object_map.clear();
	objectID_counter = 0;
	Generic.OnSay.clear();
}

void Environment::cleanupUnusedListeners(ListenerList& list) {
	for(ListenerList::iterator giter = list.begin(),
		gend = list.end();
		giter != gend;)
	{
		if((*giter)->isActive() == false) {
			giter = list.erase(giter);
		} else ++giter;
	}
}

 void Environment::cleanupUnusedListeners() {
	cleanupUnusedListeners(Generic.OnSay);
}

void Environment::registerSpecificListener(Listener_ptr listener) {
	specific_listeners[listener->getID()] = listener;
}

bool Environment::stopListener(ListenerList& list, uint32_t id) {
	for(ListenerList::iterator giter = list.begin(),
		gend = list.end(); giter != gend;
		++giter)
	{
		if((*giter)->getID() == id && (*giter)->isActive()) {
			(*giter)->deactivate();
			return true;
		}
	}
	return false;
}

bool Environment::stopListener(ListenerStringMap& list, uint32_t id) {

	for(ListenerStringMap::iterator giter = list.begin(),
		gend = list.end(); giter != gend;
		++giter)
	{
		stopListener(giter->second, id);
	}
	return false;
}

bool Environment::stopListener(ListenerItemMap& item_map, uint32_t id) {

	Script::ListenerMap::iterator list_iter;
	for(list_iter = item_map.ItemId.begin(); list_iter != item_map.ItemId.end(); ++list_iter){
		if(stopListener(list_iter->second, id)){
			return true;
		}
	}

	for(list_iter = item_map.ActionId.begin(); list_iter != item_map.ActionId.end(); ++list_iter){
		if(stopListener(list_iter->second, id)){
			return true;
		}
	}

	return false;
}

bool Environment::stopListener(ListenerType type, uint32_t id) {
	switch(type.value())
	{
		case enums::ON_SAY_LISTENER:
			if(stopListener(Generic.OnSay, id))
				return true;
			break;
		case enums::ON_USE_ITEM_LISTENER:
			if(stopListener(Generic.OnUseItem, id))
				return true;
			break;
		case enums::ON_EQUIP_ITEM_LISTENER:
			if(stopListener(Generic.OnEquipItem, id))
				return true;
			break;
		case enums::ON_MOVE_CREATURE_LISTENER:
			if(stopListener(Generic.OnMoveCreature, id))
				return true;
			break;
		case enums::ON_MOVE_ITEM_LISTENER:
			if(stopListener(Generic.OnMoveItem, id))
				return true;
			break;
		case enums::ON_OPEN_CHANNEL_LISTENER:
			if(stopListener(Generic.OnJoinChannel, id))
				return true;
			break;
		case enums::ON_CLOSE_CHANNEL_LISTENER:
			if(stopListener(Generic.OnLeaveChannel, id))
				return true;
			break;
		case enums::ON_LOGIN_LISTENER:
			if(stopListener(Generic.OnLogin, id))
				return true;
			return false; // No specific listeners
		case enums::ON_LOGOUT_LISTENER:
			if(stopListener(Generic.OnLogout, id))
				return true;
			break;
		case enums::ON_TURN_LISTENER:
			if(stopListener(Generic.OnTurn, id))
				return true;
			break;
		case enums::ON_SPAWN_LISTENER:
			if(stopListener(Generic.OnSpawn, id))
				return true;
			return false; // No specific listeners
		case enums::ON_ADVANCE_LISTENER:
			if(stopListener(Generic.OnAdvance, id))
				return true;
		case enums::ON_ATTACK:
			if(stopListener(Generic.OnAttack, id))
				return true;
		case enums::ON_KILLED_LISTENER:
			if(stopListener(Generic.OnKilled, id))
				return true;
		case enums::ON_KILL_LISTENER:
			if(stopListener(Generic.OnKill, id))
				return true;
		case enums::ON_DEATH_BY_LISTENER:
			if(stopListener(Generic.OnDeathBy, id))
				return true;
		case enums::ON_DEATH_LISTENER:
			if(stopListener(Generic.OnDeath, id))
				return true;
		default:
			break;
	}
	
	// Try specific
	SpecificListenerMap::iterator iter = specific_listeners.find(id);
	if(iter != specific_listeners.end()) {
		Listener_ptr listener = iter->second;
		listener->deactivate();
		return true;
	}

	return false;
}

bool Environment::stopListener(Listener* listener) {
	return stopListener(listener->type(), listener->getID());
}
