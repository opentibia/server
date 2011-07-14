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
#include <libxml/xmlschemas.h>
#include <boost/algorithm/string/predicate.hpp>
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

	floorChangeDown = false;
	floorChangeNorth = false;
	floorChangeSouth = false;
	floorChangeEast = false;
	floorChangeWest = false;

	blockSolid = false;
	blockProjectile = false;
	blockPathFind = false;
	allowPickupable = false;

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
	wield_position= SLOT_HAND;
	amuType       = AMMO_NONE;
	ammoAction    = AMMOACTION_NONE;
	shootType     = (ShootType_t)0;
	magicEffect   = NM_ME_NONE;
	attack        = 0;
	defense       = 0;
	extraDef      = 0;
	armor         = 0;
	decayTo       = -1;
	decayTime     = 0;
	stopTime      = false;
	corpseType    = RACE_NONE;
	fluidSource  = -1;
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
	//[ added for beds system
	bedPartnerDir = NORTH;
	maleSleeperID = 0;
	femaleSleeperID = 0;
	noSleeperID = 0;
	//]

	currency = 0;
}

ItemType::~ItemType()
{
	delete condition;
}

std::string ItemType::getDescription(uint8_t count) const
{
	int32_t subType = 0;
	if(isFluidContainer()){
		int32_t maxFluidType = sizeof(reverseFluidMap) / sizeof(uint8_t);
		if(count < maxFluidType){
			subType = reverseFluidMap[count];
		}
	}
	else{
		subType = count;
	}
	return Item::getDescription(*this, 1, NULL, subType);
}


Items::Items() :
items(8000)
{
	//
	currencyMap.clear();
}

Items::~Items()
{
	clear();
}

void Items::clear()
{
	currencyMap.clear();
	//TODO. clear items?
}

