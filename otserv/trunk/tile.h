//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// a Tile represents a single field on the map.
// it is a list of Items
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


#ifndef __OTSERV_TILE_H__
#define __OTSERV_TILE_H__

#include "cylinder.h"
#include "item.h"

class Creature;
class Teleport;
class TrashHolder;
class Mailbox;
class MagicField;

typedef std::vector<Item*> ItemVector;
typedef std::vector<Creature*> CreatureVector;

enum tileflags_t{
	TILESTATE_NONE = 0,
	TILESTATE_PROTECTIONZONE = 1,
	TILESTATE_HOUSE = 2
};

class Tile : public Cylinder
{
public:
	static Tile null_tile;
	Tile(int x, int y, int z)
	{
		tilePos.x = x;
		tilePos.y = y;
		tilePos.z = z;

		flags = 0;
		ground = NULL;
	}

	virtual int getThrowRange() const {return 0;};
	virtual bool isPushable() const {return false;};

	Item*          ground;
	ItemVector     topItems;
	CreatureVector creatures;
	ItemVector     downItems;

	MagicField* getFieldItem() const;
	Teleport* getTeleportItem() const;
	TrashHolder* getTrashHolder() const;
	Mailbox* getMailbox() const;
    
	Creature* getTopCreature();
	Item* getTopTopItem();
	Item* getTopDownItem();
	bool isMoveableBlocking() const;
	Thing* getTopThing();
	
	uint32_t getThingCount() const {return (uint32_t)(ground ? 1 : 0) + topItems.size() + creatures.size() + downItems.size();}

	bool hasProperty(enum ITEMPROPERTY prop) const;

	bool hasFlag(tileflags_t flag) const {return ((flags & (uint32_t)flag) == (uint32_t)flag);}
	void setFlag(tileflags_t flag) {flags |= flag;}
	bool isPz() const {return hasFlag(TILESTATE_PROTECTIONZONE);}
	void setPz() {setFlag(TILESTATE_PROTECTIONZONE);}
  
	bool floorChange() const;
	bool floorChangeDown() const;
	bool floorChange(Direction direction) const;
	bool hasHeight(uint32_t n) const;
	uint32_t getHeight() const;

	virtual std::string getDescription(int32_t lookDistance) const;

	void moveCreature(Creature* creature, Cylinder* toCylinder, bool teleport = false);

	//cylinder implementations
	virtual ReturnValue __queryAdd(int32_t index, const Thing* thing, uint32_t count,
		uint32_t flags) const;
	virtual ReturnValue __queryMaxCount(int32_t index, const Thing* thing, uint32_t count,
		uint32_t& maxQueryCount, uint32_t flags) const;
	virtual ReturnValue __queryRemove(const Thing* thing, uint32_t count) const;
	virtual Cylinder* __queryDestination(int32_t& index, const Thing* thing, Item** destItem,
		uint32_t& flags);

	virtual void __addThing(Thing* thing);
	virtual void __addThing(int32_t index, Thing* thing);

	virtual void __updateThing(Thing* thing, uint32_t count);
	virtual void __replaceThing(uint32_t index, Thing* thing);

	virtual void __removeThing(Thing* thing, uint32_t count);

	virtual int32_t __getIndexOfThing(const Thing* thing) const;
	virtual int32_t __getFirstIndex() const;
	virtual int32_t __getLastIndex() const;
	virtual uint32_t __getItemTypeCount(uint16_t itemId) const;
	virtual Thing* __getThing(uint32_t index) const;

	virtual void postAddNotification(Thing* thing, int32_t index, cylinderlink_t link = LINK_OWNER);
	virtual void postRemoveNotification(Thing* thing, int32_t index, bool isCompleteRemoval, cylinderlink_t link = LINK_OWNER);

	virtual void __internalAddThing(Thing* thing);
	virtual void __internalAddThing(uint32_t index, Thing* thing);

	const Position& getTilePosition() const {return tilePos;};

	virtual bool isRemoved() const {return false;};

private:
	void onAddTileItem(Item* item);
	void onUpdateTileItem(uint32_t index, Item* oldItem, Item* newItem);
	void onRemoveTileItem(uint32_t index, Item* item);
	void onUpdateTile();

protected:
	Position tilePos;
	uint32_t flags;
};

#endif
