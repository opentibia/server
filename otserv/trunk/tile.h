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

#include "definitions.h"
#include "cylinder.h"
#include "item.h"
#include <boost/shared_ptr.hpp>

class Creature;
class Teleport;
class TrashHolder;
class Mailbox;
class MagicField;
class QTreeLeafNode;
class BedItem;

typedef std::vector<Creature*> CreatureVector;
typedef std::list<Creature*> SpectatorVec;
typedef std::list<Player*> PlayerList;
typedef std::map<Position, boost::shared_ptr<SpectatorVec> > SpectatorCache;
typedef std::vector<Item*> ItemVector;

enum tileflags_t :
uint32_t
{
	TILESTATE_NONE						= 0,
	TILESTATE_PROTECTIONZONE			= 1 << 0,
	TILESTATE_DEPRECATED_HOUSE			= 1 << 1,
	TILESTATE_NOPVPZONE					= 1 << 2,
	TILESTATE_NOLOGOUT					= 1 << 3,
	TILESTATE_PVPZONE					= 1 << 4,
	TILESTATE_REFRESH					= 1 << 5,

	//internal usage
	TILESTATE_HOUSE						= 1 << 6,
	TILESTATE_FLOORCHANGE				= 1 << 7,
	TILESTATE_FLOORCHANGE_DOWN			= 1 << 8,
	TILESTATE_FLOORCHANGE_NORTH			= 1 << 9,
	TILESTATE_FLOORCHANGE_SOUTH			= 1 << 10,
	TILESTATE_FLOORCHANGE_EAST			= 1 << 11,
	TILESTATE_FLOORCHANGE_WEST			= 1 << 12,
	TILESTATE_TELEPORT      			= 1 << 13,
	TILESTATE_MAGICFIELD                = 1 << 14,
	TILESTATE_MAILBOX                   = 1 << 15,
	TILESTATE_TRASHHOLDER               = 1 << 16,
	TILESTATE_BED                       = 1 << 17,
	TILESTATE_DEPOT                     = 1 << 18,
	TILESTATE_BLOCKSOLID				= 1 << 19,
	TILESTATE_BLOCKPATH					= 1 << 20,
	TILESTATE_IMMOVABLEBLOCKSOLID		= 1 << 21,
	TILESTATE_IMMOVABLEBLOCKPATH		= 1 << 22,
	TILESTATE_IMMOVABLENOFIELDBLOCKPATH = 1 << 23,
	TILESTATE_NOFIELDBLOCKPATH			= 1 << 24,
	TILESTATE_DYNAMIC_TILE				= 1 << 25
};

enum ZoneType_t
{
	ZONE_PROTECTION,
	ZONE_NOPVP,
	ZONE_PVP,
	ZONE_NOLOGOUT,
	ZONE_NORMAL
};

class HouseTile;

class TileItemVector
{
public:
	TileItemVector();

	ItemVector::iterator begin();
	ItemVector::const_iterator begin() const;
	ItemVector::reverse_iterator rbegin();
	ItemVector::const_reverse_iterator rbegin() const;

	ItemVector::iterator end();
	ItemVector::const_iterator end() const;
	ItemVector::reverse_iterator rend();
	ItemVector::const_reverse_iterator rend() const;

	size_t size() const;
	bool empty() const;

	ItemVector::iterator insert(ItemVector::iterator _where, Item* item);
	ItemVector::iterator erase(ItemVector::iterator _pos);
	Item* at(size_t _pos);
	Item* at(size_t _pos) const;
	Item* back();
	const Item* back() const;
	void push_back(Item* item);

	ItemVector::iterator getBeginDownItem();
	ItemVector::const_iterator getBeginDownItem() const;
	ItemVector::iterator getEndDownItem();
	ItemVector::const_iterator getEndDownItem() const;

	ItemVector::iterator getBeginTopItem();
	ItemVector::const_iterator getBeginTopItem() const;
	ItemVector::iterator getEndTopItem();
	ItemVector::const_iterator getEndTopItem() const;

