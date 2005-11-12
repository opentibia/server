//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
//
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

#ifndef __OTSERV_CONTAINER_H
#define __OTSERV_CONTAINER_H

#include "item.h"

typedef std::list<Item *> ContainerList;

class Container : public Item
{
	private:
		int useCount;
		Container *parent;
		unsigned short maxitems; //number of max items in container  
		unsigned short actualitems; // number of items in container
		ContainerList lcontained;

	public:
		Container(const unsigned short _type);
		virtual ~Container();
		virtual void useThing() {
			//std::cout << "Container: useThing() " << this << std::endl;
			useCount++;
		};
	
		virtual void releaseThing() {
			useCount--;
			//std::cout << "Container: releaseThing() " << this << std::endl;
			//if (useCount == 0)
			if (useCount <= 0)
				delete this;
		};

		unsigned long depot;
		int size() const {return actualitems;};
		int capacity() const {return maxitems;};
		void setParent(Container* container) {parent = container;};
		Container *getParent() {return parent;}
		Container *getParent() const {return parent;}
		Container *getTopParent();
		const Container *getTopParent() const;

		ContainerList::const_iterator getItems() const;     // begin();
		ContainerList::const_iterator getEnd() const;       // iterator beyond the last element
		bool addItem(Item* newitem);     // add an item to the container
		bool removeItem(Item* item); //remove an item from the container
		void moveItem(unsigned char from_slot, unsigned char to_slot);
		Item* getItem(unsigned long slot_num);
		const Item* getItem(unsigned long slot_num) const;
		unsigned char getSlotNumberByItem(const Item* item) const;
		bool isHoldingItem(const Item* item) const;
		long getItemHoldingCount() const;
		virtual double getWeight() const;
};

#endif //__OTSERV_CONTAINER_H

