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
#include "container.h"

#include <iostream>
#include <sstream>
#include <iomanip>

Item* Item::CreateItem(const unsigned short _type, unsigned char _count /*= 0*/)
{
	if(items[_type].iscontainer)
		return new Container(_type);
	else
		return new Item(_type, _count);
}

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
	if(isStackable() || isMultiType()) {
		return count;
	}
	else if(chargecount != 0)
		return chargecount;
	else
		return 0;
}


Item::Item(const unsigned short _type) {
	id = _type;
	count = 0;
	chargecount = 0;

	throwRange = 6;
}

Item* Item::tranform()
{
	unsigned short decayTo   = Item::items[getID()].decayTo;
	//unsigned short decayTime = Item::items[getID()].decayTime;
	
	if(decayTo == 0) {
		return 0;
	}

	Item *item = Item::CreateItem(decayTo);
	item->pos = this->pos;
	return item;
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

Item::Item()
{
	id = 0;
	count = 0;
	chargecount = 0;
		
	throwRange = 6;
}

Item::~Item()
{
	//
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
		s << (int)count;
		xmlSetProp(ret, (const xmlChar*)"count", (const xmlChar*)s.str().c_str());
	}
	return ret;
}

bool Item::isBlockingProjectile() const {
	return items[id].blockingProjectile;
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

bool Item::isPickupable() const {
	return items[id].pickupable;
}

bool Item::noFloorChange() const {
	return items[id].noFloorChange;
}

bool Item::floorChangeNorth() const {
	return items[id].floorChangeNorth;
}
bool Item::floorChangeSouth() const {
	return items[id].floorChangeSouth;
}
bool Item::floorChangeEast() const {
	return items[id].floorChangeEast;
}
bool Item::floorChangeWest() const {
	return items[id].floorChangeWest;
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

std::string Item::getDescription() const
{
	std::stringstream s;
	std::string str;
	if (items[id].name.length()) {
		if(isStackable() && count > 1) {
			s<<"You see "<< (int)count << " " << items[id].name << "s" << "." << std::endl;
			s << "They weight " << std::fixed << std::setprecision(1) << ((double) count * items[id].weight) << " oz.";
		}
		else {
			s << "You see a " << items[id].name << "." << std::endl;
			s << "It weighs " << std::fixed << std::setprecision(1) << items[id].weight << " oz.";
		}
	}
	else
		s<<"You see an item of type " << id <<".";
	
	if(this->getItemCharge() > 0)
	{
		s << std::endl << "It has " << (int)this->getItemCharge() << " charges left.";
	}
	
	str = s.str();
	return str;
}

std::string Item::getName() const
{
	return items[id].name;
}
