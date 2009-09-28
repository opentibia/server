//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// a Tile represents a single field on the map.
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

#include "classes.h"
#include "cylinder.h"
#include "item.h"
#include <boost/shared_ptr.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/random_access_index.hpp>

#define INDEXED_TILE_ITEM_COUNT 50

typedef std::vector<Creature*> CreatureVector;
typedef CreatureVector::iterator CreatureIterator;
typedef CreatureVector::const_iterator CreatureConstIterator;
typedef std::list<Creature*> SpectatorVec;
typedef std::map<Position, boost::shared_ptr<SpectatorVec> > SpectatorCache;
typedef std::vector<Item*> ItemVector;

typedef boost::multi_index::multi_index_container<
		Item*,
		boost::multi_index::indexed_by<
			boost::multi_index::random_access<>,

			//item id
			boost::multi_index::ordered_non_unique<
			  BOOST_MULTI_INDEX_CONST_MEM_FUN(Item,uint16_t,getID)
			>,

			//action id
			boost::multi_index::ordered_non_unique<
			  BOOST_MULTI_INDEX_CONST_MEM_FUN(Item,int32_t,getActionId)
			>,

			//item type
			boost::multi_index::ordered_non_unique<
			  BOOST_MULTI_INDEX_CONST_MEM_FUN(Item,ItemTypes_t,getType)
			>

			//Notice: When adding new attributes, make sure items_onItemModified is called whenever you change
			//that attribute so that the item is re-indexed.
		>
> ItemMultiIndex;

typedef boost::multi_index::nth_index<ItemMultiIndex, 0>::type ItemMultiIndexRnd;
typedef ItemMultiIndexRnd::iterator ItemMultiIndexRndIterator;

typedef boost::multi_index::nth_index<ItemMultiIndex, 1>::type ItemMultiIndexItemId;
typedef ItemMultiIndexItemId::iterator ItemMultiIndexItemIdIterator;

typedef boost::multi_index::nth_index<ItemMultiIndex, 2>::type ItemMultiIndexActionId;
typedef ItemMultiIndexActionId::iterator ItemMultiIndexActionIdIterator;

typedef boost::multi_index::nth_index<ItemMultiIndex, 3>::type ItemMultiIndexType;
typedef ItemMultiIndexType::iterator ItemMultiIndexTypeIterator;

template<class T, class T_iter, class E, class E_iter, class C>
class TileItemBaseIterator : public std::iterator<std::bidirectional_iterator_tag, C*> {
public:
	TileItemBaseIterator() : 
	  vector(NULL), multiIndex(NULL) {}
	TileItemBaseIterator(T _vector, T_iter pos)
	{
		vector = _vector;
		vector_pos = pos;
		multiIndex = NULL;
	}

	TileItemBaseIterator(T _vector)
	{
		vector = _vector;
		vector_pos = vector->begin();
		multiIndex = NULL;
	}

	TileItemBaseIterator(E _multiindex)
	{
		vector = NULL;
		multiIndex = _multiindex;
		multiIndex_pos = multiIndex->begin();
	}

	TileItemBaseIterator(E _multiindex, E_iter pos)
	{
		vector = NULL;
		multiIndex = _multiindex;
		multiIndex_pos = pos;
	}

	TileItemBaseIterator(const TileItemBaseIterator& rhs)
	{
		vector = rhs.vector;
		vector_pos = rhs.vector_pos;

		multiIndex = rhs.multiIndex;
		multiIndex_pos = rhs.multiIndex_pos;
	}

	const TileItemBaseIterator& operator=(const TileItemBaseIterator& rhs)
	{
		vector = rhs.vector;
		vector_pos = rhs.vector_pos;

		multiIndex = rhs.multiIndex;
		multiIndex_pos = rhs.multiIndex_pos;

		return(*this);
	}

	template<class T, class T_iter, class E, class E_iter, class C>
	bool operator==(const TileItemBaseIterator<T, T_iter, E, E_iter, C>& rhs)
	{
		return !(*this != rhs);
	}

	template<class T, class T_iter, class E, class E_iter, class C>
	bool operator!=(const TileItemBaseIterator<T, T_iter, E, E_iter, C>& rhs)
	{
		if(vector){
			if(vector != rhs.vector){
				return true;
			}

			return vector_pos != rhs.vector_pos;
		}
		else{
			if(multiIndex != rhs.multiIndex){
				return true;
			}

			return multiIndex_pos != rhs.multiIndex_pos;
		}
	}

	template<class T, class T_iter, class E, class E_iter, class C>
	bool operator>=(const TileItemBaseIterator<T, T_iter, E, E_iter, C>& rhs)
	{
		if(vector){
			return vector_pos >= rhs.vector_pos;
		}
		else{
			return multiIndex_pos >= rhs.multiIndex_pos;
		}
	}

	template<class T, class T_iter, class E, class E_iter, class C>
	bool operator>(const TileItemBaseIterator<T, T_iter, E, E_iter, C>& rhs)
	{
		if(vector){
			return vector_pos > rhs.vector_pos;
		}
		else{
			return multiIndex_pos > rhs.multiIndex_pos;
		}
	}

	template<class T, class T_iter, class E, class E_iter, class C>
	bool operator<(const TileItemBaseIterator<T, T_iter, E, E_iter, C>& rhs)
	{
		return !(*this > rhs);
	}

