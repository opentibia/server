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
#include "otpch.h"

#include "tile.h"
#include "housetile.h"
#include "game.h"
#include "combat.h"
#include "actor.h"
#include "player.h"
#include "teleport.h"
#include "trashholder.h"

extern Game g_game;

StaticTile real_null_tile(0xFFFF, 0xFFFF, 0xFFFF);
Tile& Tile::null_tile = real_null_tile;
ItemVector StaticTile::null_items;
CreatureVector StaticTile::null_creatures;

HouseTile* Tile::getHouseTile()
{
	if(isHouseTile())
		return static_cast<HouseTile*>(this);
	return NULL;
}

const HouseTile* Tile::getHouseTile() const
{
	if(isHouseTile())
		return static_cast<const HouseTile*>(this);
	return NULL;
}

bool Tile::isHouseTile() const
{
	return hasFlag(TILEPROP_HOUSE_TILE);
}

bool Tile::hasHeight(uint32_t n) const
{
	uint32_t height = 0;

	if(ground){
		if(ground->hasHeight()){
			++height;
		}

		if(n == height){
			return true;
		}
	}

	for(TileItemConstIterator it = items_begin(); it != items_end(); ++it){
		if((*it)->hasHeight()){
			++height;
		}

		if(n == height){
			return true;
		}
	}

	return false;
}

Teleport* Tile::getTeleportItem() const
{
	Item* item = items_getItemWithType(ITEM_TYPE_TELEPORT);
	if(item){
		return item->getTeleport();
	}
	return NULL;
}

MagicField* Tile::getFieldItem() const
{
	Item* item = items_getItemWithType(ITEM_TYPE_MAGICFIELD);
	if(item){
		return item->getMagicField();
	}
	return NULL;
}

TrashHolder* Tile::getTrashHolder() const
{
	Item* item = items_getItemWithType(ITEM_TYPE_TRASHHOLDER);
	if(item){
		return item->getTrashHolder();
	}
	return NULL;
}

BedItem* Tile::getBedItem() const
{
	Item* item = items_getItemWithType(ITEM_TYPE_BED);
	if(item){
		return item->getBed();
	}
	return NULL;
}

Creature* Tile::getTopCreature()
{
	return *creatures_begin();
}

Item* Tile::getItemByTopOrder(uint32_t topOrder)
{
	//topOrder:
	//1: borders
	//2: ladders, signs, splashes
	//3: doors etc
	//4: creatures

	for(TileItemIterator reverse_it = items_topEnd(); reverse_it != items_topBegin();){
		--reverse_it;
		if(Item::items[(*reverse_it)->getID()].alwaysOnTopOrder == (int32_t)topOrder){
			return (*reverse_it);
		}
	}

	return NULL;
}

Thing* Tile::getTopVisibleThing(const Creature* creature)
{
	for(CreatureConstIterator cit = creatures_begin(); cit != creatures_end(); ++cit){
		if(creature->canSeeCreature(*cit)){
			return (*cit);
		}
	}

	for(TileItemIterator it = items_downBegin(); it != items_downEnd(); ++it){
		const ItemType& iit = Item::items[(*it)->getID()];
		if(!iit.lookThrough){
			return (*it);
		}
	}

	for(TileItemIterator reverse_it = items_topEnd(); reverse_it != items_topBegin();){
		--reverse_it;
		const ItemType& iit = Item::items[(*reverse_it)->getID()];
		if(!iit.lookThrough){
			return (*reverse_it);
		}
	}

	if(ground)
		return ground;

	return NULL;
}

Creature* Tile::getTopVisibleCreature(const Creature* creature)
{
	for(CreatureConstIterator cit = creatures_begin(); cit != creatures_end(); ++cit){
		if((*cit)->getPlayer() && (*cit)->getPlayer()->hasFlag(PlayerFlag_CannotBeSeen))
			continue;

		return (*cit);
	}

	return NULL;
}

const Creature* Tile::getTopVisibleCreature(const Creature* creature) const
{
	for(CreatureConstIterator cit = creatures_begin(); cit != creatures_end(); ++cit){
		if((*cit)->getPlayer() && (*cit)->getPlayer()->hasFlag(PlayerFlag_CannotBeSeen))
			continue;

		return (*cit);
	}

	return NULL;
}

void Tile::onAddTileItem(Item* item)
{
	updateTileFlags(item, false);

	const Position& cylinderMapPos = getPosition();

	const SpectatorVec& list = g_game.getSpectators(cylinderMapPos);
	SpectatorVec::const_iterator it;

	//send to client
	Player* tmpPlayer = NULL;
	for(it = list.begin(); it != list.end(); ++it){
		if((tmpPlayer = (*it)->getPlayer())){
			tmpPlayer->sendAddTileItem(this, cylinderMapPos, item);
		}
	}

	//event methods
	for(it = list.begin(); it != list.end(); ++it){
		(*it)->onAddTileItem(this, cylinderMapPos, item);
	}
}

void Tile::onUpdateTileItem(Item* oldItem, const ItemType& oldType, Item* newItem, const ItemType& newType)
{
	const Position& cylinderMapPos = getPosition();

	const SpectatorVec& list = g_game.getSpectators(cylinderMapPos);
	SpectatorVec::const_iterator it;

	//send to client
	Player* tmpPlayer = NULL;
	for(it = list.begin(); it != list.end(); ++it){
		if((tmpPlayer = (*it)->getPlayer())){
			tmpPlayer->sendUpdateTileItem(this, cylinderMapPos, oldItem, newItem);
		}
	}

	//event methods
	for(it = list.begin(); it != list.end(); ++it){
		(*it)->onUpdateTileItem(this, cylinderMapPos, oldItem, oldType, newItem, newType);
	}
}

