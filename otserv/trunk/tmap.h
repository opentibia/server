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
// Revision 1.10  2003/10/17 22:25:02  tliffrag
// Addes SorryNotPossible; added configfile; basic lua support
//
// Revision 1.9  2003/09/25 21:17:52  timmit
// Adding PlayerList in TMap and getID().  Not workigng!
//
// Revision 1.8  2003/09/23 20:00:51  tliffrag
// added !g command
//
// Revision 1.7  2003/09/23 16:41:19  tliffrag
// Fixed several map bugs
//
// Revision 1.6  2003/09/17 16:35:08  tliffrag
// added !d command and fixed lag on windows
//
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

enum tmapEnum{
	TMAP_SUCCESS,
	TMAP_ERROR,
	TMAP_ERROR_NO_COUNT,
	TMAP_ERROR_TILE_OCCUPIED,
};


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
  int getStackPosPlayer();
  int getStackPosItem();
  bool isBlocking();
  std::string getDescription();
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
		Creature* getPlayerByID( unsigned long id );
		int removeCreature(position pos);
		int requestAction(Creature* c, Action* a);

		int summonItem(Action* a);
		int summonItem(position pos,int id );
		int changeGround(position pos, int id);

		int saveMap();
		int removeItem(position pos);
        Tile *tile(unsigned short _x, unsigned short _y, unsigned char _z);
		std::list <Creature*> PlayerList;
        ~Map();
};

#endif
