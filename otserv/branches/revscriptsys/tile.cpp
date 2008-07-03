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
#include "otpch.h"

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

bool Tile::hasProperty(Item* exclude, enum ITEMPROPERTY prop) const
{
	assert(exclude);
	if(ground && exclude != ground && ground->hasProperty(prop)){
		return true;
	}

	ItemVector::const_iterator iit;
	for(iit = topItems.begin(); iit != topItems.end(); ++iit){
		Item* item = *iit;
		if(item != exclude && item->hasProperty(prop))
			return true;
	}

	for(iit = downItems.begin(); iit != downItems.end(); ++iit){
		Item* item = *iit;
		if(item != exclude && item->hasProperty(prop))
			return true;
	}

	return false;
}

bool Tile::hasHeight(uint32_t n) const
{
	uint32_t height = 0;
	Item* iiItem = NULL;
	for(uint32_t i = 0; i < getThingCount(); ++i){
		iiItem = __getThing(i)->getItem();

		if(iiItem && iiItem->hasProperty(HASHEIGHT))
			++height;

		if(n == height){
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
	if(!hasFlag(TILESTATE_MAGICFIELD)){
		return NULL;
	}

	MagicField* field = NULL;
	for(ItemVector::const_iterator iit = downItems.begin(); iit != downItems.end(); ++iit){
		field = (*iit)->getMagicField();
		if(field)
			return field;
	}

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
//[ added for beds system
BedItem* Tile::getBedItem() const
{
	BedItem* bed = NULL;
	Item* iiItem = NULL;
	for(uint32_t i = 0; i < getThingCount(); ++i){
		iiItem = __getThing(i)->getItem();
		if(iiItem && (bed = iiItem->getBed()))
			return bed;
	}

	return NULL;
}
//]

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

void Tile::onUpdateTileItem(uint32_t index, Item* oldItem,
		const ItemType& oldType, Item* newItem, const ItemType& newType)
{
	updateTileFlags(oldItem, true);
	updateTileFlags(newItem, false);

	const Position& cylinderMapPos = getPosition();

	const SpectatorVec& list = g_game.getSpectators(cylinderMapPos);
	SpectatorVec::const_iterator it;

	//send to client
	Player* tmpPlayer = NULL;
	for(it = list.begin(); it != list.end(); ++it){
		if((tmpPlayer = (*it)->getPlayer())){
			tmpPlayer->sendUpdateTileItem(this, cylinderMapPos, index, oldItem, newItem);
		}
	}

	//event methods
	for(it = list.begin(); it != list.end(); ++it){
		(*it)->onUpdateTileItem(this, cylinderMapPos, index, oldItem, oldType, newItem, newType);
	}
}

void Tile::onRemoveTileItem(uint32_t index, Item* item)
{
	updateTileFlags(item, true);

	const Position& cylinderMapPos = getPosition();
	const ItemType& iType = Item::items[item->getID()];

	const SpectatorVec& list = g_game.getSpectators(cylinderMapPos);
	SpectatorVec::const_iterator it;

	//send to client
	Player* tmpPlayer = NULL;
	for(it = list.begin(); it != list.end(); ++it){
		if((tmpPlayer = (*it)->getPlayer())){
			tmpPlayer->sendRemoveTileItem(this, cylinderMapPos, index, item);
		}
	}

	//event methods
	for(it = list.begin(); it != list.end(); ++it){
		(*it)->onRemoveTileItem(this, cylinderMapPos, index, iType, item);
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

void Tile::moveCreature(Creature* creature, Cylinder* toCylinder, bool teleport /* = false*/)
{
	int32_t oldStackPos = __getIndexOfThing(creature);

	//remove the creature
	__removeThing(creature, 0);

	//add the creature
	Tile* toTile = dynamic_cast<Tile*>(toCylinder);
	toTile->__addThing(creature);
	int32_t newStackPos = toTile->__getIndexOfThing(creature);

	Position fromPos = getPosition();
	Position toPos = toTile->getPosition();

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
	g_game.getSpectators(list, fromPos, false, true);
	g_game.getSpectators(list, toPos, true, true);

	//send to client
	Player* tmpPlayer = NULL;
	for(it = list.begin(); it != list.end(); ++it) {
		if((tmpPlayer = (*it)->getPlayer())){
			tmpPlayer->sendCreatureMove(creature, toTile, toPos, this, fromPos, oldStackPos, teleport);
		}
	}

	//event method
	for(it = list.begin(); it != list.end(); ++it) {
		(*it)->onCreatureMove(creature, toTile, toPos, this, fromPos, oldStackPos, teleport);
	}

	postRemoveNotification(creature, oldStackPos, true);
	toTile->postAddNotification(creature, newStackPos);
}

ReturnValue Tile::__queryAdd(int32_t index, const Thing* thing, uint32_t count,
	uint32_t flags) const
{
	Thing* iithing = NULL;

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

		if(const Monster* monster = creature->getMonster()){
			if(hasFlag(TILESTATE_PROTECTIONZONE))
				return RET_NOTPOSSIBLE;

			if(floorChange() || positionChange()){
				return RET_NOTPOSSIBLE;
			}

			if(monster->canPushCreatures() && !monster->isSummon()){
				Creature* creature;
				for(uint32_t i = 0; i < creatures.size(); ++i){
					creature = creatures[i];
					if( !creature->getMonster() ||
						!creature->isPushable() ||
						(creature->getMonster()->isSummon() && creature->getMonster()->getMaster()->getPlayer()))
					{
						return RET_NOTPOSSIBLE;
					}
				}
			}
			else if(!creatures.empty()){
				return RET_NOTENOUGHROOM; //RET_NOTPOSSIBLE
			}

			if(hasFlag(TILESTATE_IMMOVABLEBLOCKSOLID)){
				return RET_NOTPOSSIBLE;
			}

			if(hasBitSet(FLAG_PATHFINDING, flags) && hasFlag(TILESTATE_IMMOVABLENOFIELDBLOCKPATH)){
				return RET_NOTPOSSIBLE;
			}

			if(hasFlag(TILESTATE_BLOCKSOLID) || (hasBitSet(FLAG_PATHFINDING, flags) && hasFlag(TILESTATE_NOFIELDBLOCKPATH))){
				if(!(monster->canPushItems() || hasBitSet(FLAG_IGNOREBLOCKITEM, flags) ) ){
					return RET_NOTPOSSIBLE;
				}
			}

			if(hasFlag(TILESTATE_MAGICFIELD)){
				MagicField* field = getFieldItem();
				if(!field->isBlocking()){
					CombatType_t combatType = field->getCombatType();
					//There is 3 options for a monster to enter a magic field
					//1) Monster is immune
					if(!monster->isImmune(combatType)){
						//1) Monster is "strong" enough to handle the damage
						//2) Monster is already afflicated by this type of condition
						if(hasBitSet(FLAG_IGNOREFIELDDAMAGE, flags)){
							if( !(monster->canPushItems() ||
								monster->hasCondition(Combat::DamageToConditionType(combatType))) ){
								return RET_NOTPOSSIBLE;
							}
						}
						else{
							return RET_NOTPOSSIBLE;
						}
					}
				}
			}

			/*
			for(uint32_t i = 0; i < getThingCount(); ++i){
				iithing = __getThing(i);
				if(const Item* iitem = iithing->getItem()){
					const ItemType& iiType = Item::items[iitem->getID()];
					if(iiType.isMagicField() && !iiType.blockSolid){
						CombatType_t combatType = iitem->getMagicField()->getCombatType();
						//There is 3 options for a monster to enter a magic field
						//1) Monster is immune
						if(!monster->isImmune(combatType)){
							//1) Monster is "strong" enough to handle the damage
							//2) Monster is already afflicated by this type of condition
							if(hasBitSet(FLAG_IGNOREFIELDDAMAGE, flags)){
								if( !(monster->canPushItems() ||
									monster->hasCondition(Combat::DamageToConditionType(combatType))) ){
									if(ret != RET_NOTPOSSIBLE){
										this->__queryAdd(index, thing, count, flags);
										std::cout << "missmatch" << std::endl;
									}
									return RET_NOTPOSSIBLE;
								}
							}
							else{
								if(ret != RET_NOTPOSSIBLE){
									this->__queryAdd(index, thing, count, flags);
									std::cout << "missmatch" << std::endl;
								}
								return RET_NOTPOSSIBLE;
							}
						}
					}
					else if(iiType.blockSolid || (hasBitSet(FLAG_PATHFINDING, flags) && iiType.blockPathFind) ){
						if(!iiType.moveable || iitem->getUniqueId() != 0){
							//its not moveable
							if(ret != RET_NOTPOSSIBLE){
								this->__queryAdd(index, thing, count, flags);
								std::cout << "missmatch" << std::endl;
							}
							return RET_NOTPOSSIBLE;
						}
						//moveable
						else if(!(hasBitSet(FLAG_IGNOREBLOCKITEM, flags) || monster->canPushItems()) ){
							assert(ret == RET_NOTPOSSIBLE);
							if(ret != RET_NOTPOSSIBLE){
								this->__queryAdd(index, thing, count, flags);
								std::cout << "missmatch" << std::endl;
							}
							return RET_NOTPOSSIBLE;
						}
					}
				}
			}

			if(ret != RET_NOERROR){
				this->__queryAdd(index, thing, count, flags);
				std::cout << "missmatch RET_NOERROR" << std::endl;
			}
			*/
			return RET_NOERROR;
		}
		else if(const Player* player = creature->getPlayer()){
			if(!creatures.empty() && !hasBitSet(FLAG_IGNOREBLOCKCREATURE, flags)){
				return RET_NOTENOUGHROOM;
			}

			if(player->getParent() == NULL && hasFlag(TILESTATE_NOLOGOUT)){
				//player is trying to login to a "no logout" tile
				return RET_NOTPOSSIBLE;
			}

			if(player->isPzLocked() && !player->getTile()->hasFlag(TILESTATE_PVPZONE) && hasFlag(TILESTATE_PVPZONE)){
				//player is trying to enter a pvp zone while being pz-locked
				return RET_PLAYERISPZLOCKEDENTERPVPZONE;
			}

			if(player->isPzLocked() && player->getTile()->hasFlag(TILESTATE_PVPZONE) && !hasFlag(TILESTATE_PVPZONE)){
				//player is trying to leave a pvp zone while being pz-locked
				return RET_PLAYERISPZLOCKEDLEAVEPVPZONE;
			}

			if(hasFlag(TILESTATE_NOPVPZONE) && player->isPzLocked()){
				return RET_PLAYERISPZLOCKED;
			}

			if(hasFlag(TILESTATE_PROTECTIONZONE) && player->isPzLocked()){
				return RET_PLAYERISPZLOCKED;
			}
		}
		else{
			if(!creatures.empty() && !hasBitSet(FLAG_IGNOREBLOCKCREATURE, flags)){
				return RET_NOTENOUGHROOM; //RET_NOTPOSSIBLE
			}
		}

		for(uint32_t i = 0; i < getThingCount(); ++i){
			iithing = __getThing(i);
			if(const Item* iitem = iithing->getItem()){
				const ItemType& iiType = Item::items[iitem->getID()];
				if(iiType.blockSolid){
					if(hasBitSet(FLAG_IGNOREBLOCKITEM, flags)){
						if(!iiType.moveable || iitem->getUniqueId() != 0)
							return RET_NOTPOSSIBLE;
					}
					else
						return RET_NOTENOUGHROOM;
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

		if(hasBitSet(FLAG_NOLIMIT, flags)){
			return RET_NOERROR;
		}

		bool itemIsHangable = item->isHangable();

		if(ground == NULL && !itemIsHangable){
			return RET_NOTPOSSIBLE;
		}

		if(!creatures.empty() && item->isBlocking() && !hasBitSet(FLAG_IGNOREBLOCKCREATURE, flags)){
			return RET_NOTENOUGHROOM;
		}

		bool hasHangable = false;
		bool supportHangable = false;
		for(uint32_t i = 0; i < getThingCount(); ++i){
			iithing = __getThing(i);
			if(const Item* iitem = iithing->getItem()){
				const ItemType& iiType = Item::items[iitem->getID()];

				if(iiType.isHangable){
					hasHangable = true;
				}

				if(iiType.isHorizontal || iiType.isVertical){
					supportHangable = true;
				}
				
				if(itemIsHangable && (iiType.isHorizontal || iiType.isVertical)){
					//
				}
				else if(iiType.blockSolid){
					if(item->isPickupable()){
						if(iitem->getTrashHolder()){
							continue;
						}

						if(!iiType.hasHeight || iiType.pickupable){
							return RET_NOTENOUGHROOM;
						}
					}
					else{
						return RET_NOTENOUGHROOM;
					}
				}
			}
		}

		if(itemIsHangable && hasHangable && supportHangable){
			return RET_NEEDEXCHANGE;
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
		g_game.clearSpectatorCache();
		creature->setParent(this);
		creatures.insert(creatures.begin(), creature);
		++thingCount;
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
				ground = item;
				++thingCount;
				onAddTileItem(item);
			}
			else{
				int32_t index = __getIndexOfThing(ground);
				const ItemType& oldType = Item::items[ground->getID()];
				const ItemType& newType = Item::items[item->getID()];

				Item* oldGround = ground;
				ground->setParent(NULL);
				g_game.FreeThing(ground);
				ground = item;
				onUpdateTileItem(index, oldGround, oldType, item, newType);
			}
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
					++thingCount;
					isInserted = true;
					break;
				}
			}

			if(!isInserted){
				topItems.push_back(item);
				++thingCount;
			}

			onAddTileItem(item);
		}
		else{
			if(item->isMagicField()){
				//remove old field item if exists
				MagicField* oldField = NULL;
				ItemVector::iterator iit;
				for(iit = downItems.begin(); iit != downItems.end(); ++iit){
					if((oldField = (*iit)->getMagicField())){
						if(oldField->isReplaceable()){
							__removeThing(oldField, 1);

							oldField->setParent(NULL);
							g_game.FreeThing(oldField);
							break;
						}
						else{
							//This magic field cannot be replaced.
							item->setParent(NULL);
							g_game.FreeThing(item);
							return;
						}
					}
				}
			}

			downItems.insert(downItems.begin(), item);
			++thingCount;
			onAddTileItem(item);
		}
	}
}

void Tile::__updateThing(Thing* thing, uint16_t itemId, uint32_t count)
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

	item->setID(itemId);
	item->setItemCountOrSubtype(count);
	onUpdateTileItem(index, item, oldType, item, newType);
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
	bool isInserted = false;

	if(!isInserted && ground){
		if(pos == 0){
			oldItem = ground;
			ground = item;
			isInserted = true;
		}

		--pos;
	}

	if(!isInserted && pos < (int32_t)topItems.size()){
		ItemVector::iterator it = topItems.begin();
		it += pos;
		pos = 0;

		oldItem = (*it);
		it = topItems.erase(it);
		topItems.insert(it, item);
		isInserted = true;
	}

	pos -= (uint32_t)topItems.size();

	if(!isInserted && pos < (int32_t)creatures.size()){
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Tile::__updateThing] Update object is a creature" << std::endl;
		DEBUG_REPORT
#endif
		return /*RET_NOTPOSSIBLE*/;
	}

	pos -= (uint32_t)creatures.size();

	if(!isInserted && pos < (int32_t)downItems.size()){
		ItemVector::iterator it = downItems.begin();
		it += pos;
		pos = 0;

		oldItem = (*it);
		it = downItems.erase(it);
		downItems.insert(it, item);
		isInserted = true;
	}

	if(isInserted){
		item->setParent(this);

		const ItemType& oldType = Item::items[oldItem->getID()];
		const ItemType& newType = Item::items[item->getID()];
		onUpdateTileItem(index, oldItem, oldType, item, newType);

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

		g_game.clearSpectatorCache();
		creatures.erase(it);
		--thingCount;
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

		int32_t index = __getIndexOfThing(item);
		if(index == -1){
#ifdef __DEBUG__MOVESYS__
			std::cout << "Failure: [Tile::__removeThing] index == -1" << std::endl;
			DEBUG_REPORT
#endif
			return /*RET_NOTPOSSIBLE*/;
		}

		if(item == ground){
			ground->setParent(NULL);
			ground = NULL;
			--thingCount;
			onRemoveTileItem(index, item);

			return /*RET_NOERROR*/;
		}

		ItemVector::iterator iit;
		if(item->isAlwaysOnTop()){
			for(iit = topItems.begin(); iit != topItems.end(); ++iit){
				if(*iit == item){
					(*iit)->setParent(NULL);
					topItems.erase(iit);
					--thingCount;
					onRemoveTileItem(index, item);
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

						const ItemType& it = Item::items[item->getID()];
						onUpdateTileItem(index, item, it, item, it);
					}
					else{
						(*iit)->setParent(NULL);
						downItems.erase(iit);
						--thingCount;
						onRemoveTileItem(index, item);
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

uint32_t Tile::__getItemTypeCount(uint16_t itemId, int32_t subType /*= -1*/, bool itemCount /*= true*/) const
{
	uint32_t count = 0;
	Thing* thing = NULL;
	for(uint32_t i = 0; i < getThingCount(); ++i){
		thing = __getThing(i);

		if(const Item* item = thing->getItem()){
			if(item->getID() == itemId && (subType == -1 || subType == item->getSubType())){

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

void Tile::postAddNotification(Thing* thing, int32_t index, cylinderlink_t link /*= LINK_OWNER*/)
{
	const Position& cylinderMapPos = getPosition();

	const SpectatorVec& list = g_game.getSpectators(cylinderMapPos);
	SpectatorVec::const_iterator it;

	Player* tmpPlayer = NULL;
	for(it = list.begin(); it != list.end(); ++it){
		if((tmpPlayer = (*it)->getPlayer())){
			tmpPlayer->postAddNotification(thing, index, LINK_NEAR);
		}
	}

	//add a reference to this item, it may be deleted after being added (mailbox for example)
	thing->useThing2();

	bool removal = false;

	if(link == LINK_OWNER){
		//calling movement scripts
		Creature* creature = thing->getCreature();
		if(creature){
			// REVSCRIPT TODO Event callback
			//g_moveEvents->onCreatureMove(creature, this, true);
		}
		else{
			Item* item = thing->getItem();
			if(item){
				// REVSCRIPT TODO Event callback
				//g_moveEvents->onItemMove(item, this, true);
			}
		}

		if(Teleport* teleport = getTeleportItem()){
			teleport->__addThing(thing);
		}
		else if(TrashHolder* trashHolder = getTrashHolder()){
			trashHolder->__addThing(thing);
			removal = thing != trashHolder;
		}
		else if(Mailbox* mailbox = getMailbox()){
			mailbox->__addThing(thing);
		}
	}

	//update floor change flags
	Item* item = thing->getItem();
	if(item){
		updateTileFlags(item, removal);
	}

	//release the reference to this item onces we are finished
	g_game.FreeThing(thing);
}

void Tile::postRemoveNotification(Thing* thing, int32_t index, bool isCompleteRemoval, cylinderlink_t link /*= LINK_OWNER*/)
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
			tmpPlayer->postRemoveNotification(thing, index, isCompleteRemoval, LINK_NEAR);
		}
	}

	//calling movement scripts
	Creature* creature = thing->getCreature();
	if(creature){
		// REVSCRIPT TODO Event callback
		//g_moveEvents->onCreatureMove(creature, this, false);
	}
	else{
		Item* item = thing->getItem();
		if(item){
			// REVSCRIPT TODO Event callback
			//g_moveEvents->onItemMove(item, this, false);
		}
	}

	//update floor change flags
	Item* item = thing->getItem();
	if(item){
		updateTileFlags(item, true);
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
		creatures.insert(creatures.begin(), creature);
		++thingCount;
	}
	else{
		Item* item = thing->getItem();

		if(item == NULL)
			return;

		if(item->isGroundTile()){
			if(ground == NULL){
				ground = item;
				++thingCount;
			}
		}
		else if(item->isAlwaysOnTop()){
			bool isInserted = false;
			ItemVector::iterator iit;
			for(iit = topItems.begin(); iit != topItems.end(); ++iit){
				if(Item::items[(*iit)->getID()].alwaysOnTopOrder > Item::items[item->getID()].alwaysOnTopOrder){
					topItems.insert(iit, item);
					++thingCount;
					isInserted = true;
					break;
				}
			}

			if(!isInserted){
				topItems.push_back(item);
				++thingCount;
			}
		}
		else{
			if(!downItems.empty()){
				downItems.insert(downItems.begin(), item);
			}
			else{
				downItems.push_back(item);
			}
			++thingCount;
		}

		//update floor change flags
		updateTileFlags(item, false);
	}
}

void Tile::updateTileFlags(Item* item, bool removing)
{
	if(!removing){
		//!removing is adding an item to the tile
		if(!hasFlag(TILESTATE_FLOORCHANGE)){
			if(item->floorChangeDown()){
				setFlag(TILESTATE_FLOORCHANGE);
				setFlag(TILESTATE_FLOORCHANGE_DOWN);
			}
			if(item->floorChangeNorth()){
				setFlag(TILESTATE_FLOORCHANGE);
				setFlag(TILESTATE_FLOORCHANGE_NORTH);
			}
			if(item->floorChangeSouth()){
				setFlag(TILESTATE_FLOORCHANGE);
				setFlag(TILESTATE_FLOORCHANGE_SOUTH);
			}
			if(item->floorChangeEast()){
				setFlag(TILESTATE_FLOORCHANGE);
				setFlag(TILESTATE_FLOORCHANGE_EAST);
			}
			if(item->floorChangeWest()){
				setFlag(TILESTATE_FLOORCHANGE);
				setFlag(TILESTATE_FLOORCHANGE_WEST);
			}
		}

		if(item->getTeleport()){
			setFlag(TILESTATE_POSITIONCHANGE);
		}
		if(item->getMagicField()){
			setFlag(TILESTATE_MAGICFIELD);
		}

		if(item->hasProperty(BLOCKSOLID)){
			setFlag(TILESTATE_BLOCKSOLID);
		}
		if(item->hasProperty(IMMOVABLEBLOCKSOLID)){
			setFlag(TILESTATE_IMMOVABLEBLOCKSOLID);
		}
		if(item->hasProperty(BLOCKPATH)){
			setFlag(TILESTATE_BLOCKPATH);
		}
		if(item->hasProperty(NOFIELDBLOCKPATH)){
			setFlag(TILESTATE_NOFIELDBLOCKPATH);
		}
		if(item->hasProperty(IMMOVABLENOFIELDBLOCKPATH)){
			setFlag(TILESTATE_IMMOVABLENOFIELDBLOCKPATH);
		}
	}
	else{
		if(item->floorChangeDown()){
			resetFlag(TILESTATE_FLOORCHANGE);
			resetFlag(TILESTATE_FLOORCHANGE_DOWN);
		}
		if(item->floorChangeNorth()){
			resetFlag(TILESTATE_FLOORCHANGE);
			resetFlag(TILESTATE_FLOORCHANGE_NORTH);
		}
		if(item->floorChangeSouth()){
			resetFlag(TILESTATE_FLOORCHANGE);
			resetFlag(TILESTATE_FLOORCHANGE_SOUTH);
		}
		if(item->floorChangeEast()){
			resetFlag(TILESTATE_FLOORCHANGE);
			resetFlag(TILESTATE_FLOORCHANGE_EAST);
		}
		if(item->floorChangeWest()){
			resetFlag(TILESTATE_FLOORCHANGE);
			resetFlag(TILESTATE_FLOORCHANGE_WEST);
		}
		if(item->getTeleport()){
			resetFlag(TILESTATE_POSITIONCHANGE);
		}
		if(item->getMagicField()){
			// If transformItem is called on a field, this might not be true
			//if(getFieldItem() == item) {
				resetFlag(TILESTATE_MAGICFIELD);
			//}
		}

		if(item->hasProperty(BLOCKSOLID) && !hasProperty(item, BLOCKSOLID)){
			resetFlag(TILESTATE_BLOCKSOLID);
		}
		if(item->hasProperty(IMMOVABLEBLOCKSOLID) && !hasProperty(item, IMMOVABLEBLOCKSOLID)){
			resetFlag(TILESTATE_IMMOVABLEBLOCKSOLID);
		}
		if(item->hasProperty(BLOCKPATH) && !hasProperty(item, BLOCKPATH)){
			resetFlag(TILESTATE_BLOCKPATH);
		}
		if(item->hasProperty(NOFIELDBLOCKPATH) && !hasProperty(item, NOFIELDBLOCKPATH)){
			resetFlag(TILESTATE_NOFIELDBLOCKPATH);
		}
		if(item->hasProperty(IMMOVABLEBLOCKPATH) && !hasProperty(item, IMMOVABLEBLOCKPATH)){
			resetFlag(TILESTATE_IMMOVABLEBLOCKPATH);
		}
		if(item->hasProperty(IMMOVABLENOFIELDBLOCKPATH) && !hasProperty(item, IMMOVABLENOFIELDBLOCKPATH)){
			resetFlag(TILESTATE_IMMOVABLENOFIELDBLOCKPATH);
		}
	}
}
