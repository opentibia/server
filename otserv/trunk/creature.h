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
#include "definitions.h"

// include Map headers
#include "tmap.h"

//////////////////////////////////////////////////////////////////////
// Defines the Base class for all creatures and base functions which 
// every creature has
class Map;

class Creature {

    public:

        // set creature on map position...
        virtual void setMap(position, Map&) throw(texception)=0;

		virtual bool isPlayer(){return false;}

		virtual void sendAction(Action*){}
        // get the next Action of the creature
        //			virtual action* GetNextAction()=0;

        // Do the action
        //			virtual int DoAction(action*, Map*, Creature*)=0;

        // get information about the Creature
        // the information which is displayed if you click with both buttons on a creature in tibia
        //			virtual infotyp* GetInfo()=0;

        //			virtual pos GetPosition()=0;

        // returns 0 if the creature is not valid anymore or 1 if it is valid
        //			virtual int isValid()=0;

        // if changes on the map occured near the creature 
        // (like person moved, item moved...)
        // this method will get called
        //			virtual void UpdateMap(int, action*, unsigned long, char*, Creature*, Map*)=0;

        // update a single tile on the map
        //			virtual void UpdateTile(pos, unsigned)=0;

        // Show the creature...
        //			virtual void ShowCreature(int*, char**)=0;

        // virtual destructor to be overloaded...
        virtual ~Creature() {};
		int id;

};

#endif // __creature_h
