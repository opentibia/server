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
#include "magic.h"

#include <iostream>
#include <sstream>
#include <iomanip>

Item* Item::CreateItem(const unsigned short _type, unsigned char _count /*= 0*/)
{
	if(items[_type].iscontainer)
		return new Container(_type);
	else if(items[_type].isteleport)
		return new Teleport(_type);
	else if(items[_type].ismagicfield){		
		return new Item(_type, _count);
	}
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
	if(isStackable() ) {
		return count;
	}
	else if(isFluidContainer() || isMultiType())
		return fluid;
	else if(chargecount != 0)
		return chargecount;	
	else
		return 0;
}


Item::Item(const unsigned short _type) {
	id = _type;
	count = 0;
	chargecount = 0;
	fluid = 0;
	throwRange = 6;
	useCount = 0;
	specialDescription = NULL;
	text = NULL;
}

Item* Item::tranform()
{
	unsigned short decayTo   = Item::items[getID()].decayTo;
	//unsigned short decayTime = Item::items[getID()].decayTime;
	
	if(decayTo == 0) {
		return 0;
	}
	
	Item *item = Item::CreateItem(decayTo,getItemCountOrSubtype());
	item->pos = this->pos;
	//move items if they both are containers
	Container *containerfrom = dynamic_cast<Container*>(this);
	Container *containerto = dynamic_cast<Container*>(item);
	if(containerto && containerfrom){
		std::vector<Item*> itemlist;
		for (ContainerList::const_iterator cit = containerfrom->getItems(); cit != containerfrom->getEnd(); ++cit) {
			itemlist.push_back(*cit);							
		}
		for(std::vector<Item*>::reverse_iterator it = itemlist.rbegin(); it != itemlist.rend(); ++it){
			containerfrom->removeItem(*it);
			containerto->addItem(*it);
		}						
	}

	return item;
}

Item::Item(const unsigned short _type, unsigned char _count) {
	id = _type;
	count = 0;
	chargecount = 0;
	fluid = 0;
	useCount = 0;
	specialDescription = NULL;
	text = NULL;

	if(isStackable()){
		if(_count == 0){
			count = 1;
		}
		else if(_count > 100){
			count = 100;
		}
		else{
			count = _count;
		}
	}
	else if(isFluidContainer() || isMultiType() )
		fluid = _count;
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
	useCount = 0;
	specialDescription = NULL;
	text = NULL;
}

Item::~Item()
{
	if(specialDescription)
		delete specialDescription;
	if(text)
		delete text;
}

int Item::unserialize(xmlNodePtr p){
	id=atoi((const char*)xmlGetProp(p, (const xmlChar *) "id"));
	
	const char* tmp=(const char*)xmlGetProp(p, (const xmlChar *) "special_description");
	if(tmp)
		specialDescription = new std::string(tmp);
		
	tmp=(const char*)xmlGetProp(p, (const xmlChar *) "text");
	if(tmp)
		text = new std::string(tmp);
	
	tmp=(const char*)xmlGetProp(p, (const xmlChar *) "count");
	if(tmp && isStackable() )
		count=atoi(tmp);
	else if(tmp && (isFluidContainer() || isMultiType()))
		fluid=atoi(tmp);
	else if(tmp)
		chargecount=atoi(tmp);	
	
	return 0;
}

