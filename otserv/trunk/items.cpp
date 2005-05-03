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
	iscontainer     = false;
	fluidcontainer	= false;
	stackable       = false;
  multitype       = false;
	useable	        = false;
	notMoveable     = false;
	alwaysOnTop     = false;
	groundtile      = false;
	blocking        = false; // people can walk on it
	pickupable      = false; // people can pick it up
	blockingProjectile = false;
	noFloorChange = false;
	floorChangeNorth = false;
	floorChangeSouth = false;
	floorChangeEast = false;
	floorChangeWest = false;

	isteleport = false;
	
	runeMagLevel    = -1;
	
	speed		= 0;
	id         =  100;
	maxItems   =    8;  // maximum size if this is a container
	weight     =   50;  // weight of the item, e.g. throwing distance depends on it
  weaponType = NONE;
  slot_position = SLOTP_RIGHT | SLOTP_LEFT | SLOTP_AMMO;
  amuType = AMU_NONE;
  shootType = DIST_NONE;
  attack     =    0;
  defence    =    0;
  armor      =    0;
  decayTo    =    0;
  decayTime  =   60;

	damage = 0;
}

ItemType::~ItemType()
{
}


Items::Items()
{
}

Items::~Items()
{
	for (ItemMap::iterator it = items.begin(); it != items.end(); it++)
		delete it->second;
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
            //TODO: some other way of finding levelchange items
            if(iType->id == 1396 || iType->id ==1385 || iType->id ==1394 || iType->id ==1404){
                         iType->floorChangeNorth = true;
                         }
            if(iType->id == 1392|| iType->id ==1402){
                         iType->floorChangeSouth = true;
                         }
            if(iType->id == 1388|| iType->id ==1398){
                         iType->floorChangeEast = true;
                         }
            if(iType->id == 1390|| iType->id ==1400){
                         iType->floorChangeWest = true;
                         }
                         
            //diagonal             
            if(iType->id == 1559){
                         iType->floorChangeSouth = true;
                         iType->floorChangeWest = true;
                         }
            if(iType->id == 1557){
                         iType->floorChangeSouth = true;
                         iType->floorChangeEast = true;
                         }
            if(iType->id == 1553){
                         iType->floorChangeNorth = true;
                         iType->floorChangeWest = true;
                         }
            if(iType->id == 1555){
                         iType->floorChangeNorth = true;
                         iType->floorChangeEast = true;
                         }                                       
                                     
			switch (optbyte)
			{
	   		case 0x00:
		   		//is groundtile	   				
    			iType->groundtile = true;
    			iType->speed=(int)fgetc(f);
				if(iType->speed==0) {
                  iType->blocking=true;
                  }    
                  fgetc(f);
		   		break;

        case 0x01: // all OnTop
        case 0x02: // can walk trough (open doors, arces, bug pen fence ??)
          iType->alwaysOnTop=true;
          break;

				case 0x03:
					//is a container
					iType->iscontainer=true;
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
                    iType->multitype=true;
					break;

				case 0x0B:
					//is blocking
					iType->blocking=true;
					break;
				
				case 0x0C:
					//is on moveable
                    iType->notMoveable=true;
					break;
	
				case 0x0F:
					//can be equipped
					iType->pickupable=true;
					break;

				case 0x10:
					//makes light (skip 4 bytes)
                    fgetc(f); //number of tiles around
                    fgetc(f); // always 0
                    fgetc(f); // 215 items, 208 fe non existant items other values
                    fgetc(f); // always 0
					break;

        case 0x06: // ladder up (id 1386)   why a group for just 1 item ???   
            break;
        case 0x09: //can contain fluids
        	iType->fluidcontainer = true;
            break;
        case 0x0D: // blocks missiles (walls, magic wall etc)
					iType->blockingProjectile = true;
          break;
        case 0x0E: // blocks monster movement (flowers, parcels etc)
            break;
        case 0x11: // can see what is under (ladder holes, stairs holes etc)
            break;
        case 0x12: // ground tiles that don't cause level change
					iType->noFloorChange = true;
            break;
        case 0x18: // cropses that don't decay
            break;
        /*case 0x19: //(removed in 7.4) monster has animation even when iddle (rot, wasp, slime, fe)
            break;*/
        case 0x14: // player color templates
            break;

				case 0x07: // writtable objects
                    fgetc(f); //max characters that can be written in it (0 unlimited)
                    fgetc(f); //max number of  newlines ? 0, 2, 4, 7
				    break;
				case 0x08: // writtable objects that can't be edited 
                    fgetc(f); //always 0 max characters that can be written in it (0 unlimited) 
                    fgetc(f); //always 4 max number of  newlines ? 
				    break;
				case 0x13: // mostly blocking items, but also items that can pile up in level (boxes, chairs etc)
                    fgetc(f); //always 8
                    fgetc(f); //always 0
                    break;
				case 0x16: // ground, blocking items and mayby some more 
				unsigned char a;
                    a = fgetc(f); //12, 186, 210, 129 and other.. 
                    fgetc(f); //always 0
                    //if(a == 210)
                    //printf("%d - %d %d\n", iType->id, a);
                    //iType->floorChange = true;
				    break;
				case 0x1A: 
                    //7.4 (change no data ?? ) action that can be performed (doors-> open, hole->open, book->read) not all included ex. wall torches
				    break;
				//new from 7.4    
				case 0x1D:  // line spot ...
                    fgetc(f); // 86 -> openable holes, 77-> can be used to go down, 76 can be used to go up, 82 -> stairs up, 79 switch,    
                    fgetc(f); // always 4                  
                    break;         
				case 0x1B:  // walls 2 types of them same material (total 4 pairs)                  
                    break;
                case 0x19:  // wall items                 
                    break;    
                case 0x17:  // seems like decorables with 4 states of turning (exception first 4 are unique statues)                 
                    break;
                case 0x1C:  // ?? ...                 
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
 		id++;
   	}
   	
   	fclose(f);
    
	return 0;
}