	uint32_t getTopItemCount() const;
	uint32_t getDownItemCount() const;
	Item* getTopTopItem();
	Item* getTopDownItem();

private:
	ItemVector items;
	uint16_t downItemCount;
	friend class Tile;
};

class Tile : public Cylinder
{
public:
	static Tile& null_tile;
	Tile(const uint16_t& x, const uint16_t& y, const uint16_t& z);
	virtual ~Tile();

	TileItemVector* getItemList();
	const TileItemVector* getItemList() const;
	TileItemVector* makeItemList();

	CreatureVector* getCreatures();
	const CreatureVector* getCreatures() const;
	CreatureVector* makeCreatures();

	HouseTile* getHouseTile();
	const HouseTile* getHouseTile() const;
	bool isHouseTile() const;

	virtual int getThrowRange() const;
	virtual bool isPushable() const;

	MagicField* getFieldItem() const;
	Teleport* getTeleportItem() const;
	TrashHolder* getTrashHolder() const;
	Mailbox* getMailbox() const;
	BedItem* getBedItem() const;

	Creature* getTopCreature();
	Item* getTopTopItem();
	Item* getTopDownItem();
	bool isMoveableBlocking() const;
	Thing* getTopVisibleThing(const Creature* creature, bool checkVisibility = true);
	Creature* getTopVisibleCreature(const Creature* creature, bool checkVisibility = true);
	const Creature* getTopVisibleCreature(const Creature* creature, bool checkVisibility = true) const;
	Item* getItemByTopOrder(const int32_t& topOrder);

	const uint32_t& getThingCount() const;
	// If these return != 0 the associated vectors are guaranteed to exists
	uint32_t getCreatureCount() const;
	uint32_t getItemCount() const;
	uint32_t getTopItemCount() const;
	uint32_t getDownItemCount() const;

	bool hasProperty(ITEMPROPERTY prop, bool checkSemiSolid = false) const;
	bool hasProperty(Item* exclude, ITEMPROPERTY prop) const;

	bool hasFlag(const tileflags_t& flag) const;
	void setFlag(const tileflags_t& flag);
	void resetFlag(const tileflags_t& flag);

	bool positionChange() const;
	bool floorChange() const;
	bool floorChangeDown() const;
	bool floorChange(const Direction& direction) const;

	ZoneType_t getZone() const;

	bool hasHeight(const uint32_t& n) const;
	virtual std::string getDescription(const int32_t& lookDistance) const;

	void moveCreature(Creature* creature, Cylinder* toCylinder, bool teleport = false);
	int32_t getClientIndexOfThing(const Player* player, const Thing* thing) const;

	//cylinder implementations
	virtual ReturnValue __queryAdd(int32_t index, const Thing* thing, uint32_t count,
	                               uint32_t flags) const;
	virtual ReturnValue __queryMaxCount(int32_t index, const Thing* thing, uint32_t count,
	                                    uint32_t& maxQueryCount, uint32_t flags) const;
	virtual ReturnValue __queryRemove(const Thing* thing, uint32_t count, uint32_t flags) const;
	virtual Cylinder* __queryDestination(int32_t& index, const Thing* thing, Item** destItem,
	                                     uint32_t& flags);

	virtual void __addThing(Thing* thing);
	virtual void __addThing(int32_t index, Thing* thing);

	virtual void __updateThing(Thing* thing, uint16_t itemId, uint32_t count);
	virtual void __replaceThing(uint32_t index, Thing* thing);

	virtual void __removeThing(Thing* thing, uint32_t count);

	virtual int32_t __getIndexOfThing(const Thing* thing) const;
	virtual int32_t __getFirstIndex() const;
	virtual int32_t __getLastIndex() const;
	virtual uint32_t __getItemTypeCount(uint16_t itemId, int32_t subType = -1) const;
	virtual Thing* __getThing(uint32_t index) const;

