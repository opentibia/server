//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// 
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

#ifndef __OTSERV_ITEMLOADER_H__
#define __OTSERV_ITEMLOADER_H__

#include "fileloader.h"

typedef unsigned char attribute_t;
typedef unsigned short datasize_t;
typedef unsigned long flags_t;

enum itemgroup_t{
	ITEM_GROUP_NONE = 0,
	ITEM_GROUP_GROUND,
	ITEM_GROUP_CONTAINER,
	ITEM_GROUP_WEAPON,
	ITEM_GROUP_AMMUNITION,
	ITEM_GROUP_ARMOR,
	ITEM_GROUP_RUNE,
	ITEM_GROUP_TELEPORT,
	ITEM_GROUP_MAGICFIELD,
	ITEM_GROUP_WRITEABLE,
	ITEM_GROUP_KEY,
	ITEM_GROUP_SPLASH,
	ITEM_GROUP_FLUID,
	ITEM_GROUP_LAST
};

/////////OTB specific//////////////
enum itemattrib_t {
	ITEM_ATTR_FIRST = 0x10,
	ITEM_ATTR_SERVERID = ITEM_ATTR_FIRST,
	ITEM_ATTR_CLIENTID,
	ITEM_ATTR_NAME,
	ITEM_ATTR_DESCR,
	ITEM_ATTR_SPEED,
	ITEM_ATTR_SLOT,
	ITEM_ATTR_MAXITEMS,
	ITEM_ATTR_WEIGHT,
	ITEM_ATTR_WEAPON,
	ITEM_ATTR_AMU,
	ITEM_ATTR_ARMOR,
	ITEM_ATTR_MAGLEVEL,
	ITEM_ATTR_MAGFIELDTYPE,
	ITEM_ATTR_WRITEABLE,
	ITEM_ATTR_ROTATETO,
	ITEM_ATTR_DECAY,
	ITEM_ATTR_SPRITEHASH,
	ITEM_ATTR_MINIMAPCOLOR,
	ITEM_ATTR_07,
	ITEM_ATTR_08,
	ITEM_ATTR_LIGHT,
	ITEM_ATTR_LAST
};

enum itemflags_t {
 FLAG_BLOCK_SOLID = 1,
 FLAG_BLOCK_PROJECTILE = 2, 
 FLAG_BLOCK_PATHFIND = 4, 
 FLAG_HAS_HEIGHT = 8,
 FLAG_USEABLE = 16,
 FLAG_PICKUPABLE = 32,
 FLAG_MOVEABLE = 64,
 FLAG_STACKABLE = 128,
 FLAG_FLOORCHANGEDOWN = 256,
 FLAG_FLOORCHANGENORTH = 512,
 FLAG_FLOORCHANGEEAST = 1024,
 FLAG_FLOORCHANGESOUTH = 2048,
 FLAG_FLOORCHANGEWEST = 4096,
 FLAG_ALWAYSONTOP = 8192,
 FLAG_READABLE = 16384,
 FLAG_ROTABLE = 32768
};

struct decayBlock{
	unsigned short decayTo;
	unsigned short decayTime;
};

struct weaponBlock {
	unsigned char weaponType;
	unsigned char amuType;
	unsigned char shootType;
	unsigned char attack;
	unsigned char defence;
};

struct amuBlock {
	unsigned char amuType;
	unsigned char shootType;
	unsigned char attack;
};

struct armorBlock {
	unsigned short armor;
	double weight;
	unsigned short slot_position;
};

struct writeableBlock {
	unsigned short readOnlyId;
};

struct lightBlock {
	unsigned short lightLevel;
	unsigned short lightColor;
};
/////////OTB specific//////////////

class ItemLoader : public FileLoader {
public:
	int setFlags(flags_t flags);
	int setProps(attribute_t attr, void* data, datasize_t size);
};

#endif
