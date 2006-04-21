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


#include "definitions.h"

#include <string>
#include <iostream>

#include "definitions.h"
#include "tile.h"
#include "game.h"
#include "player.h"
#include "creature.h"
#include "teleport.h"
#include "trashholder.h"
#include "mailbox.h"
#include "combat.h"

extern Game g_game;

Tile Tile::null_tile(0xFFFF, 0xFFFF, 0xFFFF);

bool Tile::hasProperty(enum ITEMPROPERTY prop) const
{
	if(prop == PROTECTIONZONE && isPz())
		return true;

	if(ground && ground->hasProperty(prop)){
		return true;
	}

	ItemVector::const_iterator iit;
	for(iit = topItems.begin(); iit != topItems.end(); ++iit){
		if((*iit)->hasProperty(prop))
			return true;
	}

	for(iit = downItems.begin(); iit != downItems.end(); ++iit){
		if((*iit)->hasProperty(prop))
			return true;
	}

	return false;
}

bool Tile::floorChange() const
{
	ItemVector::const_iterator iit;
	if(ground && ground->floorChangeDown())
		return true;

	for (iit = topItems.begin(); iit != topItems.end(); ++iit){  
		if ((*iit)->floorChangeNorth() || (*iit)->floorChangeSouth() || (*iit)->floorChangeEast() || (*iit)->floorChangeWest())
			return true;      
	}

	for (iit = downItems.begin(); iit != downItems.end(); ++iit){ 
		if ((*iit)->floorChangeNorth() || (*iit)->floorChangeSouth() || (*iit)->floorChangeEast() || (*iit)->floorChangeWest())
			return true;
	}

	return false;
}

bool Tile::floorChangeDown() const
{
	if(ground && ground->floorChangeDown())
		return true;

	ItemVector::const_iterator iit;
	for(iit = topItems.begin(); iit != topItems.end(); ++iit){
		if((*iit)->floorChangeDown())
			return true;
	}

	for(iit = downItems.begin(); iit != downItems.end(); ++iit){
		if((*iit)->floorChangeDown())
			return true;
	}

	return false;
}

bool Tile::floorChange(Direction direction) const
{  
	ItemVector::const_iterator iit;
	for(iit = topItems.begin(); iit != topItems.end(); ++iit){
		if(direction == NORTH){  
			if((*iit)->floorChangeNorth())
				return true;
		}
		else if(direction == SOUTH){
			if((*iit)->floorChangeSouth())
				return true;
		}
		else if(direction == EAST){
			if((*iit)->floorChangeEast())
				return true;
		}
		else if(direction == WEST){
			if((*iit)->floorChangeWest())
				return true;
		}
	}

	for(iit = downItems.begin(); iit != downItems.end(); ++iit){
		if(direction == NORTH){  
			if((*iit)->floorChangeNorth())
				return true;
		}
		else if(direction == SOUTH){
			if((*iit)->floorChangeSouth())
				return true;
		}
		else if(direction == EAST){
			if((*iit)->floorChangeEast())
				return true;
		}
		else if(direction == WEST){
			if((*iit)->floorChangeWest())
				return true;
		}
	}

	return false;
}

uint32_t Tile::getHeight() const
{
	uint32_t height = 0;
	Item* iiItem = NULL;
	for(uint32_t i = 0; i < getThingCount(); ++i){
		iiItem = __getThing(i)->getItem();

		if(iiItem && iiItem->hasProperty(HASHEIGHT))
			++height;
	}

	return height;
}

int Tile::getThingCount() const
{
	return (uint32_t) (ground ? 1 : 0) + topItems.size() + creatures.size() + downItems.size();
}

std::string Tile::getDescription(int32_t lookDistance) const
{
	std::string ret = "You dont know why, but you cant see anything!";
	return ret;
}

Teleport* Tile::getTeleportItem() const
{
	Teleport* teleport = NULL;
	for(ItemVector::const_iterator iit = topItems.begin(); iit != topItems.end(); ++iit){
		teleport = (*iit)->getTeleport();
		if(teleport)
			return teleport;
	}

	return NULL;
}

