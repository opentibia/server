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

#ifndef __CONTAINER_H__
#define __CONTAINER_H__

#include "definitions.h"
#include "cylinder.h"
#include "item.h"

typedef std::list<Item *> ItemList;

class Container : public Item, public Cylinder
{
public:
	Container(const uint16_t _type);
	virtual ~Container();
	
	virtual std::string getDescription(uint32_t lookDistance) const {return Item::getDescription(lookDistance);};
	uint32_t size() const {return (uint32_t)itemlist.size();};
	uint32_t capacity() const {return maxSize;};

	ItemList::const_iterator getItems() const;
	ItemList::const_iterator getEnd() const;

	Item* getItem(uint32_t index);
	bool isHoldingItem(const Item* item) const;

	uint32_t getItemHoldingCount() const;
	virtual double getWeight() const;

	//
	virtual ReturnValue __moveThingTo(Creature* creature, Cylinder* toCylinder, uint32_t index, Thing* thing, uint32_t count);
	//virtual ReturnValue __queryCanMove(uint32_t index, Thing* thing, uint32_t inCount, uint32_t& outCount);

	virtual ReturnValue __addThing(Thing* thing);
	virtual ReturnValue __addThing(uint32_t index, Thing* thing);

	virtual ReturnValue __updateThing(Thing* thing, uint32_t count);
	virtual ReturnValue __updateThing(uint32_t index, Thing* thing);

	virtual ReturnValue __removeThing(Thing* thing);
	virtual ReturnValue __removeThing(Thing* thing, uint32_t count);

	virtual uint32_t __getIndexOfThing(const Thing* thing) const;
	Thing* __getThing(uint32_t index);

	virtual void __internalAddThing(Thing* thing);
	virtual void __internalAddThing(uint32_t index, Thing* thing);

	virtual int getThrowRange() const {return 10;};

	//
	//Cylinder* getParent() {return Cylinder::getParent();};
	//void setParent(Cylinder* cylinder) {Cylinder::setParent(cylinder);};

	//Cylinder* getTopParent() {return Cylinder::getTopParent();}; //returns Tile/Container or a Player
	//Tile* getTile() {return Cylinder::getTile();};

	//const Position& getPosition() const {return Cylinder::getPosition();};
	
	//
	//void useThing2() {Cylinder::useThing2();};
	//void releaseThing2() {Cylinder::releaseThing2();};

private:
	uint32_t depotId;
	uint32_t maxSize; //number of max items in container  

	ItemList itemlist;
};

#endif //__CONTAINER_H__

