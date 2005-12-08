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

	virtual int getThrowRange() const {return Item::getThrowRange();};
	virtual bool isPushable() const {return Item::isPushable();};

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
	virtual ReturnValue __queryMaxCount(int32_t index, const Thing* thing, uint32_t count,
		uint32_t& maxQueryCount) const;
	virtual ReturnValue __queryAdd(uint32_t index, const Thing* thing, uint32_t count) const;
	virtual ReturnValue __queryRemove(const Thing* thing, uint32_t count) const;
	virtual Cylinder* __queryDestination(int32_t& index, const Thing* thing, Thing** destThing);

	virtual ReturnValue __addThing(Thing* thing);
	virtual ReturnValue __addThing(uint32_t index, Thing* thing);

	virtual ReturnValue __updateThing(Thing* thing, uint32_t count);
	virtual ReturnValue __updateThing(uint32_t index, Thing* thing);

	virtual ReturnValue __removeThing(Thing* thing, uint32_t count);

	virtual int32_t __getIndexOfThing(const Thing* thing) const;
	Thing* __getThing(uint32_t index);
	virtual Thing* __getThing(uint32_t index) const;

	virtual void postAddNotification(const Thing* thing, bool hasOwnership = true);
	virtual void postRemoveNotification(const Thing* thing, bool hadOwnership = true);

	virtual void __internalAddThing(Thing* thing);
	virtual void __internalAddThing(uint32_t index, Thing* thing);

private:
	uint32_t depotId;
	uint32_t maxSize; //number of max items in container  

	ItemList itemlist;
};

#endif //__CONTAINER_H__

