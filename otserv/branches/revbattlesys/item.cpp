//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Represents an item
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

#include "definitions.h"
#include "item.h"
#include "container.h"
#include "depot.h"
#include "teleport.h"
#include "trashholder.h"
#include "mailbox.h"
#include "house.h"
#include "game.h"
#include "luascript.h"

#include "actions.h"
#include "combat.h"

#include <iostream>
#include <sstream>
#include <iomanip>

extern Game g_game;

Item* Item::CreateItem(const unsigned short _type, unsigned short _count /*= 1*/)
{
	Item* newItem = NULL;

	const ItemType& it = Item::items[_type];
	
	if(it.id != 0){
		if(_type == ITEM_LOCKER1 || _type == ITEM_LOCKER2 || _type == ITEM_LOCKER3 || _type == ITEM_LOCKER4){
			newItem = new Depot(_type);
		}
		else if(it.isContainer()){
			newItem = new Container(_type);
		}
		else if(it.isTeleport()){
			newItem = new Teleport(_type);
		}
		else if(it.isMagicField()){
			newItem = new MagicField(_type);
		}
		else if(it.isDoor()){
			newItem = new Door(_type);
		}
		else if(_type == ITEM_DUSTBIN){
			newItem = new TrashHolder(_type /*, NM_ME_PUFF*/);
		}
		else if(_type == ITEM_MAILBOX1 || _type == ITEM_MAILBOX2 || _type == ITEM_MAILBOX3){
			newItem = new Mailbox(_type);
		}
		else{
			newItem = new Item(_type, _count);
		}

		newItem->useThing2();
	}

	return newItem;
}

Item* Item::CreateItem(PropStream& propStream)
{
	unsigned short _id;
	if(!propStream.GET_USHORT(_id)){
		return NULL;
	}

	ItemType iType = Item::items[_id];
	unsigned char _count = 1;

	if(iType.stackable || iType.isSplash() || iType.isFluidContainer()){
		if(!propStream.GET_UCHAR(_count)){
			return NULL;
		}
	}

	return Item::CreateItem(_id, _count);
}

Item::Item(const unsigned short _type, unsigned short _count /*= 0*/) :
	ItemAttributes()
{
	id = _type;
	count = 1;

	const ItemType& it = items[id];

	charges = it.charges;
	fluid = 0;

	if(it.isFluidContainer() || it.isSplash()){
		fluid = _count;
	}
	else if(it.stackable){
		count = _count;
	}
	else if(it.charges != 0){
		charges = _count;
	}

	/*
	setItemCountOrSubtype(_count);

	if(count == 0){
		count = 1;
	}
	*/
}

/*
Item::Item() :
	ItemAttributes()
{
	//std::cout << "Item constructor3 " << this << std::endl;
	id = 0;
	count = 1;
	charges = 0;
	fluid = 0;
}

Item::Item(const unsigned short _type) :
	ItemAttributes()
{
	//std::cout << "Item constructor1 " << this << std::endl;
	id = _type;
	count = 1;	
	charges = 0;
	fluid = 0;
}
*/

Item::Item(const Item &i) :
	ItemAttributes()
{
	//std::cout << "Item copy constructor " << this << std::endl;
	id = i.id;
	count = i.count;
	charges = i.charges;
	fluid = i.fluid;
	
	unsigned short v;
	if(v = i.getActionId())
		setActionId(v);
	
	if(v = i.getUniqueId())
		setUniqueId(v);
	
	if(i.getSpecialDescription() != ""){
		setSpecialDescription(i.getSpecialDescription());
	}
	
	if(i.getText() != ""){
		setText(i.getText());
	}
}

Item::~Item()
{
	//std::cout << "Item destructor " << this << std::endl;
	//
}

unsigned char Item::getItemCountOrSubtype() const
{
	const ItemType& it = items[getID()];

	if(it.isFluidContainer() || it.isSplash()){
		return fluid;
	}
	//else if(items[id].runeMagLevel != -1)
	else if(charges != 0){
		return charges;
	}
	else{
		return count;
	}
}

void Item::setItemCountOrSubtype(unsigned char n)
{
	const ItemType& it = items[id];

	if(it.isFluidContainer() || it.isSplash()){
		fluid = n;
	}
	else if(it.charges != 0){
		charges = n;
	}
	else{
		count = n;
	}

	/*
	if(isFluidContainer() || isSplash())
		fluid = n;
	else if(items[id].runeMagLevel != -1)
		chargecount = n;
	else{
		count = n;
	}
	*/
}

bool Item::hasSubType() const
{
	const ItemType& it = items[id];
	return (it.isFluidContainer() || it.isSplash() || it.stackable || it.runeMagLevel != -1);
}