void Tile::onRemoveTileItem(const SpectatorVec& list, std::vector<uint32_t>& oldStackPosVector, Item* item)
{
	updateTileFlags(item, true);

	const Position& cylinderMapPos = getPosition();
	const ItemType& iType = Item::items[item->getID()];

	SpectatorVec::const_iterator it;

	//send to client
	Player* tmpPlayer = NULL;
	uint32_t i = 0;
	for(it = list.begin(); it != list.end(); ++it){
		if((tmpPlayer = (*it)->getPlayer())){
			tmpPlayer->sendRemoveTileItem(this, cylinderMapPos, oldStackPosVector[i], item);
			++i;
		}
	}

	//event methods
	for(it = list.begin(); it != list.end(); ++it){
		(*it)->onRemoveTileItem(this, cylinderMapPos, iType, item);
	}
}

void Tile::onUpdateTile()
{
	const Position& cylinderMapPos = getPosition();

	const SpectatorVec& list = g_game.getSpectators(cylinderMapPos);
	SpectatorVec::const_iterator it;

	//send to client
	Player* tmpPlayer = NULL;
	for(it = list.begin(); it != list.end(); ++it){
		if((tmpPlayer = (*it)->getPlayer())){
			tmpPlayer->sendUpdateTile(this, cylinderMapPos);
		}
	}

	//event methods
	for(it = list.begin(); it != list.end(); ++it){
		(*it)->onUpdateTile(this, cylinderMapPos);
	}
}

void Tile::moveCreature(Creature* actor, Creature* creature, Cylinder* toCylinder, bool teleport /* = false*/)
{
	Tile* newTile = toCylinder->getTile();
	int32_t oldStackPos = __getIndexOfThing(creature);

	Position oldPos = getPosition();
	Position newPos = newTile->getPosition();

	Player* tmpPlayer = NULL;
	SpectatorVec list;
	SpectatorVec::iterator it;

	g_game.getSpectators(list, oldPos, false, true);
	g_game.getSpectators(list, newPos, true, true);

	std::vector<uint32_t> oldStackPosVector;
	for(it = list.begin(); it != list.end(); ++it){
		if((tmpPlayer = (*it)->getPlayer())){
			oldStackPosVector.push_back(getClientIndexOfThing(tmpPlayer, creature));
		}
	}

	//remove the creature
	__removeThing(actor, creature, 0);

	// Switch the node ownership
	if(qt_node != newTile->qt_node) {
		qt_node->removeCreature(creature);
		newTile->qt_node->addCreature(creature);
	}
	
	//add the creature
	newTile->__addThing(actor, creature);
	int32_t newStackPos = newTile->__getIndexOfThing(creature);

	if(!teleport){
		if(oldPos.y > newPos.y)
			creature->setDirection(NORTH);
		else if(oldPos.y < newPos.y)
			creature->setDirection(SOUTH);
		if(oldPos.x < newPos.x)
			creature->setDirection(EAST);
		else if(oldPos.x > newPos.x)
			creature->setDirection(WEST);
	}

	//send to client
	uint32_t i = 0;
	for(it = list.begin(); it != list.end(); ++it){
		if((tmpPlayer = (*it)->getPlayer())){
			tmpPlayer->sendCreatureMove(creature, newTile, newPos, this, oldPos, oldStackPosVector[i], teleport);
			++i;
		}
	}

	//event method
	for(it = list.begin(); it != list.end(); ++it){
		(*it)->onCreatureMove(creature, newTile, newPos, this, oldPos, teleport);
	}

	postRemoveNotification(actor, creature, toCylinder, oldStackPos, true);
	newTile->postAddNotification(actor, creature, this, newStackPos);

	g_game.onCreatureMove(actor, creature, this, newTile);
}