MagicField* Tile::getFieldItem() const
{
	/*
	MagicEffectItem* fieldItem = NULL;
	for(ItemVector::const_iterator iit = downItems.begin(); iit != downItems.end(); ++iit){
		fieldItem = dynamic_cast<MagicEffectItem*>(*iit);
		if(fieldItem)
			return fieldItem;
	}
	*/

	return NULL;
}

TrashHolder* Tile::getTrashHolder() const
{
	TrashHolder* trashholder = NULL;
	Item* iiItem = NULL;
	for(uint32_t i = 0; i < getThingCount(); ++i){
		iiItem = __getThing(i)->getItem();
		if(iiItem && (trashholder = iiItem->getTrashHolder()))
			return trashholder;
	}

	return NULL;
}

Mailbox* Tile::getMailbox() const
{
	Mailbox* mailbox = NULL;
	Item* iiItem = NULL;
	for(uint32_t i = 0; i < getThingCount(); ++i){
		iiItem = __getThing(i)->getItem();
		if(iiItem && (mailbox = iiItem->getMailbox()))
			return mailbox;
	}

	return NULL;
}

Creature* Tile::getTopCreature()
{
	if(!creatures.empty()){
		return *(creatures.begin());
	}

	return NULL;
}

Item* Tile::getTopDownItem()
{
	if(!downItems.empty()){
		return *(downItems.begin());
	}

	return NULL;
}

Item* Tile::getTopTopItem()
{
	if(!topItems.empty()){
		return topItems.back();
	}

	return NULL;
}

Thing* Tile::getTopThing()
{
	Thing* thing = NULL;
	thing = getTopCreature();
	if(thing != NULL)
		return thing;

	thing = getTopDownItem();
	if(thing != NULL)
		return thing;

	thing = getTopTopItem();
	if(thing != NULL)
		return thing;

	if(ground)
		return ground;

	return NULL;
}

Item* Tile::getMoveableBlockingItem()
{
	for(ItemVector::const_iterator iit = downItems.begin(); iit != downItems.end(); ++iit){
		const ItemType& iiType = Item::items[(*iit)->getID()];
		if((iiType.blockPathFind || iiType.blockSolid) && iiType.moveable)
			return *iit;
	}

	return NULL;
}

bool Tile::hasFlag(tileflags_t flag) const
{
	return ((flags & flag) == flag);
}

void Tile::setFlag(tileflags_t flag)
{
	flags |= flag;
}

bool Tile::isPz() const
{
	return hasFlag(TILESTATE_PROTECTIONZONE);
}

void Tile::setPz()
{
	setFlag(TILESTATE_PROTECTIONZONE);
}

void Tile::onAddTileItem(Item* item)
{
	const Position& cylinderMapPos = getPosition();

	SpectatorVec list;
	SpectatorVec::iterator it;
	g_game.getSpectators(Range(cylinderMapPos, true), list);

	//send to client
	Player* player = NULL;
	for(it = list.begin(); it != list.end(); ++it){
		if(player = (*it)->getPlayer()){
			player->sendAddTileItem(cylinderMapPos, item);
		}
	}
	
	//event methods
	for(it = list.begin(); it != list.end(); ++it){
		(*it)->onAddTileItem(cylinderMapPos, item);
	}
}

void Tile::onUpdateTileItem(uint32_t index, Item* oldItem, Item* newItem)
{
	const Position& cylinderMapPos = getPosition();

	SpectatorVec list;
	SpectatorVec::iterator it;
	g_game.getSpectators(Range(cylinderMapPos, true), list);

	//send to client
	Player* player = NULL;
	for(it = list.begin(); it != list.end(); ++it){
		if(player = (*it)->getPlayer()){
			player->sendUpdateTileItem(cylinderMapPos, index, oldItem, newItem);
		}
	}

	//event methods
	for(it = list.begin(); it != list.end(); ++it){
		(*it)->onUpdateTileItem(cylinderMapPos, index, oldItem, newItem);
	}
}

