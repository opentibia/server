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
#include "otpch.h"

#include "item.h"
#include "tile.h"
#include "creature.h"
#include "weapons.h"
#include "house.h"
#include "beds.h"
#include "container.h"
#include "depot.h"
#include "teleport.h"
#include "trashholder.h"
#include <iomanip>

Items Item::items;

Item* Item::CreateItem(const uint16_t _type, uint16_t _count /*= 1*/)
{
	Item* newItem = NULL;

	const ItemType& it = Item::items[_type];

	if(it.group == ITEM_GROUP_DEPRECATED){
#ifdef __DEBUG__
		std::cout << "Error: [Item::CreateItem] Item id " << it.id << " has been declared deprecated."  << std::endl;
#endif
		return NULL;
	}

	if(it.id != 0){
		if(it.isDepot()){
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
		else if(it.isTrashHolder()){
			newItem = new TrashHolder(_type, it.magicEffect);
		}
		else if(it.isBed()){
			newItem = new BedItem(_type);
		}
		else{
			newItem = new Item(_type, _count);
		}

		newItem->addRef();
	}
	return newItem;
}

Item* Item::CreateItem(PropStream& propStream)
{
	uint16_t _id;
	if(!propStream.GET_USHORT(_id)){
		return NULL;
	}

	return Item::CreateItem(_id, 0);
}


bool Item::loadItem(xmlNodePtr node, Container* parent)
{
	int32_t intValue;
	std::string strValue;

	if(xmlStrcmp(node->name, (const xmlChar*)"item") == 0){
		Item* item = NULL;
		if(readXMLInteger(node, "id", intValue)){
			item = Item::CreateItem(intValue);
		}

		if(!item){
			return false;
		}

		//optional
		if(readXMLInteger(node, "subtype", intValue)){
			item->setSubType(intValue);
		}

		if(readXMLInteger(node, "actionid", intValue)){
			item->setActionId(intValue);
		}

		if(readXMLString(node, "text", strValue)){
			item->setText(strValue);
		}

		if(item->getContainer()){
			loadContainer(node, item->getContainer());
		}

		if(parent){
			parent->addItem(item);
		}

		return true;
	}

	return false;
}

bool Item::loadContainer(xmlNodePtr parentNode, Container* parent)
{
	xmlNodePtr node = parentNode->children;
	while(node){
		if(node->type != XML_ELEMENT_NODE){
			node = node->next;
			continue;
		}

		if(xmlStrcmp(node->name, (const xmlChar*)"item") == 0){
			if(!loadItem(node, parent)){
				return false;
			}
		}

		node = node->next;
	}

	return true;
}

Item::Item(const uint16_t _type, uint16_t _count /*= 0*/) :
	ItemAttributes()
{
	id = _type;

	const ItemType& it = items[id];

	count = 1;
	if(it.charges != 0){
		setCharges(it.charges);
	}

	if(it.isFluidContainer() || it.isSplash()){
		setFluidType(_count);
	}
	else if(it.stackable && _count != 0){
		count = _count;
	}
	else if(it.charges != 0 && _count != 0){
		setCharges(_count);
	}

	setDefaultDuration();
}

Item::Item(const Item &i) :
	Thing(), ItemAttributes(i)
{
	//std::cout << "Item copy constructor " << this << std::endl;
	id = i.id;
	count = i.count;
}

Item* Item::clone() const
{
	Item* _item = Item::CreateItem(id, count);
	if(attributes){
		_item->attributes = new AttributeMap();
		*_item->attributes = *attributes;
	}

	return _item;
}

void Item::copyAttributes(Item* item)
{
	if(item->attributes){
		attributes = new AttributeMap();
		*attributes = *item->attributes;
	}

	eraseAttribute("decaying");
	eraseAttribute("duration");
}

Item::~Item()
{
}

void Item::setDefaultSubtype()
{
	const ItemType& it = items[id];

	count = 1;
	if(it.charges != 0){
		setCharges(it.charges);
	}
}

void Item::onRemoved()
{
}

void Item::setID(uint16_t newid)
{
	const ItemType& prevIt = Item::items[id];
	id = newid;

	const ItemType& it = Item::items[newid];
	uint32_t newDuration = it.decayTime * 1000;

	if(newDuration == 0 && !it.stopTime && it.decayTo == -1){
		eraseAttribute("decaying");
		eraseAttribute("duration");
	}

	eraseAttribute("corpseowner");

	if(newDuration > 0 && (!prevIt.stopTime || getIntegerAttribute("duration") == NULL) ){
		setDecaying(DECAYING_FALSE);
		setDuration(newDuration);
	}

	if(getParent()){
		if(Tile* tile = getParent()->getParentTile()){
			tile->items_onItemModified(this);
		}
	}
}

void Item::setActionId(int32_t n)
{
	setAttribute("aid", n);

	if(getParent()){
		if(Tile* tile = getParent()->getTile()){
			tile->items_onItemModified(this);
		}
	}
}

bool Item::hasSubType() const
{
	const ItemType& it = items[id];
	return it.hasSubType();
}

uint16_t Item::getSubType() const
{
	const ItemType& it = items[getID()];

	if(it.isFluidContainer() || it.isSplash()){
		return getFluidType();
	}
	else if(it.charges != 0){
		return getCharges();
	}

	return count;
}

Player* Item::getHoldingPlayer()
{
	Cylinder* p = getParent();
	while(p){
		if(p->getCreature())
			// Must be a player, creatures are not cylinders
			return p->getCreature()->getPlayer();
		p = p->getParent();
	}
	return NULL;
}

const Player* Item::getHoldingPlayer() const
{
	return const_cast<Item*>(this)->getHoldingPlayer();
}

void Item::setSubType(uint16_t n)
{
	const ItemType& it = items[id];

	if(it.isFluidContainer() || it.isSplash()){
		setFluidType(n);
	}
	else if(it.charges != 0){
		setCharges(n);
	}
	else{
		count = n;
	}
}

Attr_ReadValue Item::readAttr(AttrTypes_t attr, PropStream& propStream)
{
	switch(attr){
		case ATTR_COUNT:
		{
			uint8_t _count = 0;
			if(!propStream.GET_UCHAR(_count)){
				return ATTR_READ_ERROR;
			}

			setSubType(_count);
			break;
		}

		case ATTR_ACTION_ID:
		{
			uint16_t _actionid = 0;
			if(!propStream.GET_USHORT(_actionid)){
				return ATTR_READ_ERROR;
			}

			setActionId(_actionid);
			break;
		}

		case ATTR_UNIQUE_ID:
		{
			uint16_t _uniqueid;
			if(!propStream.GET_USHORT(_uniqueid)){
				return ATTR_READ_ERROR;
			}

			setAttribute("uid", (int32_t)_uniqueid);
			break;
		}

		case ATTR_TEXT:
		{
			std::string _text;
			if(!propStream.GET_STRING(_text)){
				return ATTR_READ_ERROR;
			}

			setText(_text);
			break;
		}

		case ATTR_WRITTENDATE:
		{
			uint32_t _writtenDate;
			if(!propStream.GET_ULONG(_writtenDate)){
				return ATTR_READ_ERROR;
			}

			setWrittenDate(_writtenDate);
			break;
		}

		case ATTR_WRITTENBY:
		{
			std::string _writer;
			if(!propStream.GET_STRING(_writer)){
				return ATTR_READ_ERROR;
			}

			setWriter(_writer);
			break;
		}

		case ATTR_DESC:
		{
			std::string _text;
			if(!propStream.GET_STRING(_text)){
				return ATTR_READ_ERROR;
			}

			setSpecialDescription(_text);
			break;
		}

		case ATTR_RUNE_CHARGES:
		{
			uint8_t _charges = 1;
			if(!propStream.GET_UCHAR(_charges)){
				return ATTR_READ_ERROR;
			}

			setSubType(_charges);
			break;
		}

		case ATTR_CHARGES:
		{
			uint16_t _charges = 1;
			if(!propStream.GET_USHORT(_charges)){
				return ATTR_READ_ERROR;
			}

			setSubType(_charges);
			break;
		}

		case ATTR_DURATION:
		{
			uint32_t duration = 0;
			if(!propStream.GET_ULONG(duration)){
				return ATTR_READ_ERROR;
			}

			if(((int32_t)duration) < 0){
				duration = 0;
			}
			setDuration(duration);
			break;
		}

		case ATTR_DECAYING_STATE:
		{
			uint8_t state = 0;
			if(!propStream.GET_UCHAR(state)){
				return ATTR_READ_ERROR;
			}

			if(state != DECAYING_FALSE){
				setDecaying(DECAYING_PENDING);
			}
			break;
		}

		//these should be handled through derived classes
		//If these are called then something has changed in the items.xml since the map was saved
		//just read the values

		//Depot class
		case ATTR_DEPOT_ID:
		{
			uint16_t _depotId;
			if(!propStream.GET_USHORT(_depotId)){
				return ATTR_READ_ERROR;
			}

			return ATTR_READ_CONTINUE;
		}

		//Door class
		case ATTR_HOUSEDOORID:
		{
			uint8_t _doorId;
			if(!propStream.GET_UCHAR(_doorId)){
				return ATTR_READ_ERROR;
			}

			return ATTR_READ_CONTINUE;
		}

		//Bed class
		case ATTR_SLEEPERGUID:
		{
			uint32_t _guid;
			if(!propStream.GET_ULONG(_guid)){
				return ATTR_READ_ERROR;
			}

			return ATTR_READ_CONTINUE;
		}

		case ATTR_SLEEPSTART:
		{
			uint32_t sleep_start;
			if(!propStream.GET_ULONG(sleep_start)){
				return ATTR_READ_ERROR;
			}

			return ATTR_READ_CONTINUE;
		}

		//Teleport class
		case ATTR_TELE_DEST:
		{
			TeleportDest* tele_dest;
			if(!propStream.GET_STRUCT(tele_dest)){
				return ATTR_READ_ERROR;
			}

			return ATTR_READ_CONTINUE;
		}

		//Container class
		case ATTR_CONTAINER_ITEMS:
		{
			uint32_t count;
			if(!propStream.GET_ULONG(count)){
				return ATTR_READ_ERROR;
			}

			//We cant continue parse attributes since there is
			//container data after this attribute.
			return ATTR_READ_ERROR;
		}

		default:
			return ATTR_READ_ERROR;
		break;
	}

	return ATTR_READ_CONTINUE;
}

bool Item::unserializeAttr(PropStream& propStream)
{
	// Backwards-compatible reading of attributes
	uint8_t attr_type;
	while(propStream.GET_UCHAR(attr_type) && attr_type != 0){
		if(attr_type == ATTR_ATTRIBUTE_MAP){
			if(!ItemAttributes::unserializeAttributeMap(propStream))
				return false;
		}
		else{
			Attr_ReadValue ret = readAttr((AttrTypes_t)attr_type, propStream);
			if(ret == ATTR_READ_ERROR){
				return false;
			}
			else if(ret == ATTR_READ_END){
				return true;
			}
		}
	}

	return true;
}

bool Item::unserializeItemNode(FileLoader& f, NODE node, PropStream& propStream)
{
	return unserializeAttr(propStream);
}

bool Item::serializeAttr(PropWriteStream& propWriteStream) const
{
	if(isStackable() || isFluidContainer() || isSplash()){
		uint8_t _count = getSubType();
		propWriteStream.ADD_UCHAR(ATTR_COUNT);
		propWriteStream.ADD_UCHAR(_count);
	}

	if(attributes && attributes->size()){
		propWriteStream.ADD_UCHAR(ATTR_ATTRIBUTE_MAP);
		serializeAttributeMap(propWriteStream);
	}

	return true;
}

bool Item::hasProperty(uint32_t props) const
{
	const ItemType& it = items[id];

	if(hasBitSet(ITEMPROP_BLOCKSOLID, props) && !it.blockSolid){
		return false;
	}

	if(hasBitSet(ITEMPROP_BLOCKPATHFIND, props) && !it.blockPathFind){
		return false;
	}

	if(hasBitSet(ITEMPROP_BLOCKPROJECTILE, props) && !it.blockProjectile){
		return false;
	}

	if(hasBitSet(ITEMPROP_ALLOWPICKUPABLE, props) && !it.allowPickupable){
		return false;
	}

	if(hasBitSet(ITEMPROP_HASHEIGHT, props) && !it.hasHeight){
		return false;
	}

	if(hasBitSet(ITEMPROP_ISVERTICAL, props) && !it.isVertical){
		return false;
	}

	if(hasBitSet(ITEMPROP_ISHORIZONTAL, props) && !it.isHorizontal){
		return false;
	}

	if(hasBitSet(ITEMPROP_CLIENTCHARGES, props) && !it.clientCharges){
		return false;
	}

	if(hasBitSet(ITEMPROP_LOOKTHROUGH, props) && !it.lookThrough){
		return false;
	}

	if(hasBitSet(ITEMPROP_PICKUPABLE, props) && !it.pickupable){
		return false;
	}

	if(hasBitSet(ITEMPROP_ROTATEABLE, props) && !it.rotateable){
		return false;
	}

	if(hasBitSet(ITEMPROP_STACKABLE, props) && !it.stackable){
		return false;
	}

	if(hasBitSet(ITEMPROP_USEABLE, props) && !it.useable){
		return false;
	}

	if(hasBitSet(ITEMPROP_MOVEABLE, props) && !it.moveable){
		return false;
	}

	if(hasBitSet(ITEMPROP_ALWAYSONTOP, props) && !it.alwaysOnTop){
		return false;
	}

	if(hasBitSet(ITEMPROP_CANREADTEXT, props) && !it.canReadText){
		return false;
	}

	if(hasBitSet(ITEMPROP_CANWRITETEXT, props) && !it.canWriteText){
		return false;
	}

	if(hasBitSet(ITEMPROP_FLOORCHANGEDOWN, props) && !it.floorChangeDown){
		return false;
	}

	if(hasBitSet(ITEMPROP_FLOORCHANGENORTH, props) && !it.floorChangeNorth){
		return false;
	}

	if(hasBitSet(ITEMPROP_FLOORCHANGESOUTH, props) && !it.floorChangeSouth){
		return false;
	}

	if(hasBitSet(ITEMPROP_FLOORCHANGEEAST, props) && !it.floorChangeEast){
		return false;
	}

	if(hasBitSet(ITEMPROP_FLOORCHANGEWEST, props) && !it.floorChangeWest){
		return false;
	}

	if(hasBitSet(ITEMPROP_ALLOWDISTREAD, props) && !it.allowDistRead){
		return false;
	}

	return true;
}

double Item::getWeight() const
{
	if(isStackable()){
		return items[id].weight * std::max((int32_t)1, (int32_t)count);
	}

	return items[id].weight;
}

std::string Item::getLongName(const ItemType& it, int32_t lookDistance,
	const Item* item /*= NULL*/, int32_t subType /*= -1*/, bool addArticle /*= true*/)
{
	std::ostringstream s;

	if(item){
		subType = item->getSubType();
	}

	const std::string& name = (item? item->getName() : it.name);
	const std::string& plural = (item? item->getPluralName() : it.pluralName);
	const std::string& article = (item? item->getArticle() : it.article);

	if(name.length()){
		if(it.stackable && subType > 1){
			if(it.showCount){
				s << subType << " ";
			}

			s << plural;
		}
		else{
			if(addArticle && !article.empty()){
				s << article << " ";
			}
			s << name;
		}
	}
	else{
		s << "an item of type " << it.id;
	}

	return s.str();
}

std::string Item::getLongName() const
{
	const ItemType& it = items[id];
	return getLongName(it, 0, this);
}

std::string Item::getDescription(const ItemType& it, int32_t lookDistance,
	const Item* item /*= NULL*/, int32_t subType /*= -1*/, bool addArticle /*= true*/)
{
	std::stringstream s;

	if(item){
		subType = item->getSubType();
	}

	s << getLongName(it, lookDistance, item, subType, addArticle);

	int armor = (item? item->getArmor() : it.armor);

	if(it.isRune()){
		s << "(\"" << it.runeSpellName << "\", Charges:" << subType <<").";
		if(it.runeLevel > 0 || it.runeMagicLevel > 0){
			s << std::endl << "It can only be used with";
			if(it.runeLevel > 0){
				s << " level " << it.runeLevel;
			}
			if(it.runeMagicLevel > 0){
				if(it.runeLevel > 0){
					s << " and";
				}
				s << " magic level " << it.runeMagicLevel;
			}
			s << " or higher.";
		}
	}
	else if(it.weaponType != WEAPON_NONE){
		if(it.weaponType == WEAPON_DIST && it.ammoType != AMMO_NONE){
			s << " (Range:" << it.shootRange;
			if(it.attack != 0){
				s << ", Atk" << std::showpos << it.attack << std::noshowpos;
			}
			if(it.hitChance != 0){
				s << ", Hit%" << std::showpos << it.hitChance << std::noshowpos;
			}
			s << ")";
		}
		else if(it.weaponType != WEAPON_AMMO && it.weaponType != WEAPON_WAND){ // Arrows and Bolts doesn't show atk
			s << " (";
			int attack = (item? item->getAttack() : it.attack);
			if(attack != 0)
				s << "Atk:" << attack;

			int defense = (item? item->getDefense() : it.defense);
			int extraDefense = (item? item->getExtraDef() : it.extraDefense);
			if(defense != 0 || extraDefense != 0){
				if(attack != 0)
					s << " ";

				s << "Def:" << (int32_t)defense;
				if(extraDefense != 0){
					s << " " << std::showpos << (int)extraDefense << std::noshowpos;
				}
			}

			if(it.abilities.stats[*STAT_MAGICPOINTS] != 0){
				if(it.attack != 0 || it.defense != 0 || it.extraDefense != 0)
					s << ", ";

				s << "magic level " << std::showpos << (int32_t)it.abilities.stats[*STAT_MAGICPOINTS] << std::noshowpos;
			}
			s << ")";
		}

		if(it.showCharges){
			if(subType > 1){
				s << " that has " << (int32_t)subType << " charges left";
			}
			else{
				s << " that has 1 charge left";
			}
		}

		s << ".";
	}
	else if(armor != 0 || it.abilities.absorb.any()){
		if(it.showCharges){
			if(subType > 1){
				s << " that has " << (int32_t)subType << " charges left";
			}
			else{
				s << " that has 1 charge left";
			}
		}

		s << " (Arm:" << armor;

		if(it.abilities.absorb.any()){
			s << ", protection";
			it.abilities.absorb.getDescription(s);
		}
		s << ").";
	}
	else if(it.isFluidContainer()){
		if(subType > 0){
			s << " of " << items[subType].name << ".";
		}
		else{
			s << ". It is empty.";
		}
	}
	else if(it.isSplash()){
		s << " of ";
		if(subType > 0){
			s << items[subType].name << ".";
		}
		else{
			s << items[1].name << ".";
		}
	}
	else if(it.isContainer()){
		s << " (Vol:" << (int32_t)it.maxItems << ").";
	}
	else if(it.isKey()){
		if(item){
			s << " (Key:" << (int32_t)item->getActionId() << ").";
		}
		else{
			s << " (Key:0).";
		}
	}
	else if(it.allowDistRead){
		s << std::endl;

		if(item && item->getText() != ""){
			if(lookDistance <= 4){
				if(item->getWriter().length()){
					s << item->getWriter() << " wrote";

					time_t wDate = item->getWrittenDate();
					if(wDate > 0){
						char date[16];
						formatDateShort(wDate, date);
						s << " on " << date;
					}
					s << ": ";
				}
				else{
					s << "You read: ";
				}

				s << item->getText();
			}
			else{
				s << "You are too far away to read it.";
			}
		}
		else{
			s << "Nothing is written on it.";
		}
	}
	else if(it.showCharges){
		if(subType > 1){
			s << " that has " << (int32_t)subType << " charges left.";
		}
		else{
			s << " that has 1 charge left.";
		}
	}
	else if(it.showDuration){
		if(item && item->hasIntegerAttribute("duration")){
			int32_t duration = item->getDuration() / 1000;
			s << " that has energy for ";

			if(duration >= 120){
				s << duration / 60 << " minutes left.";
			}
			else if(duration > 60){
				s << "1 minute left.";
			}
			else{
				s << "less than a minute left.";
			}
		}
		else{
			s << " that is brand-new.";
		}
	}
	else{
		s << ".";
	}

	if(it.wieldInfo != 0){
		s << std::endl << "It can only be wielded properly by ";
		if(it.wieldInfo & WIELDINFO_PREMIUM){
			s << "premium ";
		}

		if(it.wieldInfo & WIELDINFO_VOCREQ){
			s << it.vocationString;
		}
		else{
			s << "players";
		}

		if(it.wieldInfo & WIELDINFO_LEVEL){
			s << " of level " << (int)it.minRequiredLevel << " or higher";
		}

		if(it.wieldInfo & WIELDINFO_MAGLV){
			if(it.wieldInfo & WIELDINFO_LEVEL){
				s << " and";
			}
			else{
				s << " of";
			}
			s << " magic level " << (int)it.minRequiredMagicLevel << " or higher";
		}

		s << ".";
	}

	if(lookDistance <= 1){
		double weight = (item == NULL ? it.weight : item->getWeight());
		if(weight > 0){
			s << std::endl << getWeightDescription(it, weight);
		}
	}

	if(it.abilities.elementType != COMBAT_NONE && it.charges != 0){
		s << " It is temporarily enchanted with ";
		std::string strElement = combatTypeToString(it.abilities.elementType);
		int32_t elementDamage = it.abilities.elementDamage;
		s << strElement << " (" << it.attack - elementDamage << " physical + " << elementDamage << " " << strElement << " damage).";
	}

	if(item && item->getSpecialDescription() != ""){
		s << std::endl << item->getSpecialDescription().c_str();
	}
	else if(it.description.length() && lookDistance <= 1){
		s << std::endl << it.description;
	}

	return s.str();
}

std::string Item::getDescription(int32_t lookDistance) const
{
	std::stringstream s;
	const ItemType& it = items[id];

	s << getDescription(it, lookDistance, this);
	return s.str();
}

std::string Item::getWeightDescription() const
{
	double weight = getWeight();
	if(weight > 0){
		return getWeightDescription(weight);
	}
	else{
		return "";
	}
}

std::string Item::getWeightDescription(double weight) const
{
	const ItemType& it = Item::items[id];
	return getWeightDescription(it, weight, count);
}

std::string Item::getWeightDescription(const ItemType& it, double weight, uint32_t count /*= 1*/)
{
	std::stringstream ss;
	if(it.stackable && count > 1){
		ss << "They weigh " << std::fixed << std::setprecision(2) << weight << " oz.";
	}
	else{
		ss << "It weighs " << std::fixed << std::setprecision(2) << weight << " oz.";
	}
	return ss.str();
}

bool Item::canDecay()
{
	if(isRemoved()){
		return false;
	}

	const bool* candecay = getBooleanAttribute("canDecay");

	if(candecay && *candecay == false){
		return false;
	}

	const ItemType& it = Item::items[id];
	if(it.decayTo == -1 || it.decayTime == 0){
		return false;
	}

	return true;
}

uint32_t Item::getWorth() const
{
	const ItemType& it = Item::items[id];
	return getItemCount() * it.currency;
}

void Item::getLight(LightInfo& lightInfo)
{
	const ItemType& it = items[id];
	lightInfo.color = it.lightColor;
	lightInfo.level = it.lightLevel;
}







