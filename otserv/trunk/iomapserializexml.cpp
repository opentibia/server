//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// SQL map serialization
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

#include "iomapserializexml.h"

#include "house.h"
#include "town.h"

#include <sstream>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

IOMapSerializeXML::IOMapSerializeXML()
{
	//
}

IOMapSerializeXML::~IOMapSerializeXML()
{
	//
}

bool IOMapSerializeXML::loadMap(Map* map, const std::string& identifier)
{
	map->setLastError(LOADMAPERROR_NONE);

	xmlNodePtr nodeChild;
	char* nodeValue;

	//xmlLineNumbersDefault(1);
	xmlSubstituteEntitiesDefault(1);
	xmlDocPtr doc = xmlParseFile(identifier.c_str());
	if(!doc){
		map->setLastError(LOADMAPERROR_CANNOTOPENFILE);
		return false;
	}

	xmlNodePtr root = xmlDocGetRootElement(doc);
	if(xmlStrcmp(root->name, (const xmlChar*) "map") != 0){
		xmlFreeDoc(doc);
		map->setLastError(LOADMAPERROR_GETROOTHEADERFAILED);
		return false;
	}

	if(nodeValue = (char*)xmlGetProp(root, (const xmlChar *) "width")){
		map->mapwidth = atoi(nodeValue);
		xmlFreeOTSERV(nodeValue);
	}

	if(nodeValue = (char*)xmlGetProp(root, (const xmlChar *) "height")){
		map->mapheight = atoi(nodeValue);
		xmlFreeOTSERV(nodeValue);
	}
	
	if(nodeValue = (char*)xmlGetProp(root, (const xmlChar *) "spawnfile")){
		map->spawnfile = identifier.substr(0, identifier.rfind('/') + 1);
		map->spawnfile += nodeValue;

		xmlFreeOTSERV(nodeValue);
	}

	if(nodeValue = (char*)xmlGetProp(root, (const xmlChar *) "housefile")){
		map->housefile = identifier.substr(0, identifier.rfind('/') + 1);
		map->housefile += nodeValue;

		xmlFreeOTSERV(nodeValue);
	}

	nodeChild = root->children;

	Tile* tile = NULL;

	while(nodeChild){
		if(xmlStrcmp(nodeChild->name, (const xmlChar*)"tile") == 0){

			Position posTile;
			if(nodeValue = (char*)xmlGetProp(nodeChild, (const xmlChar *) "x")){
				posTile.x = atoi(nodeValue);
				xmlFreeOTSERV(nodeValue);
			}
			else{
				map->setLastError(LOADMAPERROR_GETPROPFAILED);
				return false;
			}

			if(nodeValue = (char*)xmlGetProp(nodeChild, (const xmlChar *) "y")){
				posTile.y = atoi(nodeValue);
				xmlFreeOTSERV(nodeValue);
			}
			else{
				map->setLastError(LOADMAPERROR_GETPROPFAILED);
				return false;
			}

			if(nodeValue = (char*)xmlGetProp(nodeChild, (const xmlChar *) "z")){
				posTile.z = atoi(nodeValue);
				xmlFreeOTSERV(nodeValue);
			}
			else{
				map->setLastError(LOADMAPERROR_GETPROPFAILED);
				return false;
			}
			
			tile = map->getTile(posTile);
			if(!tile){
				int32_t houseid = 0;
				if(nodeValue = (char*)xmlGetProp(nodeChild, (const xmlChar *) "houseid")){
					houseid = atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}

				if(houseid == 0){
					tile = new Tile(posTile.x, posTile.y, posTile.z);
				}
				else{
					House* house = Houses::getInstance().getHouse(houseid, true);
					if(!house){
						map->setLastError(LOADMAPERROR_FAILEDTOCREATEITEM);
						return false;
					}

					HouseTile* houseTile = new HouseTile(posTile.x, posTile.y, posTile.z, house);
					house->addTile(houseTile);
					tile = houseTile;
				}

				map->setTile(posTile.x, posTile.y, posTile.z, tile);
			}

			loadTile(map, nodeChild, tile);
		}
		else if(xmlStrcmp(nodeChild->name, (const xmlChar*)"towns") == 0){
			xmlNodePtr nodeTown = nodeChild->children;

			if(xmlStrcmp(nodeTown->name, (const xmlChar*) "town") == 0){
				Position templePos;
				uint32_t townid = 0;

				if(nodeValue = (char*)xmlGetProp(nodeTown, (const xmlChar *) "townid")){
					townid = atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}
				else{
					map->setLastError(LOADMAPERROR_GETPROPFAILED);
					return false;
				}
		
				Town* town = Towns::getInstance().getTown(townid);
				if(!town){
					town = new Town(townid);
					Towns::getInstance().addTown(townid, town);
				}

				if(nodeValue = (char*)xmlGetProp(nodeTown, (const xmlChar *) "name")){
					town->setName(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}

				if(nodeValue = (char*)xmlGetProp(nodeTown, (const xmlChar *) "templex")){
					templePos.x = atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}

				if(nodeValue = (char*)xmlGetProp(nodeTown, (const xmlChar *) "templey")){
					templePos.y = atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}

				if(nodeValue = (char*)xmlGetProp(nodeTown, (const xmlChar *) "templez")){
					templePos.z = atoi(nodeValue);
					xmlFreeOTSERV(nodeValue);
				}
				
				town->setTemplePos(templePos);

				nodeTown = nodeTown->next;
			}
		}

		nodeChild = nodeChild->next;
	}

 	xmlFreeDoc(doc);

	return (map->getLastError() == LOADMAPERROR_NONE);
}

bool IOMapSerializeXML::saveMap(Map* map, const std::string& identifier)
{
	xmlDocPtr doc = xmlNewDoc((xmlChar*) "1.0");
  xmlNodePtr nodeMap = xmlNewNode(NULL, (xmlChar*) "map");
  xmlDocSetRootElement(doc, nodeMap);

	for(HouseMap::iterator it = Houses::getInstance().getHouseBegin(); it != Houses::getInstance().getHouseEnd(); ++it){
		//save house items
		House* house = it->second;
		for(HouseTileList::iterator it = house->getHouseTileBegin(); it != house->getHouseTileEnd(); ++it){
			saveTile(nodeMap, *it);
		}
	}
	
	xmlSaveFormatFileEnc(identifier.c_str(), doc, "UTF-8", 1);

	xmlFreeDoc(doc);
	
	return true;
}

bool IOMapSerializeXML::saveTile(xmlNodePtr nodeMap, const Tile* tile)
{
	Item* item;
	xmlNodePtr nodeTile = NULL;

	for(int i = tile->getThingCount() - 1; i >= 0; --i){
		item = tile->__getThing(i)->getItem();

		if(!item)
			continue;

		if(!(!item->isNotMoveable() || item->isDoor() || item->getContainer() /*|| item->isBed()*/))
			continue;

		if(!nodeTile){
			nodeTile = xmlNewChild(nodeMap, NULL, (xmlChar*) "tile", NULL);

			const Position& tilePos = tile->getPosition();

			std::stringstream ss;
			ss.str("");
			ss << tilePos.x;
			xmlNewProp(nodeTile, (xmlChar*) "x", (xmlChar*) ss.str().c_str());

			ss.str("");
			ss << tilePos.y;
			xmlNewProp(nodeTile, (xmlChar*) "y", (xmlChar*) ss.str().c_str());

			ss.str("");
			ss << tilePos.z;
			xmlNewProp(nodeTile, (xmlChar*) "z", (xmlChar*) ss.str().c_str());
		}

		xmlNodePtr nodeItem = item->serialize();
		xmlAddChild(nodeTile, nodeItem);
	}
	
	return true;
}

bool IOMapSerializeXML::loadTile(Map* map, xmlNodePtr nodeTile, Tile* tile)
{
	xmlNodePtr nodeItem;
	char* nodeValue;
	Item* item = NULL;

	unsigned short ground = 0;
	if(nodeValue = (char*)xmlGetProp(nodeTile, (const xmlChar *) "ground")){
		ground = atoi(nodeValue);
		xmlFreeOTSERV(nodeValue);
	}

	if(ground != 0){
		Item* myGround = Item::CreateItem(ground);
		tile->__internalAddThing(myGround);
	}

	if(nodeValue = (char*)xmlGetProp(nodeTile, (const xmlChar *) "pz")){ 
		if(strcmp(nodeValue, "1") == 0){
			tile->setFlag(TILESTATE_PROTECTIONZONE);
		}

		xmlFreeOTSERV(nodeValue);
	}
		  
	nodeItem = nodeTile->children;
	while(nodeItem){
		if(xmlStrcmp(nodeItem->name,(const xmlChar*) "item")==0){          
			unsigned int id = 0;
			item = NULL;

			if(nodeValue = (char*)xmlGetProp(nodeItem, (const xmlChar *) "id")){
				id = atoi(nodeValue);
				xmlFreeOTSERV(nodeValue);
			}
			
			if(!map->defaultMapLoaded || Item::items[id].moveable){
				//create a new item
				item = Item::CreateItem(id);

				if(!item){
					map->setLastError(LOADMAPERROR_FAILEDTOCREATEITEM);
					return false;
				}

				if(!item->unserialize(nodeItem)){
					std::cout << "WARNING: Serialize error in IOMapSerializeXML::loadTile()" << std::endl;
				}

				tile->__internalAddThing(item);

				Door* door = item->getDoor();
				if(door && door->getDoorId() != 0){
					if(tile->hasFlag(TILESTATE_HOUSE)){
						HouseTile* houseTile = dynamic_cast<HouseTile*>(tile);
						House* house = houseTile->getHouse();
						if(!house){
							map->setLastError(LOADMAPERROR_FAILEDUNSERIALIZEITEM);
							return false;
						}
						
						house->addDoor(door);
					}
				}
			}
			else{
				//find this type in the tile
				for(int i = 0; i < tile->getThingCount(); ++i){
					Item* findItem = tile->__getThing(i)->getItem();

					if(!findItem)
						continue;

					if(findItem->getID() == id){
						item = findItem;
						if(!item->unserialize(nodeItem)){
							std::cout << "WARNING: Serialize error in IOMapSerializeXML::loadTile()" << std::endl;
						}

						break;
					}
				}
			}
		}
		
		nodeItem = nodeItem->next;
	}

	return true;
}

bool IOMapSerializeXML::loadHouseInfo(Map* map, const std::string& identifier)
{
	return false;
}

bool IOMapSerializeXML::saveHouseInfo(Map* map, const std::string& identifier)
{
	return false;
}