ReturnValue Tile::__queryAdd(int32_t index, const Thing* thing, uint32_t count,
	uint32_t flags) const
{
	if(const Creature* creature = thing->getCreature()){
		if(hasBitSet(FLAG_NOLIMIT, flags)){
			return RET_NOERROR;
		}

		if(hasBitSet(FLAG_PATHFINDING, flags)){
			if(floorChange() || positionChange()){
				return RET_NOTPOSSIBLE;
			}
		}

		if(ground == NULL)
			return RET_NOTPOSSIBLE;

		if(const Actor* monster = creature->getActor()){
			if(hasFlag(TILEPROP_PROTECTIONZONE))
				return RET_NOTPOSSIBLE;

			if(floorChange() || positionChange()){
				return RET_NOTPOSSIBLE;
			}

			if(monster->canPushCreatures() && !monster->isSummon()){
				Creature* creature;
				for(uint32_t i = 0; i < creatures_count(); ++i){
					creature = creatures_get(i);
					if(creature->getPlayer() && creature->getPlayer()->hasFlag(PlayerFlag_CannotBeSeen))
						continue;

					if( !creature->getActor() ||
						!creature->isPushable() ||
						(creature->getActor()->isPlayerSummon()))
					{
						return RET_NOTPOSSIBLE;
					}
				}
			}
			else if(!creatures_empty()){
				for(CreatureConstIterator cit = creatures_begin(); cit != creatures_end(); ++cit){
					if(!(*cit)->getPlayer() || !(*cit)->getPlayer()->hasFlag(PlayerFlag_CannotBeSeen)){
						return RET_NOTENOUGHROOM;
					}
				}
			}

			if(hasFlag(TILEPROP_BLOCKSOLIDNOTMOVEABLE)){
				return RET_NOTPOSSIBLE;
			}

			if(hasBitSet(FLAG_PATHFINDING, flags) && (hasFlag(TILEPROP_BLOCKPATHNOTFIELD) && hasFlag(TILEPROP_BLOCKPATHNOTMOVEABLE)) ){
				return RET_NOTPOSSIBLE;
			}

			if(hasFlag(TILEPROP_BLOCKSOLID) || (hasBitSet(FLAG_PATHFINDING, flags) && hasFlag(TILEPROP_BLOCKPATHNOTFIELD))){
				if(!(monster->canPushItems() || hasBitSet(FLAG_IGNOREBLOCKITEM, flags) ) ){
					return RET_NOTPOSSIBLE;
				}
			}

			MagicField* field = getFieldItem();
			if(field && !field->blockSolid()){
				CombatType combatType = field->getCombatType();
				//There is 3 options for a monster to enter a magic field
				//1) Actor is immune
				if(!monster->isImmune(combatType)){
					//1) Actor is "strong" enough to handle the damage
					//3) Actor is already afflicated by this type of condition
					if(hasBitSet(FLAG_IGNOREFIELDDAMAGE, flags)){
						if( !(monster->canPushItems() ||
							monster->hasCondition(Combat::DamageToConditionType(combatType), false)) ){
							return RET_NOTPOSSIBLE;
						}
					}
					else{
						return RET_NOTPOSSIBLE;
					}
				}
			}

			return RET_NOERROR;
		}
		else if(const Player* player = creature->getPlayer()){
			if(!creatures_empty() && !hasBitSet(FLAG_IGNOREBLOCKCREATURE, flags)){
				for(CreatureConstIterator cit = creatures_begin(); cit != creatures_end(); ++cit){
					if(!(*cit)->getPlayer() || !(*cit)->getPlayer()->hasFlag(PlayerFlag_CannotBeSeen)){
						return RET_NOTENOUGHROOM; //RET_NOTPOSSIBLE
					}
				}
			}

			if(player->getParent() == NULL && hasFlag(TILEPROP_NOLOGOUT)){
				//player is trying to login to a "no logout" tile
				return RET_NOTPOSSIBLE;
			}

			if(player->isPzLocked() && !player->getTile()->hasFlag(TILEPROP_PVPZONE) && hasFlag(TILEPROP_PVPZONE)){
				//player is trying to enter a pvp zone while being pz-locked
				return RET_PLAYERISPZLOCKEDENTERPVPZONE;
			}

			if(player->isPzLocked() && player->getTile()->hasFlag(TILEPROP_PVPZONE) && !hasFlag(TILEPROP_PVPZONE)){
				//player is trying to leave a pvp zone while being pz-locked
				return RET_PLAYERISPZLOCKEDLEAVEPVPZONE;
			}

			if(hasFlag(TILEPROP_NOPVPZONE) && player->isPzLocked()){
				return RET_PLAYERISPZLOCKED;
			}

			if(hasFlag(TILEPROP_PROTECTIONZONE) && player->isPzLocked()){
				return RET_PLAYERISPZLOCKED;
			}
		}
		else{
			if(!creatures_empty() && !hasBitSet(FLAG_IGNOREBLOCKCREATURE, flags)){
				for(CreatureConstIterator cit = creatures_begin(); cit != creatures_end(); ++cit){
					if(!(*cit)->getPlayer() || !(*cit)->getPlayer()->hasFlag(PlayerFlag_CannotBeSeen)){
						return RET_NOTENOUGHROOM;
					}
				}
			}
		}

		if(!hasBitSet(FLAG_IGNOREBLOCKITEM, flags)){
			//If the FLAG_IGNOREBLOCKITEM bit isn't set we dont have to iterate every single item
			if(hasFlag(TILEPROP_BLOCKSOLID)){
				return RET_NOTENOUGHROOM;
			}
		}
		else{
			//FLAG_IGNOREBLOCKITEM is set
			if(ground){
				if(ground->blockSolid() && !ground->isMoveable()){
					return RET_NOTPOSSIBLE;
				}
			}

			const Item* iitem;
			for(TileItemConstIterator it = items_begin(); it != items_end(); ++it){
				iitem = (*it);
				if(iitem->blockSolid() && !iitem->isMoveable()){
					return RET_NOTPOSSIBLE;
				}
			}
		}
	}
	else if(const Item* item = thing->getItem()){
#ifdef __DEBUG__
		if(thing->getParent() == NULL && !hasBitSet(FLAG_NOLIMIT, flags)){
			std::cout << "Notice: Tile::__queryAdd() - thing->getParent() == NULL" << std::endl;
		}
#endif
		if(items_count() >= 0xFFFF){
			return RET_NOTPOSSIBLE;
		}

		if(hasBitSet(FLAG_NOLIMIT, flags)){
			return RET_NOERROR;
		}

		if(!creatures_empty() && item->blockSolid() && !hasBitSet(FLAG_IGNOREBLOCKCREATURE, flags)){
			for(CreatureConstIterator cit = creatures_begin(); cit != creatures_end(); ++cit){
				if(!(*cit)->getPlayer() || !(*cit)->getPlayer()->hasFlag(PlayerFlag_CannotBeSeen)){
					return RET_NOTENOUGHROOM;
				}
			}
		}

		//TODO: Move to scripts(?)
		if(item->isHangable() && (hasFlag(TILEPROP_VERTICAL) || hasFlag(TILEPROP_HORIZONTAL)) ){
			for(TileItemConstIterator it = items_begin(); it != items_end(); ++it){
				if((*it)->isHangable()){
					return RET_NEEDEXCHANGE;
				}
			}
		}
		else{
			if(ground == NULL){
				return RET_NOTPOSSIBLE;
			}

			if(hasFlag(TILEPROP_BLOCKSOLID)){
				if(item->isPickupable()){
					ItemVector vector = items_getListWithProps(ITEMPROP_BLOCKSOLID);
					for(ItemVector::iterator it = vector.begin(); it != vector.end(); ++it){
						const ItemType& iType = Item::items[(*it)->getID()];
						if(iType.allowPickupable){
							continue;
						}

						if(!iType.hasHeight || iType.pickupable || iType.isBed()){
							return RET_NOTENOUGHROOM;
						}
					}
				}
				else{
					return RET_NOTENOUGHROOM;
				}
			}
		}
	}

	return RET_NOERROR;
}

