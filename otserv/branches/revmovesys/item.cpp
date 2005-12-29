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
#include "container.h"
#include "depot.h"
#include "teleport.h"
#include "actions.h"
#include "magic.h"

#include <iostream>
#include <sstream>
#include <iomanip>

Item* Item::CreateItem(const unsigned short _type, unsigned short _count /*= 0*/)
{
	Item* newItem = NULL;

	if(_type == 2589 || _type == 2590 || _type == 2591 || _type == 2592){
		newItem = new Depot(_type);
	}
	else if(items[_type].isContainer()){
		newItem = new Container(_type);
	}
	else if(items[_type].isTeleport()){		
		newItem = new Teleport(_type);
	}
	else if(items[_type].isMagicField()){
		newItem = new Item(_type, _count);
	}	
	else{
		newItem = new Item(_type, _count);
	}
	
	newItem->useThing2();
	return newItem;
}

Item::Item(const unsigned short _type, unsigned short _count)
{
	//std::cout << "Item constructor2 " << this << std::endl;
	id = _type;
	chargecount = 0;
	fluid = 0;
	actionId = 0;
	uniqueId = 0;
	isDecaying = false;
	specialDescription = NULL;
	text = NULL;
	setItemCountOrSubtype(_count);
	count = std::max((unsigned char)1, count);
}

Item::Item()
{
	//std::cout << "Item constructor3 " << this << std::endl;
	id = 0;
	count = 1;
	chargecount = 0;
	isDecaying  = false;
	actionId = 0;
	uniqueId = 0;
	specialDescription = NULL;
	text = NULL;
}
Item::Item(const unsigned short _type) {
	//std::cout << "Item constructor1 " << this << std::endl;
	id = _type;
	count = 1;	
	chargecount = 0;
	fluid = 0;
	actionId = 0;
	uniqueId = 0;
	isDecaying = false;
	specialDescription = NULL;
	text = NULL;
}

Item::Item(const Item &i){
	//std::cout << "Item copy constructor " << this << std::endl;
	id = i.id;
	count = i.count;
	chargecount = i.chargecount;
	isDecaying  = false;
	actionId = i.actionId;
	uniqueId = i.uniqueId;
	if(i.specialDescription != NULL){
		specialDescription = new std::string(*(i.specialDescription));
	}
	else{
		specialDescription = NULL;
	}
	if(i.text != NULL){
		text = new std::string(*(i.text));
	}
	else{
		text = NULL;
	}	
}

