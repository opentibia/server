//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// the map of OpenTibia
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
// $Id$
//////////////////////////////////////////////////////////////////////
// $Log$
// Revision 1.5  2003/09/08 13:28:41  tliffrag
// Item summoning and maploading/saving now working.
//
// Revision 1.4  2003/05/19 16:48:37  tliffrag
// Loggingin, talking, walking around, logging out working
//
// Revision 1.3  2002/05/28 13:55:57  shivoc
// some minor changes
//
// Revision 1.2  2002/04/08 15:57:03  shivoc
// made some changes to be more ansi compliant
//
// Revision 1.1  2002/04/08 13:53:59  acrimon
// Added some very basic map support
//
//////////////////////////////////////////////////////////////////////

#ifndef __OTSERV_MAP_H
#define __OTSERV_MAP_H
#include "pos.h"
#include "action.h"
#include "item.h"
#include "creature.h"


//////////////////////////////////////////////////
// a Tile represents a single field on the map.
// it is a list of Items.
class Creature; //see creature.h
class Tile : public std::list<Item *> {
 public:
  Creature* creature;
 public:
  Creature* getCreature(){
    return creature;
  }

  Tile(){
   creature = NULL;
  }

};


class Map {
    // should use an Space Partitioning Tree.
    // I am using a very simple array now though.
    public:
        static const unsigned short MINX = 32832;
        static const unsigned short MINY = 32832;
        static const unsigned short MAXX = 32960;
        static const unsigned short MAXY = 32960;

    private:
		int distributeAction(position pos, Action* a);
        Tile *tiles[MAXX - MINX][MAXY - MINY];

    public:
        Map();
        Map(char *filename);
		position placeCreature(position pos, Creature* c);
		int removeCreature(position pos, Creature* c);
		int requestAction(Creature* c, Action* a);
		int summonItem(position pos, int id);
        Tile *tile(unsigned short _x, unsigned short _y, unsigned char _z);
        ~Map();
};

#endif
