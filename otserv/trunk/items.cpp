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
	group           = ITEM_GROUP_NONE;

	RWInfo          = 0;
	readOnlyId      = 0;
	stackable       = false;
	useable	        = false;
	moveable        = true;
	alwaysOnTop     = false;
	alwaysOnTopOrder = 0;
	pickupable      = false;
	rotable         = false;
	rotateTo		= 0;
	hasHeight       = false;

	floorChangeDown = true;
	floorChangeNorth = false;
	floorChangeSouth = false;
	floorChangeEast = false;
	floorChangeWest = false;

	blockSolid = false;
	blockProjectile = false;
	blockPathFind = false;
		
	runeMagLevel    = -1;
	magicfieldtype = -1;
	
	speed		      = 0;
	id            = 0;
	clientId      = 0;
	maxItems      = 8;  // maximum size if this is a container
	weight        = 0;  // weight of the item, e.g. throwing distance depends on it
	weaponType    = WEAPON_NONE;
	slot_position = SLOTP_RIGHT | SLOTP_LEFT | SLOTP_AMMO;
	amuType       = AMMO_NONE;
	shootType     = SHOOT_NONE;
	attack        = 0;
	defence       = 0;
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

	transformEquipTo   = 0;
	transformDeEquipTo = 0;
	showDuration  = false;
	showCharges   = false;
	charges       = 0;
	breakChance   = 0;

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
	else if(Items::dwMinorVersion != CLIENT_VERSION_792){
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
			case ITEM_GROUP_NONE:
			case ITEM_GROUP_GROUND:
			case ITEM_GROUP_CONTAINER:
			case ITEM_GROUP_RUNE:
			case ITEM_GROUP_TELEPORT:
			case ITEM_GROUP_MAGICFIELD:
			case ITEM_GROUP_SPLASH:
			case ITEM_GROUP_FLUID:
			case ITEM_GROUP_DOOR:
				break;
			default:
				return ERROR_INVALID_FORMAT;
				break;
		}

		//read 4 byte flags
		if(!props.GET_VALUE(flags)){
			return ERROR_INVALID_FORMAT;
		}

		iType->blockSolid = ((flags & FLAG_BLOCK_SOLID) == FLAG_BLOCK_SOLID);
		iType->blockProjectile = ((flags & FLAG_BLOCK_PROJECTILE) == FLAG_BLOCK_PROJECTILE);
		iType->blockPathFind = ((flags & FLAG_BLOCK_PATHFIND) == FLAG_BLOCK_PATHFIND);
		iType->hasHeight = ((flags & FLAG_HAS_HEIGHT) == FLAG_HAS_HEIGHT);
		iType->useable = ((flags & FLAG_USEABLE) == FLAG_USEABLE);
		iType->pickupable = ((flags & FLAG_PICKUPABLE) == FLAG_PICKUPABLE);
		iType->moveable = ((flags & FLAG_MOVEABLE) == FLAG_MOVEABLE);
		iType->stackable = ((flags & FLAG_STACKABLE) == FLAG_STACKABLE);
		iType->floorChangeDown = ((flags & FLAG_FLOORCHANGEDOWN) == FLAG_FLOORCHANGEDOWN);
		iType->floorChangeNorth = ((flags & FLAG_FLOORCHANGENORTH) == FLAG_FLOORCHANGENORTH);
		iType->floorChangeEast = ((flags & FLAG_FLOORCHANGEEAST) == FLAG_FLOORCHANGEEAST);
		iType->floorChangeSouth = ((flags & FLAG_FLOORCHANGESOUTH) == FLAG_FLOORCHANGESOUTH);
		iType->floorChangeWest = ((flags & FLAG_FLOORCHANGEWEST) == FLAG_FLOORCHANGEWEST);
		iType->alwaysOnTop = ((flags & FLAG_ALWAYSONTOP) == FLAG_ALWAYSONTOP);
		iType->isVertical = ((flags & FLAG_VERTICAL) == FLAG_VERTICAL);
		iType->isHorizontal = ((flags & FLAG_HORIZONTAL) == FLAG_HORIZONTAL);
		iType->isHangable = ((flags & FLAG_HANGABLE) == FLAG_HANGABLE);
		iType->allowDistRead = ((flags & FLAG_ALLOWDISTREAD) == FLAG_ALLOWDISTREAD);
		iType->rotable = ((flags & FLAG_ROTABLE) == FLAG_ROTABLE);

		if((flags & FLAG_READABLE) == FLAG_READABLE)
			iType->RWInfo |= CAN_BE_READ;

		
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
									}
									else if(strcasecmp(strValue.c_str(), "depot") == 0){
										//it.group = ITEM_GROUP_DEPOT;
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
							else if(strcasecmp(strValue.c_str(), "description") == 0){
								if(readXMLString(itemAttributesNode, "value", strValue)){
									it.description = strValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "weight") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.weight = intValue / 100.f;
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
							else if(strcasecmp(strValue.c_str(), "maxTextLen") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.maxTextLen = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "readOnceItemId") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.readOnlyId = intValue;
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
									if(strcasecmp(strValue.c_str(), "arrow") == 0){
										it.amuType = AMMO_ARROW;
									}
									else if(strcasecmp(strValue.c_str(), "bolt") == 0){
										it.amuType = AMMO_BOLT;
									}
									else{
										std::cout << "Warning: [Items::loadFromXml] " << "Unknown ammoType " << strValue  << std::endl;
									}
								}
							}
							else if(strcasecmp(strValue.c_str(), "shootType") == 0){
								if(readXMLString(itemAttributesNode, "value", strValue)){
									if(strcasecmp(strValue.c_str(), "spear") == 0){
										it.shootType = SHOOT_SPEAR;
									}
									else if(strcasecmp(strValue.c_str(), "bolt") == 0){
										it.shootType = SHOOT_BOLT;
									}
									else if(strcasecmp(strValue.c_str(), "arrow") == 0){
										it.shootType = SHOOT_ARROW;
									}
									else if(strcasecmp(strValue.c_str(), "fire") == 0){
										it.shootType = SHOOT_FIRE;
									}
									else if(strcasecmp(strValue.c_str(), "energy") == 0){
										it.shootType = SHOOT_ENERGY;
									}
									else if(strcasecmp(strValue.c_str(), "poisonarrow") == 0){
										it.shootType = SHOOT_POISONARROW;
									}
									else if(strcasecmp(strValue.c_str(), "burstarrow") == 0){
										it.shootType = SHOOT_BURSTARROW;
									}
									else if(strcasecmp(strValue.c_str(), "throwingstar") == 0){
										it.shootType = SHOOT_THROWINGSTAR;
									}
									else if(strcasecmp(strValue.c_str(), "throwingknife") == 0){
										it.shootType = SHOOT_THROWINGKNIFE;
									}
									else if(strcasecmp(strValue.c_str(), "smallstone") == 0){
										it.shootType = SHOOT_SMALLSTONE;
									}
									else if(strcasecmp(strValue.c_str(), "suddendeath") == 0){
										it.shootType = SHOOT_SUDDENDEATH;
									}
									else if(strcasecmp(strValue.c_str(), "largerock") == 0){
										it.shootType = SHOOT_LARGEROCK;
									}
									else if(strcasecmp(strValue.c_str(), "snowball") == 0){
										it.shootType = SHOOT_SNOWBALL;
									}
									else if(strcasecmp(strValue.c_str(), "powerbolt") == 0){
										it.shootType = SHOOT_POWERBOLT;
									}
									else if(strcasecmp(strValue.c_str(), "poison") == 0){
										it.shootType = SHOOT_POISONFIELD;
									}
									else if(strcasecmp(strValue.c_str(), "infernalbolt") == 0){
										it.shootType = SHOOT_INFERNALBOLT;
									}
									else{
										std::cout << "Warning: [Items::loadFromXml] " << "Unknown shootType " << strValue  << std::endl;
									}
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
									it.showDuration = true;
								}
							}
							else if(strcasecmp(strValue.c_str(), "charges") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.charges = intValue;
								}
							}
							else if(strcasecmp(strValue.c_str(), "showcharges") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.showCharges = true;
								}
							}
							else if(strcasecmp(strValue.c_str(), "invisible") == 0){
								it.abilities.invisible = true;
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
									it.abilities.manaShield = true;
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
							else if(strcasecmp(strValue.c_str(), "absorbPercentPoison") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.absorbPercentPoison = intValue;
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
							/*
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
							else if(strcasecmp(strValue.c_str(), "suppressManaDrain") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.conditionSuppressions |= CONDITION_MANADRAIN;
								}
							}
							else if(strcasecmp(strValue.c_str(), "suppressPhysical") == 0){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.conditionSuppressions |= CONDITION_PHYSICAL;
								}
							}
							*/
							else if(strcasecmp(strValue.c_str(), "field") == 0){
								it.group = ITEM_GROUP_MAGICFIELD;
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
										combatType = COMBAT_POISONDAMAGE;
									}
									//else if(strcasecmp(strValue.c_str(), "physical") == 0){
									//	damageCondition = new ConditionDamage(CONDITIONID_COMBAT, CONDITION_PHYSICAL);
									//	combatType = COMBAT_PHYSICALDAMAGE;
									//}

									if(combatType != COMBAT_NONE){
										it.combatType = combatType;
										it.condition = conditionDamage;
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
						}

						itemAttributesNode = itemAttributesNode->next;
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

ItemType& Items::getItemIdByClientId(int32_t spriteId)
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

#ifdef __DEBUG__
	std::cout << "WARNING! unknown sprite id " << spriteId << ". using defaults." << std::endl;
	#endif
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
	if(id >= 0 && id < m_size){
		return m_data[id];
	}
	else{
		return 0;
	}
}

template<typename A>
void Array<A>::addElement(A a, uint32_t pos)
{
	if(pos >= 0){
		#define INCREMENT 5000
		if(pos >= m_size){
			m_data = (A*)realloc(m_data, sizeof(A)*(pos + INCREMENT));
			memset(m_data + m_size, 0, sizeof(A)*(pos + INCREMENT - m_size));
			m_size = pos + INCREMENT;
		}
		m_data[pos] = a;
	}
}
