//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Item represents an existing item.
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
// Revision 1.6  2003/10/21 17:55:07  tliffrag
// Added items on player
//
// Revision 1.5  2003/10/19 21:32:19  tliffrag
// Reworked the Tile class; stackable items now working
//
// Revision 1.4  2003/10/17 22:25:02  tliffrag
// Addes SorryNotPossible; added configfile; basic lua support
//
// Revision 1.3  2002/05/28 13:55:56  shivoc
// some minor changes
//
// Revision 1.2  2002/04/08 13:53:59  acrimon
// Added some very basic map support
//
//////////////////////////////////////////////////////////////////////

// include header file
#include "item.h"
#include <iostream>
#include <sstream>

//////////////////////////////////////////////////
// returns the ID of this item's ItemType
unsigned Item::getID() {
    return id;
}

//////////////////////////////////////////////////
// return how many items are stacked or 0 if non stackable
unsigned short Item::getItemCount() {
    return count;
}


Item::Item(const unsigned short _type) {
    id = _type;
    count = 0;
}


Item::Item() {
    id = 0;
    count = 0;
}

Item::~Item() {
    lcontained.clear();
}


//////////////////////////////////////////////////
// add an item to (this) container if possible
void Item::addItem(Item *newitem) {
    //first check if we are a container, there is an item to be added and if we can add more items...
    //if (!iscontainer) throw TE_NoContainer();
    if (newitem == NULL) throw TE_BadItem();
    //if (maxitems <=actualitems) throw TE_ContainerFull();

    // seems we should add the item...
    // new items just get placed in front of the items we already have...
    lcontained.push_front(newitem);

    // increase the itemcount
    actualitems++;
}

//////////////////////////////////////////////////
// returns iterator to itemlist
Item::iterator Item::getItems() {
    return lcontained.begin();
}

//////////////////////////////////////////////////
// return iterator to one beyond the last item
Item::iterator Item::getEnd() {
    return lcontained.end();
}

bool Item::isBlocking() {
	return items.items[id]->blocking;
}

bool Item::isStackable() {
	return items.items[id]->stackable;
}

bool Item::isAlwaysOnTop() {
	return items.items[id]->alwaysOnTop;
}

bool Item::isAlwaysOnBottom() {
	return items.items[id]->alwaysOnBottom;
}

bool Item::isGroundTile() {
	return items.items[id]->groundtile;
	//return true;
}

std::string Item::getDescription() {
	std::stringstream s;
	std::string str;
	str = s.str();
	return str;
	//return true;
}

//////////////////////////////////////////////////
// add item into the container
Item& Item::operator<<(Item* toAdd) {
    addItem(toAdd);
    return *this;
}
