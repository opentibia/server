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
// Revision 1.6  2003/08/25 21:28:12  tliffrag
// Fixed all warnings.
//
// Revision 1.5  2003/05/19 16:48:37  tliffrag
// Loggingin, talking, walking around, logging out working
//
// Revision 1.4  2002/05/28 13:55:57  shivoc
// some minor changes
//
// Revision 1.3  2002/04/08 15:57:03  shivoc
// made some changes to be more ansi compliant
//
// Revision 1.2  2002/04/08 14:12:49  shivoc
// fixed a segfault because of out of bounds access to tiles array
//
// Revision 1.1  2002/04/08 13:53:59  acrimon
// Added some very basic map support
//
//////////////////////////////////////////////////////////////////////

#include "items.h"
#include "tmap.h"
#include "player.h"
#include <iostream>

Map::Map() {
    // generating some standard map.
    // yes I could have done that better ...
    for (unsigned short y = MINY; y < MAXY; y++)
        for (unsigned short x = MINX; x < MAXX; x++) {
            tiles[x-MINX][y-MINY] = new Tile;
            tiles[x-MINX][y-MINY]->push_back(new Item(ItemType::WATER));
        }
    std::cout << "region watered." << std::endl;
    for (unsigned short x = MINX + 20; x <= MAXX - 20; x++)
        for (unsigned short y = MINY + 20; y <= MAXY - 20; y++) {
            delete tiles[x-MINX][y-MINY]->back();
            tiles[x-MINX][y-MINY]->pop_back();
            tiles[x-MINX][y-MINY]->push_back(new Item(ItemType::GRASS));
        }
    std::cout << "created land." << std::endl;
}

Map::Map(char *filename) {
    // load the map from a file
}

Tile *Map::tile(unsigned short _x, unsigned short _y, unsigned char _z) {
    return tiles[_x - MINX][_y - MINY];
}

Map::~Map() {

}

position Map::placeCreature(position pos, Creature* c){
  if( tiles[pos.x-MINX][pos.y-MINY]->creature){
  	//crap we need to find another spot
	pos.x++;
	return placeCreature(pos, c);
  }
  tiles[pos.x-MINX][pos.y-MINY]->creature=c;
  //we placed the creature, now tell everbody who saw it about the login
  Action* a = new Action;
  a->type=ACTION_LOGIN;
  a->pos1=pos;
  a->creature=c;
  distributeAction(pos, a);
  return pos;
}

int Map::removeCreature(position pos, Creature* c){
	if(tiles[pos.x-MINX][pos.y-MINY]->creature==c)
		tiles[pos.x-MINX][pos.y-MINY]->creature=0;
	//now distribute the action
	Action* a= new Action;
	a->type=ACTION_LOGOUT;
	a->pos1=pos;
	distributeAction(pos, a);
	delete a;
	return true;
}

int Map::requestAction(Creature* c, Action* a){
	if(a->type==ACTION_MOVE){
		a->pos2=a->pos1;
		if(a->direction%2){
			int x=a->direction-2;
			a->pos2.x-=x;
		}
		else{
			int y=a->direction-1;
			a->pos2.y+=y;
		}
		//we got the new action, now distribute it
		//FIXME THIS IS A BUG!!!
			std::cout << "Going to distribute an action" << std::endl;
		//we move the pointer
		tiles[a->pos2.x-MINX][a->pos2.y-MINY]->creature=tiles[a->pos1.x-MINX][a->pos1.y-MINY]->creature;
		tiles[a->pos1.x-MINX][a->pos1.y-MINY]->creature=NULL;
		a->creature=c;
		distributeAction(a->pos1, a);
	}
	if(a->type==ACTION_SAY){
		//says should be ok most of the time
		distributeAction(a->pos1, a);
	}
	return true;
}

int Map::distributeAction(position pos, Action* a){
	//finds out how saw a certain action and tells him about it
	//the region in which we can be seen is
	// -9/-7 to 9/7
	std::list<Creature*> victims;
	for(int x=-7; x <=7; x++){
		for(int y=-6; y <=6; y++){
			if(tiles[a->pos1.x+x-MINX][a->pos1.y+y-MINY]->creature){
				victims.push_back(tiles[a->pos1.x+x-MINX][a->pos1.y+y-MINY]->creature);
			}
		}
	}
	for(int x=-7; x <=7; x++){
		for(int y=-6; y <=6; y++){
			if(tiles[a->pos2.x+x-MINX][a->pos2.y+y-MINY]->creature){
				victims.push_back(tiles[a->pos2.x+x-MINX][a->pos2.y+y-MINY]->creature);
			}
		}
	}
	victims.sort();
	victims.unique();
	std::list<Creature*>::iterator i;
	for(i=victims.begin(); i!=victims.end();++i)
		(*i)->sendAction(a);
	delete a;
	return true;
}


