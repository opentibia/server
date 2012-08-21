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

#ifndef __OTSERV_CYLINDER_H__
#define __OTSERV_CYLINDER_H__

#include <stdint.h>
#include <map>
#include "const.h"

#define INDEX_WHEREEVER -1

class Thing;
class Tile;
class Position;
class Item;
class Creature;

enum cylinderflags_t {
	FLAG_PATHFINDING         = 1,	//An additional check is done for floor changing/teleport items
	FLAG_IGNOREBLOCKITEM     = 2,	//Bypass moveable blocking item checks
	FLAG_IGNOREBLOCKCREATURE = 4,	//Bypass creature checks
	FLAG_IGNOREFIELDDAMAGE   = 8,	//Bypass field damage checks
	FLAG_IGNORENOTMOVEABLE   = 16,	//Bypass check for movability
	FLAG_IGNORECAPACITY      = 32	//Bypass checks for capacity (container size/player capacity)
};

enum cylinderlink_t{
	LINK_OWNER,
	LINK_PARENT,
	LINK_TOPPARENT,
	LINK_NEAR
};

class Cylinder{
public:
	virtual ~Cylinder() {}

	virtual Cylinder* getParent() = 0;
	virtual const Cylinder* getParent() const = 0;
	virtual bool isRemoved() const = 0;
	virtual Position getPosition() const = 0;
	virtual Tile* getTile() = 0;
	virtual const Tile* getTile() const = 0;
	virtual Item* getItem() = 0;
	virtual const Item* getItem() const = 0;
	virtual Creature* getCreature() = 0;
	virtual const Creature* getCreature() const = 0;

	virtual Tile* getParentTile() = 0;
	virtual const Tile* getParentTile() const = 0;

	/**
	  * Query if the cylinder can add an object
	  * \param index points to the destination index (inventory slot/container position)
		* -1 is a internal value and means add to a empty position, with no destItem
	  * \param thing the object to move/add
	  * \param count is the amount that we want to move/add
	  * \param flags optional flags to modifiy the default behaviour
	  * \return ReturnValue holds the return value
	  */
	virtual ReturnValue __queryAdd(int32_t index, const Thing* Item, uint32_t count,
		uint32_t flags) const = 0;

	/**
	  * Query the cylinder how much it can accept
	  * \param index points to the destination index (inventory slot/container position)
		* -1 is a internal value and means add to a empty position, with no destItem
	  * \param item the object to move/add
	  * \param count is the amount that we want to move/add
	  * \param maxQueryCount is the max amount that the cylinder can accept
	  * \param flags optional flags to modifiy the default behaviour
	  * \return ReturnValue holds the return value
	  */
	virtual ReturnValue __queryMaxCount(int32_t index, const Thing* thing, uint32_t count, uint32_t& maxQueryCount,
		uint32_t flags) const = 0;

	/**
	  * Query if the cylinder can remove an object
	  * \param item the object to move/remove
	  * \param count is the amount that we want to remove
	  * \param flags optional flags to modifiy the default behaviour
	  * \return ReturnValue holds the return value
	  */
	virtual ReturnValue __queryRemove(const Thing* thing, uint32_t count, uint32_t flags) const = 0;

	/**
	  * Query the destination cylinder
	  * \param index points to the destination index (inventory slot/container position),
		* -1 is a internal value and means add to a empty position, with no destItem
		* this method can change the index to point to the new cylinder index
	  * \param destItem is the destination object
	  * \param flags optional flags to modifiy the default behaviour
		* this method can modifiy the flags
	  * \return Cylinder returns the destination cylinder
	  */
	virtual Cylinder* __queryDestination(int32_t& index, const Thing* thing, Item** destItem,
		uint32_t& flags) = 0;

	/**
	  * Add the object to the cylinder
	  * \param actor is the creature that is responsible (can be NULL)
	  * \param item is the object to add
	  */
	virtual void __addThing(Creature* actor, Thing* thing) = 0;

	/**
	  * Add the object to the cylinder
	  * \param actor is the creature that is responsible (can be NULL)
	  * \param index points to the destination index (inventory slot/container position)
	  * \param item is the object to add
	  */
	virtual void __addThing(Creature* actor, int32_t index, Thing* thing) = 0;

	/**
	  * Update the item count or type for an object
	  * \param actor is the creature that is responsible (can be NULL)
	  * \param thing is the object to update
	  * \param itemId is the new item id
	  * \param count is the new count value
	  */
	virtual void __updateThing(Creature* actor, Thing* thing, uint16_t itemId, uint32_t count) = 0;

	/**
	  * Replace an object with a new
	  * \param actor is the creature that is responsible (can be NULL)
	  * \param index is the position to change (inventory slot/container position)
	  * \param thing is the object to update
	  */
	virtual void __replaceThing(Creature* actor, uint32_t index, Thing* thing) = 0;