	C* operator*() const
	{
		if(vector){
			if(vector_pos != vector->end()){
				return *vector_pos;
			}

			return NULL;
		}
		else{
			if(multiIndex_pos != multiIndex->end()){
				return *multiIndex_pos;
			}

			return NULL;
		}
	}

	C* operator->() const
	{
		return *(*this);
	}

	const TileItemBaseIterator& operator++()
	{
		if(vector){
			++vector_pos;
		}
		else{
			++multiIndex_pos;
		}

		return(*this);
	}

	TileItemBaseIterator operator++(int)
	{
		TileItemIterator tmp(*this);
		++(*this);
		return(tmp);
	}

	const TileItemBaseIterator& operator+=(const int32_t& rhs)
	{
		if(vector){
			vector_pos += rhs;
		}
		else{
			multiIndex_pos += rhs;
		}
		return *this;
	}

	TileItemBaseIterator operator+(const int32_t& rhs) const
	{
		TileItemBaseIterator tmp(*this);
		return (tmp += rhs);
	}

	const TileItemBaseIterator& operator--()
	{
		(*this)+= -1;
		return *this;
	}

	TileItemBaseIterator operator--(int)
	{
		TileItemIterator tmp(*this);
		--(*this);
		return(tmp);
	}

	const TileItemBaseIterator& operator-=(const int32_t& rhs)
	{
		return (*this += -rhs);
	}

	TileItemBaseIterator operator-(const int32_t& rhs) const
	{
		TileItemBaseIterator tmp(*this);
		return (tmp -= rhs);
	}

	const T_iter& getVectorPos() const {return vector_pos;}
	const E_iter& getMultiIndexPos() const {return multiIndex_pos;}

private:
	T vector;
	T_iter vector_pos;

	E multiIndex;
	E_iter multiIndex_pos;

	friend class Tile;
};

typedef TileItemBaseIterator<ItemVector*, ItemVector::iterator, ItemMultiIndex*, ItemMultiIndex::iterator, Item> TileItemIterator;
typedef TileItemBaseIterator<const ItemVector*, ItemVector::const_iterator, const ItemMultiIndex*, ItemMultiIndex::const_iterator, const Item> TileItemConstIterator;

class Tile : public Cylinder
{
public:
	static Tile& null_tile;
	Tile(uint16_t x, uint16_t y, uint16_t z);
	virtual ~Tile();

	//
	TileItemIterator items_begin();
	TileItemConstIterator items_begin() const;
	TileItemIterator items_end();
	TileItemConstIterator items_end() const;

	Item* items_getItemWithItemId(uint16_t itemId) const;
	ItemVector items_getListWithItemId(uint16_t itemId, int32_t max_result = -1) const;
	Item* items_getItemWithActionId(int32_t actionId) const;
	ItemVector items_getListWithActionId(int32_t actionId, int32_t max_result = -1) const;
	Item* items_getItemWithType(ItemTypes_t type) const;
	ItemVector items_getListWithType(ItemTypes_t type, int32_t max_result = -1) const;
	Item* items_getItemWithProps(ItemProp props) const;
	ItemVector items_getListWithProps(ItemProp props, int32_t max_result = -1) const;

	Item* items_get(size_t _pos) const {return const_cast<Item*>(*(items_begin() + _pos));}
	uint32_t items_count() const;
	bool items_empty() const {return items_count() == 0;}

	TileItemIterator items_downBegin(){return items_begin();}
	TileItemConstIterator items_downBegin() const {return items_begin();}
	TileItemIterator items_downEnd() {return items_begin() + downItemCount;}
	TileItemConstIterator items_downEnd() const {return items_begin() + downItemCount; }
	TileItemIterator items_topBegin() {return items_begin() + downItemCount;}
	TileItemConstIterator items_topBegin() const {return items_begin() + downItemCount;}
	TileItemIterator items_topEnd() {return items_end();}
	TileItemConstIterator items_topEnd() const {return items_end();}
	uint32_t items_topCount() const  {return std::distance(items_topBegin(), items_topEnd()); }
	uint32_t items_downCount() const {return std::distance(items_downBegin(), items_downEnd());}
	Item* items_firstDown() {return *items_downBegin();}
	Item* items_firstTop() { return (items_topCount() > 0 ? *(items_topEnd() - 1) : NULL);}
	//

	//
	CreatureIterator creatures_begin();
	CreatureConstIterator creatures_begin() const;
	CreatureIterator creatures_end();
	CreatureConstIterator creatures_end() const;

	Creature* creatures_get(size_t _pos) const {return const_cast<Creature*>(*(creatures_begin() + _pos));}
	uint32_t creatures_count() const;
	bool creatures_empty() const {return creatures_count() == 0;}
	//

	HouseTile* getHouseTile();
	const HouseTile* getHouseTile() const;
	bool isHouseTile() const;

	MagicField* getFieldItem() const;
	Teleport* getTeleportItem() const;
	TrashHolder* getTrashHolder() const;
	Mailbox* getMailbox() const;
	BedItem* getBedItem() const;

	Creature* getTopCreature();
	bool isMoveableBlocking() const;
	Thing* getTopVisibleThing(const Creature* creature);
	Creature* getTopVisibleCreature(const Creature* creature);
	const Creature* getTopVisibleCreature(const Creature* creature) const;
	Item* getItemByTopOrder(uint32_t topOrder);

	uint32_t getThingCount() const {return (ground ? 1 : 0) + items_count() + creatures_count();}
	uint32_t getCreatureCount() const;

