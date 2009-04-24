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
class QTreeLeafNode;
class BedItem;

typedef std::vector<Item*> ItemVector;
typedef std::vector<Creature*> CreatureVector;

enum tileflags_t{
	TILESTATE_NONE						= 0,
	TILESTATE_PROTECTIONZONE			= 1 << 0,
	TILESTATE_DEPRICATED_HOUSE			= 1 << 1,
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
	TILESTATE_POSITIONCHANGE			= 1 << 13,
	TILESTATE_MAGICFIELD				= 1 << 14,
	TILESTATE_BLOCKSOLID				= 1 << 15,
	TILESTATE_BLOCKPATH					= 1 << 16,
	TILESTATE_IMMOVABLEBLOCKSOLID		= 1 << 17,
	TILESTATE_IMMOVABLEBLOCKPATH		= 1 << 18,
	TILESTATE_IMMOVABLENOFIELDBLOCKPATH = 1 << 19,
	TILESTATE_NOFIELDBLOCKPATH			= 1 << 20,
};

class HouseTile;

class Tile : public Cylinder
{
public:
	static Tile& null_tile;
	Tile(uint16_t x, uint16_t y, uint16_t z);
	~Tile();

	// Get the different content vectors
	virtual ItemVector* getTopItems() = 0;
	virtual CreatureVector* getCreatures() = 0;
	virtual ItemVector* getDownItems() = 0;
	virtual const ItemVector* getTopItems() const = 0;
	virtual const CreatureVector* getCreatures() const = 0;
	virtual const ItemVector* getDownItems() const = 0;
	virtual ItemVector* makeTopItems() = 0;
	virtual CreatureVector* makeCreatures() = 0;
	virtual ItemVector* makeDownItems() = 0;
	virtual void getThings(ItemVector*& topItems, CreatureVector*& creatures, ItemVector*& downItems) = 0;
	virtual void getThings(const ItemVector*& topItems, const CreatureVector*& creatures, const ItemVector*& downItems) const = 0;

	HouseTile* getHouseTile();
	const HouseTile* getHouseTile() const;
	bool isHouseTile() const;

	virtual int getThrowRange() const {return 0;};
	virtual bool isPushable() const {return false;};

	MagicField* getFieldItem() const;
	Teleport* getTeleportItem() const;
	TrashHolder* getTrashHolder() const;
	Mailbox* getMailbox() const;
	BedItem* getBedItem() const;

	Creature* getTopCreature();
	Item* getTopTopItem();
	Item* getTopDownItem();
	bool isMoveableBlocking() const;
	Thing* getTopThing();
	Item* getItemByTopOrder(uint32_t topOrder);

	uint32_t getThingCount() const {return thingCount;}
	// If these return != 0 the associated vectors are guaranteed to exists
	uint32_t getCreatureCount() const;
	uint32_t getItemCount() const;
	uint32_t getTopItemCount() const;
	uint32_t getDownItemCount() const;

	bool hasProperty(enum ITEMPROPERTY prop) const;
	bool hasProperty(Item* exclude, enum ITEMPROPERTY prop) const;

	bool hasFlag(tileflags_t flag) const {return ((m_flags & (uint32_t)flag) == (uint32_t)flag);}
	void setFlag(tileflags_t flag) {m_flags |= (uint32_t)flag;}
	void resetFlag(tileflags_t flag) {m_flags &= ~(uint32_t)flag;}

	bool positionChange() const {return hasFlag(TILESTATE_POSITIONCHANGE);}
	bool floorChange() const {return hasFlag(TILESTATE_FLOORCHANGE);}
	bool floorChangeDown() const {return hasFlag(TILESTATE_FLOORCHANGE_DOWN);}
	bool floorChange(Direction direction) const
	{
		switch(direction){
		case NORTH:
			return hasFlag(TILESTATE_FLOORCHANGE_NORTH);
		case SOUTH:
			return hasFlag(TILESTATE_FLOORCHANGE_SOUTH);
		case EAST:
			return hasFlag(TILESTATE_FLOORCHANGE_EAST);
		case WEST:
			return hasFlag(TILESTATE_FLOORCHANGE_WEST);
		default:
			return false;
		}
	}
	bool hasHeight(uint32_t n) const;
	uint32_t getHeight() const;

	virtual std::string getDescription(int32_t lookDistance) const;

	void moveCreature(Creature* creature, Cylinder* toCylinder, bool teleport = false);

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
	virtual uint32_t __getItemTypeCount(uint16_t itemId, int32_t subType = -1, bool itemCount = true) const;
	virtual Thing* __getThing(uint32_t index) const;

