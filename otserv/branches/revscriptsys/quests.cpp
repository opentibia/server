//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Quests
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

#include "quests.h"
#include "tools.h"

MissionState::MissionState(std::string _description, uint32_t _missionID)
{
	description = _description;
 	missionID = _missionID;
}

Mission::Mission(std::string _missionName, uint32_t _storageID, uint32_t _startValue, int32_t _endValue)
{
	missionName = _missionName;
	endValue = _endValue;
	startValue = _startValue;
	storageID = _storageID;
}

Mission::~Mission()
{
	for(uint32_t it = 0; it != state.size(); it++){
		delete state[it];
	}
	state.clear();
}

std::string Mission::getDescription(Player* player)
{
	uint32_t current = endValue;
	uint32_t value;
	while(current >= startValue){
		player->getStorageValue(storageID, (int32_t&)value);
		if(value == current){
			StateList::const_iterator it2;
			it2 = state.find(current);
			if(it2 != state.end())
				return it2->second->getMissionDescription();
		}
		current--;
	}
	return "An error has occurred, please contact a gamemaster.";
}

bool Mission::isStarted(Player* player) const
{
	uint32_t value;
	if(player){
		player->getStorageValue(storageID, (int32_t&)value);
		if(value >= startValue && value <= endValue){
			return true;
		}
	}
	return false;
}

bool Mission::isCompleted(Player* player) const
{
	uint32_t value;
	if(player){
		player->getStorageValue(storageID, (int32_t&)value);
		if(value == endValue){
			return true;
		}
	}
	return false;
}

std::string Mission::getName(Player* player)
{
	if(isCompleted(player)){
		return missionName + " (completed)";
	}
	else{
		return missionName;
	}
}

Quest::Quest(std::string _name, uint16_t _id, uint32_t _startStorageID, uint32_t _startStorageValue)
{
	name = _name;
	id = _id;
	startStorageID = _startStorageID;
	startStorageValue = _startStorageValue;
}

Quest::~Quest()
{
	MissionsList::iterator it;
	for(it = missions.begin(); it != missions.end(); it++){
		delete (*it);
	}
	missions.clear();
}

uint16_t Quest::getMissionsCount(Player* player) const
{
	uint16_t count = 0;
	for(MissionsList::const_iterator it = missions.begin(); it != missions.end(); it++){
		if((*it)->isStarted(player)){
			count++;
		}
	}
	return count;
}

bool Quest::isCompleted(Player* player)
{
	MissionsList::iterator it;
	for(it = missions.begin(); it != missions.end(); it++){
		if(!(*it)->isCompleted(player))
			return false;
	}
	return true;
}

bool Quest::isStarted(Player* player) const
{
	uint32_t value;
	if(player){
		player->getStorageValue(startStorageID, (int32_t&)value);
		if(value >= startStorageValue)
			return true;
	}
	return false;
}
