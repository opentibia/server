/* OpenTibia - an opensource roleplaying game
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/****************************************************************
  Implementation of the class Item
 ****************************************************************/

// include header file
#include "item.h"

namespace Items {

/****************************************************************
Method: unsigned Item::GetItemNumber();
--------------------------------

Get the Number of the Item...

 ****************************************************************/


unsigned Item::GetItemNumber()
{
	return itemnumber;
} // unsigned Item::GetItemNumber()

/****************************************************************
  end of unsigned Item::GetItemNumber()
 ****************************************************************/




/****************************************************************
Method: unsigned short Item::GetItemCount();
--------------------------------

return how many items are stacked or 0 if non stackable

 ****************************************************************/

unsigned short Item::GetItemCount()
{
	return itemcount;
} // unsigned short Item::GetItemCount()

/****************************************************************
  end of unsigned short Item::GetItemCount()
 ****************************************************************/



/****************************************************************
Method: Item::Item(unsigned, unsigned short)
--------------------------------------------

create an item

 ****************************************************************/

Item::Item(unsigned num, unsigned short count)
{
	itemnumber=num;
	itemcount=count;
	iscontainer=0;
} // Item::Item(unsigned num, unsigned short count)

/****************************************************************
  end of Item::Item(unsigned, unsigned short)
 ****************************************************************/

Item::~Item() {
	if (lcontained.size()) {
		while(lcontained.size() != 0) {
			delete lcontained.front();
			lcontained.pop_front();
		}
	}
}



/****************************************************************
Method: Item::Item(unsigned, unsigned short,int)
--------------------------------

create an container item

 ****************************************************************/

Item::Item(unsigned num, unsigned short count,int newmaxitems)
{
	itemnumber=num;
	itemcount=count;
	iscontainer=1;
	maxitems=newmaxitems;
	actualitems=0;
} // Item::Item(unsigned num, unsigned short count,int maxitems)

/****************************************************************
  end of Item::Item(unsigned, unsigned short,int)
 ****************************************************************/



/****************************************************************
Method: int Item::isContainer()
--------------------------------

return if the Item is an Container

 ****************************************************************/

int Item::isContainer()
{
	return iscontainer;
} // int Item::isContainer()

/****************************************************************
  end of Item::isContainer()
 ****************************************************************/



/****************************************************************
Method: int Item::addItem(const Item*)
--------------------------------

add an item into the container if possible

 ****************************************************************/

void Item::addItem(Item* newitem) {
	//first check if we are a container, there is an item to be added and if we can add more items...
	if (!iscontainer) throw nocontainer();
	if (newitem==NULL) throw bad_item();
	if (maxitems <=actualitems) throw container_full();

	// seems we should add the item...
	// new items just get placed in front of the items we already have...
	lcontained.push_front(newitem);

	// increase the itemcount
	actualitems++;

} // int Item::addItem(Item* newitem) 

/****************************************************************
  end of Item::addItem(Item*)
 ****************************************************************/



/****************************************************************
Method: Item::iterator Item::getItems() 
----------------------------------

returns iterator to itemlist

 ****************************************************************/

Item::iterator Item::getItems() {
	return lcontained.begin();
} // Item::iterator Item::getItems() 

/****************************************************************
  end of Item::iterator Item::getItems()
 ****************************************************************/

/****************************************************************
Method: Item::iterator Item::getEnd() 
----------------------------------

return iterator to one beyond the last item

 ****************************************************************/
Item::iterator Item::getEnd() {
	return lcontained.end();
} // Item::iterator Item::getEnd() 

/****************************************************************
  end of Item::iterator Item::getEnd()
 ****************************************************************/

/****************************************************************
Method: Item& Item::operator<<(Item&)
----------------------------------

add item into the container

 ****************************************************************/

Item& Item::operator<<(Item* toAdd) {
	addItem(toAdd);
	return *this;
} // Item& Item::operator<<(Item&) 
/****************************************************************
  end of Item::iterator Item::getItems()
 ****************************************************************/

} // namespace Items 
