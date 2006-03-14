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

typedef unsigned char attribute_t;
typedef unsigned long flags_t;

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
	
	if(root_header->majorVersionItems > Items::dwMajorVersion){
		map->setLastError(LOADMAPERROR_OUTDATEDHEADER, root);
		return false;
	}

	if(root_header->minorVersionItems > Items::dwMinorVersion){
		std::cout << "WARNING: This map needs an updated items OTB file." <<std::endl;
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
						unsigned long _houseid;
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
							unsigned long flags;
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

							if(item){
								tile->__internalAddThing(item);
							}
							else{
								map->setLastError(LOADMAPERROR_FAILEDUNSERIALIZEITEM, nodeTile);
								return false;
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
								tile->__internalAddThing(item);
								if(isHouseTile){
									Door* door = item->getDoor();
									if(door && door->getDoorId() != 0){
										house->addDoor(door);
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
					
					unsigned long townid = 0;
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

	return (map->getLastError() == LOADMAPERROR_NONE);
}

/*
Item* IOMapOTBM::unserializeItemAttr(PropStream& propStream)
{
	unsigned short _id;
	unsigned char _count;
	
	if(!propStream.GET_USHORT(_id)){
		return NULL;
	}

	Item* item = Item::CreateItem(_id);

	if(!item){
		return NULL;
	}

	if(item->isSplash() || item->isStackable() || item->isFluidContainer()){
		if(!propStream.GET_UCHAR(_count)){
			delete item;
			return NULL;
		}
	}

	return item;
}
*/

/*
Item* IOMapOTBM::unserializeItemNode(FileLoader* f, NODE node)
{
	PropStream propStream;
	f->getProps(node, propStream);

	unsigned short _id;
	if(!propStream.GET_USHORT(_id)){
		return NULL;
	}

	ItemType iType = Item::items[_id];
	Item* item;
	if(iType.id != 0){
		unsigned char _count = 0;

		if(iType.stackable || iType.isSplash() || iType.isFluidContainer()){
			if(!propStream.GET_UCHAR(_count)){
				return NULL;
			}
		}

		item = Item::CreateItem(_id, _count);
		
		if(!item){
			return NULL;
		}

		if(!item->unserializeItemNode(f, node, propStream)){
			delete item;
			return NULL;
		}
		
		unsigned char attr_type;
		while(propStream.GET_UCHAR(attr_type)){
			
			unsigned short tmp_short;
			std::string a;
			
			switch(attr_type){
			case OTBM_ATTR_ACTION_ID:
				if(!propStream.GET_USHORT(tmp_short)){
					delete item;
					return NULL;
				}
				item->setActionId(tmp_short);
				break;
			case OTBM_ATTR_UNIQUE_ID:
				if(!propStream.GET_USHORT(tmp_short)){
					delete item;
					return NULL;
				}
				item->setUniqueId(tmp_short);
				break;
			case OTBM_ATTR_TEXT:
				if(!propStream.GET_STRING(a)){
					delete item;
					return NULL;
				}
				item->setText(a);
				break;
			case OTBM_ATTR_DESC:
				if(!propStream.GET_STRING(a)){
					delete item;
					return NULL;
				}
				item->setSpecialDescription(a);
				break;
			case OTBM_ATTR_TELE_DEST:
				OTBM_TeleportDest* tele_dest;
				if(!propStream.GET_STRUCT(tele_dest)){
					delete item;
					return NULL;
				}

				if(Teleport* tele = item->getTeleport()){
					tele->setDestPos(Position(tele_dest->_x, tele_dest->_y, tele_dest->_z));
				}
				else{
					delete item;
					return NULL;
				}

				break;
			case OTBM_ATTR_DEPOT_ID:
				if(!propStream.GET_USHORT(tmp_short)){
					delete item;
					return NULL;
				}
				if(Depot* depot = dynamic_cast<Depot*>(item)){
					depot->setDepotId(tmp_short);
				}
				else{
					delete item;
					return NULL;
				}
				break;

			case OTBM_ATTR_RUNE_CHARGES:
			{
				unsigned char _charges = 1;
				if(!propStream.GET_UCHAR(_charges)){
					delete item;
					return NULL;
				}

				item->setItemCharge(_charges);
				break;
			}

			case OTBM_ATTR_HOUSEDOORID:
			{
				unsigned char _doorid = 0;
				if(!propStream.GET_UCHAR(_doorid)){
					delete item;
					return NULL;
				}
				Door* door = item->getDoor();
				if(door){
					door->setDoorId(_doorid);
				}
				else{
					delete item;
					return NULL;
				}	
				break;
			}

			default:
				delete item;
				return NULL;
				break;
			}
		}

		if(Container* container = item->getContainer()){
			unsigned long type;
			NODE item_node = f->getChildNode(node, type);
			while(item_node){
				if(type == OTBM_ITEM){
					Item* item = unserializeItemNode(f, item_node);
					if(item){
						container->__internalAddThing(item);
					}
					else{
						return false;
					}
				}
				else{
					OTBM_UNK_NODE_MSG
				}

				item_node = f->getNextNode(item_node, type);
			}
		}
	}
	else{
		return NULL;
	}

	return item;
}
*/

bool IOMapOTBM::loadSpawns(Map* map)
{
	if(!map->spawnfile.empty()){
		SpawnManager::getInstance().loadSpawnsXML(map->spawnfile);
		SpawnManager::getInstance().startup();
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