	bool hasItemWithProperty(uint32_t props) const;
	bool hasItemWithProperty(Item* exclude, uint32_t props) const;

	bool blockSolid() const {return hasFlag(TILEPROP_BLOCKSOLID);}
	bool blockPathFind() const {return hasFlag(TILEPROP_BLOCKPATH);}
	bool blockProjectile() const {return hasFlag(TILEPROP_BLOCKPROJECTILE);}
	bool isVertical() const {return hasFlag(TILEPROP_VERTICAL);}
	bool isHorizontal() const {return hasFlag(TILEPROP_HORIZONTAL);}

	bool hasFlag(TileProp flag) const {return ((m_flags & (uint32_t)flag.value()) == (uint32_t)flag.value());}
	void setFlag(TileProp flag) {m_flags |= (uint32_t)flag.value();}
	void resetFlag(TileProp flag) {m_flags &= ~(uint32_t)flag.value();}

	bool positionChange() const {return hasFlag(TILEPROP_POSITIONCHANGE);}
	bool floorChange() const {return hasFlag(TILEPROP_FLOORCHANGE);}
	bool floorChangeDown() const {return hasFlag(TILEPROP_FLOORCHANGE_DOWN);}
	bool floorChange(Direction direction) const
	{
		switch(direction.value()){
		case enums::NORTH:
			return hasFlag(TILEPROP_FLOORCHANGE_NORTH);
		case enums::SOUTH:
			return hasFlag(TILEPROP_FLOORCHANGE_SOUTH);
		case enums::EAST:
			return hasFlag(TILEPROP_FLOORCHANGE_EAST);
		case enums::WEST:
			return hasFlag(TILEPROP_FLOORCHANGE_WEST);
		default:
			return false;
		}
	}

	ZoneType getZone() const {
		if(hasFlag(TILEPROP_PROTECTIONZONE)){
			return ZONE_PROTECTION;
		}
		else if(hasFlag(TILEPROP_NOPVPZONE)){
			return ZONE_NOPVP;
		}
		else if(hasFlag(TILEPROP_PVPZONE)){
			return ZONE_PVP;
		}
		else{
			return ZONE_NORMAL;
		}
	}

	bool hasHeight(uint32_t n) const;
	void moveCreature(Creature* actor, Creature* creature, Cylinder* toCylinder, bool teleport = false);
	int32_t getClientIndexOfThing(const Player* player, const Thing* thing) const;

	//cylinder implementations
	virtual Cylinder* getParent() {return NULL;}
	virtual const Cylinder* getParent() const {return NULL;}
	virtual bool isRemoved() const {return false;}
	virtual Position getPosition() const {return tilePos;}
	virtual Tile* getTile() {return this;}
	virtual const Tile* getTile() const {return this;}
	virtual Item* getItem() {return NULL;}
	virtual const Item* getItem() const {return NULL;}
	virtual Creature* getCreature() {return NULL;}
	virtual const Creature* getCreature() const {return NULL;}
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

	virtual int32_t __getIndexOfThing(const Thing* thing) const;
	virtual int32_t __getFirstIndex() const;
	virtual int32_t __getLastIndex() const;
	virtual uint32_t __getItemTypeCount(uint16_t itemId, int32_t subType = -1, bool itemCount = true) const;
	virtual Thing* __getThing(uint32_t index) const;

	virtual void postAddNotification(Creature* actor, Thing* thing, const Cylinder* oldParent, int32_t index, cylinderlink_t link = LINK_OWNER);
	virtual void postRemoveNotification(Creature* actor, Thing* thing, const Cylinder* newParent, int32_t index, bool isCompleteRemoval, cylinderlink_t link = LINK_OWNER);

	virtual void __internalAddThing(Thing* thing);
	virtual void __internalAddThing(uint32_t index, Thing* thing);

	//Should be called when itemId/actionId or any other data associated with ItemMultiIndex is modified.
	void items_onItemModified(Item* item);

private:
	void onAddTileItem(Item* item);
	void onUpdateTileItem(Item* oldItem, const ItemType& oldType, Item* newItem, const ItemType& newType);
	void onRemoveTileItem(const SpectatorVec& list, std::vector<uint32_t>& oldStackPosVector, Item* item);
	void onUpdateTile();

	void updateTileFlags(Item* item, bool removed);

 protected:
	bool is_indexed() const {return hasFlag(TILEPROP_INDEXED_TILE);}
	bool is_dynamic() const {return hasFlag(TILEPROP_DYNAMIC_TILE);}

	TileItemIterator items_insert(TileItemIterator _where, Item* item);
	TileItemIterator items_erase(TileItemIterator _pos);
	void items_push_back(Item* item) {items_insert(items_end(), item);}

	CreatureIterator creatures_insert(CreatureIterator _where, Creature* creature);
	CreatureIterator creatures_erase(CreatureIterator _pos);
	void creatures_push_back(Creature* creature) {creatures_insert(creatures_end(), creature);}

public:
	QTreeLeafNode*	qt_node;
	Item* ground;

protected:
	uint16_t downItemCount;
	Position tilePos;
	uint32_t m_flags;

	friend class Map;
};

// Used for walkable tiles, where there is high likeliness of
// items being added/removed
class DynamicTile : public Tile
{
	// By allocating the vectors in-house, we avoid some memory fragmentation
	CreatureVector creatures;
	ItemVector items;

public:
	DynamicTile(uint16_t x, uint16_t y, uint16_t z);
	~DynamicTile();