void Item::setDefaultDuration()
{
	uint32_t duration = getDefaultDuration();

	if(duration != 0){
		setDuration(duration);
	}
}

uint32_t Item::getDefaultDuration() const
{
	return items[id].decayTime * 1000;
}

bool Item::unserialize(xmlNodePtr nodeItem)
{
	char* nodeValue;
	nodeValue = (char*)xmlGetProp(nodeItem, (const xmlChar *) "id");
	if(nodeValue){
		id = atoi(nodeValue);
		xmlFreeOTSERV(nodeValue);
	}
	else
		return false;
	
	nodeValue = (char*)xmlGetProp(nodeItem, (const xmlChar *) "count");
	if(nodeValue){
		setItemCountOrSubtype(atoi(nodeValue));
		xmlFreeOTSERV(nodeValue);
	}

	nodeValue = (char*)xmlGetProp(nodeItem, (const xmlChar *) "special_description");
	if(nodeValue){
		setSpecialDescription(nodeValue);
		xmlFreeOTSERV(nodeValue);
	}
		
	nodeValue = (char*)xmlGetProp(nodeItem, (const xmlChar *) "text");
	if(nodeValue){
		setText(nodeValue);
		xmlFreeOTSERV(nodeValue);
	}
		
	nodeValue = (char*)xmlGetProp(nodeItem, (const xmlChar *) "actionId");
	if(nodeValue){
		setActionId(atoi(nodeValue));
		xmlFreeOTSERV(nodeValue);
	}
	
	nodeValue = (char*)xmlGetProp(nodeItem, (const xmlChar *) "uniqueId");
	if(nodeValue){
		setUniqueId(atoi(nodeValue));
		xmlFreeOTSERV(nodeValue);
	}

	nodeValue = (char*)xmlGetProp(nodeItem, (const xmlChar *) "duration");
	if(nodeValue){
		setDuration(atoi(nodeValue));
		xmlFreeOTSERV(nodeValue);
	}

	nodeValue = (char*)xmlGetProp(nodeItem, (const xmlChar *) "decayState");
	if(nodeValue){
		ItemDecayState_t decayState = (ItemDecayState_t)atoi(nodeValue);
		if(decayState != DECAYING_FALSE){
			setDecaying(DECAYING_PENDING);
		}

		xmlFreeOTSERV(nodeValue);
	}

	return true;
}

xmlNodePtr Item::serialize()
{
	xmlNodePtr nodeItem = xmlNewNode(NULL,(const xmlChar*)"item");

	std::stringstream ss;
	ss.str("");
	ss << getID();
	xmlSetProp(nodeItem, (const xmlChar*)"id", (const xmlChar*)ss.str().c_str());
	
	if(hasSubType()){
		ss.str("");
		ss << (int)getItemCountOrSubtype();
		xmlSetProp(nodeItem, (const xmlChar*)"count", (const xmlChar*)ss.str().c_str());
	}

	if(getSpecialDescription() != ""){
		ss.str("");
		ss << getSpecialDescription();
		xmlSetProp(nodeItem, (const xmlChar*)"special_description", (const xmlChar*)ss.str().c_str());
	}

	if(getText() != ""){
		ss.str("");
		ss << getText();
		xmlSetProp(nodeItem, (const xmlChar*)"text", (const xmlChar*)ss.str().c_str());
	}

	if(!isNotMoveable() /*moveable*/){
		if(getActionId() != 0){
			ss.str("");
			ss << getActionId();
			xmlSetProp(nodeItem, (const xmlChar*)"actionId", (const xmlChar*)ss.str().c_str());
		}
	}
	
	uint32_t duration = getDuration();
	if(duration != 0){
		ss.str("");
		ss << duration;
		xmlSetProp(nodeItem, (const xmlChar*)"duration", (const xmlChar*)ss.str().c_str());
	}

	uint32_t decayState = getDecaying();
	if(decayState == DECAYING_TRUE || decayState == DECAYING_PENDING){
		ss.str("");
		ss << decayState;
		xmlSetProp(nodeItem, (const xmlChar*)"decayState", (const xmlChar*)ss.str().c_str());
	}

	return nodeItem;
}

