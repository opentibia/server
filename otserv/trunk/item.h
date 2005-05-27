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


#ifndef __OTSERV_ITEM_H
#define __OTSERV_ITEM_H

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <iostream>
#include <list>
#include <vector>

#include "texcept.h"

#include "thing.h"
#include "items.h"

class Creature;
class Player;

class Item : public Thing
{
protected:
    unsigned id;  // the same id as in ItemType
	unsigned char count; // number of stacked items
	unsigned char chargecount; //number of charges on the item
	unsigned char fluid;
	unsigned short actionId;
private:
	int useCount;
	
public:
	static Item* CreateItem(const unsigned short _type, unsigned short _count = 0); //Factory member to create item of right type based on type
	static Items items;

  unsigned short getID() const;    // ID as in ItemType
  void setID(unsigned short newid);
		    
	WeaponType getWeaponType() const;
	amu_t	getAmuType() const;
	subfight_t getSubfightType() const;
	int getAttack() const;
	int getArmor() const;
	int getDefense() const;
	int getSlotPosition() const;
	int getRWInfo() const;
		
	bool isBlockingProjectile() const;
	bool isBlocking(bool ispickupable = false) const;
	bool isStackable() const;
	bool isFluidContainer() const;
    bool isMultiType() const;
	bool isAlwaysOnTop() const;
	bool isGroundTile() const;
	bool isNotMoveable() const;
	bool isPickupable() const;
	bool isWeapon() const;
	//bool isContainer() const;
	bool noFloorChange() const;
	bool floorChangeNorth() const;
	bool floorChangeSouth() const;
	bool floorChangeEast() const;
	bool floorChangeWest() const;
	
	std::string *specialDescription;
	std::string *text;	//text written

	int use(){std::cout << "use " << id << std::endl; return 0;};
	int use(Item*){std::cout << "use with item ptr " << id << std::endl; return 0;};
	int use(Creature*){std::cout << "use with creature ptr " << id << std::endl; return 0;};
	std::string getDescription() const;
	std::string getName() const ;
	void setSpecialDescription(std::string desc);
	void clearSpecialDescription();
	void setText(std::string desc);
	void clearText();
	std::string Item::getText();
	
	virtual int unserialize(xmlNodePtr p);
	virtual xmlNodePtr serialize();

    // get the number of items
    unsigned short getItemCountOrSubtype() const;
	void setItemCountOrSubtype(unsigned char n);

	unsigned char getItemCharge() const {return chargecount;};
	void setItemCharge(unsigned char n) {chargecount = n;};

	unsigned char getFluidType() const {return fluid;};
	void setFluidType(unsigned char n) {fluid = n;};
	
	void setActionId(unsigned short n);
	
	virtual long getDecayTime();
	bool canDecay();

	/**
	 * Called when the item is about to decay/transform to the next step.
	 * \returns The item to decay to.
	 */
	virtual Item* decay();
	bool isDecaying;

  // Constructor for items
  Item(const unsigned short _type);
  Item(const unsigned short _type, unsigned short _count);
	Item();
	Item(const Item &i);

  virtual ~Item();
  virtual void useThing() {
		//std::cout << "Item: useThing() " << this << std::endl;
		useCount++;
	};
	
	virtual void releaseThing() {
		useCount--;
		//std::cout << "Item: releaseThing() " << this << std::endl;
		//if (useCount == 0)
		if (useCount <= 0)
			delete this;
	};
	
	virtual bool canMovedTo(const Tile *tile) const;
};

class Teleport : public Item
{
public:
	Teleport(const unsigned short _type);
	virtual ~Teleport();
	virtual void useThing() {
		//std::cout << "Teleport: useThing() " << this << std::endl;
		useCount++;
	};
	
	virtual void releaseThing() {
		useCount--;
		//std::cout << "Teleport: releaseThing() " << this << std::endl;
		//if (useCount == 0)
		if (useCount <= 0)
			delete this;
	};
	
	void setDestPos(const Position &pos) {destPos = pos;};
	const Position& getDestPos() {return destPos;};
private:
	int useCount;
	virtual int unserialize(xmlNodePtr p);
	virtual xmlNodePtr serialize();
	Position destPos;
};

#endif