xmlNodePtr Item::serialize(){
	std::stringstream s;
	xmlNodePtr ret;
	ret=xmlNewNode(NULL,(const xmlChar*)"item");
	s.str(""); //empty the stringstream
	s << getID();
	xmlSetProp(ret, (const xmlChar*)"id", (const xmlChar*)s.str().c_str());
	
	if(specialDescription){
		s.str(""); //empty the stringstream
		s << *specialDescription;
		xmlSetProp(ret, (const xmlChar*)"special_description", (const xmlChar*)s.str().c_str());
	}
	
	if(text){
		s.str(""); //empty the stringstream
		s << *text;
		xmlSetProp(ret, (const xmlChar*)"text", (const xmlChar*)s.str().c_str());
	}
	
	s.str(""); //empty the stringstream
	if(isStackable()){		
		s << (int)count;
		xmlSetProp(ret, (const xmlChar*)"count", (const xmlChar*)s.str().c_str());
	}
	else if(isFluidContainer() || isMultiType()){
		s << (int)fluid;
		xmlSetProp(ret, (const xmlChar*)"count", (const xmlChar*)s.str().c_str());
	}
	else if(getItemCharge() > 0){		
		s << (int)getItemCharge();
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

bool Item::isFluidContainer() const {
	return items[id].fluidcontainer;
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

amu_t Item::getAmuType() const{
	 return items[id].amuType;
}

subfight_t Item::getSubfightType() const {
	return items[id].shootType;
}

int Item::getAttack() const {
	  return items[id].attack;
}

int Item::getArmor() const {
	  return items[id].armor;
}

int Item::getDefense() const {
	  return items[id].defence;
}

int Item::getSlotPosition() const {
	return items[id].slot_position;
}

std::string Item::getDescription() const
{
	std::stringstream s;
	std::string str;
	if(specialDescription){
		s << "You see " << (*specialDescription) << ".";
		if(items[id].weight > 0)
				s << "It weighs " << std::fixed << std::setprecision(1) << items[id].weight << " oz.";
	}
	else if (items[id].name.length()) {
		if(isStackable() && count > 1) {
			s<<"You see "<< (int)count << " " << items[id].name << "s" << "." << std::endl;
			s << "They weight " << std::fixed << std::setprecision(1) << ((double) count * items[id].weight) << " oz.";
		}		
		else {
			if(items[id].runeMagLevel != -1)
			{
				s << "You see a spell rune for level " << items[id].runeMagLevel << "." << std::endl;
				s << "It's an \"" << items[id].name << "\" spell (";
				if(getItemCharge())
					s << (int)getItemCharge();
				else
					s << "1";
				s << "x)." << std::endl;
			}
			else if(isWeapon() && (getAttack() || getDefense()))
			{
				if(getAttack() != 0){					
					s << "You see a " << items[id].name << " (Atk:" << (int)getAttack() << " Def:" << (int)getDefense() << ")." << std::endl;					
				}
				else{
					s << "You see a " << items[id].name << " (Def:" << (int)getDefense() << ")." << std::endl;
				}				
			}
			else if(getArmor())
			{
				s << "You see a " << items[id].name << " (Arm:"<< (int)getArmor() << ")." << std::endl;
			}
			else if(isFluidContainer()){
				s << "You see a " << items[id].name;
				if(fluid == 0){
					s << ". It is empty.";
				}
				else{
					s << " of " << items[fluid].name << ".";
				}
			}
			else if(isMultiType()){				
				s << "You see a " << items[id].name << " of ";
				if(fluid == 0){
					s << items[1].name;
				}
				else{
					s << items[fluid].name;
				}
				s << ".";								
			}
			else
			{
				s << "You see a " << items[id].name << "." << std::endl;
			}
			
			if(items[id].weight > 0)
				s << "It weighs " << std::fixed << std::setprecision(1) << items[id].weight << " oz.";
			
			if(items[id].description.length())
			{
				s << std::endl << items[id].description;
			}
		}
	}
	else
		s<<"You see an item of type " << id <<".";
	
	str = s.str();
	return str;
}

std::string Item::getName() const
{
	return items[id].name;
}

void Item::setSpecialDescription(std::string desc){
	specialDescription = new std::string(desc);	
}

void Item::clearSpecialDescription(){
	delete specialDescription;
	specialDescription = NULL;
}

Teleport::Teleport(const unsigned short _type) : Item(_type)
{
	useCount = 0;	
	destPos.x = 0;
	destPos.y = 0;
	destPos.z = 0;
}

Teleport::~Teleport()
{
}

int Teleport::unserialize(xmlNodePtr p)
{
	Item::unserialize(p);

	destPos.x = atoi((const char*)xmlGetProp(p, (const xmlChar *) "destx"));
	destPos.y = atoi((const char*)xmlGetProp(p, (const xmlChar *) "desty"));
	destPos.z = atoi((const char*)xmlGetProp(p, (const xmlChar *) "destz"));

	return 0;
}

xmlNodePtr Teleport::serialize()
{
	xmlNodePtr xmlptr = Item::serialize();

	std::stringstream s;

	s.str(""); //empty the stringstream
	s << (int) destPos.x;
	xmlSetProp(xmlptr, (const xmlChar*)"destx", (const xmlChar*)s.str().c_str());

	s.str(""); //empty the stringstream
	s << (int) destPos.y;
	xmlSetProp(xmlptr, (const xmlChar*)"desty", (const xmlChar*)s.str().c_str());

	s.str(""); //empty the stringstream
	s << (int)destPos.z;
	xmlSetProp(xmlptr, (const xmlChar*)"destz", (const xmlChar*)s.str().c_str());

	return xmlptr;
}