int Items::loadXMLInfos(std::string file)
{
	xmlDocPtr doc;
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
			if (elem == "item" && xmlGetProp(p, (xmlChar*)"id")) {
				// get the id...
				int id = atoi((const char*)xmlGetProp(p, (xmlChar*)"id"));

				// now let's find the item and load it's definition...
				ItemMap::iterator it = items.find(id);
				if ((it != items.end()) && (it->second != NULL)){
					ItemType *itemtype = it->second;

					// set general properties...
					char* name = (char*)xmlGetProp(p, (xmlChar*)"name");
          			if (name)
						itemtype->name = name;
          			else
            			std::cout << "missing name tag for item: " << id << std::endl;

          			char* weight = (char*)xmlGetProp(p, (xmlChar*)"weight");
          			if (weight)
						itemtype->weight = atof(weight);
          			else
            			std::cout << "missing weight tag for item: " << id << std::endl;

					// and optional properties
          			char* description = (char*)xmlGetProp(p, (xmlChar*)"descr");
          			if (description)
						itemtype->description = description;

          			char* decayTo = (char*)xmlGetProp(p, (xmlChar*)"decayto");
          			if (decayTo)
						itemtype->decayTo = atoi(decayTo);

          			char* decayTime = (char*)xmlGetProp(p, (xmlChar*)"decaytime");
          			if (decayTime)
						itemtype->decayTime = atoi(decayTime);
		
					char* damage = (char*)xmlGetProp(p, (xmlChar*)"damage");
					if(damage)
						itemtype->damage = atoi(damage);
					
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
					}
					
					// now set special properties...
					// first we check the type...
					char* type = (char*)xmlGetProp(p, (xmlChar*)"type");
				  	if (type){
            			if (!strcmp(type, "container")){
							// we have a container...
							// TODO set that we got a container						
              				char* maxitems = (char*)xmlGetProp(p, (xmlChar*)"maxitems");
							if (maxitems)
                				itemtype->maxItems = atoi(maxitems);
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
              						if (amutype){
						    			if (!strcmp(amutype, "bolt"))
								  			itemtype->amuType = AMU_BOLT;
							  			else if (!strcmp(amutype, "arrow"))
							    			itemtype->amuType = AMU_ARROW;
                						else
                  							std::cout << "wrong amutype tag" << std::endl;
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
										}
										else
											std::cout << "missing shoottype type for distante-item: " << id << std::endl;
									}																
								}
								else if (!strcmp(skill, "magic")){
									itemtype->weaponType = MAGIC;
									char *sshoottype = (char*)xmlGetProp(p, (xmlChar*)"shottype");
              						if (sshoottype){
										if (!strcmp(sshoottype, "fire"))
							    			itemtype->shootType = DIST_FIRE;
							    		else if (!strcmp(sshoottype, "energy"))
							    			itemtype->shootType = DIST_ENERGY;
							    		else
							    			std::cout << "wrong shootype tag" << std::endl;
									}									
									
								}
							  	else if (!strcmp(skill, "shielding"))
									itemtype->weaponType = SHIELD;
                				else
                  					std::cout << "wrong skill tag for weapon" << std::endl;
              				}
              				else
								std::cout << "missing skill tag for weapon" << std::endl;
							
							char* attack = (char*)xmlGetProp(p, (xmlChar*)"attack");

							if (attack)
								itemtype->attack = atoi(attack);
							else
								std::cout << "missing attack tag for weapon: " << id << std::endl;
							
							char* defence = (char*)xmlGetProp(p, (xmlChar*)"defence");
							if (defence)
							itemtype->defence = atoi(defence);
							else
								std::cout << "missing defence tag for weapon: " << id << std::endl;
						
						}
						else if (!strcmp(type, "amunition"))
						{
							// we got some amo
							itemtype->weaponType = AMO;							
							char *amutype = (char*)xmlGetProp(p, (xmlChar*)"amutype");
              				if (amutype){
						    	if (!strcmp(amutype, "bolt"))
								  	itemtype->amuType = AMU_BOLT;
							  	else if (!strcmp(amutype, "arrow"))
							    	itemtype->amuType = AMU_ARROW;
                				else
                  					std::cout << "wrong amutype tag for item: " << id << std::endl;
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
              				}
              				else
								std::cout << "missing shoottype for item: " << id <<  std::endl;
						}//ammunition
            			else if (!strcmp(type, "armor")){
							char* sarmor = (char*)xmlGetProp(p, (xmlChar*)"arm");
		            		if (sarmor)
								itemtype->armor = atoi(sarmor);
              				else
								std::cout << "missing arm tag for armor: " << id << std::endl;
						}//armor
						else if (!strcmp(type, "rune"))
						{
							// runes..
							char* runemaglv = (char*)xmlGetProp(p, (xmlChar*)"maglevel");
							if(runemaglv)
								itemtype->runeMagLevel = atoi(runemaglv);
							else
	       						std::cout << "missing maglevel for rune: " << id << std::endl;
						}//rune
						else if(!strcmp(type, "teleport"))
						{
							itemtype->isteleport = true;
						}
					}//type
				} 
				else {
					std::cout << "invalid item " << id << std::endl;
				}//type
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

