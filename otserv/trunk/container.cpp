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

#include "container.h"

Container::Container(const unsigned short _type) : Item(_type)
{
	maxitems = 20;
	actualitems = 0;
}

Container::~Container()
{
	for(Container::iterator cit = lcontained.begin(); cit != lcontained.end(); cit++)
	{
    delete (*cit);
  }
    
	lcontained.clear();
}

bool Container::addItem(Item *newitem) {
	//first check if we are a container, there is an item to be added and if we can add more items...
	//if (!iscontainer) throw TE_NoContainer();
	if (newitem == NULL)
		return false;

	// seems we should add the item...
	// new items just get placed in front of the items we already have...
	if(lcontained.size() < maxitems) {
		lcontained.push_front(newitem);

		// increase the itemcount
		actualitems++;
		return true;
	}

	return false;
}

bool Container::removeItem(Item* item)
{
	for (std::list<Item*>::iterator cit = lcontained.begin(); cit != lcontained.end(); cit++) {
		if((*cit) == item) {
			lcontained.erase(cit);
			actualitems--;
			return true;
		}
	}

	return false;
}

void Container::isContainerHolding(Item* item, bool& found)
{
	if(found || item == NULL)
		return;

	for (std::list<Item*>::iterator cit = lcontained.begin(); cit != lcontained.end(); cit++) {
		Container *container = dynamic_cast<Container*>(*cit);
		if(container) {

			if(container == item) {
				found = true;
				break;
			}
			else
				return container->isContainerHolding(item, found);
		}
	}
}

void Container::moveItem(unsigned char from_slot, unsigned char to_slot)
{
	int n = 0;
	for (std::list<Item*>::iterator cit = lcontained.begin(); cit != lcontained.end(); cit++) {
		if(n == from_slot) {
			Item *item = (*cit);
			lcontained.erase(cit);
			lcontained.push_front(item);
			break;
		}
		n++;
	}
}

Item* Container::getItem(unsigned long slot_num)
{
	int n = 0;			
	for (Container::iterator cit = getItems(); cit != getEnd(); cit++) {
		if(n == slot_num)
			return *cit;
		else
			n++;
	}

	return NULL;
}

unsigned char Container::getSlotNumberByItem(Item* item)
{
	unsigned char n = 0;			
	for (Container::iterator cit = getItems(); cit != getEnd(); cit++) {
		if(*cit == item)
			return n;
		else
			n++;
	}

	return 0xFF;
}

//////////////////////////////////////////////////
// returns iterator to itemlist
Container::iterator Container::getItems() {
	return lcontained.begin();
}

//////////////////////////////////////////////////
// return iterator to one beyond the last item
Container::iterator Container::getEnd() {
	return lcontained.end();
}

/*
//////////////////////////////////////////////////
// add item into the container
Item& Item::operator<<(Item* toAdd) {
    addItem(toAdd);
    return *this;
}
*/