	/**
	  * Remove an object
	  * \param actor is the creature that is responsible (can be NULL)
	  * \param thing is the object to delete
	  * \param count is the new count value
	  */
	virtual void __removeThing(Creature* actor, Thing* thing, uint32_t count) = 0;

	/**
	  * Is sent after an operation (move/add) to update internal values
	  * \param actor is the creature that is responsible (can be NULL)
	  * \param thing is the object that has been added
	  * \param index is the objects new index value
	  * \param link holds the relation the object has to the cylinder
	  */
	virtual void postAddNotification(Creature* actor, Thing* thing, const Cylinder* oldParent, int32_t index, cylinderlink_t link = LINK_OWNER) = 0;

	/**
	  * Is sent after an operation (move/remove) to update internal values
	  * \param actor is the creature that is responsible (can be NULL)
	  * \param thing is the object that has been removed
	  * \param index is the previous index of the removed object
	  * \param isCompleteRemoval indicates if the item was completely removed or just partially (stackables)
	  * \param link holds the relation the object has to the cylinder
	  */
	virtual void postRemoveNotification(Creature* actor, Thing* thing, const Cylinder* newParent, int32_t index, bool isCompleteRemoval, cylinderlink_t link = LINK_OWNER) = 0;

	/**
	  * Gets the index of an object
	  * \param thing the object to get the index value from
	  * \return the index of the object, returns -1 if not found
	  */
	virtual int32_t __getIndexOfThing(const Thing* thing) const;

	/**
	  * Returns the first index
	  * \return the first index, if not implemented -1 is returned
	  */
	virtual int32_t __getFirstIndex() const;

	/**
	  * Returns the last index
	  * \return the last index, if not implemented -1 is returned
	  */
	virtual int32_t __getLastIndex() const;

	/**
	  * Gets the object based on index
	  * \return the object, returns NULL if not found
	  */
	virtual Thing* __getThing(uint32_t index) const;

	/**
	  * Get the amount of items of a certain type
	  * \param itemId is the item type to the get the count of
	  * \param subType is the extra type an item can have such as charges/fluidtype, -1 means not used
	  * \param returns the amount of items of the asked item type
	  */
	virtual uint32_t __getItemTypeCount(uint16_t itemId, int32_t subType = -1) const;

	/**
	  * Get the amount of items of a all types
	  * \param countMap a map to put the itemID:count mapping in
	  * \param returns a map mapping item id to count (same as first argument)
	  */
	virtual std::map<uint32_t, uint32_t>& __getAllItemTypeCount(std::map<uint32_t, uint32_t>& countMap) const;

	/**
	  * Adds an object to the cylinder without sending to the client(s)
	  * \param thing is the object to add
	  */
	virtual void __internalAddThing(Thing* thing);

	/**
	  * Adds an object to the cylinder without sending to the client(s)
	  * \param thing is the object to add
	  * \param index points to the destination index (inventory slot/container position)
	  */
	virtual void __internalAddThing(uint32_t index, Thing* thing);
};


class VirtualCylinder : public Cylinder
{
public:
	virtual ~VirtualCylinder();
	static VirtualCylinder* virtualCylinder;

	//cylinder implementations
	virtual Cylinder* getParent();
	virtual bool isRemoved() const;
	virtual const Cylinder* getParent() const;
	virtual Position getPosition() const;
	virtual Tile* getTile();
	virtual const Tile* getTile() const;
	virtual Item* getItem();
	virtual const Item* getItem() const;
	virtual Creature* getCreature();
	virtual const Creature* getCreature() const;
	virtual Tile* getParentTile();
	virtual const Tile* getParentTile() const;

	virtual ReturnValue __queryAdd(int32_t index, const Thing* thing, uint32_t count,
		uint32_t flags) const;
	virtual ReturnValue __queryMaxCount(int32_t index, const Thing* thing, uint32_t count,
		uint32_t& maxQueryCount, uint32_t flags) const;
	virtual ReturnValue __queryRemove(const Thing* thing, uint32_t count, uint32_t flags) const;
	virtual Cylinder* __queryDestination(int32_t& index, const Thing* thing, Item** destItem,
		uint32_t& flags);

	virtual void __addThing(Creature* actor, Thing* thing);
	virtual void __addThing(Creature* actor, int32_t index, Thing* thing);
	virtual void __updateThing(Creature* actor, Thing* thing, uint16_t itemId, uint32_t count);
	virtual void __replaceThing(Creature* actor, uint32_t index, Thing* thing);
	virtual void __removeThing(Creature* actor, Thing* thing, uint32_t count);

	virtual void postAddNotification(Creature* actor, Thing* thing,
		const Cylinder* oldParent, int32_t index, cylinderlink_t link = LINK_OWNER);
	virtual void postRemoveNotification(Creature* actor, Thing* thing,
		const Cylinder* newParent, int32_t index, bool isCompleteRemoval, cylinderlink_t link = LINK_OWNER);

	virtual bool isPushable() const;
	virtual int getThrowRange() const;
	virtual std::string getDescription(int32_t lookDistance) const;
};


#endif
