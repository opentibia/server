//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Item represents an existing item.
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


#ifndef __ITEM_H__
#define __ITEM_H__

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <iostream>
#include <list>
#include <vector>

#include "thing.h"
#include "items.h"

class Creature;
class Container;
class Depot;
class Teleport;

enum ITEMPROPERTY{
 BLOCKSOLID,
 NOTMOVEABLEBLOCKSOLID,
 BLOCKPROJECTILE,
 BLOCKPATHFIND,
 NOTMOVEABLEBLOCKPATHFIND,
 PROTECTIONZONE
};

class Item : virtual public Thing
{
public:
  // Constructor for items
	Item(const unsigned short _type);
	Item(const unsigned short _type, unsigned short _count);
	Item();
	Item(const Item &i);

	virtual ~Item();

	virtual Item* getItem() {return this;};
	virtual const Item* getItem()const {return this;};
	virtual Container* getContainer() {return NULL;};
	virtual const Container* getContainer() const {return NULL;};
	virtual Teleport* getTeleport() {return NULL;};
	virtual const Teleport* getTeleport() const {return NULL;};

	//Factory member to create item of right type based on type
	static Item* CreateItem(const unsigned short _type, unsigned short _count = 0);
	static Items items;
	
	virtual bool isPushable() const {return !isNotMoveable();};
	virtual int getThrowRange() const {return (isPickupable() ? 15 : 1);};

	virtual std::string getDescription(int32_t lookDistance) const;

	unsigned short getID() const;    // ID as in ItemType
	virtual void setID(unsigned short newid);
		    
	WeaponType getWeaponType() const;
	amu_t	getAmuType() const;
	subfight_t getSubfightType() const;
	virtual double getWeight() const;
	int getAttack() const;
	int getArmor() const;
	int getDefense() const;
	int getSlotPosition() const;
	int getRWInfo() const;
	int getWorth() const;
		
	bool hasProperty(enum ITEMPROPERTY prop) const;
	bool isBlocking() const;
	bool isStackable() const;
	bool isRune() const;
	bool isFluidContainer() const;
	bool isAlwaysOnTop() const;
	bool isGroundTile() const;
	bool isSplash() const;
	bool isMagicField() const;
	bool isNotMoveable() const;
	bool isPickupable() const;
	bool isWeapon() const;
	bool isUseable() const;
	bool isHangable() const;

	bool floorChangeDown() const;
	bool floorChangeNorth() const;
	bool floorChangeSouth() const;
	bool floorChangeEast() const;
	bool floorChangeWest() const;

	std::string getName() const ;
	void setSpecialDescription(std::string desc);
	std::string getSpecialDescription();
	void clearSpecialDescription();
	void setText(std::string desc);
	void clearText();
	std::string Item::getText();
	
	virtual int unserialize(xmlNodePtr p);
	virtual xmlNodePtr serialize();

  // get the number of items
	unsigned short getItemCount() const;
	unsigned short getItemCountOrSubtype() const;
	void setItemCountOrSubtype(unsigned char n);

	unsigned char getItemCharge() const {return chargecount;};
	void setItemCharge(unsigned char n) {chargecount = n;};

	unsigned char getFluidType() const {return fluid;};
	void setFluidType(unsigned char n) {fluid = n;};
	
	void setActionId(unsigned short n);
	unsigned short getActionId() const;
	
	void setUniqueId(unsigned short n);
	unsigned short getUniqueId() const;
	
	virtual long getDecayTime();
	bool canDecay();

	//virtual Item* decay();
	bool isDecaying;

	bool rotate();

protected:
	unsigned short id;  // the same id as in ItemType
	unsigned char count; // number of stacked items
	unsigned char chargecount; //number of charges on the item
	unsigned char fluid;
	unsigned short actionId;
	unsigned short uniqueId;
	std::string *specialDescription;
	std::string *text;	//text written
};

#endif