void Tile::onRemoveTileItem(uint32_t index, Item* item)
{
	const Position& cylinderMapPos = getPosition();

	SpectatorVec list;
	SpectatorVec::iterator it;
	g_game.getSpectators(Range(cylinderMapPos, true), list);

	//send to client
	Player* player = NULL;
	for(it = list.begin(); it != list.end(); ++it){
		if(player = (*it)->getPlayer()){
			player->sendRemoveTileItem(cylinderMapPos, index, item);
		}
	}

	//event methods
	for(it = list.begin(); it != list.end(); ++it){
		(*it)->onRemoveTileItem(cylinderMapPos, index, item);
	}
}

void Tile::onUpdateTile()
{
	const Position& cylinderMapPos = getPosition();

	SpectatorVec list;
	SpectatorVec::iterator it;
	g_game.getSpectators(Range(cylinderMapPos, true), list);

	//send to client
	Player* player = NULL;
	for(it = list.begin(); it != list.end(); ++it){
		if(player = (*it)->getPlayer()){
			player->sendUpdateTile(cylinderMapPos);
		}
	}

	//event methods
	for(it = list.begin(); it != list.end(); ++it){
		(*it)->onUpdateTile(cylinderMapPos);
	}
}

void Tile::moveCreature(Creature* creature, Cylinder* toCylinder, bool teleport /* = false*/)
{
	int32_t oldStackPos = __getIndexOfThing(creature);

	//remove the creature
	__removeThing(creature, 0);

	//add the creature
	toCylinder->__addThing(creature);

	Position fromPos = getPosition();
	Position toPos = toCylinder->getPosition();

	if(!teleport){
		if(fromPos.y > toPos.y)
			creature->setDirection(NORTH);
		else if(fromPos.y < toPos.y)
			creature->setDirection(SOUTH);
		if(fromPos.x < toPos.x)
			creature->setDirection(EAST);
		else if(fromPos.x > toPos.x)
			creature->setDirection(WEST);
	}

	SpectatorVec list;
	SpectatorVec::iterator it;
	g_game.getSpectators(Range(fromPos, true), list);
	g_game.getSpectators(Range(toPos, true), list);

	//send to client
	Player* player = NULL;
	for(it = list.begin(); it != list.end(); ++it) {
		if(player = (*it)->getPlayer()){
			player->sendCreatureMove(creature, fromPos, oldStackPos, teleport);
		}
	}

	//event method
	for(it = list.begin(); it != list.end(); ++it) {
		(*it)->onCreatureMove(creature, fromPos, oldStackPos, teleport);
	}

	toCylinder->postAddNotification(creature);
	postRemoveNotification(creature, true);
}