	TileItemIterator items_begin() {return TileItemIterator(&items);}
	TileItemConstIterator items_begin() const {return TileItemConstIterator(&items);}
	TileItemIterator items_end() {return TileItemIterator(&items, items.end());}
	TileItemConstIterator items_end() const {return TileItemConstIterator(&items, items.end());}

	Item* items_getItemWithItemId(uint16_t itemId) const
	{
		if(ground && ground->getID() == itemId){
			return ground;
		}

		for(TileItemConstIterator it = items_begin(); it != items_end(); ++it){
			if((*it)->getID() == itemId){
				return const_cast<Item*>(*it);
			}
		}
		return NULL;
	}
	ItemVector items_getListWithItemId(uint16_t itemId, int32_t max_result = -1) const
	{
		ItemVector vector;
		if(ground && ground->getID() == itemId){
			vector.push_back(ground);
		}

		for(TileItemConstIterator it = items_begin(); (it != items_end() && (max_result == -1 || (int32_t)vector.size() < max_result) ); ++it){
			if((*it)->getID() == itemId){
				vector.push_back(const_cast<Item*>(*it));
			}
		}
		return vector;
	}
	Item* items_getItemWithActionId(int32_t actionId) const
	{
		if(ground && ground->getActionId() == actionId){
			return ground;
		}

		for(TileItemConstIterator it = items_begin(); it != items_end(); ++it){
			if((*it)->getActionId() == actionId){
				return const_cast<Item*>(*it);
			}
		}
		return NULL;
	}
	ItemVector items_getListWithActionId(int32_t actionId, int32_t max_result = -1) const
	{
		ItemVector vector;
		if(ground && ground->getActionId() == actionId){
			vector.push_back(ground);
		}

		for(TileItemConstIterator it = items_begin(); (it != items_end() && (max_result == -1 || (int32_t)vector.size() < max_result) ); ++it){
			if((*it)->getActionId() == actionId){
				vector.push_back(const_cast<Item*>(*it));
			}
		}
		return vector;
	}
	Item* items_getItemWithType(ItemTypes_t type) const
	{
		if(ground && ground->getType() == type){
			return ground;
		}

		for(TileItemConstIterator it = items_begin(); it != items_end(); ++it){
			if((*it)->getType() == type){
				return const_cast<Item*>(*it);
			}
		}
		return NULL;
	}
	ItemVector items_getListWithType(ItemTypes_t type, int32_t max_result = -1) const
	{
		ItemVector vector;
		if(ground && ground->getType() == type){
			vector.push_back(ground);
		}

		for(TileItemConstIterator it = items_begin(); (it != items_end() && (max_result == -1 || (int32_t)vector.size() < max_result) ); ++it){
			if((*it)->getType() == type){
				vector.push_back(const_cast<Item*>(*it));
			}
		}
		return vector;
	}
	Item* items_getItemWithType(ItemProp props) const
	{
		if(ground && ground->hasProperty(props)){
			return ground;
		}

		for(TileItemConstIterator it = items_begin(); it != items_end(); ++it){
			if((*it)->hasProperty(props)){
				return const_cast<Item*>(*it);
			}
		}
		return NULL;
	}
	ItemVector items_getListWithProps(ItemProp props, int32_t max_result = -1) const
	{
		ItemVector vector;
		if(ground && ground->hasProperty(props)){
			vector.push_back(ground);
		}

		for(TileItemConstIterator it = items_begin(); (it != items_end() && (max_result == -1 || (int32_t)vector.size() < max_result) ); ++it){
			if((*it)->hasProperty(props)){
				vector.push_back(const_cast<Item*>(*it));
			}
		}
		return vector;
	}
	uint32_t items_count() const {return items.size();}

	CreatureIterator creatures_begin() {return creatures.begin();}
	CreatureConstIterator creatures_begin() const {return creatures.begin();}
	CreatureIterator creatures_end() {return creatures.end();}
	CreatureConstIterator creatures_end() const {return creatures.end();}

	uint32_t creatures_count() const {return creatures.size();}

protected:
	TileItemIterator items_insert(TileItemIterator _where, Item* item)
	{
		ItemVector::iterator it = items.insert(_where.getVectorPos(), item);
		return TileItemIterator(&items, it);
	}
	TileItemIterator items_erase(TileItemIterator _pos)
	{
		ItemVector::iterator it = items.erase(_pos.getVectorPos());
		return TileItemIterator(&items, it);
	}

	CreatureIterator creatures_insert(CreatureIterator _where, Creature* creature) {return creatures.insert(_where, creature);}
	CreatureIterator creatures_erase(CreatureIterator _pos) {return creatures.erase(_pos);}

	void items_onItemModified(Item* item){}

	friend class Map;
	friend class Tile;
};

// For blocking tiles, where we very rarely actually have items
class StaticTile : public Tile
{
	// We very rarely even need the vectors, so don't keep them in memory
	ItemVector* items;
	CreatureVector*	creatures;

	static ItemVector null_items;
	static CreatureVector null_creatures;

public:
	StaticTile(uint16_t x, uint16_t y, uint16_t z);
	~StaticTile();

	TileItemIterator items_begin() {return (items ? TileItemIterator(items) : TileItemIterator(&null_items));}
	TileItemConstIterator items_begin() const {return (items ? TileItemConstIterator(items) : TileItemConstIterator(&null_items));}
	TileItemIterator items_end() {return (items ? TileItemIterator(items, items->end()) : TileItemIterator(&null_items, null_items.end()));}
	TileItemConstIterator items_end() const {return (items ? TileItemConstIterator(items, items->end()) : TileItemConstIterator(&null_items, null_items.end()));}

