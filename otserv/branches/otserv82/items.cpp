//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// The database of items.
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

#include "items.h"
#include "spells.h"
#include "condition.h"
#include "weapons.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <iostream>
#include <string>

uint32_t Items::dwMajorVersion = 0;
uint32_t Items::dwMinorVersion = 0;
uint32_t Items::dwBuildNumber = 0;

extern Spells* g_spells;

ItemType::ItemType()
{
	article          = "";
	group            = ITEM_GROUP_NONE;
	type             = ITEM_TYPE_NONE;
	stackable        = false;
	useable	         = false;
	moveable         = true;
	alwaysOnTop      = false;
	alwaysOnTopOrder = 0;
	pickupable       = false;
	rotable          = false;
	rotateTo		     = 0;
	hasHeight        = false;

	floorChangeDown = true;
	floorChangeNorth = false;
	floorChangeSouth = false;
	floorChangeEast = false;
	floorChangeWest = false;

	blockSolid = false;
	blockProjectile = false;
	blockPathFind = false;

	wieldInfo        = 0;
	minReqLevel      = 0;
	minReqMagicLevel = 0;

	runeMagLevel  = 0;
	runeLevel     = 0;

	speed		  = 0;
	id            = 0;
	clientId      = 100;
	maxItems      = 8;  // maximum size if this is a container
	weight        = 0;  // weight of the item, e.g. throwing distance depends on it
	showCount     = true;
	weaponType    = WEAPON_NONE;
	slot_position = SLOTP_RIGHT | SLOTP_LEFT | SLOTP_AMMO;
	amuType       = AMMO_NONE;
	ammoAction    = AMMOACTION_NONE;
	shootType     = (ShootType_t)0;
	magicEffect   = NM_ME_NONE;
	attack        = 0;
	defence       = 0;
	extraDef      = 0;
	armor         = 0;
	decayTo       = -1;
	decayTime     = 0;
	stopTime      = false;

	allowDistRead = false;

	isVertical		= false;
	isHorizontal	= false;
	isHangable		= false;

	lightLevel    = 0;
	lightColor    = 0;

	maxTextLen = 0;
	canReadText = false;
	canWriteText = false;
	writeOnceItemId  = 0;

	transformEquipTo   = 0;
	transformDeEquipTo = 0;
	showDuration  = false;
	showCharges   = false;
	charges       = 0;
	hitChance     = -1;
	maxHitChance  = -1;
	breakChance   = -1;
	shootRange    = 1;

	condition = NULL;
	combatType = COMBAT_NONE;

	replaceable = true;
}

ItemType::~ItemType()
{
	delete condition;
}

Items::Items() :
items(8000)
{
	//
}

Items::~Items()
{
	clear();
}

void Items::clear()
{
	//TODO. clear items?
}

bool Items::reload()
{
	//TODO?
	/*
	for (ItemMap::iterator it = items.begin(); it != items.end(); it++){
		delete it->second->condition;
	}
	return loadFromXml(m_datadir);
	*/
	return false;
}

