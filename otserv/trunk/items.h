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
// Revision 1.4  2002/05/29 16:07:38  shivoc
// implemented non-creature display for login
//
// Revision 1.3  2002/05/28 13:55:56  shivoc
// some minor changes
//
// Revision 1.2  2002/04/08 15:57:03  shivoc
// made some changes to be more ansi compliant
//
// Revision 1.1  2002/04/08 13:53:59  acrimon
// Added some very basic map support
//
//////////////////////////////////////////////////////////////////////

#ifndef __OTSERV_ITEMS_H
#define __OTSERV_ITEMS_H

#include <hash_map>
#include <string>

class ItemType {
    unsigned short id;
    unsigned short maxitems; // maximum size if this is a container
    unsigned short tibiaid; // the ID in the Tibia protocol
    unsigned short weight; // weight of the item, e.g. throwing distance depends on it
    std::string name; // the name of the item

    // other vars:
    // what items can be used on what other items? pointer to function, think on implementation

    bool iscontainer : 1;
    bool stackable : 1;
    bool groundtile : 1; // is this necessary?
    bool walkable : 1; // people can walk on it
    bool pickupable : 1; // people can pick it up
    // other bools
    public:
    const static unsigned short WATER = 10;
    const static unsigned short GRASS = 0x1c0c;

    bool isContainer(); // return if this item is a Container

    // maybe it should be enough if itemtypes van be defined with constructors only.
    ItemType();
    // some simple constructor:
    ItemType(unsigned short _id, unsigned short _tibiaid, std::string _name);
    ~ItemType();
};

class Items {
    struct eqItemType {
        bool operator() (unsigned short it1, unsigned short it2) const {
            // ...
            return 1;
        }
    };
    typedef std::hash_map<unsigned short, ItemType, std::hash<unsigned short>, eqItemType> ItemHash;
    ItemHash items;
    public:
    Items();
    ~Items();
};

#endif
