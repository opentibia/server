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

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <iostream>
#include <string>

extern Spells spells;

long Items::dwMajorVersion = 0;
long Items::dwMinorVersion = 0;
long Items::dwBuildNumber = 0;

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
	weaponType    = NONE;
	slot_position = SLOTP_RIGHT | SLOTP_LEFT | SLOTP_AMMO;
	amuType       = AMU_NONE;
	shootType     = DIST_NONE;
	attack        = 0;
	defence       = 0;
	armor         = 0;
	decayTo       = 0;
	decayTime     = 60;
	canDecay      =	true;
	allowDistRead = false;

	isVertical		= false;
	isHorizontal	= false;
	isHangable		= false;

	lightLevel    = 0;
	lightColor    = 0;
}

ItemType::~ItemType()
{
}

bool ItemType::isGroundTile() const
{
	return group == ITEM_GROUP_GROUND;
}

bool ItemType::isContainer() const
{
	return (group == ITEM_GROUP_CONTAINER);
}

bool ItemType::isDoor() const
{
	return (group == ITEM_GROUP_DOOR);
}

bool ItemType::isTeleport() const
{
	return (group == ITEM_GROUP_TELEPORT);
}

bool ItemType::isMagicField() const
{
	return (group == ITEM_GROUP_MAGICFIELD);
}

bool ItemType::isKey() const
{
	return (group == ITEM_GROUP_KEY);
}

bool ItemType::isSplash() const
{
	return (group == ITEM_GROUP_SPLASH);
}

bool ItemType::isFluidContainer() const
{
	return (group == ITEM_GROUP_FLUID);
}

bool ItemType::isRune() const
{
	return (group == ITEM_GROUP_RUNE);
}

Items::Items() :
items(8000), revItems(8000)
{
	//
}

Items::~Items()
{
	//for (ItemMap::iterator it = items.begin(); it != items.end(); it++)
	//	delete it->second;
}

inline subfight_t translateOTBsubfight_t(subfightOTB_t sf)
{
	switch(sf){
	case OTB_DIST_NONE:
		return DIST_NONE;
		break;
	case OTB_DIST_BOLT:
		return DIST_BOLT;
		break;
	case OTB_DIST_ARROW:
		return DIST_ARROW;
		break;
	case OTB_DIST_FIRE:
		return DIST_FIRE;
		break;
	case OTB_DIST_ENERGY:
		return DIST_ENERGY;
		break;
	case OTB_DIST_POISONARROW:
		return DIST_POISONARROW;
		break;
	case OTB_DIST_BURSTARROW:
		return DIST_BURSTARROW;
		break;
	case OTB_DIST_THROWINGSTAR:
		return DIST_THROWINGSTAR;
		break;
	case OTB_DIST_THROWINGKNIFE:
		return DIST_THROWINGKNIFE;
		break;
	case OTB_DIST_SMALLSTONE:
		return DIST_SMALLSTONE;
		break;
	case OTB_DIST_SUDDENDEATH:
		return DIST_SUDDENDEATH;
		break;
	case OTB_DIST_LARGEROCK:
		return DIST_LARGEROCK;
		break;
	case OTB_DIST_SNOWBALL:
		return DIST_SNOWBALL;
		break;
	case OTB_DIST_POWERBOLT:
		return DIST_POWERBOLT;
		break;
	case OTB_DIST_SPEAR:
		return DIST_SPEAR;
		break;
	case OTB_DIST_POISONFIELD:
		return DIST_POISONFIELD;
		break;
	}
	return DIST_NONE;
}

