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
// Revision 1.1  2002/04/08 13:53:59  acrimon
// Added some very basic map support
//
//////////////////////////////////////////////////////////////////////

#include "items.h"
#include "tmap.h"

Map::Map() {
    // generating some standard map.
    // yes I could have done that better ...
    for (unsigned short y = MINY; y <= MAXY; y++)
	for (unsigned short x = MINX; x <= MAXX; x++) {
	    tiles[y-MINY][x-MINX] = new Tile;
	    tiles[y-MINY][x-MINX]->push_back(new Item(ItemType::WATER));
	}
    cout << "region watered." << endl;
    for (unsigned short y = MINY + 20; y <= MAXY - 20; y++)
	for (unsigned short x = MINX + 20; x <= MAXX - 20; x++) {
	    tiles[y-MINY][x-MINX]->clear(); // creates memory leak
	    tiles[y-MINY][x-MINX]->push_back(new Item(ItemType::GRASS));
	}
    cout << "created land." << endl;
}

Map::Map(char *filename) {
    // load the map from a file
}

Tile *Map::tile(unsigned short _x, unsigned short _y, unsigned char _z) {
    return tiles[_y - MINY][_x - MINX];
}

Map::~Map() {

}
