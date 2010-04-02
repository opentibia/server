//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Waiting list for connecting clients
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

#include <iostream>
#include <sstream>

#include "waitlist.h"
#include "status.h"
#include "configmanager.h"

extern ConfigManager g_config;

WaitingList::WaitingList()
{
	//
}

WaitingList::~WaitingList()
{
	waitList.clear();
}

WaitListIterator WaitingList::findClient(const Player* player, uint32_t& slot)
{
	slot = 1;
	for(WaitListIterator it = waitList.begin(); it != waitList.end(); ++it){
		if((*it)->acc == player->getAccountId() && (*it)->ip == player->getIP() &&
			strcasecmp((*it)->name.c_str(), player->getName().c_str()) == 0){
				return it;
		}

		++slot;
	}

	return waitList.end();
}

int32_t WaitingList::getTime(int32_t slot)
{
	if(slot < 5){
		return 5;
	}
	else if(slot < 10){
		return 10;
	}
	else if(slot < 20){
		return 20;
	}
	else if(slot < 50){
		return 60;
	}
	else{
		return 120;
	}
}

int32_t WaitingList::getTimeOut(int32_t slot)
{
	//timeout is set to 15 seconds longer than expected retry attempt
	return getTime(slot) + 15;
}

bool WaitingList::clientLogin(const Player* player)
{
	if(player->hasFlag(PlayerFlag_CanAlwaysLogin)){
		return true;
	}

	if(waitList.empty() && Status::instance()->getPlayersOnline() < g_config.getNumber(ConfigManager::MAX_PLAYERS)){
		//no waiting list and enough room
		return true;
	}

	cleanUpList();

	uint32_t slot;
	WaitListIterator it = findClient(player, slot);
	if(it != waitList.end()){
		if((Status::instance()->getPlayersOnline() + slot) <= g_config.getNumber(ConfigManager::MAX_PLAYERS)){
			//should be able to login now
#ifdef __DEBUG__WATINGLIST__
			std::cout << "Name: " << (*it)->name << " can now login" << std::endl;
#endif
			delete *it;
			waitList.erase(it);
			return true;
		}
		else{
			//let them wait a bit longer
			(*it)->timeout = OTSYS_TIME() + getTimeOut(slot) * 1000;
			return false;
		}
	}

	Wait* wait = new Wait();

	if(player->isPremium()){
		slot = 1;
		for(WaitListIterator it = waitList.begin(); it != waitList.end(); ++it){
			if(!(*it)->premium){
				waitList.insert(it, wait);
				break;
			}

			++slot;
		}
	}
	else{
		waitList.push_back(wait);
		slot = (uint32_t)waitList.size();
	}

	wait->name = player->getName();
	wait->acc = player->getAccountId();
	wait->ip = player->getIP();
	wait->premium = player->isPremium();
	wait->timeout = OTSYS_TIME() + getTimeOut(slot) * 1000;

#ifdef __DEBUG__WATINGLIST__
	std::cout << "Name: " << player->getName() << "(" << waitList.size() + 1 << ")" << " has been added to the waiting list" << std::endl;
#endif

	return false;
}

int32_t WaitingList::getClientSlot(const Player* player)
{
	uint32_t slot;
	WaitListIterator it = findClient(player, slot);
	if(it != waitList.end()){
		return slot;
	}

	#ifdef __DEBUG__WATINGLIST__
	std::cout << "WaitingList::getSlot error, trying to find slot for unknown acc: " << player->getAccountId() <<
	" with ip " << player->getIP() << std::endl;
	#endif

	return -1;
}

void WaitingList::cleanUpList()
{
	uint32_t slot = 1;
	for(WaitListIterator it = waitList.begin(); it != waitList.end();){
		if((*it)->timeout - OTSYS_TIME() <= 0){
#ifdef __DEBUG__WATINGLIST__
			std::cout << "Name: " << (*it)->name << " has timed out!" << std::endl;
#endif
			delete *it;
			waitList.erase(it++);
		}
		else{
			++slot;
			++it;
		}
	}
}
