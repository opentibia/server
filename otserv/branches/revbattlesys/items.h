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


#ifndef __OTSERV_ITEMS_H__
#define __OTSERV_ITEMS_H__


#include "definitions.h"
#include "const76.h"
#include "itemloader.h"
#include "enums.h"


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

struct Abilities{
	Abilities()
	{
		absorbPercentAll = 0;
		absorbPercentPhysical = 0;
		absorbPercentFire = 0;
		absorbPercentEnergy = 0;
		absorbPercentPoison = 0;
		absorbPercentLifeDrain = 0;
		absorbPercentManaDrain = 0;

		memset(skills, 0, sizeof(skills));
	};

	/*
	uint8_t absorbAll;
	uint8_t absorbPhysical;
	uint8_t absorbFire;
	uint8_t absorbEnergy;
	uint8_t absorbPoison;
	uint8_t absorbLifeDrain;
	uint8_t absorbManaDrain;
	*/

	uint8_t absorbPercentAll;
	uint8_t absorbPercentPhysical;
	uint8_t absorbPercentFire;
	uint8_t absorbPercentEnergy;
	uint8_t absorbPercentPoison;
	uint8_t absorbPercentLifeDrain;
	uint8_t absorbPercentManaDrain;
	uint32_t skills[SKILL_LAST + 1];
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

	std::string    name;
	std::string    description;
	unsigned short maxItems;
	double         weight;
	WeaponType_t   weaponType;
	Ammo_t         amuType;
	ShootType_t    shootType;
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
	int				      rotateTo;

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

	Abilities abilities;

	//fields
	int32_t initialDamage;
	DamageType_t damageType;

	int32_t roundMin;
	int32_t roundTime;
	int32_t roundDamage;
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

	int getItemIdByName(const std::string& name);
	//int reverseLookUp(int id);
	
	static long dwMajorVersion;
	static long dwMinorVersion;
	static long dwBuildNumber;

	size_t size() {return items.size();}
	
protected:
	typedef OTSERV_HASH_MAP<unsigned short, ItemType*> ItemMap;

	ItemMap items;
	//static ReverseItemMap revItems;

	ItemType dummyItemType; // use this for invalid ids
};

#endif