bool Item::readAttr(AttrTypes_t attr, PropStream& propStream)
{
	switch(attr){
		case ATTR_COUNT:
		{
			unsigned char _count = 0;
			if(!propStream.GET_UCHAR(_count)){
				return false;
			}

			setItemCountOrSubtype(_count);
			break;
		}

		case ATTR_ACTION_ID:
		{
			unsigned short _actionid = 0;
			if(!propStream.GET_USHORT(_actionid)){
				return false;
			}

			setActionId(_actionid);
			break;
		}

		case ATTR_UNIQUE_ID:
		{
			unsigned short _uniqueid;
			if(!propStream.GET_USHORT(_uniqueid)){
				return false;
			}
			
			setUniqueId(_uniqueid);
			break;
		}

		case ATTR_TEXT:
		{
			std::string _text;
			if(!propStream.GET_STRING(_text)){
				return false;
			}

			setText(_text);
			break;
		}

		case ATTR_DESC:
		{
			std::string _text;
			if(!propStream.GET_STRING(_text)){
				return false;
			}

			setSpecialDescription(_text);
			break;
		}

		case ATTR_RUNE_CHARGES:
		{
			unsigned char _charges = 1;
			if(!propStream.GET_UCHAR(_charges)){
				return false;
			}

			setItemCountOrSubtype(_charges);
			break;
		}

		case ATTR_DURATION:
		{
			//uint32_t duration = 0;
			unsigned long duration = 0;
			if(!propStream.GET_ULONG(duration)){
				return false;
			}
			setDuration(duration);
			break;
		}
		
		case ATTR_DECAYING_STATE:
		{
			unsigned char state = 0;
			if(!propStream.GET_UCHAR(state)){
				return false;
			}

			if(state != DECAYING_FALSE){
				setDecaying(DECAYING_PENDING);
			}
			break;
		}

		//these should be handled through derived classes
		//If these are called then something has changed in the items.otb since the map was saved
		//just read the values

		//Depot class
		case ATTR_DEPOT_ID:
		{
			unsigned short _depotId;
			if(!propStream.GET_USHORT(_depotId)){
				return false;
			}
			
			return true;
		}

		//Door class
		case ATTR_HOUSEDOORID:
		{
			unsigned char _doorId;
			if(!propStream.GET_UCHAR(_doorId)){
				return false;
			}
			
			return true;
		}
	
		//Teleport class
		case ATTR_TELE_DEST:
		{
			TeleportDest* tele_dest;
			if(!propStream.GET_STRUCT(tele_dest)){
				return false;
			}

			return true;
		}

		default:
			return false;
		break;
	}

	return true;
}

bool Item::unserializeAttr(PropStream& propStream)
{
	unsigned char attr_type;
	while(propStream.GET_UCHAR(attr_type)){
		if(!readAttr((AttrTypes_t)attr_type, propStream)){
			return false;
			break;
		}
	}

	return true;
}

bool Item::unserializeItemNode(FileLoader& f, NODE node, PropStream& propStream)
{
	return unserializeAttr(propStream);
}

bool Item::serializeAttr(PropWriteStream& propWriteStream)
{
	if(isStackable() || isSplash() || isFluidContainer()){
		unsigned char _count = getItemCountOrSubtype();
		propWriteStream.ADD_UCHAR(ATTR_COUNT);
		propWriteStream.ADD_UCHAR(_count);
	}

	if(isRune()){
		unsigned char _count = getItemCharge();
		propWriteStream.ADD_UCHAR(ATTR_RUNE_CHARGES);
		propWriteStream.ADD_UCHAR(_count);
	}

	if(!isNotMoveable() /*moveable*/){
		unsigned short _actionId = getActionId();
		if(_actionId){
			propWriteStream.ADD_UCHAR(ATTR_ACTION_ID);
			propWriteStream.ADD_USHORT(_actionId);
		}
	}

	const std::string& _text = getText();
	if(_text.length() > 0){
		propWriteStream.ADD_UCHAR(ATTR_TEXT);
		propWriteStream.ADD_STRING(_text);
	}

	const std::string& _specialDesc = getSpecialDescription();
	if(_specialDesc.length() > 0){
		propWriteStream.ADD_UCHAR(ATTR_DESC);
		propWriteStream.ADD_STRING(_specialDesc);
	}

	uint32_t duration = getDuration();
	if(duration != 0){
		propWriteStream.ADD_UCHAR(ATTR_DURATION);
		propWriteStream.ADD_ULONG(duration);
	}

	uint32_t decayState = getDecaying();
	if(decayState == DECAYING_TRUE || decayState == DECAYING_PENDING){
		propWriteStream.ADD_UCHAR(ATTR_DECAYING_STATE);
		propWriteStream.ADD_UCHAR(decayState);
	}

	return true;
}

