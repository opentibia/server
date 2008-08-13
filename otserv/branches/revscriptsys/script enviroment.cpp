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

#include "script enviroment.h"
#include "script listener.h"

using namespace Script;

Enviroment::Enviroment() {
}

Enviroment::~Enviroment() {
}

void Enviroment::cleanup() {
	object_map.clear();
	objectID_counter = 0;
	Specific.OnSay.clear();
	Generic.OnSay.clear();
}

void Enviroment::cleanupUnusedListeners(GenericCreatureEventList& list) {
	for(GenericCreatureEventList::iterator giter = list.begin(),
		gend = list.end();
		giter != gend;)
	{
		if((*giter)->isActive() == false) {
			giter = list.erase(giter);
		} else ++giter;
	}
}

void Enviroment::cleanupUnusedListeners(SpecificCreatureEventMap& map) {
	for(SpecificCreatureEventMap::iterator siter = map.begin(),
		send = map.end();
		siter != send;
		++siter)
	{
		cleanupUnusedListeners(siter->second);
	}
}

void Enviroment::cleanupUnusedListeners() {
	cleanupUnusedListeners(Specific.OnSay);
	cleanupUnusedListeners(Generic.OnSay);
}

bool Enviroment::stopListener(GenericCreatureEventList& list, const std::string& tag) {
	for(GenericCreatureEventList::iterator giter = list.begin(),
		gend = list.end();
		giter != gend;
		++giter)
	{
		if((*giter)->getLuaTag() == tag && (*giter)->isActive()) {
			(*giter)->deactive();
			return true;
		}
	}
	return false;
}

bool Enviroment::stopListener(SpecificCreatureEventMap& map, const std::string& tag) {
	for(SpecificCreatureEventMap::iterator siter = map.begin(),
		send = map.end();
		siter != send;
		++siter)
	{
		GenericCreatureEventList& li = siter->second;
		if(stopListener(li, tag))
			return true;
	}
	return false;
}

bool Enviroment::stopListener(const std::string& tag) {
	size_t _pos = tag.find("_") + 1;
	if(_pos == std::string::npos) {
		return false;
	}
	size_t _2pos = tag.find("_", _pos);
	if(_2pos == std::string::npos) {
		return false;
	}
	std::string type = tag.substr(_pos, _2pos - _pos);

	bool found = false;
	if(type == "OnSay") {
		found =
			stopListener(Specific.OnSay, tag) ||
			stopListener(Generic.OnSay, tag);
	}

	return found;
}

bool Enviroment::stopListener(Script::Listener* listener) {
	return stopListener(listener->getLuaTag());
}