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

#include "definitions.h"
#include "fileloader.h"

typedef uint8_t attribute_t;
typedef uint16_t datasize_t;
typedef uint32_t flags_t;

enum itemgroup_t{
	ITEM_GROUP_NONE = 0,
	ITEM_GROUP_GROUND,
	ITEM_GROUP_CONTAINER,
	ITEM_GROUP_WEAPON,		/*deprecated*/
	ITEM_GROUP_AMMUNITION,	/*deprecated*/
	ITEM_GROUP_ARMOR,		/*deprecated*/
	ITEM_GROUP_CHARGES,
	ITEM_GROUP_TELEPORT,	/*deprecated*/
	ITEM_GROUP_MAGICFIELD,	/*deprecated*/
	ITEM_GROUP_WRITEABLE,	/*deprecated*/
	ITEM_GROUP_KEY,			/*deprecated*/
	ITEM_GROUP_SPLASH,
	ITEM_GROUP_FLUID,
	ITEM_GROUP_DOOR,		/*deprecated*/
	ITEM_GROUP_DEPRECATED,
	ITEM_GROUP_LAST
};

/////////OTB specific//////////////
enum clientVersion_t{
	CLIENT_VERSION_750 = 1,
	CLIENT_VERSION_755 = 2,
	CLIENT_VERSION_760 = 3,
	CLIENT_VERSION_770 = 3,
	CLIENT_VERSION_780 = 4,
	CLIENT_VERSION_790 = 5,
	CLIENT_VERSION_792 = 6,
	CLIENT_VERSION_800 = 7,
	CLIENT_VERSION_810 = 8,
	CLIENT_VERSION_811 = 9,
	CLIENT_VERSION_820 = 10,
	CLIENT_VERSION_830 = 11,
	CLIENT_VERSION_840 = 12,
	CLIENT_VERSION_841 = 13,
	CLIENT_VERSION_842 = 14,
	CLIENT_VERSION_850 = 15,
	CLIENT_VERSION_854_BAD = 16,
	CLIENT_VERSION_854 = 17,
	CLIENT_VERSION_855 = 18,
	CLIENT_VERSION_860_OLD = 19,
	CLIENT_VERSION_860 = 20,
	CLIENT_VERSION_861 = 21
};

enum rootattrib_t{
	ROOT_ATTR_VERSION = 0x01
};

enum itemattrib_t {
	ITEM_ATTR_FIRST = 0x10,
	ITEM_ATTR_SERVERID = ITEM_ATTR_FIRST,
	ITEM_ATTR_CLIENTID,
	ITEM_ATTR_NAME,				/*deprecated*/
	ITEM_ATTR_DESCR,			/*deprecated*/
	ITEM_ATTR_SPEED,
	ITEM_ATTR_SLOT,				/*deprecated*/
	ITEM_ATTR_MAXITEMS,			/*deprecated*/
	ITEM_ATTR_WEIGHT,			/*deprecated*/
	ITEM_ATTR_WEAPON,			/*deprecated*/
	ITEM_ATTR_AMU,				/*deprecated*/
	ITEM_ATTR_ARMOR,			/*deprecated*/
	ITEM_ATTR_MAGLEVEL,			/*deprecated*/
	ITEM_ATTR_MAGFIELDTYPE,		/*deprecated*/
	ITEM_ATTR_WRITEABLE,		/*deprecated*/
	ITEM_ATTR_ROTATETO,			/*deprecated*/
	ITEM_ATTR_DECAY,			/*deprecated*/
	ITEM_ATTR_SPRITEHASH,
	ITEM_ATTR_MINIMAPCOLOR,
	ITEM_ATTR_07,
	ITEM_ATTR_08,
	ITEM_ATTR_LIGHT,

	//1-byte aligned
	ITEM_ATTR_DECAY2,			/*deprecated*/
	ITEM_ATTR_WEAPON2,			/*deprecated*/
	ITEM_ATTR_AMU2,				/*deprecated*/
	ITEM_ATTR_ARMOR2,			/*deprecated*/
	ITEM_ATTR_WRITEABLE2,		/*deprecated*/
	ITEM_ATTR_LIGHT2,
	ITEM_ATTR_TOPORDER,
	ITEM_ATTR_WRITEABLE3,		/*deprecated*/
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
	FLAG_CANNOTDECAY = 524288,
	FLAG_ALLOWDISTREAD = 1048576,
	FLAG_UNUSED = 2097152,
	FLAG_CLIENTCHARGES = 4194304, /*deprecated*/
	FLAG_LOOKTHROUGH = 8388608
};

//1-byte aligned structs
#pragma pack(1)

struct VERSIONINFO{
	uint32_t dwMajorVersion;
	uint32_t dwMinorVersion;
	uint32_t dwBuildNumber;
	uint8_t CSDVersion[128];
};

struct lightBlock2{
	uint16_t lightLevel;
	uint16_t lightColor;
};

#pragma pack()
/////////OTB specific//////////////
#endif