ReturnValue Tile::__queryMaxCount(int32_t index, const Thing* thing, uint32_t count, uint32_t& maxQueryCount,
	uint32_t flags) const
{
	maxQueryCount = std::max((uint32_t)1, count);
	return RET_NOERROR;
}

ReturnValue Tile::__queryRemove(const Thing* thing, uint32_t count, uint32_t flags) const
{
	int32_t index = __getIndexOfThing(thing);

	if(index == -1){
		return RET_NOTPOSSIBLE;
	}

	const Item* item = thing->getItem();
	if(item == NULL){
		return RET_NOTPOSSIBLE;
	}

	if(count == 0 || (item->isStackable() && count > item->getItemCount())){
		return RET_NOTPOSSIBLE;
	}

	if(!item->isMoveable() && !hasBitSet(FLAG_IGNORENOTMOVEABLE, flags)){
		return RET_NOTMOVEABLE;
	}

	return RET_NOERROR;
}

Cylinder* Tile::__queryDestination(int32_t& index, const Thing* thing, Item** destItem,
	uint32_t& flags)
{
	Tile* destTile = NULL;
	*destItem = NULL;

	if(floorChangeDown()){
		int dx = getPosition().x;
		int dy = getPosition().y;
		int dz = getPosition().z + 1;
		Tile* downTile = g_game.getTile(dx, dy, dz);

		if(downTile){
			if(downTile->floorChange(NORTH))
				dy += 1;
			if(downTile->floorChange(SOUTH))
				dy -= 1;
			if(downTile->floorChange(EAST))
				dx -= 1;
			if(downTile->floorChange(WEST))
				dx += 1;
			destTile = g_game.getTile(dx, dy, dz);
		}
	}
	else if(floorChange()){
		int dx = getPosition().x;
		int dy = getPosition().y;
		int dz = getPosition().z - 1;

		if(floorChange(NORTH))
			dy -= 1;
		if(floorChange(SOUTH))
			dy += 1;
		if(floorChange(EAST))
			dx += 1;
		if(floorChange(WEST))
			dx -= 1;
		destTile = g_game.getTile(dx, dy, dz);
	}


	if(destTile == NULL){
		destTile = this;
	}
	else{
		flags |= FLAG_NOLIMIT; //Will ignore that there is blocking items/creatures
	}

	if(destTile){
		Thing* destThing = destTile->items_firstDown();
		if(destThing)
			*destItem = destThing->getItem();
	}

	return destTile;
}

void Tile::__addThing(Creature* actor, Thing* thing)
{
	__addThing(actor, 0, thing);
}

void Tile::__addThing(Creature* actor, int32_t index, Thing* thing)
{
	Creature* creature = thing->getCreature();
	if(creature){
		g_game.clearSpectatorCache();
		creature->setParent(this);
		creatures_insert(creatures_begin(), creature);
	}
	else{
		Item* item = thing->getItem();
		if(item == NULL){
#ifdef __DEBUG__MOVESYS__
			std::cout << "Failure: [Tile::__addThing] item == NULL" << std::endl;
			DEBUG_REPORT
#endif
			return /*RET_NOTPOSSIBLE*/;
		}

		if(items_count() > 0xFFFF){
			return /*RET_NOTPOSSIBLE*/;
		}

		item->setParent(this);

		if(item->isGroundTile()){
			if(ground == NULL){
				ground = item;
				onAddTileItem(item);
			}
			else{
				const ItemType& oldType = Item::items[ground->getID()];
				const ItemType& newType = Item::items[item->getID()];

				int32_t oldGroundIndex = __getIndexOfThing(ground);
				Item* oldGround = ground;
				ground->setParent(NULL);
				g_game.FreeThing(ground);
				ground = item;
				updateTileFlags(oldGround, true);
				updateTileFlags(item, false);

				onUpdateTileItem(oldGround, oldType, item, newType);
				postRemoveNotification(actor, oldGround, NULL, oldGroundIndex, true);
			}
		}
		else if(item->isAlwaysOnTop()){
			if(item->isSplash()){
				//remove old splash if exists
				for(TileItemIterator it = items_topBegin(); it != items_topEnd(); ++it){
					if((*it)->isSplash()){
						int32_t oldSplashIndex = __getIndexOfThing(*it);
						Item* oldSplash = *it;
						__removeThing(actor, oldSplash, 1);
						oldSplash->setParent(NULL);
						g_game.FreeThing(oldSplash);
						postRemoveNotification(actor, oldSplash, NULL, oldSplashIndex, true);
						break;
					}
				}
			}

			bool isInserted = false;

			for(TileItemIterator it = items_topBegin(); it != items_topEnd(); ++it){
				//Note: this is different from internalAddThing
				if(Item::items[item->getID()].alwaysOnTopOrder <= Item::items[(*it)->getID()].alwaysOnTopOrder){
					items_insert(it, item);
					isInserted = true;
					break;
				}
			}

			if(!isInserted){
				items_push_back(item);
			}

			onAddTileItem(item);
		}
		else{
			if(item->isMagicField()){
				//remove old field item if exists
				MagicField* oldField = getFieldItem();
				if(oldField){
					if(oldField->isReplaceable()){
						int32_t oldFieldIndex = __getIndexOfThing(oldField);
						__removeThing(actor, oldField, 1);

						oldField->setParent(NULL);
						g_game.FreeThing(oldField);
						postRemoveNotification(actor, oldField, NULL, oldFieldIndex, true);
					}
					else{
						//This magic field cannot be replaced.
						item->setParent(NULL);
						g_game.FreeThing(item);
						return;
					}
				}
			}

			items_insert(items_downBegin(), item);
			++downItemCount;
			onAddTileItem(item);
		}
	}
}