	Item* items_getItemWithItemId(uint16_t itemId) const
	{
		if(ground && ground->getID() == itemId){
			return ground;
		}

		for(TileItemConstIterator it = items_begin(); it != items_end(); ++it){
			if((*it)->getID() == itemId){
				return const_cast<Item*>(*it);
			}
		}
		return NULL;
	}
	ItemVector items_getListWithItemId(uint16_t itemId, int32_t max_result = -1) const
	{
		ItemVector vector;
		if(ground && ground->getID() == itemId){
			vector.push_back(ground);
		}

		for(TileItemConstIterator it = items_begin(); (it != items_end() && (max_result == -1 || (int32_t)vector.size() < max_result) ); ++it){
			if((*it)->getID() == itemId){
				vector.push_back(const_cast<Item*>(*it));
			}
		}
		return vector;
	}
	Item* items_getItemWithActionId(int32_t actionId) const
	{
		if(ground && ground->getActionId() == actionId){
			return ground;
		}

		for(TileItemConstIterator it = items_begin(); it != items_end(); ++it){
			if((*it)->getActionId() == actionId){
				return const_cast<Item*>(*it);
			}
		}
		return NULL;
	}
	ItemVector items_getListWithActionId(int32_t actionId, int32_t max_result = -1) const
	{
		ItemVector vector;
		if(ground && ground->getActionId() == actionId){
			vector.push_back(ground);
		}
		for(TileItemConstIterator it = items_begin(); (it != items_end() && (max_result == -1 || (int32_t)vector.size() < max_result) ); ++it){
			if((*it)->getActionId() == actionId){
				vector.push_back(const_cast<Item*>(*it));
			}
		}
		return vector;
	}
	Item* items_getItemWithType(ItemTypes_t type) const
	{
		if(ground && ground->getType() == type){
			return ground;
		}

		for(TileItemConstIterator it = items_begin(); it != items_end(); ++it){
			if((*it)->getType() == type){
				return const_cast<Item*>(*it);
			}
		}
		return NULL;
	}
	ItemVector items_getListWithType(ItemTypes_t type, int32_t max_result = -1) const
	{
		ItemVector vector;
		if(ground && ground->getType() == type){
			vector.push_back(ground);
		}

		for(TileItemConstIterator it = items_begin(); (it != items_end() && (max_result == -1 || (int32_t)vector.size() < max_result) ); ++it){
			if((*it)->getType() == type){
				vector.push_back(const_cast<Item*>(*it));
			}
		}
		return vector;
	}	
	Item* items_getItemWithProps(ItemProp props) const
	{
		if(ground && ground->hasProperty(props)){
			return ground;
		}

		for(TileItemConstIterator it = items_begin(); it != items_end(); ++it){
			if((*it)->hasProperty(props)){
				return const_cast<Item*>(*it);
			}
		}
		return NULL;
	}
	ItemVector items_getListWithProps(ItemProp props, int32_t max_result = -1) const
	{
		ItemVector vector;
		if(ground && ground->hasProperty(props)){
			vector.push_back(ground);
		}

		for(TileItemConstIterator it = items_begin(); (it != items_end() && (max_result == -1 || (int32_t)vector.size() < max_result) ); ++it){
			if((*it)->hasProperty(props)){
				vector.push_back(const_cast<Item*>(*it));
			}
		}
		return vector;
	}

	uint32_t items_count() const {return (items ? (uint32_t)items->size() : 0);}

	CreatureIterator creatures_begin() {return (creatures ? creatures->begin() : null_creatures.begin());}
	CreatureConstIterator creatures_begin() const {return (creatures ? creatures->begin() : null_creatures.begin());}
	CreatureIterator creatures_end() {return (creatures ? creatures->end() : null_creatures.end());}
	CreatureConstIterator creatures_end() const {return (creatures ? creatures->end() : null_creatures.end());}

	uint32_t creatures_count() const {return (creatures ? (uint32_t)creatures->size() : 0);}

protected:
	TileItemIterator items_insert(TileItemIterator _where, Item* item)
	{
		if(!items){
			items = new ItemVector();
			ItemVector::iterator it = items->insert(items->begin(), item);
			return TileItemIterator(items, it);
		}
		else{
			ItemVector::iterator it = items->insert(_where.getVectorPos(), item);
			return TileItemIterator(items, it);
		}
	}
	TileItemIterator items_erase(TileItemIterator _pos)
	{
		assert(items);
		ItemVector::iterator it = items->erase(_pos.getVectorPos());
		return TileItemIterator(items, it);
	}

	CreatureIterator creatures_insert(CreatureIterator _where, Creature* creature)
	{
		if(!creatures){
			creatures = new CreatureVector();
			return creatures->insert(creatures->begin(), creature);
		}
		else{
			return creatures->insert(_where, creature);
		}
	}
	CreatureIterator creatures_erase(CreatureIterator _pos)
	{
		assert(creatures);
		return creatures->erase(_pos);
	}

	void items_onItemModified(Item* item){}

	friend class Map;
	friend class Tile;
};

