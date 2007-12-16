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

Waitlist* Waitlist::_Wait = NULL;

Waitlist::Waitlist()
{
	OTSYS_THREAD_LOCKVARINIT(waitListLock);
	waitList.clear();
}

Waitlist::~Waitlist()
{
	waitList.clear();
}

Waitlist* Waitlist::instance(){
	if(_Wait == NULL)
		_Wait = new Waitlist();
	
	return _Wait;
}

WaitinglistIterator Waitlist::findClient(const Player* player)
{
	for(WaitinglistIterator it = waitList.begin(); it != waitList.end();){
		if( (*it)->acc == player->getAccount() && (*it)->ip == player->getIP() &&
		strcasecmp((*it)->name.c_str(), player->getName().c_str()) == 0){
			(*it)->timeout = OTSYS_TIME();	//update timeout
			return it;
		}
		else{
			++it;
		}
	}
	
	return waitList.end();
}

int32_t Waitlist::getClientSlot(const Player* player)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(waitListLock, "Waitlist::getClientSlot()");
	
	WaitinglistIterator it = findClient(player);
	if(it != waitList.end()){
		return (*it)->slot;
	}
	else{
		#ifdef __DEBUG__WATINGLIST__
		std::cout << "WaitList::getSlot error, trying to find slot for unknown acc: " << acc << 
		" with ip " << ip << std::endl;
		#endif
		return -1;
	}
}

int32_t Waitlist::getTime(int32_t slot)
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
		return 240;
	}
}

bool Waitlist::clientLogin(const Player* player)
{		
	OTSYS_THREAD_LOCK_CLASS lockClass(waitListLock, "Waitlist::clientLogin()");
	
	if(waitList.empty() && Status::instance()->getPlayersOnline() < Status::instance()->getMaxPlayersOnline()){
		//no waiting list and enough room
		return true;
	}

	cleanUpList();

	WaitinglistIterator it = findClient(player);	
	if(it != waitList.end()){
		if((Status::instance()->getPlayersOnline() + (*it)->slot) <= Status::instance()->getMaxPlayersOnline()){ 
			//should be able to login now
			delete *it;
			waitList.erase(it);
			return true;
		}
		else{
			//let them wait a bit longer
			return false;
		}
	}

	Wait* wait = new Wait(player, waitList.size() + 1);
	waitList.push_back(wait);
	return false;
}

void Waitlist::cleanUpList()
{
	uint32_t newSlot = 1;
	for(WaitinglistIterator it = waitList.begin(); it != waitList.end();){
		(*it)->slot = newSlot;

		if((OTSYS_TIME() - (*it)->timeout) > getTime((*it)->slot)*1.5*1000){
			delete *it;
			waitList.erase(it++);
		}
		else{
			++newSlot;
			++it;
		}
	}	
}
