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
#include <iostream>

Map::Map() {
    // generating some standard map.
    // yes I could have done that better ...
    for (unsigned short y = MINY; y < MAXY; y++)
        for (unsigned short x = MINX; x < MAXX; x++) {
            tiles[y-MINY][x-MINX] = new Tile;
            tiles[y-MINY][x-MINX]->push_back(new Item(ItemType::WATER));
        }
    std::cout << "region watered." << std::endl;
    for (unsigned short y = MINY + 20; y <= MAXY - 20; y++)
        for (unsigned short x = MINX + 20; x <= MAXX - 20; x++) {
            delete tiles[y-MINY][x-MINX]->back();
            tiles[y-MINY][x-MINX]->pop_back();
            tiles[y-MINY][x-MINX]->push_back(new Item(ItemType::GRASS));
        }
    std::cout << "created land." << std::endl;
}

Map::Map(char *filename) {
    // load the map from a file
}

Tile *Map::tile(unsigned short _x, unsigned short _y, unsigned char _z) {
    return tiles[_y - MINY][_x - MINX];
}

Map::~Map() {

}