// For tiles with alot of items on them for quicker searching
// DynamicTile/StaticTile can be changed to IndexedTile dynamically
class IndexedTile : public Tile
{
	// By allocating the vectors in-house, we avoid some memory fragmentation
	ItemMultiIndex	items;
	CreatureVector	creatures;

public:
	IndexedTile(uint16_t x, uint16_t y, uint16_t z);
	~IndexedTile();

	TileItemIterator items_begin() {return TileItemIterator(&items);}
	TileItemConstIterator items_begin() const {return TileItemConstIterator(&items);}
	TileItemIterator items_end() {return TileItemIterator(&items, items.end());}
	TileItemConstIterator items_end() const {return TileItemConstIterator(&items, items.end());}

	Item* items_getItemWithItemId(uint16_t itemId) const
	{
		if(ground && ground->getID() == itemId){
			return ground;
		}
		ItemMultiIndexItemIdIterator ic0 = items.get<1>().find(itemId);
		if(ic0 != items.get<1>().end()){
			return *ic0;
		}
		return NULL;
	}
	ItemVector items_getListWithItemId(uint16_t itemId, int32_t max_result = -1) const
	{
		ItemVector vector;
		if(ground && ground->getID() == itemId){
			vector.push_back(ground);
		}

		ItemMultiIndexItemIdIterator ic0,ic1;
		boost::tuples::tie(ic0,ic1) = items.get<1>().equal_range(itemId);
		while( (max_result == -1 || (int32_t)vector.size() < max_result) && ic0 != ic1){
			vector.push_back(*ic0);
			++ic0;
		}
		return vector;
	}
	Item* items_getItemWithActionId(int32_t actionId) const
	{
		if(ground && ground->getActionId() == actionId){
			return ground;
		}
		ItemMultiIndexActionIdIterator ic0 = items.get<2>().find(actionId);
		if(ic0 != items.get<2>().end()){
			return *ic0;
		}
		return NULL;
	}
	ItemVector items_getListWithActionId(int32_t actionId, int32_t max_result = -1) const
	{
		ItemVector vector;
		if(ground && ground->getActionId() == actionId){
			vector.push_back(ground);
		}

		ItemMultiIndexActionIdIterator ic0,ic1;
		boost::tuples::tie(ic0,ic1) = items.get<2>().equal_range(actionId);
		while( (max_result == -1 || (int32_t)vector.size() < max_result) && ic0 != ic1){
			vector.push_back(*ic0);
			++ic0;
		}
		return vector;
	}
	Item* items_getItemWithType(ItemTypes_t type) const
	{
		if(ground && ground->getType() == type){
			return ground;
		}
		ItemMultiIndexTypeIterator ic0 = items.get<3>().find(type);
		if(ic0 != items.get<3>().end()){
			return *ic0;
		}
		return NULL;
	}
	ItemVector items_getListWithType(ItemTypes_t type, int32_t max_result = -1) const
	{
		ItemVector vector;
		if(ground && ground->getType() == type){
			vector.push_back(ground);
		}

		ItemMultiIndexTypeIterator ic0,ic1;
		boost::tuples::tie(ic0,ic1) = items.get<3>().equal_range(type);
		while( (max_result == -1 || (int32_t)vector.size() < max_result) && ic0 != ic1){
			vector.push_back(*ic0);
			++ic0;
		}
		return vector;
	}
	Item* items_getItemWithProps(ItemProp props) const
	{
		//TODO: Optimize if possible
		ItemVector vector;
		if(ground && ground->hasProperty(props)){
			return ground;
		}

		for(TileItemConstIterator it = items_begin(); it != items_end(); ++it){
			if((*it)->hasProperty(props)){
				return const_cast<Item*>(*it);
			}
		}
		return NULL;
	}
	ItemVector items_getListWithProps(ItemProp props, int32_t max_result = -1) const
	{
		//TODO: Optimize if possible
		ItemVector vector;
		if(ground && ground->hasProperty(props)){
			vector.push_back(ground);
		}

		for(TileItemConstIterator it = items_begin(); (it != items_end() && (max_result == -1 || max_result < (int32_t)vector.size()) ); ++it){
			if((*it)->hasProperty(props)){
				vector.push_back(const_cast<Item*>(*it));
			}
		}
		return vector;
	}
	uint32_t items_count() const {return items.size();}

	CreatureIterator creatures_begin() {return creatures.begin();}
	CreatureConstIterator creatures_begin() const {return creatures.begin();}
	CreatureIterator creatures_end() {return creatures.end();}
	CreatureConstIterator creatures_end() const {return creatures.end();}

	uint32_t creatures_count() const {return creatures.size();}

protected:
	TileItemIterator items_insert(TileItemIterator _where, Item* item)
	{
		std::pair<ItemMultiIndexRndIterator, bool> it = items.insert(_where.getMultiIndexPos(), item);
		return TileItemIterator(&items, it.first);
	}
	TileItemIterator items_erase(TileItemIterator _pos)
	{
		ItemMultiIndex::iterator it = items.erase(_pos.getMultiIndexPos());
		return TileItemIterator(&items, it);
	}

	//Basically, we call NoOperationFunctor to make the MultiIndex think we are modifying the item
	//and force the MultiIndex to update itself
	struct NoOperationFunctor
	{
	  void operator()(Item*) const {}
	};

	void items_onItemModified(Item* item)
	{
		for(TileItemIterator it = items_begin(); it != items_end(); ++it){
			if(*it == item){
				items.modify(it.getMultiIndexPos(), NoOperationFunctor() );
				break;
			}
		}
	}