int Items::loadFromOtb(std::string file)
{
	ItemLoader f;
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
			Items::dwMinorVersion = vi->dwMinorVersion; //client version
			Items::dwBuildNumber = vi->dwBuildNumber;	//revision
		}
	}
	
	if(Items::dwMajorVersion != 1){
		std::cout << "Not supported items.otb version." << std::endl;
		return ERROR_INVALID_FORMAT;
	}
	
	if(Items::dwMinorVersion != CLIENT_VERSION_780){
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
		case ITEM_GROUP_WEAPON:
		case ITEM_GROUP_AMMUNITION:
		case ITEM_GROUP_ARMOR:
		case ITEM_GROUP_RUNE:
		case ITEM_GROUP_TELEPORT:
		case ITEM_GROUP_MAGICFIELD:
		case ITEM_GROUP_WRITEABLE:
		case ITEM_GROUP_KEY:
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
		iType->canDecay = !((flags & FLAG_CANNOTDECAY) == FLAG_CANNOTDECAY);
		iType->isVertical = ((flags & FLAG_VERTICAL) == FLAG_VERTICAL);
		iType->isHorizontal = ((flags & FLAG_HORIZONTAL) == FLAG_HORIZONTAL);
		iType->isHangable = ((flags & FLAG_HANGABLE) == FLAG_HANGABLE);
		iType->allowDistRead = ((flags & FLAG_ALLOWDISTREAD) == FLAG_ALLOWDISTREAD);
							
							
		if(type == ITEM_GROUP_WRITEABLE) {
			iType->RWInfo |= CAN_BE_WRITTEN;
		}

		if((flags & FLAG_READABLE) == FLAG_READABLE)
			iType->RWInfo |= CAN_BE_READ;

		iType->rotable = ((flags & FLAG_ROTABLE) == FLAG_ROTABLE);
		
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
				
				if(serverid > 20000 && serverid < 20100)
					serverid = serverid - 20000;
						
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
			case ITEM_ATTR_NAME:
			{
				uint8_t name[128];
				if(datalen >= sizeof(name))
					return ERROR_INVALID_FORMAT;

				if(!props.GET_NSTRING(datalen, iType->name))
					return ERROR_INVALID_FORMAT;
				
				break;
			}	
			case ITEM_ATTR_DESCR:
			{
				uint8_t descr[128];
				if(datalen >= sizeof(descr))
					return ERROR_INVALID_FORMAT;
	
				if(!props.GET_NSTRING(datalen, iType->description))
					return ERROR_INVALID_FORMAT;

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
			case ITEM_ATTR_SLOT:
			{
				if(datalen != sizeof(uint16_t))
					return ERROR_INVALID_FORMAT;
				
				uint16_t otb_slot;
				
				if(!props.GET_USHORT(otb_slot))
					return ERROR_INVALID_FORMAT;
				
				switch(otb_slot){
				case OTB_SLOT_DEFAULT:
				case OTB_SLOT_WEAPON:
				case OTB_SLOT_HAND:
					//default	
					break;
				case OTB_SLOT_HEAD:
					iType->slot_position = SLOTP_HEAD;
					break;
				case OTB_SLOT_BODY:
					iType->slot_position = SLOTP_ARMOR;
					break;
				case OTB_SLOT_LEGS:
					iType->slot_position = SLOTP_LEGS;
					break;
				case OTB_SLOT_BACKPACK:
					iType->slot_position = SLOTP_BACKPACK;
					break;
				case OTB_SLOT_2HAND:
					iType->slot_position  = SLOTP_TWO_HAND;
					break;
				case OTB_SLOT_FEET:
					iType->slot_position = SLOTP_FEET;
					break;
				case OTB_SLOT_AMULET:
					iType->slot_position = SLOTP_NECKLACE;
					break;
				case OTB_SLOT_RING:
					iType->slot_position = SLOTP_RING;
					break;
					}
				iType->slot_position = iType->slot_position | SLOTP_LEFT | SLOTP_RIGHT | SLOTP_AMMO;
				break;
			}	
			case ITEM_ATTR_MAXITEMS:
			{
				if(datalen != sizeof(uint16_t))
					return ERROR_INVALID_FORMAT;
				
				uint16_t maxItems;
				if(!props.GET_USHORT(maxItems))
					return ERROR_INVALID_FORMAT;

				iType->maxItems = maxItems;

				break;
			}		
			case ITEM_ATTR_WEIGHT:
			{
				if(datalen != sizeof(double))
					return ERROR_INVALID_FORMAT;

				if(!props.GET_VALUE(iType->weight))
					return ERROR_INVALID_FORMAT;

				break;
			}
			case ITEM_ATTR_MAGLEVEL:
			{
				if(datalen != sizeof(uint16_t))
					return ERROR_INVALID_FORMAT;
					
				uint16_t maglev;
				if(!props.GET_USHORT(maglev))
					return ERROR_INVALID_FORMAT;
				
				iType->runeMagLevel = maglev;
				break;
			}
			case ITEM_ATTR_MAGFIELDTYPE:
			{
				if(datalen != sizeof(uint8_t))
					return ERROR_INVALID_FORMAT;
				
				unsigned char fieldtype;
				if(!props.GET_UCHAR(fieldtype))
					return ERROR_INVALID_FORMAT;

				iType->magicfieldtype = fieldtype;
				break;
			}	
			case ITEM_ATTR_ROTATETO:
			{
				if(datalen != sizeof(uint16_t))
					return ERROR_INVALID_FORMAT;
				
				uint16_t rotate;
				if(!props.GET_USHORT(rotate))
					return ERROR_INVALID_FORMAT;
				
				iType->rotateTo = rotate;
				break;
			}	
			case ITEM_ATTR_DECAY2:
			{
				if(datalen != sizeof(decayBlock2))
					return ERROR_INVALID_FORMAT;

				decayBlock2* db2;
				if(!props.GET_STRUCT(db2))
					return ERROR_INVALID_FORMAT;
				
				iType->decayTime = db2->decayTime;
				iType->decayTo = db2->decayTo;
				break;
			}	
			case ITEM_ATTR_WEAPON2:
			{
				if(datalen != sizeof(weaponBlock2))
					return ERROR_INVALID_FORMAT;

				weaponBlock2* wb2;
				if(!props.GET_STRUCT(wb2))
					return ERROR_INVALID_FORMAT;
				
				iType->weaponType = (WeaponType)wb2->weaponType;
				iType->shootType = translateOTBsubfight_t((subfightOTB_t)wb2->shootType);
				iType->amuType = (amu_t)wb2->amuType;
				iType->attack = wb2->attack;
				iType->defence = wb2->defence;
				break;
			}
			case ITEM_ATTR_AMU2:
			{
				if(datalen != sizeof(amuBlock2))
					return ERROR_INVALID_FORMAT;

				amuBlock2* ab2;
				if(!props.GET_STRUCT(ab2))
					return ERROR_INVALID_FORMAT;
					
				iType->weaponType = AMO;
				iType->shootType = translateOTBsubfight_t((subfightOTB_t)ab2->shootType);
				iType->amuType = (amu_t)ab2->amuType;
				iType->attack = ab2->attack;
				break;
			}
			case ITEM_ATTR_ARMOR2:
			{
				if(datalen != sizeof(armorBlock2))
					return ERROR_INVALID_FORMAT;

				armorBlock2* ab2;
				if(!props.GET_STRUCT(ab2))
					return ERROR_INVALID_FORMAT;
				
				iType->armor = ab2->armor;
				iType->weight = ab2->weight;
				//ignore this value
				//iType->slot_position = (slots_t)ab2.slot_position;

				break;
			}
			case ITEM_ATTR_WRITEABLE3:
			{
				if(datalen != sizeof(writeableBlock3))
					return ERROR_INVALID_FORMAT;

				writeableBlock3* wb3;
				if(!props.GET_STRUCT(wb3))
					return ERROR_INVALID_FORMAT;
					
				iType->readOnlyId = wb3->readOnlyId;
				iType->maxTextLen = wb3->maxTextLen;

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
		
		//get rune mag level from spells.xml
		if(iType->group == ITEM_GROUP_RUNE){
			std::map<unsigned short, Spell*>::iterator it = spells.getAllRuneSpells()->find(iType->id);
			if(it != spells.getAllRuneSpells()->end()){
				iType->runeMagLevel = it->second->getMagLv();
			}
			else{
				iType->runeMagLevel = 0;
			}	
		}
		// store the found item	 
		//items[iType->id] = iType;
		//revItems[iType->clientId] = iType->id;
		items.addElement(iType, iType->id);
		revItems.addElement(iType->id, iType->clientId);
		
		node = f.getNextNode(node, type);
	}
	
	return ERROR_NONE;
}

const ItemType& Items::operator[](int id)
{
	/*
	ItemMap::iterator it = items.find(id);
	if ((it != items.end()) && (it->second != NULL))
		return *it->second;
	   
	return dummyItemType;
	*/
	
	ItemType* iType = items.getElement(id);
	if(!iType){
		return dummyItemType;
	}
	else{
		return *iType;
	}
}

int Items::getItemIdByName(const std::string& name)
{
	if(!name.empty()){
		/*
		for(ItemMap::iterator it = items.begin(); it != items.end(); ++it){
			if(strcasecmp(name.c_str(), it->second->name.c_str()) == 0){
				return it->first;
			}
		}
		*/
		long i = 100;
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

int Items::reverseLookUp(int id)
{
	return revItems.getElement(id);
}

template<typename A> 
Array<A>::Array(long n)
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
A Array<A>::getElement(long id)
{
	if(id < 0 || id >= m_size){
		return 0;
	}
	else{
		return m_data[id];
	}
}

template<typename A>
void Array<A>::addElement(A a, long pos)
{
	#define INCREMENT 5000
	if(pos >= m_size){
		m_data = (A*)realloc(m_data, sizeof(A)*(pos + INCREMENT));
		memset(m_data + m_size, 0, sizeof(A)*(pos + INCREMENT - m_size));
		m_size = pos + INCREMENT;
	}
	if(pos < 0){
		return;
	}
	m_data[pos] = a;
}