void Tile::__updateThing(Creature* actor, Thing* thing, uint16_t itemId, uint32_t count)
{
	int32_t index = __getIndexOfThing(thing);
	if(index == -1){
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Tile::__updateThing] index == -1" << std::endl;
		DEBUG_REPORT
#endif
		return /*RET_NOTPOSSIBLE*/;
	}

	Item* item = thing->getItem();
	if(item == NULL){
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Tile::__updateThing] item == NULL" << std::endl;
		DEBUG_REPORT
#endif
		return /*RET_NOTPOSSIBLE*/;
	}

	const ItemType& oldType = Item::items[item->getID()];
	const ItemType& newType = Item::items[itemId];

	updateTileFlags(item, true);

	item->setID(itemId);
	item->setSubType(count);

	updateTileFlags(item, false);

	onUpdateTileItem(item, oldType, item, newType);
}

void Tile::__replaceThing(Creature* actor, uint32_t index, Thing* thing)
{
	int32_t pos = index;

	Item* item = thing->getItem();
	if(item == NULL){
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Tile::__updateThing] item == NULL" << std::endl;
		DEBUG_REPORT
#endif
		return /*RET_NOTPOSSIBLE*/;
	}

	Item* oldItem = NULL;
	bool isInserted = false;

	if(!isInserted && ground){
		if(pos == 0){
			oldItem = ground;
			ground = item;
			isInserted = true;
		}

		--pos;
	}

	if(!isInserted && items_count() > 0){
		int32_t topItemSize = items_topCount();
		if(pos < topItemSize){
			TileItemIterator it = items_topBegin();
			it += pos;

			oldItem = (*it);
			it = items_erase(it);
			items_insert(it, item);
			isInserted = true;
		}

		pos -= topItemSize;
	}

	if(!isInserted && creatures_count() > 0){
		if(pos >= (int32_t)creatures_count()){
			pos -= (uint32_t)creatures_count();
		}
		else{
#ifdef __DEBUG__MOVESYS__
			std::cout << "Failure: [Tile::__updateThing] Update object is a creature" << std::endl;
			DEBUG_REPORT
#endif
			return /*RET_NOTPOSSIBLE*/;
		}
	}

	if(!isInserted && items_count() > 0){
		int32_t downItemSize = items_downCount();
		if(pos < downItemSize){
			TileItemIterator it = items_begin();
			it += pos;
			pos = 0;

			oldItem = (*it);
			it = items_erase(it);
			items_insert(it, item);
			isInserted = true;
		}
	}

	if(isInserted){
		item->setParent(this);

		updateTileFlags(oldItem, true);
		updateTileFlags(item, false);
		const ItemType& oldType = Item::items[oldItem->getID()];
		const ItemType& newType = Item::items[item->getID()];
		onUpdateTileItem(oldItem, oldType, item, newType);

		oldItem->setParent(NULL);
		return /*RET_NOERROR*/;
	}

#ifdef __DEBUG__MOVESYS__
	std::cout << "Failure: [Tile::__updateThing] Update object not found" << std::endl;
	DEBUG_REPORT
#endif
}