int Items::loadFromOtb(std::string file)
{
	FileLoader f;
	if(!f.openFile(file.c_str(), false, true)){
		return f.getError();
	}

	unsigned long type;
	NODE node = f.getChildNode(NO_NODE, type);

	PropStream props;
	if(f.getProps(node,props)){
		//4 byte flags
		//attributes
		//0x01 = version data
		uint32_t flags;
		if(!props.GET_ULONG(flags)){
			return ERROR_INVALID_FORMAT;
		}
		attribute_t attr;
		if(!props.GET_VALUE(attr)){
			return ERROR_INVALID_FORMAT;
		}
		if(attr == ROOT_ATTR_VERSION){
			datasize_t datalen = 0;
			if(!props.GET_VALUE(datalen)){
				return ERROR_INVALID_FORMAT;
			}
			if(datalen != sizeof(VERSIONINFO)){
				return ERROR_INVALID_FORMAT;
			}
			VERSIONINFO *vi;
			if(!props.GET_STRUCT(vi)){
				return ERROR_INVALID_FORMAT;
			}
			Items::dwMajorVersion = vi->dwMajorVersion;	//items otb format file version
			Items::dwMinorVersion = vi->dwMinorVersion;	//client version
			Items::dwBuildNumber = vi->dwBuildNumber;	//revision
		}
	}

	if(Items::dwMajorVersion != 2){
		std::cout << "Not supported items.otb version." << std::endl;
		return ERROR_INVALID_FORMAT;
	}

	if(Items::dwMajorVersion == 0xFFFFFFFF){
		std::cout << "[Warning] Items::loadFromOtb items.otb using generic client version." << std::endl;
	}
	else if(Items::dwMinorVersion != CLIENT_VERSION_820){
		std::cout << "Not supported items.otb client version." << std::endl;
		return ERROR_INVALID_FORMAT;
	}

	node = f.getChildNode(node, type);

	while(node != NO_NODE){
		PropStream props;
		if(!f.getProps(node,props)){
			return f.getError();
		}

		flags_t flags;
		ItemType* iType = new ItemType();
		iType->group = (itemgroup_t)type;

		switch(type){
			case ITEM_GROUP_CONTAINER:
				iType->type = ITEM_TYPE_CONTAINER;
				break;
			case ITEM_GROUP_DOOR:
				iType->type = ITEM_TYPE_DOOR;
				break;
			case ITEM_GROUP_MAGICFIELD:
				iType->type = ITEM_TYPE_MAGICFIELD;
				break;
			case ITEM_GROUP_TELEPORT:
				iType->type = ITEM_TYPE_TELEPORT;
				break;
			case ITEM_GROUP_NONE:
			case ITEM_GROUP_GROUND:
			case ITEM_GROUP_RUNE:
			case ITEM_GROUP_SPLASH:
			case ITEM_GROUP_FLUID:
			case ITEM_GROUP_DEPRECATED:
				break;
			default:
				return ERROR_INVALID_FORMAT;
				break;
		}

		//read 4 byte flags
		if(!props.GET_VALUE(flags)){
			return ERROR_INVALID_FORMAT;
		}

		iType->blockSolid = hasBitSet(FLAG_BLOCK_SOLID, flags);
		iType->blockProjectile = hasBitSet(FLAG_BLOCK_PROJECTILE, flags);
		iType->blockPathFind = hasBitSet(FLAG_BLOCK_PATHFIND, flags);
		iType->hasHeight = hasBitSet(FLAG_HAS_HEIGHT, flags);
		iType->useable = hasBitSet(FLAG_USEABLE, flags);
		iType->pickupable = hasBitSet(FLAG_PICKUPABLE, flags);
		iType->moveable = hasBitSet(FLAG_MOVEABLE, flags);
		iType->stackable = hasBitSet(FLAG_STACKABLE, flags);
		iType->floorChangeDown = hasBitSet(FLAG_FLOORCHANGEDOWN, flags);
		iType->floorChangeNorth = hasBitSet(FLAG_FLOORCHANGENORTH, flags);
		iType->floorChangeEast = hasBitSet(FLAG_FLOORCHANGEEAST, flags);
		iType->floorChangeSouth = hasBitSet(FLAG_FLOORCHANGESOUTH, flags);
		iType->floorChangeWest = hasBitSet(FLAG_FLOORCHANGEWEST, flags);
		iType->alwaysOnTop = hasBitSet(FLAG_ALWAYSONTOP, flags);
		iType->isVertical = hasBitSet(FLAG_VERTICAL, flags);
		iType->isHorizontal = hasBitSet(FLAG_HORIZONTAL, flags);
		iType->isHangable = hasBitSet(FLAG_HANGABLE, flags);
		iType->allowDistRead = hasBitSet(FLAG_ALLOWDISTREAD, flags);
		iType->rotable = hasBitSet(FLAG_ROTABLE, flags);

		if(hasBitSet(FLAG_READABLE, flags)){
			iType->canReadText = true;
		}

		attribute_t attrib;
		datasize_t datalen = 0;
		while(props.GET_VALUE(attrib)){
			//size of data
			if(!props.GET_VALUE(datalen)){
				delete iType;
				return ERROR_INVALID_FORMAT;
			}
			switch(attrib){
			case ITEM_ATTR_SERVERID:
			{
				if(datalen != sizeof(uint16_t))
					return ERROR_INVALID_FORMAT;

				uint16_t serverid;
				if(!props.GET_USHORT(serverid))
					return ERROR_INVALID_FORMAT;

				if(serverid > 20000)
					return ERROR_INVALID_FORMAT;

				iType->id = serverid;
				break;
			}
			case ITEM_ATTR_CLIENTID:
			{
				if(datalen != sizeof(uint16_t))
					return ERROR_INVALID_FORMAT;

				uint16_t clientid;
				if(!props.GET_USHORT(clientid))
					return ERROR_INVALID_FORMAT;

				iType->clientId = clientid;
				break;
			}
			case ITEM_ATTR_SPEED:
			{
				if(datalen != sizeof(uint16_t))
					return ERROR_INVALID_FORMAT;

				uint16_t speed;
				if(!props.GET_USHORT(speed))
					return ERROR_INVALID_FORMAT;

				iType->speed = speed;

				break;
			}
			case ITEM_ATTR_LIGHT2:
			{
				if(datalen != sizeof(lightBlock2))
					return ERROR_INVALID_FORMAT;

				lightBlock2* lb2;
				if(!props.GET_STRUCT(lb2))
					return ERROR_INVALID_FORMAT;

				iType->lightLevel = lb2->lightLevel;
				iType->lightColor = lb2->lightColor;
				break;
			}
			case ITEM_ATTR_TOPORDER:
			{
				if(datalen != sizeof(uint8_t))
					return ERROR_INVALID_FORMAT;

				uint8_t v;
				if(!props.GET_UCHAR(v))
					return ERROR_INVALID_FORMAT;

				iType->alwaysOnTopOrder = v;
				break;
			}
			default:
				//skip unknown attributes
				if(!props.SKIP_N(datalen))
					return ERROR_INVALID_FORMAT;
				break;
			}
		}

		reverseItemMap[iType->clientId] = iType->id;

		// store the found item
		items.addElement(iType, iType->id);
		node = f.getNextNode(node, type);
	}

	return ERROR_NONE;
}

