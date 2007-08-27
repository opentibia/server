//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// OTBM map loader
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

#include "iomapotbm.h"
#include "game.h"
#include "map.h"

#include "tile.h"
#include "item.h"
#include "container.h"
#include "depot.h"
#include "teleport.h"
#include "fileloader.h"
#include "town.h"
#include "house.h"

typedef uint8_t attribute_t;
typedef uint32_t flags_t;

extern Game g_game;

enum tile_flags_t{
	TILE_PZ = 1,
};
/*
	OTBM_ROOTV1
	|
	|--- OTBM_MAP_DATA
	|	|
	|	|--- OTBM_TILE_AREA
	|	|	|--- OTBM_TILE
	|	|	|--- OTBM_TILE_SQUARE (not implemented)
	|	|	|--- OTBM_TILE_REF (not implemented)
	|	|	|--- OTBM_HOUSETILE
	|	|
	|	|--- OTBM_SPAWNS (not implemented)
	|	|	|--- OTBM_SPAWN_AREA (not implemented)
	|	|	|--- OTBM_MONSTER (not implemented)
	|	|
	|	|--- OTBM_TOWNS
	|		|--- OTBM_TOWN
	|
	|--- OTBM_ITEM_DEF (not implemented)
*/

bool IOMapOTBM::loadMap(Map* map, const std::string& identifier)
{
	int64_t start = OTSYS_TIME();
	map->setLastError(LOADMAPERROR_NONE);

	FileLoader f;
	if(!f.openFile(identifier.c_str(), false, true)){
		map->setLastError(LOADMAPERROR_CANNOTOPENFILE);
		return false;
	}
	
	unsigned long type;
	PropStream propStream;

	NODE root = f.getChildNode((NODE)NULL, type);

	if(!f.getProps(root, propStream)){
		map->setLastError(LOADMAPERROR_GETPROPFAILED, root);
		return false;
	}

	OTBM_root_header* root_header;
	if(!propStream.GET_STRUCT(root_header)){
		map->setLastError(LOADMAPERROR_GETROOTHEADERFAILED, root);
		return false;
	}
	
	if(root_header->version != 0){
		map->setLastError(LOADMAPERROR_OUTDATEDHEADER, root);
		return false;
	}
	
	if(root_header->majorVersionItems > (unsigned long)Items::dwMajorVersion){
		map->setLastError(LOADMAPERROR_OUTDATEDHEADER, root);
		return false;
	}

	// Prevent load maps saved with items.otb previous to
	// version 800, because of the change to stackable of 
	// itemid 3965
	if(root_header->minorVersionItems < CLIENT_VERSION_800){
		map->setLastError(LOADMAPERROR_OUTDATEDHEADER, root);
		return false;
	}

	if(root_header->minorVersionItems > (unsigned long)Items::dwMinorVersion){
		std::cout << "Warning: [OTBM loader] This map needs an updated items OTB file." <<std::endl;
	}

	std::cout << "Map size: " << root_header->width << "x" << root_header->height << std::endl;

	NODE nodeMap = f.getChildNode(root, type);
	
	if(type != OTBM_MAP_DATA){
		map->setLastError(LOADMAPERROR_UNKNOWNNODETYPE, nodeMap);
		return false;
	}

	if(!f.getProps(nodeMap, propStream)){
		map->setLastError(LOADMAPERROR_GETPROPFAILED, nodeMap);
		return false;
	}

	unsigned char attribute;
	std::string mapDescription;
	std::string tmp;
	while(propStream.GET_UCHAR(attribute)){
		switch(attribute){
		case OTBM_ATTR_DESCRIPTION:
			if(!propStream.GET_STRING(mapDescription)){
				map->setLastError(LOADMAPERROR_GETPROPFAILED, nodeMap);
				return false;
			}
			
			std::cout << "Map description: " << mapDescription << std::endl;
			break;
		case OTBM_ATTR_EXT_SPAWN_FILE:
			if(!propStream.GET_STRING(tmp)){
				map->setLastError(LOADMAPERROR_GETPROPFAILED, nodeMap);
				return false;
			}

			map->spawnfile = identifier.substr(0, identifier.rfind('/') + 1);
			map->spawnfile += tmp;

			break;
		case OTBM_ATTR_EXT_HOUSE_FILE:
			if(!propStream.GET_STRING(tmp)){
				map->setLastError(LOADMAPERROR_GETPROPFAILED, nodeMap);
				return false;
			}

			map->housefile = identifier.substr(0, identifier.rfind('/') + 1);
			map->housefile += tmp;
			break;

		default:
			map->setLastError(LOADMAPERROR_UNKNOWNNODETYPE, nodeMap);
			return false;
			break;
		}
	}
	
	Tile* tile = NULL;

	NODE nodeMapData = f.getChildNode(nodeMap, type);
	while(nodeMapData != NO_NODE){
		if(f.getError() != ERROR_NONE){
			map->setLastError(LOADMAPERROR_FAILEDTOREADCHILD, nodeMapData);
			return false;
		}

		if(type == OTBM_TILE_AREA){
			if(!f.getProps(nodeMapData, propStream)){
				map->setLastError(LOADMAPERROR_GETPROPFAILED, nodeMapData);
				return false;
			}
			
			OTBM_Tile_area_coords* area_coord;
			if(!propStream.GET_STRUCT(area_coord)){
				map->setLastError(LOADMAPERROR_GETPROPFAILED, nodeMapData);
				return false;
			}
			
			int base_x, base_y, base_z;
			base_x = area_coord->_x;
			base_y = area_coord->_y;
			base_z = area_coord->_z;
			
			NODE nodeTile = f.getChildNode(nodeMapData, type);
			while(nodeTile != NO_NODE){
				if(f.getError() != ERROR_NONE){
					map->setLastError(LOADMAPERROR_FAILEDTOREADCHILD, nodeTile);
					return false;
				}

				if(type == OTBM_TILE || type == OTBM_HOUSETILE){
					if(!f.getProps(nodeTile, propStream)){
						map->setLastError(LOADMAPERROR_GETPROPFAILED, nodeTile);
						return false;
					}
					
					unsigned short px, py, pz;
					OTBM_Tile_coords* tile_coord;
					if(!propStream.GET_STRUCT(tile_coord)){
						map->setLastError(LOADMAPERROR_GETPROPFAILED, nodeTile);
						return false;
					}

					px = base_x + tile_coord->_x;
					py = base_y + tile_coord->_y;
					pz = base_z;
					
					bool isHouseTile = false;
					House* house = NULL;

					if(type == OTBM_TILE){
						tile = new Tile(px, py, pz);
					}
					else if(type == OTBM_HOUSETILE){
						uint32_t _houseid;
						if(!propStream.GET_ULONG(_houseid)){
							map->setLastError(LOADMAPERROR_GETPROPFAILED, nodeTile);
							return false;
						}

						house = Houses::getInstance().getHouse(_houseid, true);
						if(!house){
							map->setLastError(LOADMAPERROR_FAILEDTOCREATEITEM, nodeTile);
							return false;
						}

						tile = new HouseTile(px, py, pz, house);
						house->addTile(static_cast<HouseTile*>(tile));
						isHouseTile = true;
					}
					else{
						map->setLastError(LOADMAPERROR_UNKNOWNNODETYPE, nodeTile);
						return false;
					}
					
					map->setTile(px, py, pz, tile);

					//read tile attributes
					unsigned char attribute;
					while(propStream.GET_UCHAR(attribute)){
						switch(attribute){
						case OTBM_ATTR_TILE_FLAGS:
						{
							uint32_t flags;
							if(!propStream.GET_ULONG(flags)){
								map->setLastError(LOADMAPERROR_GETPROPFAILED, nodeTile);
								return false;
							}
								
							if(flags & TILE_PZ)
								tile->setPz();
								
							break;
						}

						case OTBM_ATTR_ITEM:
						{
							Item* item = Item::CreateItem(propStream);
							if(!item){
								map->setLastError(LOADMAPERROR_FAILEDTOCREATEITEM, nodeTile);
								return false;
							}

							if(isHouseTile && !item->isNotMoveable()){
								std::cout << "Warning: [OTBM loader] Moveable item in house id = " << house->getHouseId() << " Item type = " << item->getID() << std::endl;
								delete item;
								item = NULL;
							}
							else{
								tile->__internalAddThing(item);
								item->__startDecaying();
							}
							
							break;
						}

						default:
							map->setLastError(LOADMAPERROR_UNKNOWNNODETYPE, nodeTile);
							return false;
							break;
						}
					}

					NODE nodeItem = f.getChildNode(nodeTile, type);
					while(nodeItem){
						if(type == OTBM_ITEM){

							PropStream propStream;
							f.getProps(nodeItem, propStream);
							
							Item* item = Item::CreateItem(propStream);
							if(!item){
								map->setLastError(LOADMAPERROR_FAILEDTOCREATEITEM, nodeItem);
								return false;
							}

							if(item->unserializeItemNode(f, nodeItem, propStream)){
								if(isHouseTile && !item->isNotMoveable()){
									std::cout << "Warning: [OTBM loader] Moveable item in house id = " << house->getHouseId() << " Item type = " << item->getID() << std::endl;
									delete item;
								}
								else{
									tile->__internalAddThing(item);
									item->__startDecaying();

									if(isHouseTile){
										Door* door = item->getDoor();
										if(door && door->getDoorId() != 0){
											house->addDoor(door);
										}
									}
								}
							}
							else{
								delete item;
								map->setLastError(LOADMAPERROR_FAILEDUNSERIALIZEITEM, nodeItem);
								return false;
							}
						}
						else{
							map->setLastError(LOADMAPERROR_UNKNOWNNODETYPE, nodeItem);
						}

						nodeItem = f.getNextNode(nodeItem, type);
					}
				}
				else{
					map->setLastError(LOADMAPERROR_UNKNOWNNODETYPE, nodeTile);
				}

				nodeTile = f.getNextNode(nodeTile, type);
			}
		}
		else if(type == OTBM_TOWNS){
			NODE nodeTown = f.getChildNode(nodeMapData, type);
			while(nodeTown != NO_NODE){
				if(type == OTBM_TOWN){
					if(!f.getProps(nodeTown, propStream)){
						map->setLastError(LOADMAPERROR_UNKNOWNNODETYPE, nodeTown);
						return false;
					}
					
					uint32_t townid = 0;
					if(!propStream.GET_ULONG(townid)){
						map->setLastError(LOADMAPERROR_GETPROPFAILED, nodeTown);
						return false;
					}

					Town* town = Towns::getInstance().getTown(townid);
					if(!town){
						town = new Town(townid);
						Towns::getInstance().addTown(townid, town);
					}

					std::string townName = "";
					if(!propStream.GET_STRING(townName)){
						map->setLastError(LOADMAPERROR_GETPROPFAILED, nodeTown);
						return false;
					}

					town->setName(townName);

					OTBM_TownTemple_coords *town_coords;
					if(!propStream.GET_STRUCT(town_coords)){
						map->setLastError(LOADMAPERROR_GETPROPFAILED, nodeTown);
						return false;
					}

					Position pos;
					pos.x = town_coords->_x;
					pos.y = town_coords->_y;
					pos.z = town_coords->_z;
					town->setTemplePos(pos);
				}
				else{
					map->setLastError(LOADMAPERROR_UNKNOWNNODETYPE, nodeTown);
				}

				nodeTown = f.getNextNode(nodeTown, type);
			}
		}
		else{
			map->setLastError(LOADMAPERROR_UNKNOWNNODETYPE, nodeMapData);
		}

		nodeMapData = f.getNextNode(nodeMapData, type);
	}
	
	if(f.getError() != ERROR_NONE){
		map->setLastError(LOADMAPERROR_FAILEDTOREADCHILD);
		return false;
	}
	std::cout << "Notice: [OTBM Loader] Loading time : " << (OTSYS_TIME() - start)/(1000.) << " s" << std::endl;
	return (map->getLastError() == LOADMAPERROR_NONE);
}

bool IOMapOTBM::loadSpawns(Map* map)
{
	if(!map->spawnfile.empty()){
		Spawns::getInstance()->loadFromXml(map->spawnfile);
		Spawns::getInstance()->startup();
	}
	
	return true;
}

bool IOMapOTBM::loadHouses(Map* map)
{
	if(!map->housefile.empty()){
		return Houses::getInstance().loadHousesXML(map->housefile);
	}

	return true;
}
