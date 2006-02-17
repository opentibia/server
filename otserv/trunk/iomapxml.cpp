//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// XML implementation of the Map Loader/Saver
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

#include "iomapxml.h"
#include "definitions.h"
#include "game.h"
#include "map.h"

#include "depot.h"
#include "house.h"
#include "housetile.h"
#include "town.h"
#include "spawn.h"

#include <iostream>

extern Game g_game;

bool IOMapXML::loadMap(Map* map, const std::string& identifier)
{
	xmlDocPtr doc;
	xmlNodePtr root, rootChildren, p, tmpNode;
	char* tmp;

	xmlLineNumbersDefault(1);
	std::cout << ":: Loaded map " << identifier << std::endl;
	doc=xmlParseFile(identifier.c_str());
	if(!doc){
		setLastError(LOADMAPERROR_CANNOTOPENFILE);
		return false;

		//std::cout << "FATAL: couldnt load map. exiting" << std::endl;
		//exit(1);
	}

	root = xmlDocGetRootElement(doc);
	if(xmlStrcmp(root->name,(const xmlChar*) "map")){
		xmlFreeDoc(doc);
		setLastError(LOADMAPERROR_GETROOTHEADERFAILED);
		return false;
		
		//std::cout << "FATAL: couldnt load map. exiting" << std::endl;
		//exit(1);
	}

	tmp = (char*)xmlGetProp(root, (const xmlChar *) "width");
	if(tmp){
		map->mapwidth = atoi(tmp);
		xmlFreeOTSERV(tmp);
	}

	tmp = (char*)xmlGetProp(root, (const xmlChar *) "height");
	if(tmp){
		map->mapheight = atoi(tmp);
		xmlFreeOTSERV(tmp);
	}
	std::cout << ":: W: " << map->mapwidth << "  H: " << map->mapheight << std::endl;
	
	if(tmp = (char*)xmlGetProp(root, (const xmlChar *) "spawnfile")){
		spawnfile = identifier.substr(0, identifier.rfind('/') + 1);
		spawnfile += tmp;

		xmlFreeOTSERV(tmp);
	}

	if(tmp = (char*)xmlGetProp(root, (const xmlChar *) "housefile")){
		housefile = identifier.substr(0, identifier.rfind('/') + 1);
		housefile += tmp;

		xmlFreeOTSERV(tmp);
	}

	rootChildren = root->children;

	int px,py,pz;
	int houseid = 0;
	Tile* tile;

	while(rootChildren){
		if(xmlStrcmp(rootChildren->name, (const xmlChar*)"tile") == 0){
			tmp = (char*)xmlGetProp(rootChildren, (const xmlChar *) "x");
			if(!tmp){
				setLastError(LOADMAPERROR_GETPROPFAILED, rootChildren->line);
				return false;

				//rootChildren = rootChildren->next;
				//continue;
			}

			px = atoi(tmp);
			xmlFreeOTSERV(tmp);

			tmp = (char*)xmlGetProp(rootChildren, (const xmlChar *) "y");
			if(!tmp){
				setLastError(LOADMAPERROR_GETPROPFAILED, rootChildren->line);
				return false;

				//rootChildren = rootChildren->next;
				//continue;
			}

			py = atoi(tmp);
			xmlFreeOTSERV(tmp);

			tmp = (char*)xmlGetProp(rootChildren, (const xmlChar *) "z");
			if(!tmp){
				setLastError(LOADMAPERROR_GETPROPFAILED, rootChildren->line);
				return false;

				//rootChildren = rootChildren->next;
				//continue;
			}

			pz = atoi(tmp);
			xmlFreeOTSERV(tmp);

			tmp = (char*)xmlGetProp(rootChildren, (const xmlChar *) "houseid");

			if(tmp){
				houseid = atoi(tmp);
				xmlFreeOTSERV(tmp);
			}
			else
				houseid = 0;

			bool isHouseTile = false;
			House* house = NULL;

			tmp = (char*)xmlGetProp(rootChildren, (const xmlChar *) "ground");
			unsigned short ground = 0;
			if(tmp){
				ground = atoi(tmp);
				xmlFreeOTSERV(tmp);
			}

			if(houseid == 0){
				tile = new Tile(px, py, pz);
			}
			else{
				house = Houses::getInstance().getHouse(houseid);
				if(!house){
					setLastError(LOADMAPERROR_FAILEDTOCREATEITEM, rootChildren->line);
					return false;
				}

				HouseTile* houseTile = new HouseTile(px, py, pz, house);
				house->addTile(houseTile);
				tile = houseTile;
				isHouseTile = true;
			}

			map->setTile(px, py, pz, tile);

			if(ground != 0){
				Item* myGround = Item::CreateItem(ground);
				tile->__internalAddThing(myGround);
			}

			tmp = (char*)xmlGetProp(rootChildren, (const xmlChar *) "pz");
			if(tmp && (strcmp(tmp, "1") == 0)){ 
				tile->setFlag(TILESTATE_PROTECTIONZONE);
				xmlFreeOTSERV(tmp);
			}
	       
			p = rootChildren->children;
			while(p){
				if(xmlStrcmp(p->name,(const xmlChar*) "item")==0){          
					tmp = (char*)xmlGetProp(p, (const xmlChar *) "id");
					unsigned int id;
					if(tmp){
						id = atoi(tmp);
						xmlFreeOTSERV(tmp);
					}
					else
						id = 0;
			
					Item* item;
					
					item = Item::CreateItem(id);

					if(!item){
						setLastError(LOADMAPERROR_FAILEDTOCREATEITEM, rootChildren->line);
						return false;
					}

					item->unserialize(p);
					Container* container = dynamic_cast<Container*>(item);

					if(container){
						//is depot?
						if(Depot* depot = container->getDepot()){
							tmp = (char*)xmlGetProp(p, (const xmlChar *) "depot");
							if(tmp){
								int depotId = atoi(tmp);					
								depot->setDepotId(depotId);
								xmlFreeOTSERV(tmp);
							}
						}

						if(p->children && strcmp((const char*)p->children->name, "inside") == 0){
							tmpNode = p->children->children;
							while(tmpNode){
								tmp = (char*)xmlGetProp(tmpNode, (const xmlChar *) "id");
								unsigned int id;
								if(tmp){
									id = atoi(tmp);
									xmlFreeOTSERV(tmp);
								}
								else
									id = 0;
							
								Item* myitem = Item::CreateItem(id);
								myitem->unserialize(tmpNode);
								container->__internalAddThing(myitem);
				
								Container* in_container = dynamic_cast<Container*>(myitem);
								if(in_container){
									LoadContainer(tmpNode,in_container);
								}
								tmpNode = tmpNode->next;
							}
						}
					}//loadContainer

					tile->__internalAddThing(item);

					if(isHouseTile){
						Door* door = item->getDoor();
						if(door && door->getDoorId() != 0){
							house->addDoor(door);
						}
					}
				}
				
				p = p->next;
			}
		}
		else if(xmlStrcmp(rootChildren->name, (const xmlChar*)"towns") == 0){
			p = rootChildren->children;

			if(xmlStrcmp(p->name,(const xmlChar*) "town") == 0){
				Position templePos;
				uint32_t townid = 0;

				tmp = (char*)xmlGetProp(p, (const xmlChar *) "townid");

				if(!tmp){
					setLastError(LOADMAPERROR_GETPROPFAILED, rootChildren->line);
					return false;
					//p = p->next;
					//continue;
				}

				townid = atoi(tmp);
				xmlFreeOTSERV(tmp);
		
				Town* town = Towns::getInstance().getTown(townid);
				if(!town){
					town = new Town(townid);
					Towns::getInstance().addTown(townid, town);
				}

				tmp = (char*)xmlGetProp(p, (const xmlChar *) "name");
				if(tmp){
					town->setName(tmp);
					xmlFreeOTSERV(tmp);
				}

				tmp = (char*)xmlGetProp(p, (const xmlChar *) "templex");
				if(tmp){
					templePos.x = atoi(tmp);
					xmlFreeOTSERV(tmp);
				}

				tmp = (char*)xmlGetProp(p, (const xmlChar *) "templey");
				if(tmp){
					templePos.y = atoi(tmp);
					xmlFreeOTSERV(tmp);
				}

				tmp = (char*)xmlGetProp(p, (const xmlChar *) "templez");
				if(tmp){
					templePos.z = atoi(tmp);
					xmlFreeOTSERV(tmp);
				}
				
				town->setTemplePos(templePos);

				p = p->next;
			}
		}

		rootChildren = rootChildren->next;
	}

 	xmlFreeDoc(doc);

	setLastError(LOADMAPERROR_NONE);
	return true;
}

