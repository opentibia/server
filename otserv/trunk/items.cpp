//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// The database of items.
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

bool ItemType::isContainer() {
    return iscontainer;
}
    
ItemType::ItemType() {
}

ItemType::ItemType(unsigned short _id, unsigned short _tibiaid, string _name) {
    id = _id;
    tibiaid = _tibiaid;
    name = _name;
}

ItemType::~ItemType() {

}

Items::Items() {
    // add a few items
    unsigned short id;
    id = ItemType::WATER; items[id] = ItemType(id, 0x0E00, "water");
    id = ItemType::GRASS; items[id] = ItemType(id, 0x0A00, "grass");
}

Items::~Items() {
    items.clear();
}

