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

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <iostream>

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
	rotateble       = false;
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
							
							if(type == ITEM_GROUP_WRITEABLE) {
								iType->RWInfo |= CAN_BE_WRITTEN;
							}

							if((flags & FLAG_READABLE) == FLAG_READABLE)
								iType->RWInfo |= CAN_BE_READ;

							iType->rotateble = ((flags & FLAG_ROTABLE) == FLAG_ROTABLE);

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

			// store the found item	  	
			items[iType->id] = iType;
			revItems[iType->clientId] = iType->id;
		}

		node = f.getNextNode(node, type);
	}
	
	return ERROR_NONE;
}

int Items::loadFromDat(std::string file)
{
	int id = 100;  // tibia.dat start with id 100
	
#ifdef __DEBUG__
	std::cout << "Reading item data from tibia.dat" << std::endl;
#endif
	
	FILE* f=fopen(file.c_str(), "rb");
	
	if(!f){
#ifdef __DEBUG__
		std::cout << "FAILED!" << std::endl;
#endif
		return -1;
	}
	
	fseek(f,0,SEEK_END);
	long size=ftell(f);
	
#ifdef __DEBUG__
	std::cout << "tibia.dat size is " << size << std::endl;
#endif
	
#ifdef __DEBUG__
	bool warningwrongoptordershown = false;
#endif
	
	fseek(f, 0x0C, SEEK_SET);
	// loop throw all Items until we reach the end of file
	while(ftell(f) < size)
	{
		ItemType* iType= new ItemType();
		iType->id	  = id;
		iType->clientId = id;
		
#ifdef __DEBUG__
		int lastoptbyte = 0;
#endif
		
		// read the options until we find a 0xff
		int optbyte;	
		
		while (((optbyte = fgetc(f)) >= 0) &&   // no error
			(optbyte != 0xFF))			    // end of options
		{
#ifdef __DEBUG__
			if (optbyte < lastoptbyte)
			{
				if (!warningwrongoptordershown)
				{
					std::cout << "WARNING! Unexpected option order in file tibia.dat." << std::endl;
					warningwrongoptordershown = true;
				}
			}
			lastoptbyte = optbyte;
#endif
			
			switch (optbyte)
			{
			case 0x00:
				unsigned short read_short;
				fread(&read_short, 2, 1, f); 
				iType->speed = read_short;
				iType->group = ITEM_GROUP_GROUND;
				break;
				
			case 0x01: // all OnTop
				iType->alwaysOnTop=true;
				break;
				
			case 0x02: // can walk through (open doors, arces, bug pen fence ??)
				//iType->canWalkThrough = true;
				iType->alwaysOnTop = true;
				break;
				
			case 0x03:
				//is a container
				//iType->iscontainer=true;
				iType->group = ITEM_GROUP_CONTAINER;
				break;	   			
				
			case 0x04:
				//is stackable
				iType->stackable=true;
				break;
				
			case 0x05:
				//is useable
				iType->useable=true;
				break;
				
			case 0x0A:
				//is multitype !!! wrong definition (only water splash on floor)
				//iType->multitype=true;
				iType->group = ITEM_GROUP_SPLASH;
				break;
				
			case 0x0B:
				//is blocking
				iType->blockSolid=true;
				//std::cout << "0x0B\t"<< (int) id << std::endl;
				break;
				
			case 0x0C:
				//is not moveable
				iType->moveable=false;
				break;
				
			case 0x0F:
				//can be equipped
				iType->pickupable=true;
				break;
				
			case 0x10:
				//makes light (skip 4 bytes)
				fgetc(f); //number of tiles around
				fgetc(f);

				fgetc(f); // light color
				fgetc(f);
				break;
				
			case 0x06: // ladder up (id 1386)   why a group for just 1 item ???   
				break;
			case 0x09: //can contain fluids
				//iType->fluidcontainer = true;
				iType->group = ITEM_GROUP_FLUID;
				break;
			case 0x0D: // blocks missiles (walls, magic wall etc)
				iType->blockProjectile = true;
				//std::cout << "0x0D\t"<< (int) id << std::endl;
				break;
			case 0x0E: // blocks monster movement (flowers, parcels etc)
				iType->blockPathFind = true;
				break;
			case 0x11: // can see what is under (ladder holes, stairs holes etc)
				break;
			case 0x12: // ground tiles that don't cause level change
				iType->floorChangeDown = false;
				break;
			case 0x18: //draw with height offset for all parts (2x2) of the sprite
				break;
				/*case 0x19: //(changed to 0x1C in 7.4) monster has animation even when iddle (rot, wasp, slime, fe)
				break;*/
			case 0x14: // player color templates
				break;
				
			case 0x07: // writtable objects
			{
				iType->group = ITEM_GROUP_WRITEABLE;
				iType->RWInfo |= CAN_BE_WRITTEN | CAN_BE_READ;

				unsigned short subopt;
				subopt = fgetc(f);
				subopt += fgetc(f) << 8;

				//std::cout << "id:" << (int) id << "0x07: " << (int) subopt << std::endl;

				//fgetc(f); //max characters that can be written in it (0 unlimited)
				//fgetc(f); //max number of  newlines ? 0, 2, 4, 7                    
				break;
			}
			case 0x08: // writtable objects that can't be edited
			{
				iType->RWInfo |= CAN_BE_READ;
				unsigned short subopt;
				subopt = fgetc(f);
				subopt += fgetc(f) << 8;

				//std::cout << "id:" << (int) id << "0x08: " << (int) subopt << std::endl;
				//fgetc(f); //always 0 max characters that can be written in it (0 unlimited) 
				//fgetc(f); //always 4 max number of  newlines ? 
				break;
			}
			case 0x13: //items that have height
				iType->hasHeight = true;
				//std::cout << "0x13: " << id << std::endl;
				
				fgetc(f); //height offset
				fgetc(f);
				break;
			case 0x16: //for minimap drawing					
				//0: black color - tar
				//30: light-blue color - swamp
				//12: dark-green color - forrest/bushes/trees
				//24: light-green color - forrest edges
				//40: dark-blue - water
				//86: dark-grey color - various items, mountain edges, stone portals
				//87: dark-grey color - mountains
				//114: light-brown - underground edges
				//121: dark-brown - underground
				//129: light-grey color - normal ground
				//179: light-blue color - ice
				//186: red color - blocking items
				//192: orange color - lava
				//207: "sand" color - sand
				//210: yellow color
				
				unsigned short subopt;
				subopt = fgetc(f);
				subopt += fgetc(f) << 8;
				break;
			case 0x1A: //unknown
				//7.4 (change no data ?? ) action that can be performed (doors-> open, hole->open, book->read) not all included ex. wall torches
				break;
				//new from 7.4    
			case 0x1D:  // Ctrl+H
				optbyte = fgetc(f); // 86 -> openable holes, 77-> can be used to go down, 76 can be used to go up, 82 -> stairs up, 79 switch,
				switch(optbyte){
				case 0x4C: //ladders
					break;
				case 0x4D: //crate
					break;
				case 0x4E: //rope spot?
					break;
				case 0x4F: //switch
					break;
				case 0x50: //doors
					break;
				case 0x51: //doors with locks
					break;
				case 0x52: //stairs
					break;
				case 0x53: //mailbox
					break;
				case 0x54: //depot
					break;
				case 0x55: //trash
					break;
				case 0x56: //hole 	
					break;
				case 0x57: //items with special description?
					break;					
				case 0x58: //writtable?						
					iType->RWInfo |= CAN_BE_READ;
					break;
				default:
					std::cout << "unknown action byte: " << (unsigned short)optbyte << std::endl;
					break;
				}
				fgetc(f); // always value 4
				break;         
			case 0x1B:  // walls 2 types of them same material (total 4 pairs)
				break;
				
			case 0x19:  // wall items                 
				break;    
			case 0x17:  // seems like decorables with 4 states of turning (exception first 4 are unique statues)                 
				break;
			case 0x1C:  // monster has animation even when iddle (rot, wasp, slime, fe)                 
				break;        
				
			default:
				std::cout << "unknown byte: " << (unsigned short)optbyte << std::endl;
			}
		}
		
		// now skip the size and sprite data		
		int width  = fgetc(f);
		int height = fgetc(f);
		if ((width > 1) || (height > 1))
			fgetc(f);
		
		int blendframes = fgetc(f);
		int xdiv        = fgetc(f);
		int ydiv        = fgetc(f);
		int animcount   = fgetc(f);
		
		fseek(f, width*height*blendframes*xdiv*ydiv*animcount*2, SEEK_CUR);
		
		// store the found item	  	
		items[id] = iType;
		revItems[id] = id;
		
		id++;
   	}
   	
   	fclose(f);
    //create extra items for liquids
    for(int i=0;i < 40;i++){ //8*5 = 40
		ItemType* iType= new ItemType();
		iType->id = i;
		items[i] = iType;
	}
    
	return 0;
}