Item::~Item()
{
	//std::cout << "Item destructor " << this << std::endl;
	if(specialDescription)
		delete specialDescription;
	if(text)
		delete text;
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

unsigned short Item::getItemCount() const
{
	return count;
}

//////////////////////////////////////////////////
// return how many items are stacked or 0 if non stackable
unsigned short Item::getItemCountOrSubtype() const {
	if(isStackable()) {
		return count;
	}
	else if(isFluidContainer() || isSplash())
		return fluid;
	//else if(chargecount != 0)
	else if(items[id].runeMagLevel != -1)
		return chargecount;
	else
		return 0;
}

void Item::setItemCountOrSubtype(unsigned char n)
{
	if(isFluidContainer() || isSplash())
		fluid = n;
	else if(items[id].runeMagLevel != -1)
		chargecount = n;
	else{
		if(n > 100)
			count = 100;
		else
			count = n;
	}
};

void Item::setActionId(unsigned short n){
	if(n < 100)
		n = 100;
	actionId = n;
}

unsigned short Item::getActionId() const{
	return actionId;
}

void Item::setUniqueId(unsigned short n){
	//uniqueId only can be set 1 time
	if(uniqueId != 0)
		return;
	 if(n < 1000)
	 	n = 1000;
	uniqueId = n;
	ActionScript::AddThingToMapUnique(this);
}

unsigned short Item::getUniqueId() const{
	return uniqueId;
}

long Item::getDecayTime(){
	return items[id].decayTime*1000;
}

bool Item::rotate()
{
	if(items[id].rotable && items[id].rotateTo){
		id = items[id].rotateTo;
		return true;
	}
	return false;
}

int Item::unserialize(xmlNodePtr p)
{
	char *tmp;
	tmp = (char*)xmlGetProp(p, (const xmlChar *) "id");
	if(tmp){
		id = atoi(tmp);
		xmlFreeOTSERV(tmp);
	}
	
	tmp = (char*)xmlGetProp(p, (const xmlChar *) "special_description");
	if(tmp){
		specialDescription = new std::string(tmp);
		xmlFreeOTSERV(tmp);
	}
		
	tmp = (char*)xmlGetProp(p, (const xmlChar *) "text");
	if(tmp){
		text = new std::string(tmp);
		xmlFreeOTSERV(tmp);
	}
	
	tmp = (char*)xmlGetProp(p, (const xmlChar *) "count");
	if(tmp){
		setItemCountOrSubtype(atoi(tmp));
		xmlFreeOTSERV(tmp);
	}
		
	tmp = (char*)xmlGetProp(p, (const xmlChar *) "actionId");
	if(tmp){
		setActionId(atoi(tmp));
		xmlFreeOTSERV(tmp);
	}
	
	tmp = (char*)xmlGetProp(p, (const xmlChar *) "uniqueId");
	if(tmp){
		setUniqueId(atoi(tmp));
		xmlFreeOTSERV(tmp);
	}
	
	return 0;
}

xmlNodePtr Item::serialize()
{
	std::stringstream s;
	xmlNodePtr ret;
	ret = xmlNewNode(NULL,(const xmlChar*)"item");
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
	if(getItemCountOrSubtype() != 0){
		s << getItemCountOrSubtype();
		xmlSetProp(ret, (const xmlChar*)"count", (const xmlChar*)s.str().c_str());
	}
	
	s.str("");
	if(actionId != 0){
		s << actionId;
		xmlSetProp(ret, (const xmlChar*)"actionId", (const xmlChar*)s.str().c_str());
	}
	
	s.str("");	
	if(uniqueId != 0){
		s << uniqueId;
		xmlSetProp(ret, (const xmlChar*)"uniqueId", (const xmlChar*)s.str().c_str());
	}
	return ret;
}

bool Item::hasProperty(enum ITEMPROPERTY prop) const
{
	const ItemType& it = items[id];

	switch(prop){
		case BLOCKSOLID:
			if(it.blockSolid)
				return true;
		break;

		case NOTMOVEABLEBLOCKSOLID:
			if(it.blockSolid && !it.moveable)
				return true;
		break;

		case BLOCKPROJECTILE:
			if(it.blockProjectile)
				return true;
		break;

		case BLOCKPATHFIND:
			if(it.blockPathFind)
				return true;
		break;

		case NOTMOVEABLEBLOCKPATHFIND:
			if(it.blockPathFind && !it.moveable)
				return true;
		break;
	}

	return false;
}

bool Item::isBlocking() const {
	const ItemType& it = items[id];
	return it.blockSolid;
}

bool Item::isStackable() const {
	return items[id].stackable;
}

bool Item::isRune() const {
	return (items[id].group == ITEM_GROUP_RUNE);
}

bool Item::isFluidContainer() const {
	return (items[id].isFluidContainer());
}

bool Item::isAlwaysOnTop() const {
	return items[id].alwaysOnTop;
}

bool Item::isNotMoveable() const {
	return !items[id].moveable;
}

bool Item::isGroundTile() const {
	return items[id].isGroundTile();
}

bool Item::isSplash() const{
	return items[id].isSplash();
}

bool Item::isMagicField() const{
	return items[id].isMagicField();
}

bool Item::isPickupable() const {
	return items[id].pickupable;
}

bool Item::isUseable() const{
	return items[id].useable;
}

bool Item::isHangable() const{
	return items[id].isHangable;
}

bool Item::floorChangeDown() const {
	return items[id].floorChangeDown;
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
  //now also returns true on SHIELDS!!! Check back with getWeaponType!
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

double Item::getWeight() const {
	if(isStackable()){
		return items[id].weight * std::max(1, (int)count);
	}

	return items[id].weight;
}

std::string Item::getDescription(int32_t lookDistance) const
{
	std::stringstream s;
	std::string str;
	const Container* container;
	const ItemType& it = items[id];

	if(specialDescription){
		s << (*specialDescription) << ".";

		if(lookDistance <= 1) {
			if(it.weight > 0)
				s << std::endl << "It weighs " << std::fixed << std::setprecision(1) << it.weight << " oz.";
		}
	}
	else if (it.name.length()) {
		if(isStackable() && count > 1) {
			s << (int)count << " " << it.name << "s.";

			if(lookDistance <= 1) {
				s << std::endl << "They weight " << std::fixed << std::setprecision(1) << ((double) count * it.weight) << " oz.";
			}
		}		
		else {
			if(items[id].runeMagLevel != -1)
			{
				s << "a spell rune for level " << it.runeMagLevel << "." << std::endl;

				s << "It's an \"" << it.name << "\" spell (";
				if(getItemCharge())
					s << (int)getItemCharge();
				else
					s << "1";
				s << "x)";
			}
			else if(isWeapon() && (getAttack() || getDefense()))
			{
				s << "a " << it.name << " (Atk:" << (int)getAttack() << " Def:" << (int)getDefense() << ")";
			}
			else if(getArmor())
			{
				s << "a " << it.name << " (Arm:"<< (int)getArmor() << ")";
			}
			else if(isFluidContainer()){
				s << "a " << it.name;
				if(fluid == 0){
					s << ". It is empty";
				}
				else{
					s << " of " << items[fluid].name;
				}
			}
			else if(isSplash()){				
				s << "a " << it.name << " of ";
				if(fluid == 0){
					s << items[1].name;
				}
				else{
					s << items[fluid].name;
				}
			}
			else if(it.isKey()){
				s << "a " << it.name << " (Key:" << actionId << ")";
			}
			else if(it.isGroundTile()) //groundtile
			{
				s << it.name;
			}
			else if(it.isContainer() && (container = dynamic_cast<const Container*>(this))) {
				s << "a " << it.name << " (Vol:" << container->capacity() << ")";
			}
			else {
				s << "a " << it.name;
			}
			s << ".";
			if(lookDistance <= 1) {
				double weight = getWeight();
				if(weight > 0)
					s << std::endl << "It weighs " << std::fixed << std::setprecision(1) << weight << " oz.";
				
				if(items[id].description.length())
				{
					s << std::endl << items[id].description;
				}
			}
		}
	}
	else
		s<<"an item of type " << id <<".";
	
	str = s.str();
	return str;
}

std::string Item::getName() const
{
	return items[id].name;
}

void Item::setSpecialDescription(std::string desc){
	if(specialDescription){
		delete specialDescription;
		specialDescription = NULL;
	}
	if(desc.length() > 1)
		specialDescription = new std::string(desc);	
}

std::string Item::getSpecialDescription()
{
	if(!specialDescription)
		return std::string("");
	return *specialDescription;
}

void Item::clearSpecialDescription(){
	if(specialDescription)
		delete specialDescription;
	specialDescription = NULL;
}

void Item::setText(std::string desc){
	if(text){
		delete text;
		text = NULL;
	}
	if(desc.length() > 1){
		text = new std::string(desc);	
		if(items[id].readOnlyId != 0){//write 1 time
			id = items[id].readOnlyId;
		}
	}
}

void Item::clearText(){
	if(text)
		delete text;
	text = NULL;
}

std::string Item::getText()
{
	if(!text)
		return std::string("");
	return *text;
}

int Item::getRWInfo() const {
	return items[id].RWInfo;
}

bool Item::canDecay(){
	if(isRemoved())
		return false;

	return items[id].canDecay;	
}

int Item::getWorth() const
{
	switch(getID()){
	case ITEM_COINS_GOLD:
		return getItemCount();
	case ITEM_COINS_PLATINUM:
		return getItemCount() * 100;
	case ITEM_COINS_CRYSTAL:
		return getItemCount() * 10000;
	default:
		return 0;
	}
}
