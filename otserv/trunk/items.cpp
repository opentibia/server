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
	useable	        = false;
	notMoveable     = false;
	alwaysOnTop     = false;
	groundtile      = false;
	blocking        = false; // people can walk on it
	pickupable      = false; // people can pick it up
	
	id         =  100;
	maxItems   =    8;  // maximum size if this is a container
	weight     =   50;  // weight of the item, e.g. throwing distance depends on it
  weaponType = NONE;
  attack     =    0;
  defence    =    0;
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
			switch (optbyte)
			{
	   		case 0x00:
		   		//is groundtile	   				
    			iType->groundtile = true;
		   		fseek(f, 2, SEEK_CUR);
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
				
				case 0x0B:
					//is blocking
					iType->blocking=true;
					break;
				
				case 0x0C:
					//is on top
					iType->alwaysOnTop=true;
					break;
	
				case 0x0F:
					//can be equipped
					iType->pickupable=true;
					break;

				case 0x10:
					//makes light (skip 4 bytes)
		   			fseek(f, 4, SEEK_CUR);
					break;

				case 0x01:
        case 0x02:
        case 0x06:
        case 0x09:
        case 0x0A:
        case 0x0D:
        case 0x0E:
        case 0x11:
        case 0x12:
        case 0x18:
        case 0x19:
				case 0x14: // up?
          break;

				case 0x07:
				case 0x08:
				case 0x13:
				case 0x16:
				case 0x1A:
					// unknown, but skip 2 bytes to follow the "ordered option" idea
		   			fseek(f, 2, SEEK_CUR);
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

  if (doc)
  {
    xmlNodePtr root, p;
	  root = xmlDocGetRootElement(doc);

	  if (xmlStrcmp(root->name, (const xmlChar*)"items")) {
		  xmlFreeDoc(doc);
		  return -1;
	  }

    p = root->children;
	  while (p)
    {
	    const char* str = (char*)p->name;
	    if (strcmp(str, "item") == 0)
      {
        int id = atoi((const char*)xmlGetProp(p, (xmlChar*)"id"));

        ItemMap::iterator it = items.find(id);
        if ((it != items.end()) && (it->second != NULL))
        {
	        ItemType *itemtype = it->second;

          const char* name = (const char*)xmlGetProp(p, (xmlChar*)"name");
          if (name != NULL)
            itemtype->name = name;

          const char* maxitems = (const char*)xmlGetProp(p, (xmlChar*)"maxitems");
          if (maxitems != NULL)
            itemtype->maxItems = atoi(maxitems);

          const char* weight = (const char*)xmlGetProp(p, (xmlChar*)"weight");
          if (weight != NULL)
            itemtype->weight = atoi(weight);

          const char* weaponType = (const char*)xmlGetProp(p, (xmlChar*)"weapontype");
          if (weaponType != NULL)
          {
            if (!xmlStrcmp((const xmlChar*)weaponType, (const xmlChar*)"sword"))
              itemtype->weaponType = SWORD;
            else if (!xmlStrcmp((const xmlChar*)weaponType, (const xmlChar*)"club"))
              itemtype->weaponType = CLUB;
            else if (!xmlStrcmp((const xmlChar*)weaponType, (const xmlChar*)"axe"))
              itemtype->weaponType = AXE;
            else if (!xmlStrcmp((const xmlChar*)weaponType, (const xmlChar*)"bow"))
              itemtype->weaponType = BOW;
            else if (!xmlStrcmp((const xmlChar*)weaponType, (const xmlChar*)"crossbow"))
              itemtype->weaponType = XBOW;
            else if (!xmlStrcmp((const xmlChar*)weaponType, (const xmlChar*)"magicstaff"))
              itemtype->weaponType = MAGICSTAFF;
            else
              itemtype->weaponType = NONE;
          }

          const char* attack = (const char*)xmlGetProp(p, (xmlChar*)"attack");
          if (attack != NULL)
            itemtype->attack = atoi(attack);

          const char* defence = (const char*)xmlGetProp(p, (xmlChar*)"defence");
          if (defence != NULL)
            itemtype->defence = atoi(defence);
        }
        else
        {
          std::cout << "error: wrong item id " << id;
	  xmlFreeDoc(doc);
          return -1;
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

