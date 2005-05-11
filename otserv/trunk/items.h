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

#include <map>
#include <string>
#include "const74.h"


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


class ItemType {
public:
	ItemType();
	~ItemType();

	unsigned short id;

  unsigned short maxItems;   // maximum size if this is a container
	double weight;						 // weight of the item, e.g. throwing distance depends on it
	std::string    name;			 // the name of the item
	std::string description;	 // additional description... as in "The blade is a magic flame." for fireswords
  WeaponType     weaponType;
  amu_t			amuType;
  subfight_t	shootType;
  int            attack;
  int            defence;
  int 			 armor;
	unsigned long 		slot_position;
  unsigned short decayTo;
  unsigned short decayTime;

	unsigned short damage;

	uint8_t speed;

	bool isteleport;

	// other bools
	bool ismagicfield;
	int  magicfieldtype;
	bool iscontainer;
	bool fluidcontainer;
	bool stackable;
	bool multitype;
	bool useable;
	bool notMoveable;
	bool alwaysOnTop;
	bool groundtile;
	int  runeMagLevel;
	bool blocking;   // people can walk on it
	bool pickupable; // people can pick it up
	bool blockingProjectile;
	bool noFloorChange;
	bool floorChangeNorth;
	bool floorChangeSouth;
	bool floorChangeEast;
	bool floorChangeWest;
};


class Items {
public:
	Items();
	~Items();
	
	int loadFromDat(std::string);
  int loadXMLInfos(std::string);
	
	const ItemType& operator[](int id);
		 
protected:
	typedef std::map<unsigned short, ItemType*> ItemMap;
	ItemMap items;

	ItemType dummyItemType; // use this for invalid ids
};

#endif









