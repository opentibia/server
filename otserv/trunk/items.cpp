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
	stackable       = false;
  multitype       = false;
	useable	        = false;
	notMoveable     = false;
	alwaysOnTop     = false;
	groundtile      = false;
	blocking        = false; // people can walk on it
	pickupable      = false; // people can pick it up
	
	speed		= 0;
	id         =  100;
	maxItems   =    8;  // maximum size if this is a container
	weight     =   50;  // weight of the item, e.g. throwing distance depends on it
  weaponType = NONE;
  attack     =    0;
  defence    =    0;

  decayTo    =    0;
  decayTime  =   60;
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
int a=100;
bool b = false;
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

        case 0x06: // ladder up (id 1124)   why a group for just 1 item ???  
            break;
        case 0x09: //can contain fluids
            break;
        case 0x0D: // blocks missiles (walls, magic wall etc)
            break;
        case 0x0E: // blocks monster movement (flowers, parcels etc)
            break;
        case 0x11: // can see what is under (ladder holes, stairs holes etc)
            break;
        case 0x12: // tiles that don't cause level change
            break;
        case 0x18: // cropses that don't decay
            break;
        case 0x19: // monster has animation even when iddle (rot, wasp, slime, fe)
            break;
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
                    fgetc(f); //12, 186, 210, 129 and other.. 
                    fgetc(f); //always 0
				    break;
				case 0x1A: 
                    fgetc(f); //action that can be performed (doors-> open, hole->open, book->read) not all included ex. wall torches
                    fgetc(f); //always 4
				    break;
				default:
						std::cout << "unknown byte: " << (unsigned short)optbyte << std::endl;
			}
		}
		
		// now skip the size and sprite data		
 		int width  = fgetc(f);
 		int height = fgetc(f);
 		if ((width > 1) || (height > 1))
 		   int skip = fgetc(f);
 		   
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
				if ((it != items.end()) && (it->second != NULL))
        {
					ItemType *itemtype = it->second;

					// set general properties...
					char* name = (char*)xmlGetProp(p, (xmlChar*)"name");
          if (name)
						itemtype->name = name;
          else
            std::cout << "missing name tag for item: " << id << std::endl;

          char* weight = (char*)xmlGetProp(p, (xmlChar*)"weight");
          if (weight)
						itemtype->weight = atoi(weight);
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

					// now set special properties...
					// first we check the type...
					char* type = (char*)xmlGetProp(p, (xmlChar*)"type");
				  if (type)
          {
            if (!strcmp(type, "container"))
            {
							// we have a container...
							// TODO set that we got a container
              char* maxitems = (char*)xmlGetProp(p, (xmlChar*)"maxitems");
							if (maxitems)
                itemtype->maxItems = atoi(maxitems);
              else
								std::cout << "item " << id << " is a container but lacks a maxitems definition." << std::endl;

						}
            else if (!strcmp(type, "weapon"))
            {
							// we have a weapon...
							// find out which type of weapon we have...
              char *skill = (char*)xmlGetProp(p, (xmlChar*)"skill");
              if (skill)
              {
						    if (!strcmp(skill, "sword"))
								  itemtype->weaponType = SWORD;
							  else if (!strcmp(skill, "club"))
							    itemtype->weaponType = CLUB;
							  else if (!strcmp(skill, "axe"))
								  itemtype->weaponType = AXE;
							  else if (!strcmp(skill, "distance"))
								  itemtype->weaponType = DIST;
							  else if (!strcmp(skill, "magic"))
								  itemtype->weaponType = MAGIC;
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
						}
            else if (!strcmp(type, "armor"))
            {
							// armor...
						}
					}
				} else {
						  std::cout << "invalid item " << id << std::endl;
				}

			}
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

