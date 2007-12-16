//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
//
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

#ifndef __WAITLIST_H__
#define __WAITLIST_H__

#include "game.h"
#include "networkmessage.h"

struct Wait{
	uint32_t acc;
	uint32_t ip;
	std::string name;
	int64_t timeout;
	int slot;

	Wait(const Player* player, int place){
		name = player->getName();
		acc = player->getAccount();
		ip = player->getIP();
		timeout = OTSYS_TIME();
		slot = place;
	};
};

typedef std::list<Wait*> Waitinglist;
typedef Waitinglist::iterator WaitinglistIterator;

class Waitlist
{
public:
	Waitlist();
	virtual ~Waitlist();

	static Waitlist* instance();

	bool clientLogin(const Player* player);
	int32_t getClientSlot(const Player* player);
	int32_t getTime(int32_t slot);

	OTSYS_THREAD_LOCKVAR waitListLock;

protected:
	Waitinglist waitList;
	WaitinglistIterator findClient(const Player* player);
	void cleanUpList();

private:
	static Waitlist* _Wait;
};
#endif