	CreatureIterator creatures_insert(CreatureIterator _where, Creature* creature) {return creatures.insert(_where, creature);}
	CreatureIterator creatures_erase(CreatureIterator _pos) {return creatures.erase(_pos);}

	friend class Map;
	friend class Tile;
};

inline Tile::Tile(uint16_t x, uint16_t y, uint16_t z) :
	qt_node(NULL),
	ground(NULL),
	downItemCount(0),
	tilePos(x, y, z),
	m_flags(0)
{
}

inline Tile::~Tile()
{
	// We don't need to free any memory as tiles are always deallocated
	// and OS will free up anything left when the server is shutdown
}

inline TileItemIterator Tile::items_begin()
{
	if(is_dynamic())
		return static_cast<DynamicTile*>(this)->DynamicTile::items_begin();
	else if(is_indexed())
		return static_cast<IndexedTile*>(this)->IndexedTile::items_begin();

	return static_cast<StaticTile*>(this)->StaticTile::items_begin();
}

inline TileItemConstIterator Tile::items_begin() const
{
	if(is_dynamic())
		return static_cast<const DynamicTile*>(this)->DynamicTile::items_begin();
	else if(is_indexed())
		return static_cast<const IndexedTile*>(this)->IndexedTile::items_begin();

	return static_cast<const StaticTile*>(this)->StaticTile::items_begin();
}

inline TileItemIterator Tile::items_end()
{
	if(is_dynamic())
		return static_cast<DynamicTile*>(this)->DynamicTile::items_end();
	else if(is_indexed())
		return static_cast<IndexedTile*>(this)->IndexedTile::items_end();

	return static_cast<StaticTile*>(this)->StaticTile::items_end();
}

inline Item* Tile::items_getItemWithItemId(uint16_t itemId) const
{
	if(is_dynamic())
		return static_cast<const DynamicTile*>(this)->DynamicTile::items_getItemWithItemId(itemId);
	else if(is_indexed())
		return static_cast<const IndexedTile*>(this)->IndexedTile::items_getItemWithItemId(itemId);

	return static_cast<const StaticTile*>(this)->StaticTile::items_getItemWithItemId(itemId);
}

inline ItemVector Tile::items_getListWithItemId(uint16_t itemId, int32_t max_result /*= -1*/) const
{
	if(is_dynamic())
		return static_cast<const DynamicTile*>(this)->DynamicTile::items_getListWithItemId(itemId, max_result);
	else if(is_indexed())
		return static_cast<const IndexedTile*>(this)->IndexedTile::items_getListWithItemId(itemId, max_result);

	return static_cast<const StaticTile*>(this)->StaticTile::items_getListWithItemId(itemId, max_result);
}

inline Item* Tile::items_getItemWithActionId(int32_t actionId) const
{
	if(is_dynamic())
		return static_cast<const DynamicTile*>(this)->DynamicTile::items_getItemWithActionId(actionId);
	else if(is_indexed())
		return static_cast<const IndexedTile*>(this)->IndexedTile::items_getItemWithActionId(actionId);

	return static_cast<const StaticTile*>(this)->StaticTile::items_getItemWithActionId(actionId);
}

inline ItemVector Tile::items_getListWithActionId(int32_t actionId, int32_t max_result /*= -1*/) const
{
	if(is_dynamic())
		return static_cast<const DynamicTile*>(this)->DynamicTile::items_getListWithActionId(actionId, max_result);
	else if(is_indexed())
		return static_cast<const IndexedTile*>(this)->IndexedTile::items_getListWithActionId(actionId, max_result);

	return static_cast<const StaticTile*>(this)->StaticTile::items_getListWithActionId(actionId, max_result);
}

inline Item* Tile::items_getItemWithType(ItemTypes_t type) const
{
	if(is_dynamic())
		return static_cast<const DynamicTile*>(this)->DynamicTile::items_getItemWithType(type);
	else if(is_indexed())
		return static_cast<const IndexedTile*>(this)->IndexedTile::items_getItemWithType(type);

	return static_cast<const StaticTile*>(this)->StaticTile::items_getItemWithType(type);
}

inline ItemVector Tile::items_getListWithType(ItemTypes_t type, int32_t max_result /*= -1*/) const
{
	if(is_dynamic())
		return static_cast<const DynamicTile*>(this)->DynamicTile::items_getListWithType(type, max_result);
	else if(is_indexed())
		return static_cast<const IndexedTile*>(this)->IndexedTile::items_getListWithType(type, max_result);

	return static_cast<const StaticTile*>(this)->StaticTile::items_getListWithType(type, max_result);
}

inline Item* Tile::items_getItemWithProps(ItemProp props) const
{
	if(is_dynamic())
		return static_cast<const DynamicTile*>(this)->DynamicTile::items_getItemWithProps(props);
	else if(is_indexed())
		return static_cast<const IndexedTile*>(this)->IndexedTile::items_getItemWithProps(props);

	return static_cast<const StaticTile*>(this)->StaticTile::items_getItemWithProps(props);
}

inline ItemVector Tile::items_getListWithProps(ItemProp props, int32_t max_result /*= -1*/) const
{
	if(is_dynamic())
		return static_cast<const DynamicTile*>(this)->DynamicTile::items_getListWithProps(props, max_result);
	else if(is_indexed())
		return static_cast<const IndexedTile*>(this)->IndexedTile::items_getListWithProps(props, max_result);

	return static_cast<const StaticTile*>(this)->StaticTile::items_getListWithProps(props, max_result);
}