void Tile::__removeThing(Creature* actor, Thing* thing, uint32_t count)
{
	if(thing->getCreature()){
		CreatureIterator it = std::find(creatures_begin(), creatures_end(), thing);
		if(it != creatures_end()){
			g_game.clearSpectatorCache();
			creatures_erase(it);
		}
		else{
#ifdef __DEBUG__MOVESYS__
			std::cout << "Failure: [Tile::__removeThing] creature not found" << std::endl;
			DEBUG_REPORT
#endif
			return; //RET_NOTPOSSIBLE;
		}
	}
	else if(Item* item = thing->getItem()){
		if(item == NULL){
#ifdef __DEBUG__MOVESYS__
			std::cout << "Failure: [Tile::__removeThing] item == NULL" << std::endl;
			DEBUG_REPORT
#endif
			return /*RET_NOTPOSSIBLE*/;
		}

		int32_t index = __getIndexOfThing(item);
		if(index == -1){
#ifdef __DEBUG__MOVESYS__
			std::cout << "Failure: [Tile::__removeThing] index == -1" << std::endl;
			DEBUG_REPORT
#endif
			return /*RET_NOTPOSSIBLE*/;
		}

		if(item == ground){
			const SpectatorVec& list = g_game.getSpectators(getPosition());
			std::vector<uint32_t> oldStackPosVector;

			Player* tmpPlayer = NULL;
			for(SpectatorVec::const_iterator it = list.begin(); it != list.end(); ++it){
				if((tmpPlayer = (*it)->getPlayer())){
					oldStackPosVector.push_back(getClientIndexOfThing(tmpPlayer, ground));
				}
			}
			ground->setParent(NULL);
			ground = NULL;
			onRemoveTileItem(list, oldStackPosVector, item);

			return /*RET_NOERROR*/;
		}

		if(item->isAlwaysOnTop()){
			for(TileItemIterator it = items_topBegin(); it != items_topEnd(); ++it){
				if(*it == item){
					const SpectatorVec& list = g_game.getSpectators(getPosition());
					std::vector<uint32_t> oldStackPosVector;

					Player* tmpPlayer = NULL;
					for(SpectatorVec::const_iterator iit = list.begin(); iit != list.end(); ++iit){
						if((tmpPlayer = (*iit)->getPlayer())){
							oldStackPosVector.push_back(getClientIndexOfThing(tmpPlayer, *it));
						}
					}
					(*it)->setParent(NULL);
					items_erase(it);
					onRemoveTileItem(list, oldStackPosVector, item);
					return /*RET_NOERROR*/;
				}
			}
		}
		else{
			for(TileItemIterator it = items_downBegin(); it != items_downEnd(); ++it){
				if((*it) == item){
					if(item->isStackable() && count != item->getItemCount()){
						uint8_t newCount = (uint8_t)std::max((int32_t)0, (int32_t)(item->getItemCount() - count));
						updateTileFlags(item, true);
						item->setItemCount(newCount);
						updateTileFlags(item, false);

						const ItemType& it = Item::items[item->getID()];
						onUpdateTileItem(item, it, item, it);
					}
					else{
						const SpectatorVec& list = g_game.getSpectators(getPosition());
						std::vector<uint32_t> oldStackPosVector;

						Player* tmpPlayer = NULL;
						for(SpectatorVec::const_iterator iit = list.begin(); iit != list.end(); ++iit){
							if((tmpPlayer = (*iit)->getPlayer())){
								oldStackPosVector.push_back(getClientIndexOfThing(tmpPlayer, *it));
							}
						}

						(*it)->setParent(NULL);
						items_erase(it);
						--downItemCount;
						onRemoveTileItem(list, oldStackPosVector, item);
					}

					return /*RET_NOERROR*/;
				}
			}
		}
	}
	else{
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Tile::__removeThing] thing not found" << std::endl;
		DEBUG_REPORT
#endif
	}
}

int32_t Tile::getClientIndexOfThing(const Player* player, const Thing* thing) const
{
	int n = -1;

	if(ground){
		if(ground == thing){
			return 0;
		}

		++n;
	}

	if(thing->getItem()){
		for(TileItemConstIterator it = items_topBegin(); it != items_topEnd(); ++it){
			++n;
			if((*it) == thing)
				return n;

			if(n >= 10){
				//client cannot see items above stackpos 9 anyway
				return std::numeric_limits<int32_t>::max();
			}
		}
	}
	else{
		n += items_topCount();
	}

	for(CreatureConstIterator cit = creatures_begin(); cit != creatures_end(); ++cit){
		if((*cit) == thing || player->canSeeCreature(*cit)){
			++n;
		}
		if((*cit) == thing)
			return n;
	}

	if(thing->getItem()){
		for(TileItemConstIterator it = items_downBegin(); it != items_downEnd(); ++it){
			++n;
			if((*it) == thing)
				return n;

			if(n >= 10){
				//client cannot see items above stackpos 10 anyway
				return std::numeric_limits<int32_t>::max();
			}
		}
	}
	else{
		n += items_downCount();
	}

	return -1;
}

int32_t Tile::__getIndexOfThing(const Thing* thing) const
{
	int n = -1;

	if(ground){
		if(ground == thing){
			return 0;
		}

		++n;
	}

	if(thing->getItem()){
		for(TileItemConstIterator it = items_topBegin(); it != items_topEnd(); ++it){
			++n;
			if((*it) == thing)
				return n;
		}
	}
	else{
		n += items_topCount();
	}

	if(thing->getCreature()){
		for(CreatureConstIterator cit = creatures_begin(); cit != creatures_end(); ++cit){
			++n;
			if((*cit) == thing)
				return n;
		}
	}
	else{
		n += creatures_count();
	}

	if(thing->getItem()){
		for(TileItemConstIterator it = items_downBegin(); it != items_downEnd(); ++it){
			++n;
			if((*it) == thing)
				return n;
		}
	}
	else{
		n += items_downCount();
	}

	return -1;
}

int32_t Tile::__getFirstIndex() const
{
	return 0;
}

int32_t Tile::__getLastIndex() const
{
	return getThingCount();
}

uint32_t Tile::__getItemTypeCount(uint16_t itemId, int32_t subType /*= -1*/, bool itemCount /*= true*/) const
{
	ItemVector vector = items_getListWithItemId(itemId);

	const ItemType& it = Item::items[itemId];
	if(!it.stackable && itemCount){
		return vector.size();
	}
	
	uint32_t count = 0;
	Item* item = NULL;
	for(ItemVector::iterator it = vector.begin(); it != vector.end(); ++it){
		item = *it;
		if(subType == -1 || subType == item->getSubType()){

			if(itemCount){
				count+= item->getItemCount();
			}
			else{
				if(item->isRune()){
					count+= item->getCharges();
				}
				else{
					count+= item->getItemCount();
				}
			}
		}
	}

	return count;
}

