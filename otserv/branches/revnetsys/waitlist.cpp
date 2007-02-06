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

WaitinglistIterator Waitlist::findClient(int acc, unsigned long ip)
{
	int slot = 1;
	WaitinglistIterator it;
	for(it = waitList.begin(); it != waitList.end();) {
		if((*it)->acc == acc && (*it)->ip == ip){
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

void Waitlist::addClient(int acc, unsigned long ip)
{
	WaitinglistIterator it = findClient(acc, ip);
	
	if(it == waitList.end()){
		Wait* wait = new Wait(acc, ip, waitList.size()+1);
		waitList.push_back(wait);
	}
}

int Waitlist::getClientSlot(int acc, unsigned long ip)
{
	OTSYS_THREAD_LOCK_CLASS lockClass(waitListLock, "Waitlist::getClientSlot()");
	
	WaitinglistIterator it = findClient(acc, ip);
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

bool Waitlist::clientLogin(int acc, unsigned long ip)
{		
	OTSYS_THREAD_LOCK_CLASS lockClass(waitListLock, "Waitlist::clientLogin()");

	Status* stat = Status::instance();	
	
	if(!stat->hasSlot()){
		addClient(acc, ip);
		return false;
	}
	else{
		cleanUpList();
		/**
		For example: Client is in slot 3, maximum is 50 and its 48 online, 
		then its not this clients turn to sign in...
		But if its 47 online, let it sign in!
		**/
		WaitinglistIterator it = findClient(acc, ip);
		
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

void Waitlist::createMessage(NetworkMessage& msg, int acc, unsigned long ip)
{
	int slot = getClientSlot(acc, ip);
	std::stringstream tmp;
	
	tmp << "Too many players online.\n";
	tmp << "You are at place " << slot;
	tmp << " on the waiting list.";
	
	msg.AddByte(0x16);
	msg.AddString(tmp.str());
	msg.AddByte(getTime(slot)); //number of seconds before retry
	
	tmp.str("");
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
