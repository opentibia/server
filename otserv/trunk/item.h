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


#ifndef __OTSERV_ITEM_H
#define __OTSERV_ITEM_H

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <iostream>
#include <list>
#include <vector>

#include "texcept.h"

#include "thing.h"
#include "items.h"



class Creature;


class Item : public Thing
{
    private:
        unsigned id;  // the same id as in ItemType
	      unsigned char count; // number of stacked items
				unsigned char chargecount; //number of charges on the item

    public:
		static Item* CreateItem(const unsigned short _type, unsigned char _count = 0); //Factory member to create item of right type based on id
		static Items items;

   unsigned short getID() const;    // ID as in ItemType
   void setID(unsigned short newid);
		
    bool isWeapon() const;
		WeaponType getWeaponType() const;

		bool isBlockingProjectile() const;
	  bool isBlocking() const;

		bool isStackable() const;
    bool isMultiType() const;
		bool isAlwaysOnTop() const;
		bool isGroundTile() const;
		bool isNotMoveable() const;
		//bool isContainer() const;
		bool noFloorChange() const;
		bool floorChangeNorth() const;
		bool floorChangeSouth() const;
		bool floorChangeEast() const;
		bool floorChangeWest() const;

		int use(){std::cout << "use " << id << std::endl; return 0;};
		int use(Item*){std::cout << "use with item ptr " << id << std::endl; return 0;};
		int use(Creature*){std::cout << "use with creature ptr " << id << std::endl; return 0;};
		std::string getDescription();
		std::string getName();
		int unserialize(xmlNodePtr p);
		xmlNodePtr serialize();

    // get the number of items
    unsigned char getItemCountOrSubtype() const;
		void setItemCountOrSubtype(unsigned char n) {count = n;};

		unsigned char getItemCharge() const {return chargecount;};
		void setItemCharge(unsigned char n) {chargecount = n;};

    // Constructor for items
    Item(const unsigned short _type);
    Item(const unsigned short _type, unsigned char _count);
		Item();
    virtual ~Item();
};


#endif