bool Items::loadFromXml(const std::string& datadir)
{
	m_datadir = datadir;
	std::string filename = m_datadir + "/items/items.xml";

	xmlDocPtr doc = xmlParseFile(filename.c_str());
	int intValue;
	std::string strValue;
	uint32_t id = 0;

	if(doc){
		xmlNodePtr root = xmlDocGetRootElement(doc);

		if(xmlStrcmp(root->name,(const xmlChar*)"items") != 0){
			xmlFreeDoc(doc);
			return false;
		}

		xmlNodePtr itemNode = root->children;
		while(itemNode){
			if(xmlStrcmp(itemNode->name,(const xmlChar*)"item") == 0){
				if(readXMLInteger(itemNode, "id", intValue)){
					id = intValue;

					if(id > 20000 && id < 20100){
						id = id - 20000;

						ItemType* iType = new ItemType();
						iType->id = id;
						items.addElement(iType, iType->id);
					}

					ItemType& it = Item::items.getItemType(id);

					if(readXMLString(itemNode, "name", strValue)){
						it.name = strValue;
					}

					if(readXMLString(itemNode, "article", strValue)){
						it.article = strValue;
					}

					if(readXMLString(itemNode, "plural", strValue)){
						it.pluralName = strValue;
					}

					xmlNodePtr itemAttributesNode = itemNode->children;

					while(itemAttributesNode){
						if(readXMLString(itemAttributesNode, "key", strValue)){
							if(strcasecmp(strValue.c_str(), "type") == 0){
								if(readXMLString(itemAttributesNode, "value", strValue)){
									if(strcasecmp(strValue.c_str(), "key") == 0){
										it.group = ITEM_GROUP_KEY;
									}
									else if(strcasecmp(strValue.c_str(), "magicfield") == 0){
										it.group = ITEM_GROUP_MAGICFIELD;
										it.type = ITEM_TYPE_MAGICFIELD;
									}
									else if(strcasecmp(strValue.c_str(), "depot") == 0){
										it.type = ITEM_TYPE_DEPOT;
									}
									else if(strcasecmp(strValue.c_str(), "mailbox") == 0){
										it.type = ITEM_TYPE_MAILBOX;
									}
									else if(strcasecmp(strValue.c_str(), "trashholder") == 0){
										it.type = ITEM_TYPE_TRASHHOLDER;
									}
									else{
										std::cout << "Warning: [Items::loadFromXml] " << "Unknown type " << strValue  << std::endl;
									}
								}
							}
							else if(strcasecmp(strValue.c_str(), "name") == 0){
								if(readXMLString(itemAttributesNode, "value", strValue)){
									it.name = strValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "article") == 0){
								if(readXMLString(itemAttributesNode, "value", strValue)){
									it.article = strValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "plural") == 0){
								if(readXMLString(itemAttributesNode, "value", strValue)){
									it.pluralName = strValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "description") == 0){
								if(readXMLString(itemAttributesNode, "value", strValue)){
									it.description = strValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "runeSpellName") == 0){
								if(readXMLString(itemAttributesNode, "value", strValue)){
									it.runeSpellName = strValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "weight") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.weight = intValue / 100.f;
								}
							}
							else if(strcasecmp(strValue.c_str(), "showcount") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.showCount = (intValue != 0);
								}
							}
							else if(strcasecmp(strValue.c_str(), "armor") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.armor = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "defense") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.defence = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "extradef") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.extraDef = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "attack") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.attack = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "rotateTo") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.rotateTo = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "containerSize") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.maxItems = intValue;
								}
							}
							/*
							else if(strcasecmp(strValue.c_str(), "readable") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.canReadText = true;
								}
							}
							*/
							else if(strcasecmp(strValue.c_str(), "writeable") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.canWriteText = (intValue != 0);
									it.canReadText = (intValue != 0);
								}
							}
							else if(strcasecmp(strValue.c_str(), "maxTextLen") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.maxTextLen = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "writeOnceItemId") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.writeOnceItemId = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "weaponType") == 0){
								if(readXMLString(itemAttributesNode, "value", strValue)){
									if(strcasecmp(strValue.c_str(), "sword") == 0){
										it.weaponType = WEAPON_SWORD;
									}
									else if(strcasecmp(strValue.c_str(), "club") == 0){
										it.weaponType = WEAPON_CLUB;
									}
									else if(strcasecmp(strValue.c_str(), "axe") == 0){
										it.weaponType = WEAPON_AXE;
									}
									else if(strcasecmp(strValue.c_str(), "shield") == 0){
										it.weaponType = WEAPON_SHIELD;
									}
									else if(strcasecmp(strValue.c_str(), "distance") == 0){
										it.weaponType = WEAPON_DIST;
									}
									else if(strcasecmp(strValue.c_str(), "wand") == 0){
										it.weaponType = WEAPON_WAND;
									}
									else if(strcasecmp(strValue.c_str(), "ammunition") == 0){
										it.weaponType = WEAPON_AMMO;
									}
									else{
										std::cout << "Warning: [Items::loadFromXml] " << "Unknown weaponType " << strValue  << std::endl;
									}
								}
							}
							else if(strcasecmp(strValue.c_str(), "slotType") == 0){
								if(readXMLString(itemAttributesNode, "value", strValue)){
									if(strcasecmp(strValue.c_str(), "head") == 0){
										it.slot_position |= SLOTP_HEAD;
									}
									else if(strcasecmp(strValue.c_str(), "body") == 0){
										it.slot_position |= SLOTP_ARMOR;
									}
									else if(strcasecmp(strValue.c_str(), "legs") == 0){
										it.slot_position |= SLOTP_LEGS;
									}
									else if(strcasecmp(strValue.c_str(), "feet") == 0){
										it.slot_position |= SLOTP_FEET;
									}
									else if(strcasecmp(strValue.c_str(), "backpack") == 0){
										it.slot_position |= SLOTP_BACKPACK;
									}
									else if(strcasecmp(strValue.c_str(), "two-handed") == 0){
										it.slot_position |= SLOTP_TWO_HAND;
									}
									else if(strcasecmp(strValue.c_str(), "necklace") == 0){
										it.slot_position |= SLOTP_NECKLACE;
									}
									else if(strcasecmp(strValue.c_str(), "ring") == 0){
										it.slot_position |= SLOTP_RING;
									}
									else{
										std::cout << "Warning: [Items::loadFromXml] " << "Unknown slotType " << strValue  << std::endl;
									}
								}
							}
							else if(strcasecmp(strValue.c_str(), "ammoType") == 0){
								if(readXMLString(itemAttributesNode, "value", strValue)){
									it.amuType = getAmmoType(strValue);
									if(it.amuType == AMMO_NONE){
										std::cout << "Warning: [Items::loadFromXml] " << "Unknown ammoType " << strValue  << std::endl;
									}
								}
							}
							else if(strcasecmp(strValue.c_str(), "shootType") == 0){
								if(readXMLString(itemAttributesNode, "value", strValue)){
									ShootType_t shoot = getShootType(strValue);
									if(shoot != NM_SHOOT_UNK){
										it.shootType = shoot;
									}
									else{
										std::cout << "Warning: [Items::loadFromXml] " << "Unknown shootType " << strValue  << std::endl;
									}
								}
							}
							else if(strcasecmp(strValue.c_str(), "effect") == 0){
								if(readXMLString(itemAttributesNode, "value", strValue)){
									MagicEffectClasses effect = getMagicEffect(strValue);
									if(effect != NM_ME_UNK){
										it.magicEffect = effect;
									}
									else{
										std::cout << "Warning: [Items::loadFromXml] " << "Unknown effect " << strValue  << std::endl;
									}
								}
							}
							else if(strcasecmp(strValue.c_str(), "range") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.shootRange = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "stopduration") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.stopTime = (intValue != 0);
								}
							}
							else if(strcasecmp(strValue.c_str(), "decayTo") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.decayTo = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "transformEquipTo") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.transformEquipTo = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "transformDeEquipTo") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.transformDeEquipTo = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "duration") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.decayTime = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "showduration") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.showDuration = (intValue != 0);
								}
							}
							else if(strcasecmp(strValue.c_str(), "charges") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.charges = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "showcharges") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.showCharges = (intValue != 0);
								}
							}
							else if(strcasecmp(strValue.c_str(), "breakChance") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									if(intValue < 0){
										intValue = 0;
									}
									else if(intValue > 100){
										intValue = 100;
									}

									it.breakChance = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "ammoAction") == 0){
								if(readXMLString(itemAttributesNode, "value", strValue)){
									it.ammoAction = getAmmoAction(strValue);

									if(it.ammoAction == AMMOACTION_NONE){
										std::cout << "Warning: [Items::loadFromXml] " << "Unknown ammoAction " << strValue  << std::endl;
									}
								}
							}
							else if(strcasecmp(strValue.c_str(), "hitChance") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									if(intValue < 0){
										intValue = 0;
									}
									else if(intValue > 100){
										intValue = 100;
									}

									it.hitChance = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "maxHitChance") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									if(intValue < 0){
										intValue = 0;
									}
									else if(intValue > 100){
										intValue = 100;
									}

									it.maxHitChance = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "invisible") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.invisible = (intValue != 0);
								}
							}
							else if(strcasecmp(strValue.c_str(), "speed") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.speed = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "healthGain") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.regeneration = true;
									it.abilities.healthGain = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "healthTicks") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.regeneration = true;
									it.abilities.healthTicks = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "manaGain") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.regeneration = true;
									it.abilities.manaGain = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "manaTicks") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.regeneration = true;
									it.abilities.manaTicks = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "manaShield") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.manaShield = (intValue != 0);
								}
							}
							else if(strcasecmp(strValue.c_str(), "skillSword") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.skills[SKILL_SWORD] = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "skillAxe") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.skills[SKILL_AXE] = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "skillClub") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.skills[SKILL_CLUB] = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "skillDist") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.skills[SKILL_DIST] = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "skillFish") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.skills[SKILL_FISH] = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "skillShield") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.skills[SKILL_SHIELD] = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "skillFist") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.skills[SKILL_FIST] = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "maxHitPoints") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.stats[STAT_MAXHITPOINTS] = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "maxHitPointsPercent") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.statsPercent[STAT_MAXHITPOINTS] = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "maxManaPoints") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.stats[STAT_MAXMANAPOINTS] = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "maxManaPointsPercent") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.statsPercent[STAT_MAXMANAPOINTS] = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "soulPoints") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.stats[STAT_SOULPOINTS] = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "soulPointsPercent") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.statsPercent[STAT_SOULPOINTS] = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "magicPoints") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.stats[STAT_MAGICPOINTS] = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "magicPointsPercent") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.statsPercent[STAT_MAGICPOINTS] = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "absorbPercentAll") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.absorbPercentAll = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "absorbPercentEnergy") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.absorbPercentEnergy = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "absorbPercentFire") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.absorbPercentFire = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "absorbPercentPoison") == 0 ||
									strcasecmp(strValue.c_str(), "absorbPercentEarth") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.absorbPercentEarth = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "absorbPercentIce") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.absorbPercentIce = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "absorbPercentHoly") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.absorbPercentHoly = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "absorbPercentDeath") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.absorbPercentDeath = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "absorbPercentLifeDrain") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.absorbPercentLifeDrain = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "absorbPercentManaDrain") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.absorbPercentManaDrain = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "absorbPercentDrown") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.absorbPercentDrown = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "absorbPercentPhysical") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.absorbPercentPhysical = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "suppressDrunk") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.conditionSuppressions |= CONDITION_DRUNK;
								}
							}
							else if(strcasecmp(strValue.c_str(), "suppressEnergy") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.conditionSuppressions |= CONDITION_ENERGY;
								}
							}
							else if(strcasecmp(strValue.c_str(), "suppressFire") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.conditionSuppressions |= CONDITION_FIRE;
								}
							}
							else if(strcasecmp(strValue.c_str(), "suppressPoison") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.conditionSuppressions |= CONDITION_POISON;
								}
							}
							else if(strcasecmp(strValue.c_str(), "suppressLifeDrain") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.conditionSuppressions |= CONDITION_LIFEDRAIN;
								}
							}
							else if(strcasecmp(strValue.c_str(), "suppressDrown") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.conditionSuppressions |= CONDITION_DROWN;
								}
							}
							else if(strcasecmp(strValue.c_str(), "suppressFreeze") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue) && intValue != 0){
									it.abilities.conditionSuppressions |= CONDITION_FREEZING;
								}
							}

							else if(strcasecmp(strValue.c_str(), "suppressDazzle") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue) && intValue != 0){
									it.abilities.conditionSuppressions |= CONDITION_DAZZLED;
								}
							}

							else if(strcasecmp(strValue.c_str(), "suppressCurse") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue) && intValue != 0){
									it.abilities.conditionSuppressions |= CONDITION_CURSED;
								}
							}
							/*else if(strcasecmp(strValue.c_str(), "suppressManaDrain") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.conditionSuppressions |= CONDITION_MANADRAIN;
								}
							}
							else if(strcasecmp(strValue.c_str(), "suppressPhysical") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.conditionSuppressions |= CONDITION_PHYSICAL;
								}
							}*/
							else if(strcasecmp(strValue.c_str(), "field") == 0){
								it.group = ITEM_GROUP_MAGICFIELD;
								it.type = ITEM_TYPE_MAGICFIELD;
								CombatType_t combatType = COMBAT_NONE;
								ConditionDamage* conditionDamage = NULL;

								if(readXMLString(itemAttributesNode, "value", strValue)){
									if(strcasecmp(strValue.c_str(), "fire") == 0){
										conditionDamage = new ConditionDamage(CONDITIONID_COMBAT, CONDITION_FIRE);
										combatType = COMBAT_FIREDAMAGE;
									}
									else if(strcasecmp(strValue.c_str(), "energy") == 0){
										conditionDamage = new ConditionDamage(CONDITIONID_COMBAT, CONDITION_ENERGY);
										combatType = COMBAT_ENERGYDAMAGE;
									}
									else if(strcasecmp(strValue.c_str(), "poison") == 0){
										conditionDamage = new ConditionDamage(CONDITIONID_COMBAT, CONDITION_POISON);
										combatType = COMBAT_EARTHDAMAGE;
									}
									else if(strcasecmp(strValue.c_str(), "drown") == 0){
										conditionDamage = new ConditionDamage(CONDITIONID_COMBAT, CONDITION_DROWN);
										combatType = COMBAT_DROWNDAMAGE;
									}
									//else if(strcasecmp(strValue.c_str(), "physical") == 0){
									//	damageCondition = new ConditionDamage(CONDITIONID_COMBAT, CONDITION_PHYSICAL);
									//	combatType = COMBAT_PHYSICALDAMAGE;
									//}
									else{
										std::cout << "Warning: [Items::loadFromXml] " << "Unknown field value " << strValue  << std::endl;
									}

									if(combatType != COMBAT_NONE){
										it.combatType = combatType;
										it.condition = conditionDamage;
										it.condition->setParam(CONDITIONPARAM_FORCEUPDATE, true);
										uint32_t ticks = 0;
										int32_t damage = 0;
										int32_t start = 0;
										int32_t count = 1;

										xmlNodePtr fieldAttributesNode = itemAttributesNode->children;
										while(fieldAttributesNode){
											if(readXMLString(fieldAttributesNode, "key", strValue)){
												if(strcasecmp(strValue.c_str(), "ticks") == 0){
													if(readXMLInteger(fieldAttributesNode, "value", intValue)){
														ticks = std::max(0, intValue);
													}
												}

												if(strcasecmp(strValue.c_str(), "count") == 0){
													if(readXMLInteger(fieldAttributesNode, "value", intValue)){
														if(intValue > 0){
															count = intValue;
														}
														else{
															count = 1;
														}
													}
												}

												if(strcasecmp(strValue.c_str(), "start") == 0){
													if(readXMLInteger(fieldAttributesNode, "value", intValue)){
														if(intValue > 0){
															start = intValue;
														}
														else{
															start = 0;
														}
													}
												}

												if(strcasecmp(strValue.c_str(), "damage") == 0){
													if(readXMLInteger(fieldAttributesNode, "value", intValue)){

														damage = -intValue;

														if(start > 0){
															std::list<int32_t> damageList;
															ConditionDamage::generateDamageList(damage, start, damageList);

															for(std::list<int32_t>::iterator it = damageList.begin(); it != damageList.end(); ++it){
																conditionDamage->addDamage(1, ticks, -*it);
															}

															start = 0;
														}
														else{
															conditionDamage->addDamage(count, ticks, damage);
														}
													}
												}
											}

											fieldAttributesNode = fieldAttributesNode->next;
										}
									}
								}
							}
							else if(strcasecmp(strValue.c_str(), "replaceable") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.replaceable = (intValue != 0);
								}
							}
							else{
								std::cout << "Warning: [Items::loadFromXml] Unknown key value " << strValue  << std::endl;
							}
						}

						itemAttributesNode = itemAttributesNode->next;
					}
					// if no plural is specified we will build the default
					// plural adding "s" at the end
					if(it.pluralName.size() == 0 && it.name.size() != 0){
						it.pluralName = it.name + "s";
					}
				}
				else{
					std::cout << "Warning: [Spells::loadFromXml] - No itemid found" << std::endl;
				}
			}

			itemNode = itemNode->next;
		}

		xmlFreeDoc(doc);
	}

	return true;
}

