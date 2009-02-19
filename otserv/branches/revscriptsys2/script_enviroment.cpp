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

#include "script_enviroment.h"
#include "script_listener.h"

using namespace Script;

Enviroment::Enviroment() {
	objectID_counter = 0;
}

Enviroment::~Enviroment() {
}

void Enviroment::cleanup() {
	object_map.clear();
	objectID_counter = 0;
	Generic.OnSay.clear();
}

void Enviroment::cleanupUnusedListeners(ListenerList& list) {
	for(ListenerList::iterator giter = list.begin(),
		gend = list.end();
		giter != gend;)
	{
		if((*giter)->isActive() == false) {
			giter = list.erase(giter);
		} else ++giter;
	}
}

 void Enviroment::cleanupUnusedListeners() {
	cleanupUnusedListeners(Generic.OnSay);
}

void Enviroment::registerSpecificListener(Listener_ptr listener) {
	specific_listeners[listener->getID()] = listener;
}

bool Enviroment::stopListener(ListenerList& list, uint32_t id) {
	for(ListenerList::iterator giter = list.begin(),
		gend = list.end();
		giter != gend;
		++giter)
	{
		if((*giter)->getID() == id && (*giter)->isActive()) {
			(*giter)->deactivate();
			return true;
		}
	}
	return false;
}

bool Enviroment::stopListener(ListenerType type, uint32_t id) {
	switch(type)
	{
		case ON_USE_ITEM_LISTENER:
			if(stopListener(Generic.OnUseItem, id))
				return true;
			break;
		case ON_SAY_LISTENER:
			if(stopListener(Generic.OnSay, id))
				return true;
			break;
		case ON_OPEN_CHANNEL_LISTENER:
			if(stopListener(Generic.OnJoinChannel, id))
				return true;
			break;
		case ON_CLOSE_CHANNEL_LISTENER:
			if(stopListener(Generic.OnLeaveChannel, id))
				return true;
			break;
		case ON_EQUIP_ITEM_LISTENER:
			if(stopListener(Generic.OnEquipItem, id))
				return true;
			break;
		case ON_MOVE_CREATURE_LISTENER:
			if(stopListener(Generic.OnMoveCreature, id))
				return true;
			break;
		case ON_TURN_LISTENER:
			if(stopListener(Generic.OnTurn, id))
				return true;
			break;
		case ON_MOVE_ITEM_LISTENER:
			if(stopListener(Generic.OnMoveItem, id))
				return true;
			break;
		case ON_LOGIN_LISTENER:
			if(stopListener(Generic.OnLogin, id))
				return true;
			return false; // No specific listeners
		case ON_LOGOUT_LISTENER:
			if(stopListener(Generic.OnLogout, id))
				return true;
			break;
		default:
			break;
	}

	// Try specific
	ListenerMap::iterator iter = specific_listeners.find(id);
	if(iter != specific_listeners.end()) {
		Listener_ptr listener = iter->second;
		listener->deactivate();
		return true;
	}

	return false;
}

bool Enviroment::stopListener(Listener* listener) {
	return stopListener(listener->type(), listener->getID());
}
