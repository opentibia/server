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

// include header file

#include "definitions.h"
#include "item.h"
#include <iostream>
#include <sstream>


//////////////////////////////////////////////////
// returns the ID of this item's ItemType
unsigned short Item::getID() const {
    return id;
}

//////////////////////////////////////////////////
// sets the ID of this item's ItemType
void Item::setID(unsigned short newid) {
    id = newid;
}

//////////////////////////////////////////////////
// return how many items are stacked or 0 if non stackable
unsigned char Item::getItemCountOrSubtype() const {
    return count;
}


Item::Item(const unsigned short _type) {
    id = _type;
    count = 0;
		chargecount = 0;
		maxitems = 20;
		actualitems = 0;

    throwRange = 6;
}

Item::Item(const unsigned short _type, unsigned char _count) {
    id = _type;
		count = 0;
		chargecount = 0;

		if(isStackable() || isMultiType())
			count = _count;
		else
			chargecount = _count;

    throwRange = 6;
}

Item::Item() {
    id = 0;
    count = 0;
		chargecount = 0;
		maxitems = 20;
		actualitems = 0;

    throwRange = 6;
}

Item::~Item() {
	for(Item::iterator cit = lcontained.begin(); cit != lcontained.end(); cit++)
	{
    delete (*cit);
  }
    
	lcontained.clear();
}

int Item::unserialize(xmlNodePtr p){
	id=atoi((const char*)xmlGetProp(p, (const xmlChar *) "id"));
	const char* tmp=(const char*)xmlGetProp(p, (const xmlChar *) "count");
	if(tmp)
		count=atoi(tmp);
	return 0;
}
xmlNodePtr Item::serialize(){
	std::stringstream s;
	xmlNodePtr ret;
	ret=xmlNewNode(NULL,(const xmlChar*)"item");
	s.str(""); //empty the stringstream
	s << getID();
	xmlSetProp(ret, (const xmlChar*)"id", (const xmlChar*)s.str().c_str());
	if(isStackable()){
		s.str(""); //empty the stringstream
		s <<count;
		xmlSetProp(ret, (const xmlChar*)"count", (const xmlChar*)s.str().c_str());
	}
	return ret;
}

//////////////////////////////////////////////////
// add an item to (this) container if possible
void Item::addItem(Item *newitem) {
    //first check if we are a container, there is an item to be added and if we can add more items...
    //if (!iscontainer) throw TE_NoContainer();
    if (newitem == NULL)
      return;
    //if (maxitems <=actualitems) throw TE_ContainerFull();

    // seems we should add the item...
    // new items just get placed in front of the items we already have...
		if(lcontained.size() < maxitems) {
			lcontained.push_front(newitem);

			// increase the itemcount
			actualitems++;
		}
}

void Item::removeItem(Item* item)
{
	for (std::list<Item*>::iterator cit = lcontained.begin(); cit != lcontained.end(); cit++) {
		if((*cit) == item) {
			lcontained.erase(cit);
			actualitems--;
			break;
		}
	}
}

void Item::isContainerHolding(Item* item, bool& found)
{
	if(found || item == NULL)
		return;

	for (std::list<Item*>::iterator cit = lcontained.begin(); cit != lcontained.end(); cit++) {
		if((*cit)->isContainer()) {

			if((*cit) == item) {
				found = true;
				break;
			}
			else
				return (*cit)->isContainerHolding(item, found);
		}
	}
}

void Item::moveItem(unsigned char from_slot, unsigned char to_slot)
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

Item* Item::getItem(unsigned long slot_num)
{
	int n = 0;			
	for (Item::iterator cit = getItems(); cit != getEnd(); cit++) {
		if(n == slot_num)
			return *cit;
		else
			n++;
	}

	return NULL;
}

unsigned char Item::getSlotNumberByItem(Item* item)
{
	unsigned char n = 0;			
	for (Item::iterator cit = getItems(); cit != getEnd(); cit++) {
		if(*cit == item)
			return n;
		else
			n++;
	}

	return 0xFF;
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

bool Item::isBlocking() const {
	return items[id].blocking;
}

bool Item::isStackable() const {
	return items[id].stackable;
}

bool Item::isMultiType() const {
	return items[id].multitype;
}

bool Item::isAlwaysOnTop() const {
	return items[id].alwaysOnTop;
}

bool Item::isNotMoveable() const {
	return items[id].notMoveable;
}

bool Item::isGroundTile() const {
	return items[id].groundtile;
}

bool Item::isContainer() const {
	return items[id].iscontainer;
}

bool Item::isWeapon() const
{ 
  //now also returns true on SHIELDS!!! Check back with getWeponType!
  //old: return (items[id].weaponType != NONE && items[id].weaponType != SHIELD && items[id].weaponType != AMO);
  return (items[id].weaponType != NONE && items[id].weaponType != AMO);
}


WeaponType Item::getWeaponType() const {
		  return items[id].weaponType;
}

std::string Item::getDescription() {
	std::stringstream s;
	std::string str;
  if (items[id].name.length())
    s << "You see a " << items[id].name << ".";
  else
	  s<<"You see an item of type " << id <<".";

	if(isStackable())
		s<<"These are "<< count << " pieces.";

  s << "\nIt weights " << items[id].weight/10 << " oz.";

	str = s.str();
	return str;
}

std::string Item::getName()
{
	return items[id].name;
}

//////////////////////////////////////////////////
// add item into the container
Item& Item::operator<<(Item* toAdd) {
    addItem(toAdd);
    return *this;
}