ItemType& Items::getItemType(int32_t id)
{
	ItemType* iType = items.getElement(id);
	if(iType){
		return *iType;
	}
	else{
		#ifdef __DEBUG__
		std::cout << "WARNING! unknown itemtypeid " << id << ". using defaults." << std::endl;
		#endif
		static ItemType dummyItemType; // use this for invalid ids
		return dummyItemType;
	}
}

const ItemType& Items::getItemType(int32_t id) const
{
	ItemType* iType = items.getElement(id);
	if(iType){
		return *iType;
	}
	else{
		static ItemType dummyItemType; // use this for invalid ids
		return dummyItemType;
	}
}

const ItemType& Items::getItemIdByClientId(int32_t spriteId) const
{
	uint32_t i = 100;
	ItemType* iType;
	do{
		iType = items.getElement(i);
		if(iType && iType->clientId == spriteId){
			return *iType;
		}
		i++;
	}while(iType);

	static ItemType dummyItemType; // use this for invalid ids
	return dummyItemType;
}

int32_t Items::getItemIdByName(const std::string& name)
{
	if(!name.empty()){
		uint32_t i = 100;
		ItemType* iType;
		do{
			iType = items.getElement(i);
			if(iType){
				if(strcasecmp(name.c_str(), iType->name.c_str()) == 0){
					return i;
				}
			}
			i++;
		}while(iType);
	}
	return -1;
}

template<typename A>
Array<A>::Array(uint32_t n)
{
	m_data = (A*)malloc(sizeof(A)*n);
	memset(m_data, 0, sizeof(A)*n);
	m_size = n;
}

template<typename A>
Array<A>::~Array()
{
	free(m_data);
}

template<typename A>
A Array<A>::getElement(uint32_t id)
{
	if(id < m_size){
		return m_data[id];
	}
	else{
		return 0;
	}
}

template<typename A>
const A Array<A>::getElement(uint32_t id) const
{
	if(id < m_size){
		return m_data[id];
	}
	else{
		return 0;
	}
}

template<typename A>
void Array<A>::addElement(A a, uint32_t pos)
{
	#define INCREMENT 5000
	if(pos >= m_size){
		m_data = (A*)realloc(m_data, sizeof(A)*(pos + INCREMENT));
		memset(m_data + m_size, 0, sizeof(A)*(pos + INCREMENT - m_size));
		m_size = pos + INCREMENT;
	}
	m_data[pos] = a;
}