Thing* Tile::__getThing(uint32_t index) const
{
	if(ground){
		if(index == 0){
			return ground;
		}

		--index;
	}

	uint32_t topItemSize = items_topCount();
	if(index < topItemSize)
		return items_get(items_downCount() + index);

	index -= topItemSize;

	if(index < (uint32_t)creatures_count())
		return creatures_get(index);

	index -= (uint32_t)creatures_count();

	if(index < items_downCount())
		return items_get(index);

	return NULL;
}

void Tile::postAddNotification(Creature* actor, Thing* thing, const Cylinder* oldParent, int32_t index, cylinderlink_t link /*= LINK_OWNER*/)
{
	const Position& cylinderMapPos = getPosition();

	const SpectatorVec& list = g_game.getSpectators(cylinderMapPos);
	SpectatorVec::const_iterator it;

	Player* tmpPlayer = NULL;
	for(it = list.begin(); it != list.end(); ++it){
		if((tmpPlayer = (*it)->getPlayer())){
			tmpPlayer->postAddNotification(actor, thing, oldParent, index, LINK_NEAR);
		}
	}

	//add a reference to this item, it may be deleted after being added (trashholder for example)
	thing->addRef();

	if(link == LINK_OWNER){
		//calling movement scripts
		Creature* creature = thing->getCreature();
		if(creature){
			const Tile* fromTile = NULL;
			if(oldParent){
				fromTile = oldParent->getTile();
			}
		}
		else{
			Item* item = thing->getItem();
			if(item){
				g_game.onItemMove(actor, item, this, true);
			}
		}

		if(Teleport* teleport = getTeleportItem()){
			teleport->__addThing(actor, thing);
		}
		else if(TrashHolder* trashholder = getTrashHolder()){
			trashholder->__addThing(actor, thing);
		}
	}

	//release the reference to this item onces we are finished
	g_game.FreeThing(thing);

	if(!hasFlag(TILEPROP_INDEXED_TILE) && items_count() >= INDEXED_TILE_ITEM_COUNT){
		g_game.makeTileIndexed(this);
	}
}

void Tile::postRemoveNotification(Creature* actor, Thing* thing,  const Cylinder* newParent, int32_t index, bool isCompleteRemoval, cylinderlink_t link /*= LINK_OWNER*/)
{
	const Position& cylinderMapPos = getPosition();

	const SpectatorVec& list = g_game.getSpectators(cylinderMapPos);
	SpectatorVec::const_iterator it;

	if(/*isCompleteRemoval &&*/ getThingCount() > 8){
		onUpdateTile();
	}

	Player* tmpPlayer = NULL;
	for(it = list.begin(); it != list.end(); ++it){
		if((tmpPlayer = (*it)->getPlayer())){
			tmpPlayer->postRemoveNotification(actor, thing, newParent, index, isCompleteRemoval, LINK_NEAR);
		}
	}

	//calling movement scripts
	Creature* creature = thing->getCreature();
	if(creature){
		const Tile* toTile = NULL;
		if(newParent){
			toTile = newParent->getTile();
		}
	}
	else{
		Item* item = thing->getItem();
		if(item){
			g_game.onItemMove(actor, item, this, false);
		}
	}
}

void Tile::__internalAddThing(Thing* thing)
{
	__internalAddThing(0, thing);
}

void Tile::__internalAddThing(uint32_t index, Thing* thing)
{
	thing->setParent(this);

	Creature* creature = thing->getCreature();
	if(creature){
		g_game.clearSpectatorCache();
		creatures_insert(creatures_begin(), creature);
	}
	else{
		Item* item = thing->getItem();

		if(item == NULL)
			return;

		if(items_count() >= 0xFFFF){
			return /*RET_NOTPOSSIBLE*/;
		}

		if(item->isGroundTile()){
			if(ground == NULL){
				ground = item;
			}
		}
		else if(item->isAlwaysOnTop()){
			bool isInserted = false;
			for(TileItemIterator it = items_topBegin(); it != items_topEnd(); ++it){
				if(Item::items[(*it)->getID()].alwaysOnTopOrder > Item::items[item->getID()].alwaysOnTopOrder){
					items_insert(it, item);
					isInserted = true;
					break;
				}
			}

			if(!isInserted){
				items_push_back(item);
			}
		}
		else{
			items_insert(items_downBegin(), item);
			++downItemCount;
		}

		updateTileFlags(item, false);
	}
}

bool Tile::hasItemWithProperty(uint32_t props) const
{
	if(ground && ground->hasProperty(props)){
		return true;
	}

	for(TileItemConstIterator it = items_begin(); it != items_end(); ++it){
		if((*it)->hasProperty(props))
			return true;
	}

	return false;
}

bool Tile::hasItemWithProperty(Item* exclude, uint32_t props) const
{
	assert(exclude);
	if(ground && exclude != ground && ground->hasProperty(props)){
		return true;
	}

	for(TileItemConstIterator it = items_begin(); it != items_end(); ++it){
		const Item* item = *it;
		if(item != exclude && item->hasProperty(props))
			return true;
	}

	return false;
}

