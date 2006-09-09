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
#include "const78.h"
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
	bool isRune() const;

	uint16_t id;
	uint16_t clientId;

	uint16_t       maxItems;    // maximum size if this is a container
	double         weight;      // weight of the item, e.g. throwing distance depends on it
	std::string    name;        // the name of the item
	std::string    description; // additional description... as in "The blade is a magic flame." for fireswords
	WeaponType     weaponType;
	amu_t          amuType;
	subfight_t     shootType;
	int            attack;
	int            defence;
	int            armor;
	uint16_t       slot_position;
	uint16_t       decayTo;
	uint16_t       decayTime;
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
	int             alwaysOnTopOrder;
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

template<typename A>
class Array{
public:
	Array(long n);
	~Array();
	
	A getElement(long id);
	void addElement(A a, long pos);
	
private:
	A* m_data;
	long m_size;
};



class Items{
public:
	Items();
	~Items();
	
	int loadFromOtb(std::string);
	
	const ItemType& operator[](int id);

	int getItemIdByName(const std::string& name);
	int reverseLookUp(int id);
	
	static long dwMajorVersion;
	static long dwMinorVersion;
	static long dwBuildNumber;
	
	void addItemType(ItemType* iType);
	
protected:
	
	Array<ItemType*> items;
	Array<long> revItems;

	ItemType dummyItemType; // use this for invalid ids
};

#endif