bool IOMapXML::LoadContainer(xmlNodePtr nodeitem,Container* ccontainer)
{
	xmlNodePtr tmp,p;
	char *tmpc;
	
	if(nodeitem == NULL){
		return false;
	}
	tmp = nodeitem->children;
	if(tmp == NULL){
		return false;
	}
                  
	if(strcmp((const char*)tmp->name, "inside") == 0){
		//load items
		p = tmp->children;
		while(p){			
			tmpc = (char*)xmlGetProp(p, (const xmlChar *) "id");
			unsigned int id;
			if(tmpc){
				id = atoi(tmpc);
				xmlFreeOTSERV(tmp);
			}
			Item* myitem = Item::CreateItem(id);
			myitem->unserialize(p);			
			ccontainer->__internalAddThing(myitem);
			
			Container* in_container = dynamic_cast<Container*>(myitem);
			if(in_container){
				LoadContainer(p,in_container);
			}
			p = p->next;
		}

		return true;
	}

	return false;
}


bool IOMapXML::loadSpawns()
{
	if(!spawnfile.empty()){
		SpawnManager::initialize(&g_game);
		SpawnManager::instance()->loadSpawnsXML(spawnfile);
		SpawnManager::instance()->startup();
	}
	
	return true;
}

bool IOMapXML::loadHouses()
{
	if(!housefile.empty()){
		return Houses::getInstance().loadHousesXML(housefile);
	}

	return true;
}