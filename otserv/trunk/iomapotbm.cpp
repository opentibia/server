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

enum OTBM_NodeTypes_t{
	OTBM_ROOTV1 = 1,
	OTBM_MAP_DATA = 2,
	OTBM_ITEM_DEF = 3,
	OTBM_TILE_AREA = 4,
	OTBM_TILE = 5,
	OTBM_ITEM = 6,
	OTBM_TILE_SQUARE = 7,
	OTBM_TILE_REF = 8,
	OTBM_SPAWNS = 9,
	OTBM_SPAWN_AREA = 10,
	OTBM_MONSTER = 11,
	OTBM_TOWNS = 12,
	OTBM_TOWN = 13,
	OTBM_HOUSETILE = 14,
};

enum OTBM_AttrTypes_t{
	OTBM_ATTR_DESCRIPTION = 1,
	OTBM_ATTR_EXT_FILE = 2,
	OTBM_ATTR_TILE_FLAGS = 3,
	OTBM_ATTR_ACTION_ID = 4,
	OTBM_ATTR_UNIQUE_ID = 5,
	OTBM_ATTR_TEXT = 6,
	OTBM_ATTR_DESC = 7,
	OTBM_ATTR_TELE_DEST = 8,
	OTBM_ATTR_ITEM = 9,
	OTBM_ATTR_DEPOT_ID = 10,
	OTBM_ATTR_EXT_SPAWN_FILE = 11,
	OTBM_ATTR_RUNE_CHARGES = 12,
	OTBM_ATTR_EXT_HOUSE_FILE = 13,
	OTBM_ATTR_HOUSEDOORID = 14
};

#pragma pack(1)

struct OTBM_root_header{
	unsigned long version;
	unsigned short width;
	unsigned short height;
	unsigned long majorVersionItems;
	unsigned long minorVersionItems;
};

struct OTBM_TeleportDest{
	unsigned short _x;
	unsigned short _y;
	unsigned char	_z;
};

struct OTBM_Tile_area_coords{
	unsigned short _x;
	unsigned short _y;
	unsigned char _z;
};


struct OTBM_Tile_coords{
	unsigned char _x;
	unsigned char _y;
};

struct OTBM_TownTemple_coords{
	unsigned short _x;
	unsigned short _y;
	unsigned char _z;
};

struct OTBM_HouseTile_coords{
	unsigned char _x;
	unsigned char _y;
	unsigned long _houseid;
};

#pragma pack()

#define OTBM_UNK_NODE_MSG std::cout << "Warning: OTBM loader unknown node type." << std::endl;