ReturnValue Tile::__queryAdd(int32_t index, const Thing* thing, uint32_t count,
	uint32_t flags) const
{
	Thing* iithing = NULL;

	if(const Creature* creature = thing->getCreature()){
		if(!creatures.empty())
			return RET_NOTPOSSIBLE;

		if(ground == NULL)
			return RET_NOTPOSSIBLE;
		
		if(const Monster* monster = creature->getMonster()){
			if(hasFlag(TILESTATE_PROTECTIONZONE))
				return RET_NOTPOSSIBLE;

			if(const MagicField* fieldItem = getFieldItem()){
				if(!monster->isImmune(fieldItem->getDamageType())){
					return RET_NOTPOSSIBLE;
				}
			}

			if(floorChange() || getTeleportItem()){
				return RET_NOTPOSSIBLE;
			}

			for(uint32_t i = 0; i < getThingCount(); ++i){
				iithing = __getThing(i);

				if(const Item* iitem = iithing->getItem()){
					const ItemType& iiType = Item::items[iitem->getID()];

					if(iiType.blockSolid){
						if(!monster->canPushItems() || !iiType.moveable){
							return RET_NOTPOSSIBLE;
						}
					}
				}
			}

			return RET_NOERROR;
		}
		else if(const Player* player = creature->getPlayer()){
			if(hasFlag(TILESTATE_PROTECTIONZONE) && player->hasCondition(CONDITION_PZLOCK)){
				return RET_PLAYERISPZLOCKED;
			}
		}

		for(uint32_t i = 0; i < getThingCount(); ++i){
			iithing = __getThing(i);

			if(const Item* iitem = iithing->getItem()){
				const ItemType& iiType = Item::items[iitem->getID()];

				if(iiType.blockSolid){
					//check if this a creature that just is about to login/spawn
					//those can be placed here if the blocking item is moveable
					if(!creature->getParent()){
						if(!iiType.moveable)
							return RET_NOTPOSSIBLE;
					}
					else
						return RET_NOTPOSSIBLE;
				}
			}
		}
	}
	else if(const Item* item = thing->getItem()){
		//If its a new (summoned item) always accept it, or FLAG_NOLIMIT is set
		if(thing->getParent() == NULL || ((flags & FLAG_NOLIMIT) == FLAG_NOLIMIT) ){
			return RET_NOERROR;
		}

		if(ground == NULL)
			return RET_NOTPOSSIBLE;

		if(!creatures.empty() && item->isBlocking())
			return RET_NOTENOUGHROOM;

		for(uint32_t i = 0; i < getThingCount(); ++i){
			iithing = __getThing(i);

			if(const Item* iitem = iithing->getItem()){
				const ItemType& iiType = Item::items[iitem->getID()];

				if(iiType.blockSolid){
					if(item->isPickupable()){

						//TODO: query script interface
						if(iitem->getID() == ITEM_DUSTBIN)
							continue;

						if(!iiType.hasHeight || iiType.pickupable)
							return RET_NOTENOUGHROOM;
					}
					else
						return RET_NOTENOUGHROOM;
				}

				/*
				//TODO: query script interface
				if(iitem->getID() == ITEM_DUSTBIN)
					continue;

				if(iiType.blockSolid){
					if(item->isPickupable()){
						//experimental
						//if((iiType.isVertical || iiType.isHorizontal) && item->isHangable()){
						//	ItemVector::const_iterator iit;
						//	for(iit = downItems.begin(); iit != downItems.end(); ++iit){
						//		if((*iit)->isHangable())
						//			return RET_NOTENOUGHROOM;
						//	}
						//}
						//else
						if(!iiType.hasHeight || iiType.pickupable)
							return RET_NOTENOUGHROOM;
					}
					else
						return RET_NOTENOUGHROOM;
				}
				*/
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

ReturnValue Tile::__queryRemove(const Thing* thing, uint32_t count) const
{
	uint32_t index = __getIndexOfThing(thing);

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

	if(item->isNotMoveable()){
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
		Tile* downTile = g_game.getTile(getTilePosition().x, getTilePosition().y, getTilePosition().z + 1);

		if(downTile){
			if(downTile->floorChange(NORTH) && downTile->floorChange(EAST)){
				destTile = g_game.getTile(getTilePosition().x - 1, getTilePosition().y + 1, getTilePosition().z + 1);
			}
			else if(downTile->floorChange(NORTH) && downTile->floorChange(WEST)){
				destTile = g_game.getTile(getTilePosition().x + 1, getTilePosition().y + 1, getTilePosition().z + 1);
			}
			else if(downTile->floorChange(SOUTH) && downTile->floorChange(EAST)){
				destTile = g_game.getTile(getTilePosition().x - 1, getTilePosition().y - 1, getTilePosition().z + 1);
			}
			else if(downTile->floorChange(SOUTH) && downTile->floorChange(WEST)){
				destTile = g_game.getTile(getTilePosition().x + 1, getTilePosition().y - 1, getTilePosition().z + 1);
			}
			else if(downTile->floorChange(NORTH)){
				destTile = g_game.getTile(getTilePosition().x, getTilePosition().y + 1, getTilePosition().z + 1);
			}
			else if(downTile->floorChange(SOUTH)){
				destTile = g_game.getTile(getTilePosition().x, getTilePosition().y - 1, getTilePosition().z + 1);
			}
			else if(downTile->floorChange(EAST)){
				destTile = g_game.getTile(getTilePosition().x - 1, getTilePosition().y, getTilePosition().z + 1);
			}
			else if(downTile->floorChange(WEST)){
				destTile = g_game.getTile(getTilePosition().x + 1, getTilePosition().y, getTilePosition().z + 1);
			}
			else
				destTile = downTile;
		}
	}
	else if(floorChange()){
		if(floorChange(NORTH) && floorChange(EAST)){
			destTile = g_game.getTile(getTilePosition().x + 1, getTilePosition().y - 1, getTilePosition().z - 1);
		}
		else if(floorChange(NORTH) && floorChange(WEST)){
			destTile = g_game.getTile(getTilePosition().x - 1, getTilePosition().y - 1, getTilePosition().z - 1);
		}
		else if(floorChange(SOUTH) && floorChange(EAST)){
			destTile = g_game.getTile(getTilePosition().x + 1, getTilePosition().y + 1, getTilePosition().z - 1);
		}
		else if(floorChange(SOUTH) && floorChange(WEST)){
			destTile = g_game.getTile(getTilePosition().x - 1, getTilePosition().y + 1, getTilePosition().z - 1);
		}
		else if(floorChange(NORTH)){
			destTile = g_game.getTile(getTilePosition().x, getTilePosition().y - 1, getTilePosition().z - 1);
		}
		else if(floorChange(SOUTH)){
			destTile = g_game.getTile(getTilePosition().x, getTilePosition().y + 1, getTilePosition().z - 1);
		}
		else if(floorChange(EAST)){
			destTile = g_game.getTile(getTilePosition().x + 1, getTilePosition().y, getTilePosition().z - 1);
		}
		else if(floorChange(WEST)){
			destTile = g_game.getTile(getTilePosition().x - 1, getTilePosition().y, getTilePosition().z - 1);
		}
	}


	if(destTile == NULL){
		destTile = this;
	}
	else{
		flags |= FLAG_NOLIMIT; //Will ignore that there is blocking items/creatures
	}

	if(destTile){
		Thing* destThing = destTile->getTopDownItem();
		if(destThing)
			*destItem = destThing->getItem();
	}

	return destTile;
}

void Tile::__addThing(Thing* thing)
{
	__addThing(0, thing);
}

void Tile::__addThing(int32_t index, Thing* thing)
{
	Creature* creature = thing->getCreature();
	if(creature){
		creature->setParent(this);
		creatures.insert(creatures.begin(), creature);
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
		
		item->setParent(this);

		if(item->isGroundTile()){
			if(ground == NULL){
				onAddTileItem(item);
			}
			else{
				uint32_t index = __getIndexOfThing(ground);				
				onUpdateTileItem(index, ground, item);

				ground->setParent(NULL);
				g_game.FreeThing(ground);
				ground = NULL;
			}

			ground = item;
		}
		else if(item->isAlwaysOnTop()){
			if(item->isSplash()){
				//remove old splash if exists
				ItemVector::iterator iit;
				for(iit = topItems.begin(); iit != topItems.end(); ++iit){
					if((*iit)->isSplash()){
						Item* oldSplash = *iit;
						__removeThing(oldSplash, 1);

						oldSplash->setParent(NULL);
						g_game.FreeThing(oldSplash);
						break;
					}
				}
			}

			bool isInserted = false;
			ItemVector::iterator iit;
			for(iit = topItems.begin(); iit != topItems.end(); ++iit){
				//Note: this is different from internalAddThing
				if(Item::items[item->getID()].alwaysOnTopOrder <= Item::items[(*iit)->getID()].alwaysOnTopOrder){
					topItems.insert(iit, item);
					isInserted = true;
					break;
				}
			}

			if(!isInserted){
				topItems.push_back(item);
			}

			onAddTileItem(item);
		}
		else{
			if(item->isMagicField()){
				//remove old field item if exists
				ItemVector::iterator iit;
				for(iit = downItems.begin(); iit != downItems.end(); ++iit){
					if((*iit)->isMagicField()){
						Item* oldField = *iit;
						__removeThing(oldField, 1);

						oldField->setParent(NULL);
						g_game.FreeThing(oldField);
						break;
					}
				}
			}

			downItems.insert(downItems.begin(), item);
			onAddTileItem(item);
		}
	}
}

void Tile::__updateThing(Thing* thing, uint32_t count)
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

	item->setItemCountOrSubtype(count);
	onUpdateTileItem(index, item, item);
}

void Tile::__replaceThing(uint32_t index, Thing* thing)
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

	if(ground){
		if(pos == 0){
			oldItem = ground;
			ground = item;
		}

		--pos;
	}

	if(pos >= 0 && pos < topItems.size()){
		ItemVector::iterator it = topItems.begin();
		it += pos;
		pos = 0;

		oldItem = (*it);
		it = topItems.erase(it);
		topItems.insert(it, item);
	}

	pos -= (uint32_t)topItems.size();

	if(pos >= 0 && pos < creatures.size()){
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Tile::__updateThing] Update object is a creature" << std::endl;
		DEBUG_REPORT
#endif
		return /*RET_NOTPOSSIBLE*/;
	}

	pos -= (uint32_t)creatures.size();

	if(pos >= 0 && pos < downItems.size()){
		ItemVector::iterator it = downItems.begin();
		it += pos;
		pos = 0;

		oldItem = (*it);
		it = downItems.erase(it);
		downItems.insert(it, item);
	}

	if(pos == 0){
		item->setParent(this);
		onUpdateTileItem(index, oldItem, item);

		oldItem->setParent(NULL);
		return /*RET_NOERROR*/;
	}

#ifdef __DEBUG__MOVESYS__
	std::cout << "Failure: [Tile::__updateThing] Update object not found" << std::endl;
	DEBUG_REPORT
#endif
}

void Tile::__removeThing(Thing* thing, uint32_t count)
{
	Creature* creature = thing->getCreature();
	if(creature){
		CreatureVector::iterator it = std::find(creatures.begin(), creatures.end(), thing);

		if(it == creatures.end()){
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Tile::__removeThing] creature not found" << std::endl;
		DEBUG_REPORT
#endif
		return; //RET_NOTPOSSIBLE;
		}

		creatures.erase(it);
		return;
	}
	else{
		Item* item = thing->getItem();
		if(item == NULL){
#ifdef __DEBUG__MOVESYS__
			std::cout << "Failure: [Tile::__removeThing] item == NULL" << std::endl;
			DEBUG_REPORT
#endif
			return /*RET_NOTPOSSIBLE*/;
		}

		uint32_t index = __getIndexOfThing(item);
		if(index == -1){
#ifdef __DEBUG__MOVESYS__
			std::cout << "Failure: [Tile::__removeThing] index == -1" << std::endl;
			DEBUG_REPORT
#endif
			return /*RET_NOTPOSSIBLE*/;
		}

		if(item == ground){
			
			onRemoveTileItem(index, item);

			ground->setParent(NULL);
			ground = NULL;
			return /*RET_NOERROR*/;
		}

		ItemVector::iterator iit;
		if(item->isAlwaysOnTop()){
			for(iit = topItems.begin(); iit != topItems.end(); ++iit){
				if(*iit == item){

					onRemoveTileItem(index, item);

					(*iit)->setParent(NULL);
					topItems.erase(iit);
					return /*RET_NOERROR*/;
				}
			}
		}
		else{
			for (iit = downItems.begin(); iit != downItems.end(); ++iit){
				if(*iit == item){
					if(item->isStackable() && count != item->getItemCount()){							
						int newCount = std::max(0, (int)(item->getItemCount() - count));
						item->setItemCount(newCount);

						onUpdateTileItem(index, item, item);
					}
					else {
						
						onRemoveTileItem(index, item);

						(*iit)->setParent(NULL);
						downItems.erase(iit);
					}

					return /*RET_NOERROR*/;
				}
			}
		}
	}
#ifdef __DEBUG__MOVESYS__
	std::cout << "Failure: [Tile::__removeThing] thing not found" << std::endl;
	DEBUG_REPORT
#endif
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

	ItemVector::const_iterator iit;
	for(iit = topItems.begin(); iit != topItems.end(); ++iit){
		++n;
		if((*iit) == thing)
			return n;
	}

	CreatureVector::const_iterator cit;
	for(cit = creatures.begin(); cit != creatures.end(); ++cit){
		++n;
		if((*cit) == thing)
			return n;
	}

	for(iit = downItems.begin(); iit != downItems.end(); ++iit){
		++n;
		if((*iit) == thing)
			return n;
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

uint32_t Tile::__getItemTypeCount(uint16_t itemId) const
{
	uint32_t count = 0;
	Thing* thing = NULL;
	for(uint32_t i = 0; i < getThingCount(); ++i){
		thing = __getThing(i);

		if(const Item* item = thing->getItem()){
			if(item->getID() == itemId){
				if(item->isStackable()){
					count+= item->getItemCount();
				}
				else{
					++count;
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

	if((unsigned) index < topItems.size())
		return topItems[index];

	index -= (uint32_t)topItems.size();

	if((unsigned) index < creatures.size())
		return creatures[index];

	index -= (uint32_t)creatures.size();

	if((unsigned) index < downItems.size())
		return downItems[index];

	return NULL;
}

void Tile::postAddNotification(Thing* thing, bool hasOwnership /*= true*/)
{
	const Position& cylinderMapPos = getPosition();

	SpectatorVec list;
	SpectatorVec::iterator it;
	g_game.getSpectators(Range(cylinderMapPos, true), list);

	for(it = list.begin(); it != list.end(); ++it){
		if(Player* player = (*it)->getPlayer()){
			player->postAddNotification(thing, false);
		}
	}


	if(hasOwnership){
		//do action(s)
		if(Creature* creature = thing->getCreature()){
			MagicField* fieldItem = getFieldItem();
			if(fieldItem){

				//remove magic walls/wild growth
				if(fieldItem->isBlocking()){
					g_game.internalRemoveItem(fieldItem, 1);
				}
				else{
					/*add condition*/
					const Condition* condition = fieldItem->getCondition();

					if(condition){
						creature->addCondition(condition->clone());
					}
				}
			}
		}
		
		if(Teleport* teleport = getTeleportItem()){
			teleport->__addThing(thing);
		}
		else if(TrashHolder* trashHolder = getTrashHolder()){
			//TODO: query script interface
			trashHolder->__addThing(thing);
		}
		else if(Mailbox* mailbox = getMailbox()){
			mailbox->__addThing(thing);
		}
	}
}

void Tile::postRemoveNotification(Thing* thing, bool isCompleteRemoval, bool hadOwnership /*= true*/)
{
	const Position& cylinderMapPos = getPosition();

	SpectatorVec list;
	SpectatorVec::iterator it;
	g_game.getSpectators(Range(cylinderMapPos, true), list);

	if(/*isCompleteRemoval &&*/ getThingCount() > 8){
		onUpdateTile();
	}

	for(it = list.begin(); it != list.end(); ++it){
		if(Player* player = (*it)->getPlayer()){
			player->postRemoveNotification(thing, isCompleteRemoval, false);
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
		creatures.insert(creatures.begin(), creature);
	}
	else{
		Item* item = thing->getItem();

		if(item == NULL)
			return;

		if(item->isGroundTile()){
			if(ground == NULL){
				ground = item;
			}
		}
		else if(item->isAlwaysOnTop()){
			bool isInserted = false;
			ItemVector::iterator iit;
			for(iit = topItems.begin(); iit != topItems.end(); ++iit){
				if(Item::items[(*iit)->getID()].alwaysOnTopOrder > Item::items[item->getID()].alwaysOnTopOrder){
					topItems.insert(iit, item);
					isInserted = true;
					break;
				}
			}

			if(!isInserted){
				topItems.push_back(item);
			}
		}
		else{
			downItems.insert(downItems.begin(), item);
		}
	}
}