	virtual void postAddNotification(Thing* thing, const Cylinder* oldParent, int32_t index, cylinderlink_t link = LINK_OWNER);
	virtual void postRemoveNotification(Thing* thing, const Cylinder* newParent, int32_t index, bool isCompleteRemoval, cylinderlink_t link = LINK_OWNER);

	virtual void __internalAddThing(Thing* thing);
	virtual void __internalAddThing(uint32_t index, Thing* thing);

	virtual const Position& getPosition() const {return tilePos;}
	const Position& getTilePosition() const {return tilePos;}

	virtual bool isRemoved() const {return false;};

private:
	void onAddTileItem(Item* item);
	void onUpdateTileItem(uint32_t index, Item* oldItem,
		const ItemType& oldType, Item* newItem, const ItemType& newType);
	void onRemoveTileItem(uint32_t index, Item* item);
	void onUpdateTile();

	void updateTileFlags(Item* item, bool removing);

public:	
	Item*			ground;
	QTreeLeafNode*	qt_node;

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
	ItemVector		topItems;
	CreatureVector	creatures;
	ItemVector		downItems;
public:
	DynamicTile(uint16_t x, uint16_t y, uint16_t z);
	~DynamicTile();
	
	virtual ItemVector* getTopItems() {return &topItems;}
	virtual CreatureVector* getCreatures() {return &creatures;}
	virtual ItemVector* getDownItems() {return &downItems;}
	virtual const ItemVector* getTopItems() const {return &topItems;}
	virtual const CreatureVector* getCreatures() const {return &creatures;}
	virtual const ItemVector* getDownItems() const {return &downItems;}
	virtual ItemVector* makeTopItems() {return &topItems;}
	virtual CreatureVector* makeCreatures() {return &creatures;}
	virtual ItemVector* makeDownItems() {return &downItems;}
	
	virtual void getThings(ItemVector*& topItems, CreatureVector*& creatures, ItemVector*& downItems){
		topItems = &this->topItems;
		creatures = &this->creatures;
		downItems = &this->downItems;
	}

	virtual void getThings(const ItemVector*& topItems, const CreatureVector*& creatures, const ItemVector*& downItems) const{
		topItems = &this->topItems;
		creatures = &this->creatures;
		downItems = &this->downItems;
	}
};

// For blocking tiles, where we very rarely actually have items
class StaticTile : public Tile
{
	// We very rarely even need the vectors, so don't keep them in memory
	ItemVector*		topItems;
	CreatureVector*	creatures;
	ItemVector*		downItems;
public:
	StaticTile(uint16_t x, uint16_t y, uint16_t z);
	~StaticTile();
	
	virtual ItemVector* getTopItems() {return topItems;}
	virtual CreatureVector* getCreatures() {return creatures;}
	virtual ItemVector* getDownItems() {return downItems;}
	virtual const ItemVector* getTopItems() const {return topItems;}
	virtual const CreatureVector* getCreatures() const {return creatures;}
	virtual const ItemVector* getDownItems() const {return downItems;}
	virtual ItemVector* makeTopItems() {return (topItems)? (topItems) : (topItems = new ItemVector);}
	virtual CreatureVector* makeCreatures() {return (creatures)? (creatures) : (creatures = new CreatureVector);}
	virtual ItemVector* makeDownItems() {return (downItems)? (downItems) : (downItems = new ItemVector);}
	
	virtual void getThings(ItemVector*& topItems, CreatureVector*& creatures, ItemVector*& downItems){
		topItems = this->topItems;
		creatures = this->creatures;
		downItems = this->downItems;
	}

	virtual void getThings(const ItemVector*& topItems, const CreatureVector*& creatures, const ItemVector*& downItems) const{
		topItems = this->topItems;
		creatures = this->creatures;
		downItems = this->downItems;
	}
};

inline Tile::Tile(uint16_t x, uint16_t y, uint16_t z) :
	ground(NULL),
	qt_node(NULL),
	thingCount(0),
	tilePos(x, y, z),
	m_flags(0)
{
}

inline Tile::~Tile()
{
	// We don't need to free any memory as tiles are always deallocated
	// and OS will free up anything left when the server is shutdown
#ifdef _DEBUG
	delete ground;
#endif // _DEBUG
}

inline StaticTile::StaticTile(uint16_t x, uint16_t y, uint16_t z) :
	Tile(x, y, z),
	topItems(NULL),
	creatures(NULL),
	downItems(NULL)
{}

inline StaticTile::~StaticTile()
{
#ifdef _DEBUG
#endif
}

inline DynamicTile::DynamicTile(uint16_t x, uint16_t y, uint16_t z) :
	Tile(x, y, z)
{}

inline DynamicTile::~DynamicTile()
{}

#endif