bool Item::hasProperty(enum ITEMPROPERTY prop) const
{
	const ItemType& it = items[id];

	switch(prop){
		case BLOCKSOLID:
			if(it.blockSolid)
				return true;
			break;

		case HASHEIGHT:
			if(it.hasHeight)
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
		
		case ISVERTICAL:
			if(it.isVertical)
				return true;
			break;

		case ISHORIZONTAL:
			if(it.isHorizontal)
				return true;
			break;

		case BLOCKINGANDNOTMOVEABLE:
			if(it.blockSolid && !it.moveable)
				return true;
			break;
		
		default:
			return false;
	}
	return false;
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
	const ItemType& it = items[id];

	if (it.name.length()) {
		if(isStackable() && count > 1){
			s << (int)count << " " << it.name << "s.";

			if(lookDistance <= 1) {
				s << std::endl << "They weight " << std::fixed << std::setprecision(2) << ((double) count * it.weight) << " oz.";
			}
		}		
		else{
			if(items[id].runeMagLevel != -1)
			{
				s << "a spell rune for level " << it.runeMagLevel << "." << std::endl;

				s << "It's an \"" << it.name << "\" spell (";
				if(getItemCharge())
					s << (int)getItemCharge();
				else
					s << "1";

				s << "x).";
			}
			else if(isWeapon() && (getAttack() || getDefense()))
			{
				if(getAttack()){
					s << "a " << it.name << " (Atk:" << (int)getAttack() << " Def:" << (int)getDefense() << ").";
				}
				else{
					s << "a " << it.name << " (Def:" << (int)getDefense() << ").";	
				}
			}
			else if(getArmor()){
				s << "a " << it.name << " (Arm:" << (int)getArmor() << ").";
			}
			else if(isFluidContainer()){
				s << "a " << it.name;
				if(fluid == 0){
					s << ". It is empty.";
				}
				else{
					s << " of " << items[fluid].name << ".";
				}
			}
			else if(isSplash()){
				s << "a " << it.name << " of ";
				if(fluid == 0){
					s << items[1].name << ".";
				}
				else{
					s << items[fluid].name << ".";
				}
			}
			else if(it.isKey()){
				s << "a " << it.name << " (Key:" << getActionId() << ").";
			}
			else if(it.isGroundTile()){
				s << it.name << ".";
			}
			else if(it.isContainer()){
				s << "a " << it.name << " (Vol:" << getContainer()->capacity() << ").";
			}
			else if(it.allowDistRead){
				s << it.name << "." << std::endl;

				if(lookDistance <= 4){
					if(getText() != ""){
						s << "You read: " << getText();
					}
					else
						s << "Nothing is written on it.";
				}
				else
					s << "You are too far away to read it.";
			}
			else{
				s << "a " << it.name;

				if(it.showCharges){
					uint32_t charges = getItemCharge();

					if(charges > 1){
						s << " that has " << charges << " charges left";
					}
					else{
						s << " that has " << charges << " charge left";
					}
				}
				if(it.showDuration){
					uint32_t duration = getDuration() / 1000;
					if(duration > 0){
						s << " that has energy for ";

						if(duration >= 120){
							s << duration / 60 << " minutes left";
						}
						else if(duration > 60){
							s << duration / 60 << " minute left";
						}
						else{
							s << " less than a minute left";
						}
					}
					else{
						s << std::endl << " that is brand-new";
					}
				}

				s << ".";
			}

			if(lookDistance <= 1){
				double weight = getWeight();
				if(weight > 0)
					s << std::endl << "It weighs " << std::fixed << std::setprecision(2) << weight << " oz.";
			}

			if(getSpecialDescription() != "")
				s << std::endl << getSpecialDescription().c_str();
			else if(lookDistance <= 1 && it.description.length()){
				s << std::endl << it.description;
			}
		}
	}
	else
		s << "an item of type " << id <<".";
	
	return s.str();
}

std::string Item::getWeightDescription() const
{
	double weight = getWeight();

	std::stringstream ss;
	if(weight > 0){
		ss << " It weighs " << std::fixed << std::setprecision(2) << weight << " oz.";
	}

	return ss.str();
}

void Item::setUniqueId(unsigned short n)
{
	if(getUniqueId() != 0)
		return;
	
	ItemAttributes::setUniqueId(n);
	ScriptEnviroment::addUniqueThing(this);
}

int Item::getRWInfo(int& maxlen) const
{
	const ItemType& it = items[id];
	maxlen = it.maxTextLen;
	return it.RWInfo;
}

