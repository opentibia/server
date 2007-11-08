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
	int slot = 1;
	WaitinglistIterator it;
	for(it = waitList.begin(); it != waitList.end();) {
		if((*it)->acc == player->getAccount() && (*it)->ip == player->getIP()){
			(*it)->slot = slot; //update slot
			(*it)->timeout = OTSYS_TIME(); //update timeout
			return it;
		}
		else{
			++it;	
			slot++;
		}
	}
	
	return waitList.end();
}

void Waitlist::addClient(const Player* player)
{
	WaitinglistIterator it = findClient(player);
	
	if(it == waitList.end()){
		Wait* wait = new Wait(player->getAccount(), player->getIP(), waitList.size()+1);
		waitList.push_back(wait);
	}
}

int Waitlist::getClientSlot(const Player* player)
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

bool Waitlist::clientLogin(const Player* player)
{		
	OTSYS_THREAD_LOCK_CLASS lockClass(waitListLock, "Waitlist::clientLogin()");

	Status* stat = Status::instance();	
	
	if(!stat->hasSlot()){
		addClient(player);
		return false;
	}
	else{
		cleanUpList();
		/**
		For example: Client is in slot 3, maximum is 50 and its 48 online, 
		then its not this clients turn to sign in...
		But if its 47 online, let it sign in!
		**/
		WaitinglistIterator it = findClient(player);
		
		if(it != waitList.end()){
			if(((*it)->slot + stat->getPlayersOnline()) <= stat->getMaxPlayersOnline()){ 
				waitList.erase(it); //Should be able to sign in now, so lets erase it
				return true;	
			}
		}
		else{ //Not in queue
			return true;	
		}
	}
	
	return false;
}

void Waitlist::cleanUpList()
{
	for(WaitinglistIterator it = waitList.begin(); it != waitList.end();){
		if((OTSYS_TIME() - (*it)->timeout) > getTime((*it)->slot)*1.5*1000){
			delete *it;
			waitList.erase(it++);
		}
		else
			++it;
	}	
}
