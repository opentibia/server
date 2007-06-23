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
#include "const79.h"
#include "enums.h"
#include "itemloader.h"
#include <map>

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
		absorbPercentDrown = 0;

		/*
		absorbAll = 0;
		absorbPhysical = 0;
		absorbFire = 0;
		absorbEnergy = 0;
		absorbPoison = 0;
		absorbLifeDrain = 0;
		absorbManaDrain = 0;
		*/

		memset(skills, 0, sizeof(skills));

		speed = 0;
		manaShield = false;
		invisible = false;
		conditionImmunities = 0;
		conditionSuppressions = 0;

		regeneration = false;
		healthGain = 0;
		healthTicks = 0;

		manaGain = 0;
		manaTicks = 0;
	};

	//damage abilities modifiers
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
	uint8_t absorbPercentDrown;

	//extra skill modifiers
	int32_t skills[SKILL_LAST + 1];

	int32_t speed;
	bool manaShield;
	bool invisible;

	bool regeneration;
	uint32_t healthGain;
	uint32_t healthTicks;

	uint32_t manaGain;
	uint32_t manaTicks;

	uint32_t conditionImmunities;
	uint32_t conditionSuppressions;
};

class Condition;

class ItemType {
private:
	//It is private because calling it can cause unexpected results
	ItemType(const ItemType& it){};
	
public:
	ItemType();
	~ItemType();

	itemgroup_t group;

	bool isGroundTile() const {return (group == ITEM_GROUP_GROUND);}
	bool isContainer() const {return (group == ITEM_GROUP_CONTAINER);}
	bool isDoor() const {return (group == ITEM_GROUP_DOOR);}
	bool isTeleport() const {return (group == ITEM_GROUP_TELEPORT);}
	bool isMagicField() const {return (group == ITEM_GROUP_MAGICFIELD);}
	bool isSplash() const {return (group == ITEM_GROUP_SPLASH);}
	bool isFluidContainer() const {return (group == ITEM_GROUP_FLUID);}

	bool isKey() const {return (group == ITEM_GROUP_KEY);}
	bool isRune() const {return (group == ITEM_GROUP_RUNE);}

	uint16_t id;
	uint16_t clientId;

	std::string    name;
	std::string    description;
	unsigned short maxItems;
	float          weight;
	WeaponType_t   weaponType;
	Ammo_t         amuType;
	ShootType_t    shootType;
	int            attack;
	int            defence;
	int            armor;
	uint16_t       slot_position;
	bool           isVertical;
	bool           isHorizontal;
	bool           isHangable;
	bool           allowDistRead;
	uint16_t       speed;
	int32_t        decayTo;
	uint32_t       decayTime;
	bool           stopTime;

	bool            canReadText;
	bool            canWriteText;
	unsigned short  maxTextLen;
	unsigned short  writeOnceItemId;

	bool            stackable;
	bool            useable;
	bool            moveable;
	bool            alwaysOnTop;
	int             alwaysOnTopOrder;
	bool            pickupable;
	bool            rotable;
	int             rotateTo;

	int             runeMagLevel;
	std::string     runeSpellName;

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

	unsigned short transformEquipTo;
	unsigned short transformDeEquipTo;
	bool showDuration;
	bool showCharges;
	uint32_t charges;
	uint32_t breakChance;
	uint32_t shootRange;

	Abilities abilities;

	Condition* condition;
	CombatType_t combatType;
	bool replaceable;
};

template<typename A>
class Array{
public:
	Array(uint32_t n);
	~Array();
	
	A getElement(uint32_t id);
	void addElement(A a, uint32_t pos);
	
	uint32_t size() {return m_size;}

private:
	A* m_data;
	uint32_t m_size;
};



class Items{
public:
	Items();
	~Items();
	
	bool reload();
	void clear();

	int loadFromOtb(std::string);
	
	const ItemType& operator[](int32_t id){return getItemType(id);}
	ItemType& getItemType(int32_t id);
	ItemType& getItemIdByClientId(int32_t spriteId);

	int32_t getItemIdByName(const std::string& name);
	
	static uint32_t dwMajorVersion;
	static uint32_t dwMinorVersion;
	static uint32_t dwBuildNumber;

	bool loadFromXml(const std::string& datadir);
	
	void addItemType(ItemType* iType);
	
	uint32_t size() {return items.size();}

protected:
	typedef std::map<int32_t, int32_t> ReverseItemMap;
	ReverseItemMap reverseItemMap;

	Array<ItemType*> items;
	std::string m_datadir;
};

#endif