	virtual void postAddNotification(Thing* thing, const Cylinder* oldParent, int32_t index, cylinderlink_t link = LINK_OWNER, bool isNewItem = true);
	virtual void postRemoveNotification(Thing* thing, const Cylinder* newParent, int32_t index, bool isCompleteRemoval, cylinderlink_t link = LINK_OWNER);

	virtual void __internalAddThing(Thing* thing);
	virtual void __internalAddThing(uint32_t index, Thing* thing);

	virtual const Position& getPosition() const;
	const Position& getTilePosition() const;

	virtual bool isRemoved() const;

private:
	void onAddTileItem(Item* item);
	void onUpdateTileItem(Item* oldItem, const ItemType& oldType, Item* newItem, const ItemType& newType);
	void onRemoveTileItem(const SpectatorVec& list, std::vector<uint32_t>& oldStackPosVector, Item* item);
	void onUpdateTile();

	void updateTileFlags(Item* item, bool removed);

protected:
	bool is_dynamic() const;

public:
	QTreeLeafNode*	qt_node;
	Item* ground;

protected:
	uint32_t thingCount;
	Position tilePos;
	uint32_t m_flags;
};

// Used for walkable tiles, where there is high likeliness of
// items being added/removed
class DynamicTile : public Tile
{
	// By allocating the vectors in-house, we avoid some memory fragmentation
	TileItemVector	items;
	//TileItemVector	scriptItems;
	CreatureVector	creatures;

public:
	DynamicTile(const uint16_t& x, const uint16_t& y, const uint16_t& z);
	virtual ~DynamicTile();

	TileItemVector* getItemList();
	const TileItemVector* getItemList() const;
	TileItemVector* makeItemList();

	CreatureVector* getCreatures();
	const CreatureVector* getCreatures() const;
	CreatureVector* makeCreatures();
};

// For blocking tiles, where we very rarely actually have items
class StaticTile : public Tile
{
	// We very rarely even need the vectors, so don't keep them in memory
	TileItemVector* items;
	CreatureVector*	creatures;

public:
	StaticTile(const uint16_t& x, const uint16_t& y, const uint16_t& z);
	virtual ~StaticTile();

	TileItemVector* getItemList();
	const TileItemVector* getItemList() const;
	TileItemVector* makeItemList();

	CreatureVector* getCreatures();
	const CreatureVector* getCreatures() const;
	CreatureVector* makeCreatures();
};

inline CreatureVector* Tile::getCreatures()
{
	if (is_dynamic())
	{
		return static_cast<DynamicTile*>(this)->DynamicTile::getCreatures();
	}

	return static_cast<StaticTile*>(this)->StaticTile::getCreatures();
}

inline const CreatureVector* Tile::getCreatures() const
{
	if (is_dynamic())
	{
		return static_cast<const DynamicTile*>(this)->DynamicTile::getCreatures();
	}

	return static_cast<const StaticTile*>(this)->StaticTile::getCreatures();
}

inline CreatureVector* Tile::makeCreatures()
{
	if (is_dynamic())
	{
		return static_cast<DynamicTile*>(this)->DynamicTile::makeCreatures();
	}

	return static_cast<StaticTile*>(this)->StaticTile::makeCreatures();
}

inline TileItemVector* Tile::getItemList()
{
	if (is_dynamic())
	{
		return static_cast<DynamicTile*>(this)->DynamicTile::getItemList();
	}

	return static_cast<StaticTile*>(this)->StaticTile::getItemList();
}

inline const TileItemVector* Tile::getItemList() const
{
	if (is_dynamic())
	{
		return static_cast<const DynamicTile*>(this)->DynamicTile::getItemList();
	}

	return static_cast<const StaticTile*>(this)->StaticTile::getItemList();
}

inline TileItemVector* Tile::makeItemList()
{
	if (is_dynamic())
	{
		return static_cast<DynamicTile*>(this)->DynamicTile::makeItemList();
	}

	return static_cast<StaticTile*>(this)->StaticTile::makeItemList();
}

#endif