void Tile::updateTileFlags(Item* item, bool removed)
{
	if(!removed){
		if(!hasFlag(TILEPROP_FLOORCHANGE)){
			if(item->hasProperty(ITEMPROP_FLOORCHANGEDOWN)){
				setFlag(TILEPROP_FLOORCHANGE);
				setFlag(TILEPROP_FLOORCHANGE_DOWN);
			}
			if(item->hasProperty(ITEMPROP_FLOORCHANGENORTH)){
				setFlag(TILEPROP_FLOORCHANGE);
				setFlag(TILEPROP_FLOORCHANGE_NORTH);
			}
			if(item->hasProperty(ITEMPROP_FLOORCHANGESOUTH)){
				setFlag(TILEPROP_FLOORCHANGE);
				setFlag(TILEPROP_FLOORCHANGE_SOUTH);
			}
			if(item->hasProperty(ITEMPROP_FLOORCHANGEEAST)){
				setFlag(TILEPROP_FLOORCHANGE);
				setFlag(TILEPROP_FLOORCHANGE_EAST);
			}
			if(item->hasProperty(ITEMPROP_FLOORCHANGEWEST)){
				setFlag(TILEPROP_FLOORCHANGE);
				setFlag(TILEPROP_FLOORCHANGE_WEST);
			}
		}

		if(item->hasProperty(ITEMPROP_BLOCKSOLID)){
			setFlag(TILEPROP_BLOCKSOLID);

			if(!item->hasProperty(ITEMPROP_MOVEABLE)){
				setFlag(TILEPROP_BLOCKSOLIDNOTMOVEABLE);
			}
		}
		if(item->hasProperty(ITEMPROP_BLOCKPROJECTILE)){
			setFlag(TILEPROP_BLOCKPATH);

			if(!item->hasProperty(ITEMPROP_MOVEABLE)){
				setFlag(TILEPROP_BLOCKPATHNOTMOVEABLE);
			}

			if(!item->getMagicField()){
				setFlag(TILEPROP_BLOCKPATHNOTFIELD);
			}
		}
		if(item->hasProperty(ITEMPROP_BLOCKPROJECTILE)){
			setFlag(TILEPROP_BLOCKPROJECTILE);
		}
		if(item->hasProperty(ITEMPROP_ISVERTICAL)){
			setFlag(TILEPROP_VERTICAL);
		}
		if(item->hasProperty(ITEMPROP_ISHORIZONTAL)){
			setFlag(TILEPROP_HORIZONTAL);
		}
		if(item->getTeleport()){
			setFlag(TILEPROP_POSITIONCHANGE);
		}
	}
	else{
		if(item->hasProperty(ITEMPROP_FLOORCHANGEDOWN) && !hasItemWithProperty(item, ITEMPROP_FLOORCHANGEDOWN) ){
			resetFlag(TILEPROP_FLOORCHANGE);
			resetFlag(TILEPROP_FLOORCHANGE_DOWN);
		}
		if(item->hasProperty(ITEMPROP_FLOORCHANGENORTH) && !hasItemWithProperty(item, ITEMPROP_FLOORCHANGENORTH) ){
			resetFlag(TILEPROP_FLOORCHANGE);
			resetFlag(TILEPROP_FLOORCHANGE_NORTH);
		}
		if(item->hasProperty(ITEMPROP_FLOORCHANGESOUTH) && !hasItemWithProperty(item, ITEMPROP_FLOORCHANGESOUTH) ){
			resetFlag(TILEPROP_FLOORCHANGE);
			resetFlag(TILEPROP_FLOORCHANGE_SOUTH);
		}
		if(item->hasProperty(ITEMPROP_FLOORCHANGEEAST) && !hasItemWithProperty(item, ITEMPROP_FLOORCHANGEEAST) ){
			resetFlag(TILEPROP_FLOORCHANGE);
			resetFlag(TILEPROP_FLOORCHANGE_EAST);
		}
		if(item->hasProperty(ITEMPROP_FLOORCHANGEWEST) && !hasItemWithProperty(item, ITEMPROP_FLOORCHANGEWEST) ){
			resetFlag(TILEPROP_FLOORCHANGE);
			resetFlag(TILEPROP_FLOORCHANGE_WEST);
		}

		if(item->hasProperty(ITEMPROP_BLOCKSOLID) && !hasItemWithProperty(item, ITEMPROP_BLOCKSOLID) ){
			resetFlag(TILEPROP_BLOCKSOLID);

			if(!item->hasProperty(ITEMPROP_MOVEABLE) && !hasItemWithProperty(item, ITEMPROP_MOVEABLE) ){
				resetFlag(TILEPROP_BLOCKSOLIDNOTMOVEABLE);
			}
		}
		if(item->hasProperty(ITEMPROP_BLOCKPROJECTILE) && !hasItemWithProperty(item, ITEMPROP_BLOCKPROJECTILE) ){
			resetFlag(TILEPROP_BLOCKPATH);

			if(!item->hasProperty(ITEMPROP_MOVEABLE) && !hasItemWithProperty(item, ITEMPROP_MOVEABLE) ){
				resetFlag(TILEPROP_BLOCKPATHNOTMOVEABLE);
			}

			if(!item->getMagicField()){
				resetFlag(TILEPROP_BLOCKPATHNOTFIELD);
			}
		}
		if(item->hasProperty(ITEMPROP_BLOCKPROJECTILE) && !hasItemWithProperty(item, ITEMPROP_BLOCKPROJECTILE)){
			resetFlag(TILEPROP_BLOCKPROJECTILE);
		}
		if(item->hasProperty(ITEMPROP_ISVERTICAL) && !hasItemWithProperty(item, ITEMPROP_ISVERTICAL) ){
			resetFlag(TILEPROP_VERTICAL);
		}
		if(item->hasProperty(ITEMPROP_ISHORIZONTAL) && !hasItemWithProperty(item, ITEMPROP_ISHORIZONTAL) ){
			resetFlag(TILEPROP_HORIZONTAL);
		}
		if(item->getTeleport()){
			resetFlag(TILEPROP_POSITIONCHANGE);
		}
	}
}