bool Items::reload()
{
	clear();
	/*items.clear();
	if(!items.size())
		return false;*/

	if(loadFromOtb(m_datadir))
		return loadFromXml(m_datadir);

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
		if(!props.GET_UINT32(flags)){
			return ERROR_INVALID_FORMAT;
		}
		attribute_t attr;
		if(!props.GET_UINT8(attr)){
			return ERROR_INVALID_FORMAT;
		}
		if(attr == ROOT_ATTR_VERSION){
			datasize_t datalen = 0;
			if(!props.GET_UINT16(datalen)){
				return ERROR_INVALID_FORMAT;
			}
			if(datalen != sizeof(VERSIONINFO)){
				return ERROR_INVALID_FORMAT;
			}

			VERSIONINFO vi;
			if(		!props.GET_UINT32(vi.dwMajorVersion) ||
					!props.GET_UINT32(vi.dwMinorVersion) ||
					!props.GET_UINT32(vi.dwBuildNumber) ||
					!props.GET_RAWSTRING((char*)&vi.CSDVersion, sizeof(vi.CSDVersion)))
			{
				return ERROR_INVALID_FORMAT;
			}
			Items::dwMajorVersion = vi.dwMajorVersion;	//items otb format file version
			Items::dwMinorVersion = vi.dwMinorVersion;	//client version
			Items::dwBuildNumber = vi.dwBuildNumber;	//revision
		}
	}

	if(Items::dwMajorVersion == 0xFFFFFFFF){
		std::cout << "[Warning] Items::loadFromOtb items.otb using generic client version." << std::endl;
	}
	else if(Items::dwMajorVersion < 3){
		std::cout << "Old version of items.otb detected, a newer version of items.otb is required." << std::endl;
		return ERROR_INVALID_FORMAT;
	}
	else if(Items::dwMajorVersion > 3){
		std::cout << "New version of items.otb detected, a newer version of the server is required." << std::endl;
		return ERROR_INVALID_FORMAT;
	}
	else if(Items::dwMinorVersion != CLIENT_VERSION_870){
		std::cout << "Another (client) version of items.otb is required." << std::endl;
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
				//not used
				iType->type = ITEM_TYPE_DOOR;
				break;
			case ITEM_GROUP_MAGICFIELD:
				//not used
				iType->type = ITEM_TYPE_MAGICFIELD;
				break;
			case ITEM_GROUP_TELEPORT:
				//not used
				iType->type = ITEM_TYPE_TELEPORT;
				break;
			case ITEM_GROUP_NONE:
			case ITEM_GROUP_GROUND:
			case ITEM_GROUP_CHARGES:
			case ITEM_GROUP_SPLASH:
			case ITEM_GROUP_FLUID:
			case ITEM_GROUP_DEPRECATED:
				break;
			default:
				return ERROR_INVALID_FORMAT;
				break;
		}

		//read 4 byte flags
		if(!props.GET_UINT32(flags)){
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
		iType->alwaysOnTop = hasBitSet(FLAG_ALWAYSONTOP, flags);
		iType->isVertical = hasBitSet(FLAG_VERTICAL, flags);
		iType->isHorizontal = hasBitSet(FLAG_HORIZONTAL, flags);
		iType->isHangable = hasBitSet(FLAG_HANGABLE, flags);
		iType->allowDistRead = hasBitSet(FLAG_ALLOWDISTREAD, flags);
		iType->rotable = hasBitSet(FLAG_ROTABLE, flags);
		iType->canReadText = hasBitSet(FLAG_READABLE, flags);
		iType->lookThrough = hasBitSet(FLAG_LOOKTHROUGH, flags);

		attribute_t attrib;
		datasize_t datalen = 0;
		while(props.GET_UINT8(attrib)){
			//size of data
			if(!props.GET_UINT16(datalen)){
				delete iType;
				return ERROR_INVALID_FORMAT;
			}
			switch(attrib){
			case ITEM_ATTR_SERVERID:
			{
				if(datalen != sizeof(uint16_t))
					return ERROR_INVALID_FORMAT;

				uint16_t serverid;
				if(!props.GET_UINT16(serverid))
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
				if(!props.GET_UINT16(clientid))
					return ERROR_INVALID_FORMAT;

				iType->clientId = clientid;
				break;
			}
			case ITEM_ATTR_SPEED:
			{
				if(datalen != sizeof(uint16_t))
					return ERROR_INVALID_FORMAT;

				uint16_t speed;
				if(!props.GET_UINT16(speed))
					return ERROR_INVALID_FORMAT;

				iType->speed = speed;

				break;
			}
			case ITEM_ATTR_LIGHT2:
			{
				if(datalen != sizeof(lightBlock2))
					return ERROR_INVALID_FORMAT;

				lightBlock2 lb2;
				if(		!props.GET_UINT16(lb2.lightLevel) ||
						!props.GET_UINT16(lb2.lightColor))
				{
					return ERROR_INVALID_FORMAT;
				}

				iType->lightLevel = lb2.lightLevel;
				iType->lightColor = lb2.lightColor;
				break;
			}
			case ITEM_ATTR_TOPORDER:
			{
				if(datalen != sizeof(uint8_t))
					return ERROR_INVALID_FORMAT;

				uint8_t v;
				if(!props.GET_UINT8(v))
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
	std::string xmlSchema = m_datadir + "/items/items.xsd";

	xmlDocPtr doc = xmlParseFile(filename.c_str());
	int intValue;
	std::string strValue;
	uint32_t id = 0;

	if(doc == NULL){
		return false;
	}

	//validation against xml-schema
	xmlDocPtr schemaDoc = xmlReadFile(xmlSchema.c_str(), NULL, XML_PARSE_NONET);
	if(schemaDoc != NULL){
		xmlSchemaParserCtxtPtr schemaParserContext = xmlSchemaNewDocParserCtxt(schemaDoc);
		if(schemaParserContext != NULL){
			xmlSchemaPtr schema = xmlSchemaParse(schemaParserContext);
			if(schema != NULL){
				xmlSchemaValidCtxtPtr validContext = xmlSchemaNewValidCtxt(schema);
				if(validContext != NULL){
					int returnVal = xmlSchemaValidateDoc(validContext, doc);
					if(returnVal != 0){
						if(returnVal > 0){
							std::cout << std::endl << "Warning: [XMLSCHEMA] items.xml could not be validated against XSD" << std::endl;
						}
						else{
							std::cout << std::endl << "Warning: [XMLSCHEMA] validation generated an internal error." << std::endl;
						}
					}
					xmlSchemaFreeValidCtxt(validContext);
				}
				xmlSchemaFree(schema);
			}
			xmlSchemaFreeParserCtxt(schemaParserContext);
		}
		xmlFreeDoc(schemaDoc);
	}

	xmlNodePtr root = xmlDocGetRootElement(doc);

	if(xmlStrcmp(root->name,(const xmlChar*)"items") != 0){
		xmlFreeDoc(doc);
		return false;
	}
	
	std::map<uint16_t, bool> itemsLoaded;
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
				
				else if(itemsLoaded[id]){
					std::cout << std::endl << "Duplicate itemid \"" << intValue << "\"";
					itemNode = itemNode->next;
					continue;
				}

				itemsLoaded[id] = true;
				
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
						if(asLowerCaseString(strValue) == "type"){
							if(readXMLString(itemAttributesNode, "value", strValue)){
								if(asLowerCaseString(strValue) == "container"){
									it.group = ITEM_GROUP_CONTAINER;
									it.type = ITEM_TYPE_CONTAINER;
								}
								else if(asLowerCaseString(strValue) == "key"){
									it.type = ITEM_TYPE_KEY;
								}
								else if(asLowerCaseString(strValue) == "magicfield"){
									it.type = ITEM_TYPE_MAGICFIELD;
								}
								else if(asLowerCaseString(strValue) == "depot"){
									it.type = ITEM_TYPE_DEPOT;
								}
								else if(asLowerCaseString(strValue) == "mailbox"){
									it.type = ITEM_TYPE_MAILBOX;
								}
								else if(asLowerCaseString(strValue) == "trashholder"){
									it.type = ITEM_TYPE_TRASHHOLDER;
								}
								else if(asLowerCaseString(strValue) == "teleport"){
									it.type = ITEM_TYPE_TELEPORT;
								}
								else if(asLowerCaseString(strValue) == "door"){
									it.type = ITEM_TYPE_DOOR;
								}
								else if(asLowerCaseString(strValue) == "bed"){
									it.type = ITEM_TYPE_BED;
								}
								else if(asLowerCaseString(strValue) == "rune"){
									it.type = ITEM_TYPE_RUNE;
								}
								else{
									std::cout << "Warning: [Items::loadFromXml] " << "Unknown type " << strValue  << std::endl;
								}
							}
						}
						else if(asLowerCaseString(strValue) == "name"){
							if(readXMLString(itemAttributesNode, "value", strValue)){
								it.name = strValue;
							}
						}
						else if(asLowerCaseString(strValue) == "article"){
							if(readXMLString(itemAttributesNode, "value", strValue)){
								it.article = strValue;
							}
						}
						else if(asLowerCaseString(strValue) == "plural"){
							if(readXMLString(itemAttributesNode, "value", strValue)){
								it.pluralName = strValue;
							}
						}
						else if(asLowerCaseString(strValue) == "description"){
							if(readXMLString(itemAttributesNode, "value", strValue)){
								it.description = strValue;
							}
						}
						else if(asLowerCaseString(strValue) == "runespellname"){
							if(readXMLString(itemAttributesNode, "value", strValue)){
								it.runeSpellName = strValue;
							}
						}
						else if(asLowerCaseString(strValue) == "weight"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.weight = intValue / 100.f;
							}
						}
						else if(asLowerCaseString(strValue) == "showcount"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.showCount = (intValue != 0);
							}
						}
						else if(asLowerCaseString(strValue) == "armor"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.armor = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "defense"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.defense = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "extradef"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.extraDef = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "attack"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.attack = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "rotateto"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.rotateTo = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "moveable"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.moveable = (intValue == 1);
							}
						}
						else if(asLowerCaseString(strValue) == "pickupable"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.pickupable = (intValue == 1);
							}
						}
						else if(asLowerCaseString(strValue) == "blockprojectile"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.blockProjectile = (intValue == 1);
							}
						}
						else if(asLowerCaseString(strValue) == "allowpickupable"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.allowPickupable = (intValue == 1);
							}
						}
						else if(asLowerCaseString(strValue) == "floorchange"){
							if(readXMLString(itemAttributesNode, "value", strValue)){
								if(asLowerCaseString(strValue) == "down"){
									it.floorChangeDown = true;
								}
								else if(asLowerCaseString(strValue) == "north"){
									it.floorChangeNorth = true;
								}
								else if(asLowerCaseString(strValue) == "south"){
									it.floorChangeSouth = true;
								}
								else if(asLowerCaseString(strValue) == "west"){
									it.floorChangeWest = true;
								}
								else if(asLowerCaseString(strValue) == "east"){
									it.floorChangeEast = true;
								}
							}
						}
						else if(asLowerCaseString(strValue) == "corpsetype"){
							if(readXMLString(itemAttributesNode, "value", strValue)){
								if(asLowerCaseString(strValue) == "venom"){
									it.corpseType = RACE_VENOM;
								}
								else if(asLowerCaseString(strValue) == "blood"){
									it.corpseType = RACE_BLOOD;
								}
								else if(asLowerCaseString(strValue) == "undead"){
									it.corpseType = RACE_UNDEAD;
								}
								else if(asLowerCaseString(strValue) == "fire"){
									it.corpseType = RACE_FIRE;
								}
								else if(asLowerCaseString(strValue) == "energy"){
									it.corpseType = RACE_ENERGY;
								}
							}
						}
						else if(asLowerCaseString(strValue) == "containersize"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.maxItems = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "fluidsource"){
							if(readXMLString(itemAttributesNode, "value", strValue)){
								if(asLowerCaseString(strValue) == "water"){
									it.fluidSource = FLUID_WATER;
								}
								else if(asLowerCaseString(strValue) == "blood"){
									it.fluidSource = FLUID_BLOOD;
								}
								else if(asLowerCaseString(strValue) == "beer"){
									it.fluidSource = FLUID_BEER;
								}
								else if(asLowerCaseString(strValue) == "slime"){
									it.fluidSource = FLUID_SLIME;
								}
								else if(asLowerCaseString(strValue) == "lemonade"){
									it.fluidSource = FLUID_LEMONADE;
								}
								else if(asLowerCaseString(strValue) == "milk"){
									it.fluidSource = FLUID_MILK;
								}
								else if(asLowerCaseString(strValue) == "mana"){
									it.fluidSource = FLUID_MANA;
								}
								else if(asLowerCaseString(strValue) == "life"){
									it.fluidSource = FLUID_LIFE;
								}
								else if(asLowerCaseString(strValue) == "oil"){
									it.fluidSource = FLUID_OIL;
								}
								else if(asLowerCaseString(strValue) == "urine"){
									it.fluidSource = FLUID_URINE;
								}
								else if(asLowerCaseString(strValue) == "coconut"){
									it.fluidSource = FLUID_COCONUTMILK;
								}
								else if(asLowerCaseString(strValue) == "wine"){
									it.fluidSource = FLUID_WINE;
								}
								else if(asLowerCaseString(strValue) == "mud"){
									it.fluidSource = FLUID_MUD;
								}
								else if(asLowerCaseString(strValue) == "fruitjuice"){
									it.fluidSource = FLUID_FRUITJUICE;
								}
								else if(asLowerCaseString(strValue) == "lava"){
									it.fluidSource = FLUID_LAVA;
								}
								else if(asLowerCaseString(strValue) == "rum"){
									it.fluidSource = FLUID_RUM;
								}
								else if(asLowerCaseString(strValue) == "swamp"){
									it.fluidSource = FLUID_SWAMP;
								}
								else{
									std::cout << "Warning: [Items::loadFromXml] " << "Unknown fluidSource " << strValue  << std::endl;
								}
							}
						}
						else if(asLowerCaseString(strValue) == "readable"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.canReadText = true;
							}
						}
						else if(asLowerCaseString(strValue) == "writeable"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.canWriteText = (intValue != 0);
								it.canReadText = (intValue != 0);
							}
						}
						else if(asLowerCaseString(strValue) == "maxtextlen"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.maxTextLen = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "allowdistread"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.allowDistRead = intValue != 0;
							}
						}
						else if(asLowerCaseString(strValue) == "writeonceitemid"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.writeOnceItemId = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "weapontype"){
							if(readXMLString(itemAttributesNode, "value", strValue)){
								if(asLowerCaseString(strValue) == "sword"){
									it.weaponType = WEAPON_SWORD;
								}
								else if(asLowerCaseString(strValue) == "club"){
									it.weaponType = WEAPON_CLUB;
								}
								else if(asLowerCaseString(strValue) == "axe"){
									it.weaponType = WEAPON_AXE;
								}
								else if(asLowerCaseString(strValue) == "shield"){
									it.weaponType = WEAPON_SHIELD;
								}
								else if(asLowerCaseString(strValue) == "distance"){
									it.weaponType = WEAPON_DIST;
								}
								else if(asLowerCaseString(strValue) == "wand"){
									it.weaponType = WEAPON_WAND;
								}
								else if(asLowerCaseString(strValue) == "ammunition"){
									it.weaponType = WEAPON_AMMO;
								}
								else{
									std::cout << "Warning: [Items::loadFromXml] " << "Unknown weaponType " << strValue  << std::endl;
								}
							}
						}
						else if(asLowerCaseString(strValue) == "slottype"){
							if(readXMLString(itemAttributesNode, "value", strValue)){
								if(asLowerCaseString(strValue) == "head"){
									it.slot_position |= SLOTP_HEAD;
									it.wield_position = SLOT_HEAD;
								}
								else if(asLowerCaseString(strValue) == "body"){
									it.slot_position |= SLOTP_ARMOR;
									it.wield_position = SLOT_ARMOR;
								}
								else if(asLowerCaseString(strValue) == "legs"){
									it.slot_position |= SLOTP_LEGS;
									it.wield_position = SLOT_LEGS;
								}
								else if(asLowerCaseString(strValue) == "feet"){
									it.slot_position |= SLOTP_FEET;
									it.wield_position = SLOT_FEET;
								}
								else if(asLowerCaseString(strValue) == "backpack"){
									it.slot_position |= SLOTP_BACKPACK;
									it.wield_position = SLOT_BACKPACK;
								}
								else if(asLowerCaseString(strValue) == "two-handed"){
									it.slot_position |= SLOTP_TWO_HAND;
									it.wield_position = SLOT_HAND;
								}
								else if(asLowerCaseString(strValue) == "necklace"){
									it.slot_position |= SLOTP_NECKLACE;
									it.wield_position = SLOT_NECKLACE;
								}
								else if(asLowerCaseString(strValue) == "ring"){
									it.slot_position |= SLOTP_RING;
									it.wield_position = SLOT_RING;
								}
								else if(asLowerCaseString(strValue) == "hand"){
									it.wield_position = SLOT_HAND;
								}
								else if(asLowerCaseString(strValue) == "ammo"){
									it.wield_position = SLOT_AMMO;
								}
								else{
									std::cout << "Warning: [Items::loadFromXml] " << "Unknown slotType " << strValue  << std::endl;
								}
							}
						}
						else if(asLowerCaseString(strValue) == "ammotype"){
							if(readXMLString(itemAttributesNode, "value", strValue)){
								it.amuType = getAmmoType(strValue);
								if(it.amuType == AMMO_NONE){
									std::cout << "Warning: [Items::loadFromXml] " << "Unknown ammoType " << strValue  << std::endl;
								}
							}
						}
						else if(asLowerCaseString(strValue) == "shoottype"){
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
						else if(asLowerCaseString(strValue) == "effect"){
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
						else if(asLowerCaseString(strValue) == "range"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.shootRange = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "stopduration"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.stopTime = (intValue != 0);
							}
						}
						else if(asLowerCaseString(strValue) == "decayto"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.decayTo = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "transformequipto"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.transformEquipTo = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "transformdeequipto"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.transformDeEquipTo = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "duration"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								if(intValue < 0){
									intValue = 0;
								}
								it.decayTime = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "showduration"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.showDuration = (intValue != 0);
							}
						}
						else if(asLowerCaseString(strValue) == "charges"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.charges = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "showcharges"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.showCharges = (intValue != 0);
							}
						}
						else if(asLowerCaseString(strValue) == "breakchance"){
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
						else if(asLowerCaseString(strValue) == "ammoaction"){
							if(readXMLString(itemAttributesNode, "value", strValue)){
								it.ammoAction = getAmmoAction(strValue);

								if(it.ammoAction == AMMOACTION_NONE){
									std::cout << "Warning: [Items::loadFromXml] " << "Unknown ammoAction " << strValue  << std::endl;
								}
							}
						}
						else if(asLowerCaseString(strValue) == "hitchance"){
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
						else if(asLowerCaseString(strValue) == "maxhitchance"){
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
						else if(asLowerCaseString(strValue) == "invisible"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.invisible = (intValue != 0);
							}
						}
						else if(asLowerCaseString(strValue) == "speed"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.speed = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "healthgain"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.regeneration = true;
								it.abilities.healthGain = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "healthticks"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.regeneration = true;
								it.abilities.healthTicks = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "managain"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.regeneration = true;
								it.abilities.manaGain = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "manaticks"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.regeneration = true;
								it.abilities.manaTicks = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "manashield"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.manaShield = (intValue != 0);
							}
						}
						else if(asLowerCaseString(strValue) == "skillsword"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.skill.upgrades[SKILL_SWORD] = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "skillaxe"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.skill.upgrades[SKILL_AXE] = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "skillclub"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.skill.upgrades[SKILL_CLUB] = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "skilldist"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.skill.upgrades[SKILL_DIST] = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "skillfish"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.skill.upgrades[SKILL_FISH] = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "skillshield"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.skill.upgrades[SKILL_SHIELD] = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "skillfist"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.skill.upgrades[SKILL_FIST] = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "maxhitpoints"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.stats[STAT_MAXHITPOINTS] = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "maxhitpointspercent"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.statsPercent[STAT_MAXHITPOINTS] = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "maxmanapoints"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.stats[STAT_MAXMANAPOINTS] = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "maxmanapointspercent"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.statsPercent[STAT_MAXMANAPOINTS] = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "soulpoints"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.stats[STAT_SOULPOINTS] = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "soulpointspercent"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.statsPercent[STAT_SOULPOINTS] = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "magicpoints"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.stats[STAT_MAGICPOINTS] = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "magicpointspercent"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.statsPercent[STAT_MAGICPOINTS] = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "absorbpercentall"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.absorb.resistances[CombatTypeToIndex(COMBAT_ENERGYDAMAGE)] = intValue;
								it.abilities.absorb.resistances[CombatTypeToIndex(COMBAT_FIREDAMAGE)] = intValue;
								it.abilities.absorb.resistances[CombatTypeToIndex(COMBAT_EARTHDAMAGE)] = intValue;
								it.abilities.absorb.resistances[CombatTypeToIndex(COMBAT_ICEDAMAGE)] = intValue;
								it.abilities.absorb.resistances[CombatTypeToIndex(COMBAT_HOLYDAMAGE)] = intValue;
								it.abilities.absorb.resistances[CombatTypeToIndex(COMBAT_DEATHDAMAGE)] = intValue;
								it.abilities.absorb.resistances[CombatTypeToIndex(COMBAT_PHYSICALDAMAGE)] = intValue;
								it.abilities.absorb.resistances[CombatTypeToIndex(COMBAT_LIFEDRAIN)] = intValue;
								it.abilities.absorb.resistances[CombatTypeToIndex(COMBAT_MANADRAIN)] = intValue;
								it.abilities.absorb.resistances[CombatTypeToIndex(COMBAT_DROWNDAMAGE)] = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "absorbpercentallelements"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.absorb.resistances[CombatTypeToIndex(COMBAT_ENERGYDAMAGE)] = intValue;
								it.abilities.absorb.resistances[CombatTypeToIndex(COMBAT_FIREDAMAGE)] = intValue;
								it.abilities.absorb.resistances[CombatTypeToIndex(COMBAT_EARTHDAMAGE)] = intValue;
								it.abilities.absorb.resistances[CombatTypeToIndex(COMBAT_ICEDAMAGE)] = intValue;
								it.abilities.absorb.resistances[CombatTypeToIndex(COMBAT_HOLYDAMAGE)] = intValue;
								it.abilities.absorb.resistances[CombatTypeToIndex(COMBAT_DEATHDAMAGE)] = intValue;
								it.abilities.absorb.resistances[CombatTypeToIndex(COMBAT_PHYSICALDAMAGE)] = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "absorbpercentenergy"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.absorb.resistances[CombatTypeToIndex(COMBAT_ENERGYDAMAGE)] = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "absorbpercentfire"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.absorb.resistances[CombatTypeToIndex(COMBAT_FIREDAMAGE)] = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "absorbpercentpoison" ||
								asLowerCaseString(strValue) == "absorbpercentearth"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.absorb.resistances[CombatTypeToIndex(COMBAT_EARTHDAMAGE)] = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "absorbpercentice"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.absorb.resistances[CombatTypeToIndex(COMBAT_ICEDAMAGE)] = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "absorbpercentholy"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.absorb.resistances[CombatTypeToIndex(COMBAT_HOLYDAMAGE)] = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "absorbpercentdeath"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.absorb.resistances[CombatTypeToIndex(COMBAT_DEATHDAMAGE)] = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "absorbpercentlifedrain"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.absorb.resistances[CombatTypeToIndex(COMBAT_LIFEDRAIN)] = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "absorbpercentmanadrain"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.absorb.resistances[CombatTypeToIndex(COMBAT_MANADRAIN)] = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "absorbpercentdrown"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.absorb.resistances[CombatTypeToIndex(COMBAT_DROWNDAMAGE)] = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "absorbpercentphysical"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.absorb.resistances[CombatTypeToIndex(COMBAT_PHYSICALDAMAGE)] = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "absorbpercentbleed"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.absorb.resistances[CombatTypeToIndex(COMBAT_BLEEDDAMAGE)] = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "suppressdrunk"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.conditionSuppressions |= CONDITION_DRUNK;
							}
						}
						else if(asLowerCaseString(strValue) == "suppressenergy"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.conditionSuppressions |= CONDITION_ENERGY;
							}
						}
						else if(asLowerCaseString(strValue) == "suppressfire"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.conditionSuppressions |= CONDITION_FIRE;
							}
						}
						else if(asLowerCaseString(strValue) == "suppresspoison"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.conditionSuppressions |= CONDITION_POISON;
							}
						}
						else if(asLowerCaseString(strValue) == "suppresslifedrain"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.conditionSuppressions |= CONDITION_LIFEDRAIN;
							}
						}
						else if(asLowerCaseString(strValue) == "suppressdrown"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.conditionSuppressions |= CONDITION_DROWN;
							}
						}
						else if(asLowerCaseString(strValue) == "suppressfreeze"){
							if(readXMLInteger(itemAttributesNode, "value", intValue) && intValue != 0){
								it.abilities.conditionSuppressions |= CONDITION_FREEZING;
							}
						}
						else if(asLowerCaseString(strValue) == "suppressdazzle"){
							if(readXMLInteger(itemAttributesNode, "value", intValue) && intValue != 0){
								it.abilities.conditionSuppressions |= CONDITION_DAZZLED;
							}
						}
						else if(asLowerCaseString(strValue) == "suppresscurse"){
							if(readXMLInteger(itemAttributesNode, "value", intValue) && intValue != 0){
								it.abilities.conditionSuppressions |= CONDITION_CURSED;
							}
						}
						else if(asLowerCaseString(strValue) == "suppressbleed"){
							if(readXMLInteger(itemAttributesNode, "value", intValue) && intValue != 0){
								it.abilities.conditionSuppressions |= CONDITION_BLEEDING;
							}
						}
						else if(asLowerCaseString(strValue) == "preventitemloss"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.preventItemLoss = (intValue != 0);
							}
						}
						else if(asLowerCaseString(strValue) == "preventskillloss"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.preventSkillLoss = (intValue != 0);
							}
						}
						/*else if(asLowerCaseString(strValue) == "suppressmanadrain"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.conditionSuppressions |= CONDITION_MANADRAIN;
							}
						}
						else if(asLowerCaseString(strValue) == "suppressphysical"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.conditionSuppressions |= CONDITION_PHYSICAL;
							}
						}*/
						else if(asLowerCaseString(strValue) == "absorbpercentfirefield"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.absorbFieldDamage[ITEM_FIREFIELD] = intValue;
								it.abilities.absorbFieldDamage[ITEM_FIREFIELD+1] = intValue;
								it.abilities.absorbFieldDamage[ITEM_FIREFIELD_NOT_DECAYING] = intValue;
								it.abilities.absorbFieldDamage[ITEM_FIREFIELD_NOT_DECAYING+1] = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "absorbpercentenergyfield"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.absorbFieldDamage[ITEM_ENERGYFIELD] = intValue;
								it.abilities.absorbFieldDamage[ITEM_ENERGYFIELD_NOT_DECAYING] = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "absorbpercentpoisonfield"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.absorbFieldDamage[ITEM_POISONFIELD] = intValue;
								it.abilities.absorbFieldDamage[ITEM_POISONFIELD_NOT_DECAYING] = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "reduceconditioncount"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.conditionCount = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "field"){
							it.group = ITEM_GROUP_MAGICFIELD;
							it.type = ITEM_TYPE_MAGICFIELD;
							CombatType_t combatType = COMBAT_NONE;
							ConditionDamage* conditionDamage = NULL;

							if(readXMLString(itemAttributesNode, "value", strValue)){
								if(asLowerCaseString(strValue) == "fire"){
									conditionDamage = new ConditionDamage(CONDITIONID_COMBAT, CONDITION_FIRE);
									combatType = COMBAT_FIREDAMAGE;
								}
								else if(asLowerCaseString(strValue) == "energy"){
									conditionDamage = new ConditionDamage(CONDITIONID_COMBAT, CONDITION_ENERGY);
									combatType = COMBAT_ENERGYDAMAGE;
								}
								else if(asLowerCaseString(strValue) == "poison"){
									conditionDamage = new ConditionDamage(CONDITIONID_COMBAT, CONDITION_POISON);
									combatType = COMBAT_EARTHDAMAGE;
								}
								else if(asLowerCaseString(strValue) == "drown"){
									conditionDamage = new ConditionDamage(CONDITIONID_COMBAT, CONDITION_DROWN);
									combatType = COMBAT_DROWNDAMAGE;
								}
								//else if(asLowerCaseString(strValue) == "physical"){
								//	damageCondition = new ConditionDamage(CONDITIONID_COMBAT, CONDITION_PHYSICAL);
								//	combatType = COMBAT_PHYSICALDAMAGE;
								//}
								else if(asLowerCaseString(strValue) == "bleed"){
									conditionDamage = new ConditionDamage(CONDITIONID_COMBAT, CONDITION_BLEEDING);
									combatType = COMBAT_BLEEDDAMAGE;
								}
								else{
									std::cout << "Warning: [Items::loadFromXml] " << "Unknown field value " << strValue  << std::endl;
								}

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
											if(asLowerCaseString(strValue) == "ticks"){
												if(readXMLInteger(fieldAttributesNode, "value", intValue)){
													ticks = std::max(0, intValue);
												}
											}

											if(asLowerCaseString(strValue) == "count"){
												if(readXMLInteger(fieldAttributesNode, "value", intValue)){
													if(intValue > 0){
														count = intValue;
													}
													else{
														count = 1;
													}
												}
											}

											if(asLowerCaseString(strValue) == "start"){
												if(readXMLInteger(fieldAttributesNode, "value", intValue)){
													if(intValue > 0){
														start = intValue;
													}
													else{
														start = 0;
													}
												}
											}

											if(asLowerCaseString(strValue) == "damage"){
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

									if(conditionDamage->getTotalDamage() > 0){
										conditionDamage->setParam(CONDITIONPARAM_FORCEUPDATE, true);
									}
								}
							}
						}
						else if(asLowerCaseString(strValue) == "replaceable"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.replaceable = (intValue != 0);
							}
						}
						else if(asLowerCaseString(strValue) == "partnerdirection"){
							if(readXMLString(itemAttributesNode, "value", strValue)){
								if(asLowerCaseString(strValue) == "0" || asLowerCaseString(strValue) == "north" || asLowerCaseString(strValue) == "n") {
									it.bedPartnerDir = NORTH;
								} else if(asLowerCaseString(strValue) == "1" || asLowerCaseString(strValue) == "east" || asLowerCaseString(strValue) == "e") {
									it.bedPartnerDir = EAST;
								} else if(asLowerCaseString(strValue) == "2" || asLowerCaseString(strValue) == "south" || asLowerCaseString(strValue) == "s") {
									it.bedPartnerDir = SOUTH;
								} else if(asLowerCaseString(strValue) == "3" || asLowerCaseString(strValue) == "west" || asLowerCaseString(strValue) == "w") {
									it.bedPartnerDir = WEST;
								}
							}
						}
						else if(asLowerCaseString(strValue) == "malesleeper"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.maleSleeperID = intValue;
								ItemType& other = getItemType(intValue);
								if(other.id != 0 && other.noSleeperID == 0){
									other.noSleeperID = it.id;
								}
								if(it.femaleSleeperID == 0)
									it.femaleSleeperID = intValue;
							}
						}
						else if(asLowerCaseString(strValue) == "femalesleeper"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.femaleSleeperID = intValue;
								ItemType& other = getItemType(intValue);
								if(other.id != 0 && other.noSleeperID == 0){
									other.noSleeperID = it.id;
								}
								if(it.maleSleeperID == 0)
									it.maleSleeperID = intValue;
							}
						}
						/*
						else if(asLowerCaseString(strValue) == "nosleeper"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.noSleeperID = intValue;
							}
						}
						*/
						else if(asLowerCaseString(strValue) == "elementice"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.elementDamage = intValue;
								it.abilities.elementType = COMBAT_ICEDAMAGE;
							}
						}
						else if(asLowerCaseString(strValue) == "elementearth"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.elementDamage = intValue;
								it.abilities.elementType = COMBAT_EARTHDAMAGE;
							}
						}
						else if(asLowerCaseString(strValue) == "elementfire"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.elementDamage = intValue;
								it.abilities.elementType = COMBAT_FIREDAMAGE;
							}
						}
						else if(asLowerCaseString(strValue) == "elementenergy"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.abilities.elementDamage = intValue;
								it.abilities.elementType = COMBAT_ENERGYDAMAGE;
							}
						}
						else if(asLowerCaseString(strValue) == "currency"){
							if(readXMLInteger(itemAttributesNode, "value", intValue)){
								it.currency = intValue;
								currencyMap[it.currency] = &it;
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

	//Lets do some checks..
	for(uint32_t i = 0; i < Item::items.size(); ++i){
		const ItemType* it = Item::items.getElement(i);

		if(!it){
			continue;
		}

		//check bed items
		if((it->noSleeperID != 0 || it->maleSleeperID != 0 || it->femaleSleeperID != 0) && it->type != ITEM_TYPE_BED){
			std::cout << "Warning: [Items::loadFromXml] Item " << it->id <<  " is not set as a bed-type." << std::endl;
		}

		/*
		//check looping decaying items
		if(it->decayTo <= 0 || !it->moveable){
			continue;
		}

		std::vector<int32_t> decayList;
		decayList.push_back(it->id);
		int32_t decayTo = it->decayTo;
		while(decayTo > 0){
			if(decayList.size() >= 10){
				std::cout << "Warning: [Items::loadFromXml] Item  " << *decayList.begin() << " an unsual long decay-chain" << std::endl;
			}

			if(std::find(decayList.begin(), decayList.end(), decayTo) == decayList.end()){
				decayList.push_back(decayTo);

				const ItemType& it = Item::items.getItemType(decayTo);
				if(it.id == 0){
					break;
				}

				decayTo = it.decayTo;
			}
			else{
				std::cout << "Warning: [Items::loadFromXml] Item  " << it->id << " has an infinite decay-chain" << std::endl;
				break;
			}
		}
		*/
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

Abilities::Abilities()
{
	memset(&absorb, 0, sizeof(absorb));
	memset(&skill, 0, sizeof(skill));

	elementType = COMBAT_NONE;
	elementDamage = 0;

	memset(stats, 0 , sizeof(stats));
	memset(statsPercent, 0, sizeof(statsPercent));

	speed = 0;
	manaShield = false;
	invisible = false;
	conditionImmunities = 0;
	conditionSuppressions = 0;

	regeneration = false;
	healthGain = 0;
	healthTicks = 0;

	manaGain = 0;
	manaTicks = 0;

	preventItemLoss = false;
	preventSkillLoss = false;
}
bool Abilities::Skill::any() const
{
	for(int32_t c = 0; c != SKILL_LAST + 1; ++c)
	{
		if(upgrades[c] != 0)
			return true;
	}
	return false;
}

std::ostream& Abilities::Skill::getDescription(std::ostream& os, bool& first, int32_t type) const
{
	if(upgrades[type] == 0)
		return os;
	os << (first? " " : ", ") << Player::getSkillName(type) << " +" << std::noshowpos << upgrades[type] << "";
	first = false;
	return os;
}

std::ostream& Abilities::Skill::getDescription(std::ostream& os) const
{
	bool first = true;
	for(int32_t c = 0; c != SKILL_LAST + 1; ++c){
		getDescription(os, first, c);
	}
	return os;
}

bool Abilities::Absorb::any() const
{
	for(int32_t c = 0; c != COMBAT_COUNT; ++c){
		if(resistances[c] != 0)
			return true;
	}
	return false;
}

std::ostream& Abilities::Absorb::getDescription(std::ostream& os, bool& first, int32_t type) const
{
	if(resistances[type] == 0)
		return os;
	os << (first? " " : ", ") << CombatTypeName(type == COMBAT_NONE? COMBAT_NONE: (CombatType_t)(1 << (type-1))) << " " << std::noshowpos << resistances[type] << "%";
	first = false;
	return os;
}

std::ostream& Abilities::Absorb::getDescription(std::ostream& os) const
{
	bool first = true;
	for(int32_t c = 0; c != COMBAT_COUNT; ++c){
		getDescription(os, first, c);
	}
	return os;
}

bool Abilities::Absorb::reduce(CombatType_t ctype, int32_t& dmg) const
{
	bool r = false;
	if(ctype == COMBAT_NONE)
		return r;

	for(int32_t c = 0;  c < COMBAT_COUNT; ++c) {
		if(ctype & (1 << c)) {
			// Correct type!
			if(resistances[c+1] > 0)
				r = true;
			dmg = (int32_t)std::floor((double)dmg * (100 - resistances[c+1]) / 100.);
		}
	}

	return r;
}

int32_t Items::getItemIdByName(const std::string& name)
{
	if(!name.empty()){
		uint32_t i = 100;
		ItemType* iType;
		do{
			iType = items.getElement(i);
			if(iType){
				if(boost::algorithm::iequals(name, iType->name)){
					return i;
				}
			}
			i++;
		}while(iType);
	}
	return -1;
}