bool Item::canDecay()
{
	if(isRemoved()){
		return false;
	}
	return (items[id].canDecay && getDefaultDuration() > 0);
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

void Item::getLight(LightInfo& lightInfo)
{
	const ItemType& it = items[id];
	lightInfo.color = it.lightColor;
	lightInfo.level = it.lightLevel;
}

std::string ItemAttributes::emptyString("");

ItemAttributes::ItemAttributes()
{
	m_attributes = 0;
	m_firstAttr = NULL;
}

ItemAttributes::~ItemAttributes()
{
	if(m_firstAttr){
		deleteAttrs(m_firstAttr);
	}
}

const std::string& ItemAttributes::getStrAttr(itemAttrTypes type) const
{
	if(!validateStrAttrType(type))
		return emptyString;
		
	Attribute* attr = getAttr(type);
	if(attr){
		return *(std::string*)attr->value;
	}
	else{
		return emptyString;
	}
}

void ItemAttributes::setStrAttr(itemAttrTypes type, const std::string& value)
{
	if(!validateStrAttrType(type))
		return;
	
	if(value.length() == 0)
		return;
	
	Attribute* attr = getAttr(type);
	if(attr){
		if(attr->value){
			delete (std::string*)attr->value;
		}
		attr->value = (void*)new std::string(value);
	}
}

bool ItemAttributes::hasAttribute(itemAttrTypes type) const
{
	if(!validateIntAttrType(type))
		return false;

	Attribute* attr = getAttr(type);
	if(attr){
		return true;
	}

	return false;
}
	
uint32_t ItemAttributes::getIntAttr(itemAttrTypes type) const
{
	if(!validateIntAttrType(type))
		return 0;
		
	Attribute* attr = getAttr(type);
	if(attr){
		return (uint32_t)attr->value;
	}
	else{
		return 0;
	}
}

void ItemAttributes::setIntAttr(itemAttrTypes type, uint32_t value)
{
	if(!validateIntAttrType(type))
		return;
	
	Attribute* attr = getAttr(type);
	if(attr){
		attr->value = (void*)value;
	}
}

void ItemAttributes::increaseIntAttr(itemAttrTypes type, uint32_t value)
{
	if(!validateIntAttrType(type))
		return;
	
	Attribute* attr = getAttr(type);
	if(attr){
		attr->value = (void*)((uint32_t)attr->value + value);
	}
}
	
inline bool ItemAttributes::validateIntAttrType(itemAttrTypes type) const
{
	switch(type){
	case ATTR_ITEM_ACTIONID:
	case ATTR_ITEM_UNIQUEID:
	case ATTR_ITEM_OWNER:
	case ATTR_ITEM_DURATION:
	case ATTR_ITEM_DECAYING:
		return true;
		break;

	default:
		return false;
		break;
	}
	return false;
}

inline bool ItemAttributes::validateStrAttrType(itemAttrTypes type) const
{
	switch(type){
	case ATTR_ITEM_DESC:
		return true;
		break;
	case ATTR_ITEM_TEXT:
		return true;
		break;
	default:
		return false;
		break;
	}
	return false;
}

void ItemAttributes::addAttr(Attribute* attr)
{
	if(m_firstAttr){
		Attribute* curAttr = m_firstAttr;
		while(curAttr->next){
			curAttr = curAttr->next;
		}
		curAttr->next = attr;
	}
	else{
		m_firstAttr = attr;
	}
	m_attributes = m_attributes | attr->type;
}

ItemAttributes::Attribute* ItemAttributes::getAttr(itemAttrTypes type) const
{
	if((type & m_attributes) == 0){
		return NULL;
	}
	
	Attribute* curAttr = m_firstAttr;
	while(curAttr){
		if(curAttr->type == type){
			return curAttr;
		}
		curAttr = curAttr->next;
	}
	return NULL;
}

ItemAttributes::Attribute* ItemAttributes::getAttr(itemAttrTypes type)
{
	Attribute* curAttr;
	if((type & m_attributes) == 0){
		curAttr = new Attribute(type);
		addAttr(curAttr);
		return curAttr;
	}
	else{
		curAttr = m_firstAttr;
		while(curAttr){
			if(curAttr->type == type){
				return curAttr;
			}
			curAttr = curAttr->next;
		}
	}
	std::cout << "Warning: [ItemAttributes::getAttr] (type & m_attributes) != 0 but attribute not found" << std::endl;
	curAttr = new Attribute(type);
	addAttr(curAttr);
	return curAttr;
}

void ItemAttributes::deleteAttrs(Attribute* attr)
{
	if(attr){
		if(validateIntAttrType(attr->type)){
			//
		}
		else if(validateStrAttrType(attr->type)){
			delete (std::string*)attr->value;
		}
		Attribute* next_attr = attr->next;
		attr->next = NULL;
		if(next_attr){
			deleteAttrs(next_attr);
		}
		delete attr;
	}
}

void Item::__startDecaying()
{
	g_game.startDecay(this);
}
