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
enum clientVersion_t{
	CLIENT_VERSION_750 = 1,
	CLIENT_VERSION_755 = 2,
	CLIENT_VERSION_760 = 3,
};

enum rootattrib_t{
	ROOT_ATTR_VERSION = 0x01
};

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

	//1-byte aligned
	ITEM_ATTR_DECAY2,
	ITEM_ATTR_WEAPON2,
	ITEM_ATTR_AMU2,
	ITEM_ATTR_ARMOR2,
	ITEM_ATTR_WRITEABLE2,
	ITEM_ATTR_LIGHT2,

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
 FLAG_ROTABLE = 32768,
 FLAG_HANGABLE = 65536,
 FLAG_VERTICAL = 131072,
 FLAG_HORIZONTAL = 262144,
 FLAG_CANNOTDECAY = 524288
};

enum slotsOTB_t{
	OTB_SLOT_DEFAULT,
	OTB_SLOT_HEAD,
	OTB_SLOT_BODY,
	OTB_SLOT_LEGS,
	OTB_SLOT_BACKPACK,
	OTB_SLOT_WEAPON,
	OTB_SLOT_2HAND,
	OTB_SLOT_FEET,
	OTB_SLOT_AMULET,
	OTB_SLOT_RING,
	OTB_SLOT_HAND,
};

enum subfightOTB_t {
	OTB_DIST_NONE          = 0,
	OTB_DIST_BOLT          = 1,
	OTB_DIST_ARROW         = 2,
	OTB_DIST_FIRE          = 3,
	OTB_DIST_ENERGY        = 4,
	OTB_DIST_POISONARROW   = 5,
	OTB_DIST_BURSTARROW    = 6,
	OTB_DIST_THROWINGSTAR  = 7,
	OTB_DIST_THROWINGKNIFE = 8,
	OTB_DIST_SMALLSTONE    = 9,
	OTB_DIST_SUDDENDEATH   = 10,
	OTB_DIST_LARGEROCK     = 11,
	OTB_DIST_SNOWBALL      = 12,
	OTB_DIST_POWERBOLT     = 13,
	OTB_DIST_SPEAR         = 14,
	OTB_DIST_POISONFIELD   = 15
};

#pragma pack(8)
struct decayBlock{
	unsigned short decayTo;
	unsigned short decayTime;
};

struct weaponBlock{
	unsigned char weaponType;
	unsigned char amuType;
	unsigned char shootType;
	unsigned char attack;
	unsigned char defence;
};

struct amuBlock{
	unsigned char amuType;
	unsigned char shootType;
	unsigned char attack;
};

struct armorBlock{
	unsigned short armor;
	double weight;
	unsigned short slot_position;
};

struct writeableBlock{
	unsigned short readOnlyId;
};

struct lightBlock{
	unsigned short lightLevel;
	unsigned short lightColor;
};
#pragma pack()

//1-byte aligned structs
#pragma pack(1)

struct VERSIONINFO{
	unsigned long dwMajorVersion;
	unsigned long dwMinorVersion;
	unsigned long dwBuildNumber;
	char CSDVersion[128];
};

struct decayBlock2{
	unsigned short decayTo;
	unsigned short decayTime;
};

struct weaponBlock2{
	unsigned char weaponType;
	unsigned char amuType;
	unsigned char shootType;
	unsigned char attack;
	unsigned char defence;
};

struct amuBlock2{
	unsigned char amuType;
	unsigned char shootType;
	unsigned char attack;
};

struct armorBlock2{
	unsigned short armor;
	double weight;
	unsigned short slot_position;
};

struct writeableBlock2{
	unsigned short readOnlyId;
};

struct lightBlock2{
	unsigned short lightLevel;
	unsigned short lightColor;
};

#pragma pack()
/////////OTB specific//////////////

class ItemLoader : public FileLoader {
public:
	int setFlags(flags_t flags);
	int setProps(attribute_t attr, void* data, datasize_t size);
};

#endif
