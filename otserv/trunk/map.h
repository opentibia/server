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


#ifndef __OTSERV_MAP_H
#define __OTSERV_MAP_H


#include <vector>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "position.h"
#include "item.h"
#include "creature.h"

#include "otsystem.h"


#define GETTILEBYPOS(pos) (tiles[pos.x][pos.y])


enum tmapEnum{
	TMAP_SUCCESS,
	TMAP_ERROR,
	TMAP_ERROR_NO_COUNT,
	TMAP_ERROR_TILE_OCCUPIED,
};


//////////////////////////////////////////////////
// a Tile represents a single field on the map.
// it is a list of Items

class Creature; //see creature.h



class Player;

#define MAP_WIDTH  512
#define MAP_HEIGHT 512

class Tile;

class Map {
    // should use an Space Partitioning Tree.
    // I am using a very simple array now though.
    public:

    Tile* tiles[MAP_WIDTH][MAP_HEIGHT];

    public:
        Map();
        Map(char *filename);
        ~Map();



		int loadMapXml(const char *filename);

    Tile *tile(unsigned short _x, unsigned short _y, unsigned char _z);

    std::map<long, Creature*> playersOnline;


  bool placeCreature(Creature* c);
  bool removeCreature(Creature* c);

  void thingMove(Player *player, Thing *thing,
                 unsigned short to_x, unsigned short to_y, unsigned char to_z);

  void thingMove(Player *player,
                 unsigned short from_x, unsigned short from_y, unsigned char from_z,
                 unsigned char stackPos,
                 unsigned short to_x, unsigned short to_y, unsigned char to_z);

  void creatureTurn(Creature *creature, Direction dir);

  void playerSay(Player *player, unsigned char type, const string &text);
  void playerYell(Player *player, const string &text);
  void playerSpeakTo(Player *player, const string &receiver, const string &text);
  void playerBroadcastMessage(Player *player, const string &text);

  
  void addEvent(long ticks, int type, void *data);

protected:
  void creatureMakeDistDamage(Creature *creature, Creature *attackedCreature);

  Creature* getCreatureByID(unsigned long id);

  OTSYS_THREAD_LOCKVAR mapLock;
  OTSYS_THREAD_LOCKVAR eventLock;
  
  static OTSYS_THREAD_RETURN eventThread(void *p);

  struct MapEvent
  {
    __int64  tick;
    int      type;
    void*    data;
  };

  void checkPlayerAttacking(unsigned long id);
  void checkPlayer(unsigned long id);

  list<MapEvent> *eventLists[12000];
};

#endif
