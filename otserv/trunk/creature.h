//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// base class for every creature
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


#ifndef __creature_h
#define __creature_h

// include definitions
#include "pos.h"
#include "action.h"
#include "item.h"
#include "definitions.h"
#include "tmap.h"

enum playerLooks{
	PLAYER_MALE_1=0x80,
	PLAYER_MALE_2=0x81,
	PLAYER_MALE_3=0x82,
	PLAYER_MALE_4=0x83,
	PLAYER_FEMALE_1=0x88,
	PLAYER_FEMALE_2=0x89,
	PLAYER_FEMALE_3=0x8A,
	PLAYER_FEMALE_4=0x8B,
};

// include Map headers


//////////////////////////////////////////////////////////////////////
// Defines the Base class for all creatures and base functions which 
// every creature has
class Map;

class Creature {

    public:

        // set creature on map position...
		virtual void setMap(position, Map&) throw(texception)=0;

		virtual position getPosition() = 0;

		virtual bool isPlayer(){return false;}

		virtual unsigned long getID(){return id;}

		virtual void sendAction(Action*){}

		virtual int sendInventory(){return 0;};

		virtual int tick(double time)=0;

		virtual int addAction(Action*)=0;

		virtual int clearActionQueue()=0;

		virtual std::string getLook()=0;
		virtual std::string getName()=0;

		virtual int addItem(Item* item, int pos){return 0;};

		virtual Item* getItem(int pos){return NULL;}

        virtual ~Creature() {};
		int id;

		virtual void drainHealth(int){return;};
		virtual int getHealth(){return 100;};
};

#endif // __creature_h
