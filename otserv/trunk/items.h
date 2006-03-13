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


#ifndef __OTSERV_ITEMS_H
#define __OTSERV_ITEMS_H


#ifdef __GNUC__
#include <ext/hash_map>
#else
#include <hash_map>
#endif
#include <string>
#include "const76.h"
#include "itemloader.h"


#define SLOTP_WHEREEVER 0xFFFFFFFF
#define SLOTP_HEAD 1
#define	SLOTP_NECKLACE 2
#define	SLOTP_BACKPACK 4
#define	SLOTP_ARMOR 8
#define	SLOTP_RIGHT 16
#define	SLOTP_LEFT 32
#define	SLOTP_LEGS 64
#define	SLOTP_FEET 128
#define	SLOTP_RING 256
#define	SLOTP_AMMO 512
#define	SLOTP_DEPOT 1024
#define	SLOTP_TWO_HAND 2048

enum eRWInfo{
	CAN_BE_READ = 1,
	CAN_BE_WRITTEN = 2
};

class ItemType {
public:
	ItemType();
	~ItemType();

	itemgroup_t group;

	bool isGroundTile() const;
	bool isContainer() const;
	bool isDoor() const;
	bool isTeleport() const;
	bool isMagicField() const;
	bool isKey() const;
	bool isSplash() const;
	bool isFluidContainer() const;

	unsigned short id;
	unsigned short clientId;

	unsigned short maxItems;    // maximum size if this is a container
	double         weight;      // weight of the item, e.g. throwing distance depends on it
	std::string    name;        // the name of the item
	std::string    description; // additional description... as in "The blade is a magic flame." for fireswords
	WeaponType     weaponType;
	amu_t          amuType;
	subfight_t     shootType;
	int            attack;
	int            defence;
	int            armor;
	unsigned short slot_position;
	unsigned short decayTo;
	unsigned short decayTime;
	bool           canDecay;
	bool           isVertical;
	bool           isHorizontal;
	bool           isHangable;
	bool           allowDistRead;

	uint16_t speed;

	// other bools
	int             magicfieldtype;
	int             RWInfo;
	unsigned short  readOnlyId;
	unsigned short  maxTextLen;
	bool            stackable;
	bool            useable;
	bool            moveable;
	bool            alwaysOnTop;
	unsigned char   alwaysOnTopOrder;
	int             runeMagLevel;
	bool            pickupable;
	bool            rotable;
	int				rotateTo;

	int lightLevel;
	int lightColor;

	bool floorChangeDown;
	bool floorChangeNorth;
	bool floorChangeSouth;
	bool floorChangeEast;
	bool floorChangeWest;
	bool hasHeight;

	bool blockSolid;
	bool blockPickupable;
	bool blockProjectile;
	bool blockPathFind;
};

#ifdef __GNUC__
typedef __gnu_cxx::hash_map<unsigned long, unsigned long> ReverseItemMap;
#else
typedef stdext::hash_map<unsigned long, unsigned long> ReverseItemMap;
#endif

class Items {
public:
	Items();
	~Items();
	
	int loadFromOtb(std::string);
	
	const ItemType& operator[](int id);
	
	static unsigned long reverseLookUp(unsigned long id);
	
	static long dwMajorVersion;
	static long dwMinorVersion;
	static long dwBuildNumber;
	
protected:
	#ifdef __GNUC__
	typedef __gnu_cxx::hash_map<unsigned short, ItemType*> ItemMap;
	#else
	typedef stdext::hash_map<unsigned short, ItemType*> ItemMap;
	#endif
	
	ItemMap items;
	static ReverseItemMap revItems;

	ItemType dummyItemType; // use this for invalid ids
};

#endif