int Items::loadXMLInfos(std::string file)
{
	xmlDocPtr doc;
	char *tmp;
	doc = xmlParseFile(file.c_str());
	
	if (doc) {
		xmlNodePtr root, p;
		root = xmlDocGetRootElement(doc);
		
		if (xmlStrcmp(root->name, (const xmlChar*)"items")) {
			xmlFreeDoc(doc);
			return -1;
		}
		
		p = root->children;
		while (p) {
			std::string elem = (char*)p->name;
			if (elem == "item" && (tmp = (char*)xmlGetProp(p, (xmlChar*)"id"))) {
				// get the id...
				int id = atoi(tmp);
				xmlFreeOTSERV(tmp);
				// now let's find the item and load it's definition...
				ItemMap::iterator it = items.find(id);
				if ((it != items.end()) && (it->second != NULL)){
					ItemType *itemtype = it->second;
					
					// set general properties...
					char* name = (char*)xmlGetProp(p, (xmlChar*)"name");
					if(name){
						itemtype->name = name;
						xmlFreeOTSERV(name);
					}
					else
						std::cout << "missing name tag for item: " << id << std::endl;
					
					char* weight = (char*)xmlGetProp(p, (xmlChar*)"weight");
					if(weight){
						itemtype->weight = atof(weight);
						xmlFreeOTSERV(weight);
					}
					else
						std::cout << "missing weight tag for item: " << id << std::endl;
					
					// and optional properties
					char* description = (char*)xmlGetProp(p, (xmlChar*)"descr");
					if(description){
						itemtype->description = description;
						xmlFreeOTSERV(description);
					}
					
					char* decayTo = (char*)xmlGetProp(p, (xmlChar*)"decayto");
					if (decayTo){
						itemtype->decayTo = atoi(decayTo);
						xmlFreeOTSERV(decayTo);
						if(itemtype->decayTo == 0){
							itemtype->canDecay = false;
						}
					}
					
					char* decayTime = (char*)xmlGetProp(p, (xmlChar*)"decaytime");
					if (decayTime){
						itemtype->decayTime = atoi(decayTime);
						xmlFreeOTSERV(decayTime);
					}
					
					char* blockingProjectile = (char*)xmlGetProp(p, (xmlChar*)"blockingprojectile");
					if(blockingProjectile){
						itemtype->blockProjectile = true;
						xmlFreeOTSERV(blockingProjectile);
					}
					
					char* floorChangeDown = (char*)xmlGetProp(p, (xmlChar*)"floorchange");
					if(floorChangeDown){
						itemtype->floorChangeDown = true;
						xmlFreeOTSERV(floorChangeDown);
					}
					
					char* floorChangeNorth = (char*)xmlGetProp(p, (xmlChar*)"floorchangenorth");
					if(floorChangeNorth){
						itemtype->floorChangeNorth = true;
						xmlFreeOTSERV(floorChangeNorth);
					}
					
					char* floorChangeSouth = (char*)xmlGetProp(p, (xmlChar*)"floorchangesouth");
					if(floorChangeSouth){
						itemtype->floorChangeSouth = true;
						xmlFreeOTSERV(floorChangeSouth);
					}
					
					char* floorChangeEast = (char*)xmlGetProp(p, (xmlChar*)"floorchangeeast");
					if(floorChangeEast){
						itemtype->floorChangeEast = true;
						xmlFreeOTSERV(floorChangeEast);
					}
					
					char* floorChangeWest = (char*)xmlGetProp(p, (xmlChar*)"floorchangewest");
					if(floorChangeWest){
						itemtype->floorChangeWest = true;
						xmlFreeOTSERV(floorChangeWest);
					}
					
					/*char* damage = (char*)xmlGetProp(p, (xmlChar*)"damage");
					if(damage){
						itemtype->damage = atoi(damage);
						xmlFreeOTSERV(damage);
					}*/
					
					char *position = (char*)xmlGetProp(p, (xmlChar*)"position");
					if(position){
						if (!strcmp(position, "helmet") || !strcmp(position, "head"))
							itemtype->slot_position = SLOTP_HEAD;
						else if (!strcmp(position, "amulet"))
							itemtype->slot_position = SLOTP_NECKLACE;
						else if (!strcmp(position, "body"))
							itemtype->slot_position = SLOTP_ARMOR;
						else if (!strcmp(position, "legs"))
							itemtype->slot_position = SLOTP_LEGS;
						else if (!strcmp(position, "boots"))
							itemtype->slot_position = SLOTP_FEET;
						else if (!strcmp(position, "ring"))
							itemtype->slot_position = SLOTP_RING;
						else if (!strcmp(position, "backpack"))					 	
							itemtype->slot_position = SLOTP_BACKPACK | SLOTP_LEFT | SLOTP_RIGHT | SLOTP_AMMO;
						else if (!strcmp(position, "weapon"))
							itemtype->slot_position = SLOTP_RIGHT | SLOTP_LEFT;
						else if (!strcmp(position, "twohand"))
							itemtype->slot_position = SLOTP_RIGHT | SLOTP_LEFT | SLOTP_TWO_HAND;
						else if (!strcmp(position, "hand"))
							itemtype->slot_position = SLOTP_RIGHT | SLOTP_LEFT | SLOTP_AMMO;
						else
							std::cout << "wrong position tag for item: " << id << std::endl;
						
						itemtype->slot_position |= SLOTP_LEFT | SLOTP_RIGHT | SLOTP_AMMO;
						xmlFreeOTSERV(position);
					}
					
					// now set special properties...
					// first we check the type...
					char* type = (char*)xmlGetProp(p, (xmlChar*)"type");
					if(type){
						if (!strcmp(type, "container")){
							// we have a container...							
							char* maxitems = (char*)xmlGetProp(p, (xmlChar*)"maxitems");
							if(maxitems){
								itemtype->maxItems = atoi(maxitems);
								xmlFreeOTSERV(maxitems);
							}
							else
								std::cout << "item " << id << " is a container but lacks a maxitems definition." << std::endl;
							
						}//container
						else if (!strcmp(type, "weapon")){
							// we have a weapon...
							// find out which type of weapon we have...
							char *skill = (char*)xmlGetProp(p, (xmlChar*)"skill");
							if (skill){
								if (!strcmp(skill, "sword"))
									itemtype->weaponType = SWORD;
								else if (!strcmp(skill, "club"))
									itemtype->weaponType = CLUB;
								else if (!strcmp(skill, "axe"))
									itemtype->weaponType = AXE;
								else if (!strcmp(skill, "shielding"))
									itemtype->weaponType = SHIELD;
								else if (!strcmp(skill, "distance")){
									itemtype->weaponType = DIST;
									char *amutype = (char*)xmlGetProp(p, (xmlChar*)"amutype");
									if(amutype){
										if (!strcmp(amutype, "bolt"))
											itemtype->amuType = AMU_BOLT;
										else if (!strcmp(amutype, "arrow"))
											itemtype->amuType = AMU_ARROW;
										else
											std::cout << "wrong amutype tag" << std::endl;
										
										xmlFreeOTSERV(amutype);
									}
									else{ //no ammunition, check shoottype
										char *sshoottype = (char*)xmlGetProp(p, (xmlChar*)"shottype");
										if (sshoottype){
											if (!strcmp(sshoottype, "throwing-star"))
												itemtype->shootType = DIST_THROWINGSTAR;
											else if (!strcmp(sshoottype, "throwing-knife"))
												itemtype->shootType = DIST_THROWINGKNIFE;
											else if (!strcmp(sshoottype, "small-stone"))
												itemtype->shootType = DIST_SMALLSTONE;
											else if (!strcmp(sshoottype, "sudden-death"))
												itemtype->shootType = DIST_SUDDENDEATH;
											else if (!strcmp(sshoottype, "large-rock"))
												itemtype->shootType = DIST_LARGEROCK;
											else if (!strcmp(sshoottype, "snowball"))
												itemtype->shootType = DIST_SNOWBALL;
											else if (!strcmp(sshoottype, "spear"))
												itemtype->shootType = DIST_SPEAR;
											else
												std::cout << "wrong shootype tag" << std::endl;
											
											xmlFreeOTSERV(sshoottype);
										}
										else
											std::cout << "missing shoottype type for distante-item: " << id << std::endl;
									}
								}
								else if(!strcmp(skill, "magic")){
									itemtype->weaponType = MAGIC;
									char *sshoottype = (char*)xmlGetProp(p, (xmlChar*)"shottype");
									if(sshoottype){
										if (!strcmp(sshoottype, "fire"))
											itemtype->shootType = DIST_FIRE;
										else if (!strcmp(sshoottype, "energy"))
											itemtype->shootType = DIST_ENERGY;
										else
											std::cout << "wrong shootype tag" << std::endl;
										
										xmlFreeOTSERV(sshoottype);
									}									
									
								}
								else if(!strcmp(skill, "shielding"))
									itemtype->weaponType = SHIELD;
								else
									std::cout << "wrong skill tag for weapon" << std::endl;
								
								xmlFreeOTSERV(skill);
							}
							else
								std::cout << "missing skill tag for weapon" << std::endl;
							
							char* attack = (char*)xmlGetProp(p, (xmlChar*)"attack");
							
							if (attack){
								itemtype->attack = atoi(attack);
								xmlFreeOTSERV(attack);
							}
							else
								std::cout << "missing attack tag for weapon: " << id << std::endl;
							
							char* defence = (char*)xmlGetProp(p, (xmlChar*)"defence");
							if (defence){
								itemtype->defence = atoi(defence);
								xmlFreeOTSERV(defence);
							}
							else
								std::cout << "missing defence tag for weapon: " << id << std::endl;
							
						}
						else if (!strcmp(type, "amunition"))
						{
							// we got some amo
							itemtype->weaponType = AMO;							
							char *amutype = (char*)xmlGetProp(p, (xmlChar*)"amutype");
							if(amutype){
								if (!strcmp(amutype, "bolt"))
									itemtype->amuType = AMU_BOLT;
								else if (!strcmp(amutype, "arrow"))
									itemtype->amuType = AMU_ARROW;
								else
									std::cout << "wrong amutype tag for item: " << id << std::endl;
								
								xmlFreeOTSERV(amutype);
							}
							else
								std::cout << "missing amutype for item: " << id << std::endl;
							
							char *sshoottype = (char*)xmlGetProp(p, (xmlChar*)"shottype");
							if (sshoottype){
								if (!strcmp(sshoottype, "bolt"))
									itemtype->shootType = DIST_BOLT;
								else if (!strcmp(sshoottype, "arrow"))
									itemtype->shootType = DIST_ARROW;
								else if (!strcmp(sshoottype, "poison-arrow"))
									itemtype->shootType = DIST_POISONARROW;
								else if (!strcmp(sshoottype, "burst-arrow"))
									itemtype->shootType = DIST_BURSTARROW;
								else if (!strcmp(sshoottype, "power-bolt"))
									itemtype->shootType = DIST_POWERBOLT;
								else
									std::cout << "wrong shootype tag for item: " << id << std::endl;
								
								xmlFreeOTSERV(sshoottype);
							}
							else
								std::cout << "missing shoottype for item: " << id <<  std::endl;
							
							char* attack = (char*)xmlGetProp(p, (xmlChar*)"attack");
							if (attack){
								itemtype->attack = atoi(attack);
								xmlFreeOTSERV(attack);
							}
							else
								std::cout << "missing attack tag for ammunition: " << id << std::endl;
						}//ammunition
						else if (!strcmp(type, "armor")){
							char* sarmor = (char*)xmlGetProp(p, (xmlChar*)"arm");
							if (sarmor){
								itemtype->armor = atoi(sarmor);
								xmlFreeOTSERV(sarmor);
							}
							else
								std::cout << "missing arm tag for armor: " << id << std::endl;
						}//armor
						else if (!strcmp(type, "rune"))
						{
							// runes..
							char* runemaglv = (char*)xmlGetProp(p, (xmlChar*)"maglevel");
							if(runemaglv){
								itemtype->runeMagLevel = atoi(runemaglv);
								xmlFreeOTSERV(runemaglv);
							}
							else
								std::cout << "missing maglevel for rune: " << id << std::endl;
						}//rune
						else if(!strcmp(type, "teleport"))
						{
							//itemtype->isteleport = true;
							itemtype->group = ITEM_GROUP_TELEPORT;
						}
						else if(!strcmp(type, "magicfield"))
						{
							//itemtype->ismagicfield = true;
							itemtype->group = ITEM_GROUP_MAGICFIELD;
							char* fieldtype = (char*)xmlGetProp(p, (xmlChar*)"fieldtype");
							if(fieldtype){
								if (!strcmp(fieldtype, "fire"))
									itemtype->magicfieldtype = MAGIC_FIELD_FIRE;
								else if (!strcmp(fieldtype, "energy"))
									itemtype->magicfieldtype = MAGIC_FIELD_ENERGY;
								else if (!strcmp(fieldtype, "poison"))
									itemtype->magicfieldtype = MAGIC_FIELD_POISON_GREEN;
								else
									std::cout << "wrong field type tag for item: " << id << std::endl;
								
								xmlFreeOTSERV(fieldtype);
							}
							else
								std::cout << "missing field type for field: " << id << std::endl;
							
						}						
						else if(!strcmp(type, "write1time")){
							char* sreadonlyid = (char*)xmlGetProp(p, (xmlChar*)"readonlyid");
							if (sreadonlyid){
								itemtype->readOnlyId = atoi(sreadonlyid);
								xmlFreeOTSERV(sreadonlyid);
							}
							else{
								std::cout << "missing readonlyid tag for item: " << id << std::endl;
							}
						}
						else if(!strcmp(type, "key")){
							//itemtype->iskey = true;
							itemtype->group = ITEM_GROUP_KEY;
						}
						else if(!strcmp(type, "splash")){
							//itemtype->issplash = true;
							itemtype->group = ITEM_GROUP_SPLASH;
						}
						else{
							std::cout << "unknown type for item: " << id << std::endl;
						}
						xmlFreeOTSERV(type);
					}//type					
				}
				else {
					std::cout << "invalid item " << id << std::endl;
				}	
			}//item - id						
			p = p->next;
		}
		
		xmlFreeDoc(doc);
		
		return 0;
	}
	return -1;
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