bool IOMapOTBM::loadMap(Map* map, const std::string& identifier)
{
	setLastError(LOADMAPERROR_NONE);
	FileLoader f;
	if(!f.openFile(identifier.c_str(), false, true)){
		setLastError(LOADMAPERROR_CANNOTOPENFILE);
		return false;
	}
	
	unsigned long type;
	PropStream propStream;

	NODE root = f.getChildNode((NODE)NULL, type);

	if(!f.getProps(root, propStream)){
		setLastError(LOADMAPERROR_GETPROPFAILED, root);
		return false;
	}

	OTBM_root_header* root_header;
	if(!propStream.GET_STRUCT(root_header)){
		setLastError(LOADMAPERROR_GETROOTHEADERFAILED, root);
		return false;
	}
	
	if(root_header->version != 0){
		setLastError(LOADMAPERROR_OUTDATEDHEADER, root);
		return false;
	}
	
	if(root_header->majorVersionItems > Items::dwMajorVersion){
		setLastError(LOADMAPERROR_OUTDATEDHEADER, root);
		return false;
	}

	if(root_header->minorVersionItems > Items::dwMinorVersion){
		std::cout << "WARNING: This map needs an updated items OTB file." <<std::endl;
	}

	std::cout << "Map size: " << root_header->width << "x" << root_header->height << std::endl;

	NODE map_data = f.getChildNode(root, type);
	
	if(type != OTBM_MAP_DATA){
		setLastError(LOADMAPERROR_UNKNOWNNODETYPE, map_data);
		return false;
	}

	if(!f.getProps(map_data, propStream)){
		setLastError(LOADMAPERROR_GETPROPFAILED, map_data);
		return false;
	}

	unsigned char attribute;
	std::string mapDescription;
	std::string tmp;
	while(propStream.GET_UCHAR(attribute)){
		switch(attribute){
		case OTBM_ATTR_DESCRIPTION:
			if(!propStream.GET_STRING(mapDescription)){
				setLastError(LOADMAPERROR_GETPROPFAILED, map_data);
				return false;
			}
			
			std::cout << "Map description: " << mapDescription << std::endl;
			break;
		case OTBM_ATTR_EXT_SPAWN_FILE:
			if(!propStream.GET_STRING(tmp)){
				setLastError(LOADMAPERROR_GETPROPFAILED, map_data);
				return false;
			}
			spawnfile = identifier.substr(0, identifier.rfind('/') + 1);
			spawnfile += tmp;

			break;
		case OTBM_ATTR_EXT_HOUSE_FILE:
			if(!propStream.GET_STRING(tmp)){
				setLastError(LOADMAPERROR_GETPROPFAILED, map_data);
				return false;
			}

			housefile = identifier.substr(0, identifier.rfind('/') + 1);
			housefile += tmp;
			break;

		default:
			setLastError(LOADMAPERROR_UNKNOWNNODETYPE, map_data);
			return false;
			break;
		}
	}
	
	Tile* tile = NULL;

	NODE tile_area = f.getChildNode(map_data, type);
	while(tile_area != NO_NODE){
		if(f.getError() != ERROR_NONE){
			setLastError(LOADMAPERROR_FAILEDTOREADCHILD, tile_area);
			return false;
		}

		if(type == OTBM_TILE_AREA){
			if(!f.getProps(tile_area, propStream)){
				setLastError(LOADMAPERROR_GETPROPFAILED, tile_area);
				return false;
			}
			
			OTBM_Tile_area_coords* area_coord;
			if(!propStream.GET_STRUCT(area_coord)){
				setLastError(LOADMAPERROR_GETPROPFAILED, tile_area);
				return false;
			}
			
			int base_x, base_y, base_z;
			base_x = area_coord->_x;
			base_y = area_coord->_y;
			base_z = area_coord->_z;
			
			NODE tile_node = f.getChildNode(tile_area, type);
			while(tile_node != NO_NODE){
				if(f.getError() != ERROR_NONE){
					setLastError(LOADMAPERROR_FAILEDTOREADCHILD, tile_area);
					return false;
				}

				if(type == OTBM_TILE || type == OTBM_HOUSETILE){
					if(!f.getProps(tile_node, propStream)){
						setLastError(LOADMAPERROR_GETPROPFAILED, tile_area);
						return false;
					}
					
					unsigned short px, py, pz;
					OTBM_Tile_coords* tile_coord;
					if(!propStream.GET_STRUCT(tile_coord)){
						setLastError(LOADMAPERROR_GETPROPFAILED, tile_node);
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
						unsigned long houseid;
						if(!propStream.GET_ULONG(houseid)){
							setLastError(LOADMAPERROR_GETPROPFAILED, tile_node);
							return false;
						}

						house = Houses::getInstance().getHouse(houseid);
						if(!house){
							setLastError(LOADMAPERROR_FAILEDTOCREATEITEM, tile_node);
							return false;
						}

						tile = new HouseTile(px, py, pz, house);
						house->addTile(static_cast<HouseTile*>(tile));
						isHouseTile = true;
					}
					else{
						setLastError(LOADMAPERROR_UNKNOWNNODETYPE, tile_node);
						return false;
					}
					
					map->setTile(px, py, pz, tile);

					//read tile attributes
					unsigned char attribute;
					while(propStream.GET_UCHAR(attribute)){
						switch(attribute){
						case OTBM_ATTR_TILE_FLAGS:
							unsigned long flags;
							if(!propStream.GET_ULONG(flags)){
								setLastError(LOADMAPERROR_GETPROPFAILED, tile_node);
								return false;
							}
								
							if(flags & TILE_PZ)
								tile->setPz();;
								
							break;
						case OTBM_ATTR_ITEM:
							Item* item;
							
							item  = unserializaItemAttr(propStream);
							if(item){
								tile->__internalAddThing(item);
							}
							else{
								setLastError(LOADMAPERROR_FAILEDUNSERIALIZEITEM, tile_node);
								return false;
							}
							break;

						default:
							setLastError(LOADMAPERROR_UNKNOWNNODETYPE, tile_node);
							return false;
							break;
						}
					}
					NODE item_node = f.getChildNode(tile_node, type);
					while(item_node){
						if(type == OTBM_ITEM){
							Item* item = unserializaItemNode(&f, item_node);
							if(item){
								tile->__internalAddThing(item);
								if(isHouseTile){
									Door* door = item->getDoor();
									if(door && door->getDoorId() != 0){
										house->addDoor(door);
									}
								}
							}
							else{
								setLastError(LOADMAPERROR_FAILEDUNSERIALIZEITEM, item_node);
								return false;
							}
						}
						else{
							setLastError(LOADMAPERROR_UNKNOWNNODETYPE, item_node);
							//OTBM_UNK_NODE_MSG
						}
						item_node = f.getNextNode(item_node, type);
					}
				}
				else{
					setLastError(LOADMAPERROR_UNKNOWNNODETYPE, tile_node);
					//OTBM_UNK_NODE_MSG
				}
				tile_node = f.getNextNode(tile_node, type);
			}
		}
		else if(type == OTBM_TOWNS){
			NODE childtownnode = f.getChildNode(tile_area, type);
			while(childtownnode != NO_NODE){
				if(type == OTBM_TOWN){
					if(!f.getProps(childtownnode, propStream)){
						setLastError(LOADMAPERROR_UNKNOWNNODETYPE, childtownnode);
						return false;
					}
					
					unsigned long townid = 0;
					if(!propStream.GET_ULONG(townid)){
						setLastError(LOADMAPERROR_GETPROPFAILED, childtownnode);
						return false;
					}

					Town* town = Towns::getInstance().getTown(townid);
					if(!town){
						town = new Town(townid);
						Towns::getInstance().addTown(townid, town);
					}

					std::string townName = "";
					if(!propStream.GET_STRING(townName)){
						setLastError(LOADMAPERROR_GETPROPFAILED, childtownnode);
						return false;
					}

					town->setName(townName);

					OTBM_TownTemple_coords *town_coords;
					if(!propStream.GET_STRUCT(town_coords)){
						setLastError(LOADMAPERROR_GETPROPFAILED, childtownnode);
						return false;
					}

					Position pos;
					pos.x = town_coords->_x;
					pos.y = town_coords->_y;
					pos.z = town_coords->_z;
					town->setTemplePos(pos);
				}
				else{
					setLastError(LOADMAPERROR_UNKNOWNNODETYPE, childtownnode);
					//OTBM_UNK_NODE_MSG
				}

				childtownnode = f.getNextNode(childtownnode, type);
			}
		}
		else{
			setLastError(LOADMAPERROR_UNKNOWNNODETYPE, tile_area);
			//OTBM_UNK_NODE_MSG
		}

		tile_area = f.getNextNode(tile_area, type);
	}
	
	if(f.getError() != ERROR_NONE){
		setLastError(LOADMAPERROR_FAILEDTOREADCHILD);
		return false;
	}

	return (getLastError() == LOADMAPERROR_NONE);
	return true;
}

