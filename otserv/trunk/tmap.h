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

#include "item.h"

//////////////////////////////////////////////////
// a Tile represents a single field on the map.
// it is a list of Items.
class Tile : public std::list<Item *> {};


class Map {
    // should use an Space Partitioning Tree.
    // I am using a very simple array now though.
    public:
        static const unsigned short MINX = 0x8000;
        static const unsigned short MINY = 0x8000;
        static const unsigned short MAXX = 0x8100;
        static const unsigned short MAXY = 0x8100;

    private:
        Tile *tiles[MAXY - MINY][MAXX - MINX];

    public:
        Map();
        Map(char *filename);
        Tile *tile(unsigned short _x, unsigned short _y, unsigned char _z);
        ~Map();
};

//////////////////////////////////////////////////
// represents a map position
// for now just a 3d point
struct position {
    unsigned short x,y,z;

    // for now we just initialise the position to a startpoint
    position() : x(Map::MINX+(Map::MAXX-Map::MINX)/2),
    y(Map::MINY+(Map::MAXY-Map::MINY)/2), z(7) { };

    position(unsigned short _x, unsigned short _y, unsigned short _z)
        : x(_x), y(_y), z(_z) {};

};

#endif
