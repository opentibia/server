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
#include "map.h"
#include "tile.h"
#include "item.h"
#include "container.h"
#include "fileloader.h"

typedef unsigned char attribute_t;
typedef unsigned long flags_t;

enum tile_flags_t{
	TILE_PZ = 1,
};

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

#pragma pack()


bool IOMapOTBM::loadMap(Map* map, std::string identifier)
{
	FileLoader f;
	if(!f.openFile(identifier.c_str(), false)) {
		return false;
	}
	
	unsigned long type;
	PropStream propStream;

	NODE root = f.getChildNode((NODE)NULL, type);

	if(!f.getProps(root, propStream)){
		return false;
	}

	OTBM_root_header* root_header;
	if(!propStream.GET_STRUCT(root_header)){
		return false;
	}
	
	if(root_header->version != 0){
		return false;
	}
	
	if(root_header->majorVersionItems > Items::dwMajorVersion){
		return false;
	}

	if(root_header->minorVersionItems > Items::dwMinorVersion){
		std::cout << "Warning: This map needs an updated items OTB file." <<std::endl;
	}

	std::cout << "Map size: " << root_header->width << "x" << root_header->height << std::endl;

	NODE map_data = f.getChildNode(root, type);

	if(!f.getProps(map_data, propStream)){
		return false;
	}

	unsigned char attribute;
	std::string tmp;
	std::string map_description;
	while(propStream.GET_UCHAR(attribute)){
		switch(attribute){
		case OTBM_ATTR_DESCRIPTION:
			if(!propStream.GET_STRING(map_description)){
				return false;
			}
			break;
		case OTBM_ATTR_EXT_SPAWN_FILE:
			if(!propStream.GET_STRING(tmp)){
				return false;
			}
			map->spawnfile = identifier.substr(0, identifier.rfind('/') + 1);
			map->spawnfile += tmp;
			break;
		default:
			return false;
			break;
		}
	}
		
	std::cout << "Map description: " << map_description << std::endl;
	
	NODE tile_area = f.getChildNode(map_data, type);
	while(tile_area != NO_NODE){
		if(f.getError() != ERROR_NONE){
			return false;
		}

		if(type == OTBM_TILE_AREA){
			if(!f.getProps(tile_area, propStream)){
				return false;
			}
			
			OTBM_Tile_area_coords* area_coord;
			if(!propStream.GET_STRUCT(area_coord)){
				return false;
			}
			
			int base_x, base_y, base_z;
			base_x = area_coord->_x;
			base_y = area_coord->_y;
			base_z = area_coord->_z;
			
			NODE tile_node = f.getChildNode(tile_area, type);
			while(tile_node != NO_NODE){
				if(f.getError() != ERROR_NONE){
					return false;
				}
				
				if(type == OTBM_TILE){
					if(!f.getProps(tile_node, propStream)){
						return false;
					}
					
					OTBM_Tile_coords* tile_coord;
					if(!propStream.GET_STRUCT(tile_coord)){
						return false;
					}
					unsigned short px, py, pz;
					px = base_x + tile_coord->_x;
					py = base_y + tile_coord->_y;
					pz = base_z;
					Tile* tile = map->setTile(px, py, pz);
					if(tile){
						//read tile attributes
						unsigned char attribute;
						while(propStream.GET_UCHAR(attribute)){
							switch(attribute){
							case OTBM_ATTR_TILE_FLAGS:
								unsigned long flags;
								if(!propStream.GET_ULONG(flags)){
									return false;
								}
								
								if(flags & TILE_PZ)
									tile->setPz();;
								
								break;
							case OTBM_ATTR_ITEM:
								Item* item;
								
								item  = unserializaItemAttr(propStream);
								if(item){
									item->pos.x = px;
									item->pos.y = py;
									item->pos.z = pz;
									tile->addThing(item);
								}
								else{
									return false;
								}
								break;
							default:
								return false;
								break;
							}
						}
					}
					NODE item_node = f.getChildNode(tile_node, type);
					while(item_node){
						if(type == OTBM_ITEM){
							Item* item = unserializaItemNode(&f, item_node);
							if(item){
								item->pos.x = px;
								item->pos.y = py;
								item->pos.z = pz;
								tile->addThing(item);
							}
							else{
								return false;
							}
						}
						item_node = f.getNextNode(item_node, type);
					}
				}
				tile_node = f.getNextNode(tile_node, type);
			}
		}
		tile_area = f.getNextNode(tile_area, type);
	}
	
	if(f.getError() != ERROR_NONE){
		return false;
	}

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
				if(Container* container = dynamic_cast<Container*>(item)){
					container->depot = tmp_short;
				}
				else{
					delete item;
					return NULL;
				}
				break;
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
						container->addItem(item);
					}
					else{
						return false;
					}
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