Item* IOMapOTBM::unserializaItemAttr(PropStream &propStream)
{
	unsigned short _id;
	unsigned char _count;
	
	if(!propStream.GET_USHORT(_id)){
		return NULL;
	}
	Item* item = Item::CreateItem(_id);

	if(item->isSplash() || item->isStackable() || item->isFluidContainer()){
		if(!propStream.GET_UCHAR(_count)){
			delete item;
			return NULL;
		}
	}
	return item;
}

Item* IOMapOTBM::unserializaItemNode(FileLoader* f, NODE node)
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

				if(Teleport* tele = dynamic_cast<Teleport*>(item)){
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
		
		Container* container;
		if(container = dynamic_cast<Container*>(item)){
			unsigned long type;
			NODE item_node = f->getChildNode(node, type);
			while(item_node){
				if(type == OTBM_ITEM){
					Item* item = unserializaItemNode(f, item_node);
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


bool IOMapOTBM::loadSpawns()
{
	if(!spawnfile.empty()){
		SpawnManager::initialize(&g_game);
		SpawnManager::instance()->loadSpawnsXML(spawnfile);
		SpawnManager::instance()->startup();
	}
	
	return true;
}

bool IOMapOTBM::loadHouses()
{
	if(!housefile.empty()){
		return Houses::getInstance().loadHousesXML(housefile);
	}

	return true;
}