inline TileItemConstIterator Tile::items_end() const
{
	if(is_dynamic())
		return static_cast<const DynamicTile*>(this)->DynamicTile::items_end();
	else if(is_indexed())
		return static_cast<const IndexedTile*>(this)->IndexedTile::items_end();

	return static_cast<const StaticTile*>(this)->StaticTile::items_end();
}

inline TileItemIterator Tile::items_insert(TileItemIterator _where, Item* item)
{
	if(is_dynamic())
		return static_cast<DynamicTile*>(this)->DynamicTile::items_insert(_where, item);
	else if(is_indexed())
		return static_cast<IndexedTile*>(this)->IndexedTile::items_insert(_where, item);

	return static_cast<StaticTile*>(this)->StaticTile::items_insert(_where, item);
}

inline TileItemIterator Tile::items_erase(TileItemIterator _pos)
{
	if(is_dynamic())
		return static_cast<DynamicTile*>(this)->DynamicTile::items_erase(_pos);
	else if(is_indexed())
		return static_cast<IndexedTile*>(this)->IndexedTile::items_erase(_pos);

	return static_cast<StaticTile*>(this)->StaticTile::items_erase(_pos);
}

inline void Tile::items_onItemModified(Item* item)
{
	if(is_dynamic())
		return static_cast<DynamicTile*>(this)->DynamicTile::items_onItemModified(item);
	else if(is_indexed())
		return static_cast<IndexedTile*>(this)->IndexedTile::items_onItemModified(item);

	return static_cast<StaticTile*>(this)->StaticTile::items_onItemModified(item);
}

inline uint32_t Tile::items_count() const
{
	if(is_dynamic())
		return static_cast<const DynamicTile*>(this)->DynamicTile::items_count();
	else if(is_indexed())
		return static_cast<const IndexedTile*>(this)->IndexedTile::items_count();

	return static_cast<const StaticTile*>(this)->StaticTile::items_count();
}

//
inline CreatureIterator Tile::creatures_begin()
{
	if(is_dynamic())
		return static_cast<DynamicTile*>(this)->DynamicTile::creatures_begin();
	else if(is_indexed())
		return static_cast<IndexedTile*>(this)->IndexedTile::creatures_begin();

	return static_cast<StaticTile*>(this)->StaticTile::creatures_begin();
}

inline CreatureConstIterator Tile::creatures_begin() const
{
	if(is_dynamic())
		return static_cast<const DynamicTile*>(this)->DynamicTile::creatures_begin();
	else if(is_indexed())
		return static_cast<const IndexedTile*>(this)->IndexedTile::creatures_begin();

	return static_cast<const StaticTile*>(this)->StaticTile::creatures_begin();
}

inline CreatureIterator Tile::creatures_end()
{
	if(is_dynamic())
		return static_cast<DynamicTile*>(this)->DynamicTile::creatures_end();
	else if(is_indexed())
		return static_cast<IndexedTile*>(this)->IndexedTile::creatures_end();

	return static_cast<StaticTile*>(this)->StaticTile::creatures_end();
}

inline CreatureConstIterator Tile::creatures_end() const
{
	if(is_dynamic())
		return static_cast<const DynamicTile*>(this)->DynamicTile::creatures_end();
	else if(is_indexed())
		return static_cast<const IndexedTile*>(this)->IndexedTile::creatures_end();

	return static_cast<const StaticTile*>(this)->StaticTile::creatures_end();
}

inline uint32_t Tile::creatures_count() const
{
	if(is_dynamic())
		return static_cast<const DynamicTile*>(this)->DynamicTile::creatures_count();
	else if(is_indexed())
		return static_cast<const IndexedTile*>(this)->IndexedTile::creatures_count();

	return static_cast<const StaticTile*>(this)->StaticTile::creatures_count();
}

inline CreatureIterator Tile::creatures_insert(CreatureIterator _where, Creature* creature)
{
	if(is_dynamic())
		return static_cast<DynamicTile*>(this)->DynamicTile::creatures_insert(_where, creature);
	else if(is_indexed())
		return static_cast<IndexedTile*>(this)->IndexedTile::creatures_insert(_where, creature);

	return static_cast<StaticTile*>(this)->StaticTile::creatures_insert(_where, creature);
}

inline CreatureIterator Tile::creatures_erase(CreatureIterator _pos)
{
	if(is_dynamic())
		return static_cast<DynamicTile*>(this)->DynamicTile::creatures_erase(_pos);
	else if(is_indexed())
		return static_cast<IndexedTile*>(this)->IndexedTile::creatures_erase(_pos);

	return static_cast<StaticTile*>(this)->StaticTile::creatures_erase(_pos);
}
//

inline StaticTile::StaticTile(uint16_t x, uint16_t y, uint16_t z) :
	Tile(x, y, z),
	items(NULL),
	creatures(NULL)
{}

inline StaticTile::~StaticTile()
{}

inline DynamicTile::DynamicTile(uint16_t x, uint16_t y, uint16_t z) :
	Tile(x, y, z)
{
	m_flags |= enums::TILEPROP_DYNAMIC_TILE;
}

inline DynamicTile::~DynamicTile()
{}

inline IndexedTile::IndexedTile(uint16_t x, uint16_t y, uint16_t z) :
	Tile(x, y, z)
{
	m_flags |= enums::TILEPROP_INDEXED_TILE;
}

inline IndexedTile::~IndexedTile()
{}

#endif
