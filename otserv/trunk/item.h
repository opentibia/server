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
// Revision 1.8  2004/11/18 20:32:15  shivoc
// fix libxml2 include paths
//
// Revision 1.7  2003/11/05 23:28:23  tliffrag
// Addex XML for players, outfits working
//
// Revision 1.6  2003/11/01 15:58:52  tliffrag
// Added XML for players and map
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

#ifndef __OTSERV_ITEM_H
#define __OTSERV_ITEM_H

#include <iostream>
#include <list>
#include <vector>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include "texcept.h"
#include "items.h"


#ifndef __creature_h
class Creature;
#endif

class Item {
    private:
        unsigned id;  // the same id as in ItemType

    private: // the following, I will have to rethink:
        // could be union:

        unsigned short actualitems; // number of items in container
        // list of items if this is a container
        std::list<Item *> lcontained;
		static Items items;

    public:
	 unsigned short count; // number of stacked items
        unsigned getID();    // ID as in ItemType
		bool isBlocking();
		bool isStackable();
		bool isAlwaysOnTop();
		bool isGroundTile();
		bool isAlwaysOnBottom();
		int use(){std::cout << "use " << id << std::endl; return 0;};
		int use(Item*){std::cout << "use with item ptr " << id << std::endl; return 0;};
		int use(Creature*){std::cout << "use with creature ptr " << id << std::endl; return 0;};
		std::string getDescription();
		int unserialize(xmlNodePtr p);
		xmlNodePtr serialize();

        // get the number of items or 0 if non stackable
        unsigned short getItemCount();

        // Constructor for items
        Item(const unsigned short _type);
		Item();

        ~Item();

        // definition for iterator over backpack itemsfclose(f);
        typedef std::list<Item *>::const_iterator iterator;
        iterator getItems();     // begin();
        iterator getEnd();       // iterator beyond the last element
        void addItem(Item*);     // add an item to the container
        Item& operator<<(Item*); // put items into the container
};

// now we declare exceptions we throw...
class TE_Nocontainer : public texception {
    public:
        TE_Nocontainer() : texception("Item is not a container!", false) {}
};

class TE_BadItem : public texception {
    public:
        TE_BadItem() : texception("Item is invalid!", false) {}
};

class TE_ContainerFull : public texception {
    public:
        TE_ContainerFull() : texception("container is full!", false) {}
};

#endif
