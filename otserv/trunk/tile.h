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


#ifndef __TILE_H__
#define __TILE_H__

#include "item.h"
#include "cylinder.h"
#include "magic.h"

#include "definitions.h"
#include "templates.h"
#include "scheduler.h"

class Creature;
class Teleport;
class TrashHolder;
class Mailbox;

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

	MagicEffectItem* getFieldItem() const;
	Teleport* getTeleportItem() const;
	TrashHolder* getTrashHolder() const;
	Mailbox* getMailbox() const;
    
	Creature* getTopCreature();
	Item* getTopTopItem();
	Item* getTopDownItem();
	Item* getMoveableBlockingItem();
	Thing* getTopThing();
	
	int getThingCount() const;

	bool hasProperty(enum ITEMPROPERTY prop) const;

	bool hasFlag(tileflags_t flag) const;
	void setFlag(tileflags_t flag);
	bool isPz() const;
	void setPz();
  
	bool floorChange() const;
	bool floorChangeDown() const;
	bool floorChange(Direction direction) const;
	uint32_t getHeight() const;
  
	virtual std::string getDescription(int32_t lookDistance) const;

	void moveCreature(Creature* creature, Cylinder* toCylinder, bool teleport = false);

	//cylinder implementations
	virtual ReturnValue __queryMaxCount(int32_t index, const Thing* thing, uint32_t count,
		uint32_t& maxQueryCount) const;
	virtual ReturnValue __queryAdd(int32_t index, const Thing* thing, uint32_t count,
		bool childIsOwner = false) const;
	virtual ReturnValue __queryRemove(const Thing* thing, uint32_t count) const;
	virtual Cylinder* __queryDestination(int32_t& index, const Thing* thing, Item** destItem);

	virtual void __addThing(Thing* thing);
	virtual void __addThing(int32_t index, Thing* thing);

	virtual void __updateThing(Thing* thing, uint32_t count);
	virtual void __replaceThing(uint32_t index, Thing* thing);

	virtual void __removeThing(Thing* thing, uint32_t count);

	virtual int32_t __getIndexOfThing(const Thing* thing) const;
	virtual Thing* __getThing(uint32_t index) const;

	virtual void postAddNotification(Thing* thing, bool hasOwnership = true);
	virtual void postRemoveNotification(Thing* thing, bool hadOwnership = true);

	virtual void __internalAddThing(Thing* thing);
	virtual void __internalAddThing(uint32_t index, Thing* thing);

	const Position& getTilePosition() const {return tilePos;};

	virtual bool isRemoved() const {return false;};

private:
	void onAddTileItem(Item* item);
	void onUpdateTileItem(uint32_t index, Item* olditem, Item* newitem);
	void onRemoveTileItem(uint32_t index, Item* item);
	void onUpdateTile();

protected:
	Position tilePos;
	uint32_t flags;
};


#endif // #ifndef __TILE_H__

