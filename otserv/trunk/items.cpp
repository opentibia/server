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


#include "definitions.h"
#include "items.h"

#include "spells.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <iostream>

extern Spells spells;

ItemType::ItemType()
{
	group           = ITEM_GROUP_NONE;

	RWInfo          = 0;
	readOnlyId      = 0;
	stackable       = false;
	useable	        = false;
	moveable        = true;
	alwaysOnTop     = false;
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
	id            = 100;
	clientId      = 100;
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

	lightLevel    = 0;
	lightColor    = 0;

	//readable        = false;
	//ismagicfield    = false;
	//iskey           = false;
	//issplash		    = false;

	//damage	      =	0;
	//groundtile      = false;
	//iscontainer     = false;
	//fluidcontainer	= false;		
	//multitype       = false;
	//isteleport = false;
	//notMoveable   = false;
	//canWalkThrough = false;
	//blocking      = false; // people can walk on it
	//blockingProjectile = false;
	//blockpickupable = true;
	//isDoor = false;
	//isDoorWithLock = false;
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


Items::Items()
{
}

Items::~Items()
{
	for (ItemMap::iterator it = items.begin(); it != items.end(); it++)
		delete it->second;
}


int Items::loadFromOtb(std::string file)
{
	ItemLoader f;
	if(!f.openFile(file.c_str(), false)) {
		return f.getError();
	}
	
	unsigned long type,len;
	NODE node = f.getChildNode(NO_NODE, type);
	node = f.getChildNode(node, type);

	const unsigned char* data;
	while(node != NO_NODE) {
		data = f.getProps(node, len);
		if(data == NULL && f.getError() != ERROR_NONE)
			return f.getError();
		
		flags_t flags;
		if(data != NULL) {
			const unsigned char* p = &data[0];
			ItemType* iType = new ItemType();
			bool loadedFlags = false;

			while(p < data + len) {
				iType->group = (itemgroup_t)type;

				switch(type) {
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
					{
						if(!loadedFlags) {
							//read 4 byte flags
							memcpy((void*)&flags, p, sizeof(flags_t)); p+= sizeof(flags_t);

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
							
							if(type == ITEM_GROUP_WRITEABLE) {
								iType->RWInfo |= CAN_BE_WRITTEN;
							}

							if((flags & FLAG_READABLE) == FLAG_READABLE)
								iType->RWInfo |= CAN_BE_READ;

							iType->rotable = ((flags & FLAG_ROTABLE) == FLAG_ROTABLE);

							if(p >= data + len) //no attributes
								break;
							loadedFlags = true;
						}

						//attribute
						attribute_t attrib = *p; p+= sizeof(attribute_t);
						if(p >= data + len) {
							delete iType;
							return ERROR_INVALID_FORMAT;
						}

						datasize_t datalen = 0;
						//size of data
						memcpy(&datalen, p, sizeof(datasize_t)); p+= sizeof(datalen);
						if(p >= data + len) {
							delete iType;
							return ERROR_INVALID_FORMAT;
						}

						switch(attrib) {
							case ITEM_ATTR_SERVERID:
							{
								if(datalen != sizeof(unsigned short))
									return ERROR_INVALID_FORMAT;
								
								unsigned short serverid = *(unsigned short*)p;
								if(serverid > 20000 && serverid < 20100)
									serverid = serverid - 20000;

								iType->id = serverid;
								break;
							}

							case ITEM_ATTR_CLIENTID:
							{
								if(datalen != sizeof(unsigned short))
									return ERROR_INVALID_FORMAT;

								memcpy(&iType->clientId, p, sizeof(unsigned short));
								break;
							}
							case ITEM_ATTR_NAME:
							{
								char name[128];
								if(datalen >= sizeof(name))
									return ERROR_INVALID_FORMAT;

								memcpy(name, p, datalen);
								name[datalen] = 0;
								iType->name = name;
								break;
							}
							case ITEM_ATTR_DESCR:
							{
								char descr[128];
								if(datalen >= sizeof(descr))
									return ERROR_INVALID_FORMAT;
	
								memcpy(descr, p, datalen);
								descr[datalen] = 0;
								iType->description = descr;

								break;
							}
							case ITEM_ATTR_SPEED:
							{
								if(datalen != sizeof(unsigned short))
									return ERROR_INVALID_FORMAT;

								memcpy(&iType->speed, p, sizeof(unsigned short));
								break;
							}
							case ITEM_ATTR_SLOT:
							{
								if(datalen != sizeof(unsigned short))
									return ERROR_INVALID_FORMAT;
								
								unsigned short otb_slot = *(unsigned short*)p;
								
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
								if(datalen != sizeof(unsigned short))
									return ERROR_INVALID_FORMAT;

								memcpy(&iType->maxItems, p, sizeof(unsigned short));
								break;
							}
							case ITEM_ATTR_WEIGHT:
							{
								if(datalen != sizeof(double))
									return ERROR_INVALID_FORMAT;

								memcpy(&iType->weight, p, sizeof(double));
								break;
							}
							case ITEM_ATTR_WEAPON:
							{
								if(datalen != sizeof(weaponBlock))
									return ERROR_INVALID_FORMAT;

								weaponBlock wb;
								memcpy(&wb, p, sizeof(weaponBlock));
								iType->weaponType = (WeaponType)wb.weaponType;
								iType->shootType = (subfight_t)wb.shootType;
								iType->amuType = (amu_t)wb.amuType;
								iType->attack = wb.attack;
								iType->defence = wb.defence;
								break;
							}
							case ITEM_ATTR_AMU:
							{
								if(datalen != sizeof(amuBlock))
									return ERROR_INVALID_FORMAT;

								amuBlock ab;
								memcpy(&ab, p, sizeof(amuBlock));
								iType->weaponType = AMO;
								iType->shootType = (subfight_t)ab.shootType;
								iType->amuType = (amu_t)ab.amuType;
								iType->attack = ab.attack;
								break;
							}
							case ITEM_ATTR_ARMOR:
							{
								if(datalen != sizeof(armorBlock))
									return ERROR_INVALID_FORMAT;

								armorBlock ab;
								memcpy(&ab, p, sizeof(armorBlock));
									
								iType->armor = ab.armor;
								iType->weight = ab.weight;
								//ignore this value
								//iType->slot_position = ab.slot_position;

								break;
							}
							case ITEM_ATTR_MAGLEVEL:
							{
								if(datalen != sizeof(unsigned short))
									return ERROR_INVALID_FORMAT;
								
								memcpy(&iType->runeMagLevel, p, sizeof(unsigned short));
								
								break;
							}
							case ITEM_ATTR_MAGFIELDTYPE:
							{
								if(datalen != sizeof(unsigned char))
									return ERROR_INVALID_FORMAT;
								
								memcpy(&iType->magicfieldtype, p, sizeof(unsigned char));

								break;
							}
							case ITEM_ATTR_WRITEABLE:
							{
								if(datalen != sizeof(writeableBlock))
									return ERROR_INVALID_FORMAT;

								struct writeableBlock wb;
								memcpy(&wb, p, sizeof(writeableBlock));

								iType->readOnlyId = wb.readOnlyId;

								break;
							}
							case ITEM_ATTR_ROTATETO:
							{
								if(datalen != sizeof(unsigned short))
									return ERROR_INVALID_FORMAT;
								
								iType->rotateTo = *(unsigned short*)p;
								
								break;
							}
							case ITEM_ATTR_DECAY:
							{
								if(datalen != sizeof(decayBlock))
									return ERROR_INVALID_FORMAT;

								decayBlock db;
								memcpy(&db, p, sizeof(decayBlock));
								iType->decayTime = db.decayTime;
								iType->decayTo = db.decayTo;
								break;
							}

							case ITEM_ATTR_SPRITEHASH:
							{
								if(datalen != 16)
									return ERROR_INVALID_FORMAT;
								break;
							}

							case ITEM_ATTR_MINIMAPCOLOR:
							{
								if(datalen != sizeof(unsigned short))
									return ERROR_INVALID_FORMAT;
								break;
							}

							case ITEM_ATTR_07:
							{
								if(datalen != sizeof(unsigned short))
									return ERROR_INVALID_FORMAT;
								break;
							}

							case ITEM_ATTR_08:
							{
								if(datalen != sizeof(unsigned short))
									return ERROR_INVALID_FORMAT;
								break;
							}

							case ITEM_ATTR_LIGHT:
							{
								if(datalen != sizeof(lightBlock))
									return ERROR_INVALID_FORMAT;

								lightBlock lb;
								memcpy(&lb, p, sizeof(lightBlock));
								iType->lightLevel = lb.lightLevel;
								iType->lightColor = lb.lightColor;
								break;
							}

							default:
								delete iType;
								return ERROR_INVALID_FORMAT;
						}

						
						p+= datalen;
						break;
					}

					default:
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
			items[iType->id] = iType;
			revItems[iType->clientId] = iType->id;
		}

		node = f.getNextNode(node, type);
	}
	
	return ERROR_NONE;
}

const ItemType& Items::operator[](int id)
{
	ItemMap::iterator it = items.find(id);
	if ((it != items.end()) && (it->second != NULL))
		return *it->second;
	
#ifdef __DEBUG__
	std::cout << "WARNING! unknown itemtypeid " << id << ". using defaults." << std::endl;
#endif
	   
	return dummyItemType;
}

unsigned long Items::reverseLookUp(unsigned long id)
{
	ReverseItemMap::iterator it = revItems.find(id);
	if(it != revItems.end()){
		return it->second;
	}
	else{
		return 0;
	}
}
