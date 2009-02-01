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

#include "beds.h"
#include "house.h"
#include "ioplayer.h"
#include "game.h"
#include "player.h"
#include "configmanager.h"

extern Game g_game;
extern ConfigManager g_config;


BedItem::BedItem(uint16_t _id) : Item(_id)
{
	house = NULL;
	internalRemoveSleeper();
}

BedItem::~BedItem()
{
	//
}

bool BedItem::readAttr(AttrTypes_t attr, PropStream& propStream)
{
	switch(attr){
		case ATTR_SLEEPERGUID:
		{
			uint32_t _guid;
			if(!propStream.GET_ULONG(_guid)){
				return false;
			}

			if(_guid != 0)
			{
				std::string name;
				if(!IOPlayer::instance()->getNameByGuid(_guid, name)){
					return false;
				}

				setSpecialDescription(name + " is sleeping there.");

				// update the BedSleepersMap
				Beds::instance().setBedSleeper(this, _guid);
			}
			sleeperGUID = _guid;

			return true;
		}
		case ATTR_SLEEPSTART:
		{
			uint32_t sleep_start;
			if(!propStream.GET_ULONG(sleep_start)){
				return false;
			}
			sleepStart = (time_t)sleep_start;
			return true;
		}
		default:
		{
			break;
		}
	}

	return Item::readAttr(attr, propStream);
}

bool BedItem::serializeAttr(PropWriteStream& propWriteStream) const
{
	propWriteStream.ADD_UCHAR(ATTR_SLEEPERGUID);
	propWriteStream.ADD_ULONG(sleeperGUID);
	propWriteStream.ADD_UCHAR(ATTR_SLEEPSTART);
	propWriteStream.ADD_ULONG((int32_t)sleepStart);

	return true;
}

BedItem* BedItem::getNextBedItem()
{
	Direction dir = Item::items[getID()].bedPartnerDir;
	Position targetPos = getPosition();
	switch(dir){
		case NORTH: targetPos.y--; break;
		case SOUTH: targetPos.y++; break;
		case EAST: targetPos.x++; break;
		case WEST: targetPos.x--; break;
		default: break;
	}

	Tile* tile = g_game.getMap()->getTile(targetPos);
	if(tile != NULL) {
		return tile->getBedItem();
	}

	return NULL;
}

bool BedItem::canUse(Player* player)
{
	if(house == NULL || (g_config.getNumber(ConfigManager::PREMIUM_ONLY_BEDS) != 0 &&
		!player->isPremium())){
		return false;
	}
	else if(sleeperGUID != 0){
		if(house->getHouseAccessLevel(player) != HOUSE_OWNER){
			std::string name;

			if(IOPlayer::instance()->getNameByGuid(sleeperGUID, name)){
				Player* sleeper = new Player(name, NULL);

				if(IOPlayer::instance()->loadPlayer(sleeper, name)){
					// compares house access of the kicker (player) to the sleeper
					// kicker can only kick if he has greater or equal access to the house
					// IE: Guest cannot kick sub-owner, sub-owner can kick guest; sub-owner cannot kick owner, owner can kick sub-owner
					if(house->getHouseAccessLevel(sleeper) <= house->getHouseAccessLevel(player)){
						delete sleeper;
						sleeper = NULL;
						return isBed();
					}
				}

				delete sleeper;
				sleeper = NULL;
			}
			return false;
		}
	}

	return isBed();
}

void BedItem::sleep(Player* player)
{
	if((house == NULL) || (player == NULL) || player->isRemoved()){
		return;
	}

	if(sleeperGUID != 0){
		g_game.addMagicEffect(this->getPosition(), NM_ME_PUFF);
		wakeUp(g_game.getPlayerByID(sleeperGUID));
	}
	else{
		internalSetSleeper(player);

		BedItem* nextBedItem = getNextBedItem();
		if(nextBedItem){
			nextBedItem->internalSetSleeper(player);
		}

		// update the BedSleepersMap
		Beds::instance().setBedSleeper(this, player->getGUID());

		// make the player walk onto the bed and kick him
		player->getTile()->moveCreature(player, getTile());
		Scheduler::getScheduler().addEvent(createSchedulerTask(SCHEDULER_MINTICKS, boost::bind(&Game::kickPlayer, &g_game, player->getID())));

		// change self and partner's appearance
		updateAppearance(player);
		if(nextBedItem){
			nextBedItem->updateAppearance(player);
		}
	}
}

void BedItem::wakeUp(Player* player)
{
	if(house == NULL){
		return;
	}

	if(sleeperGUID != 0){
		if((player == NULL)){
			std::string name;
			if(IOPlayer::instance()->getNameByGuid(sleeperGUID, name)){
				Player* _player = new Player(name, NULL);

				if(IOPlayer::instance()->loadPlayer(_player, name)){
					regeneratePlayer(_player);
					IOPlayer::instance()->savePlayer(_player);
				}

				delete _player;
				_player = NULL;
			}
		}
		else{
			regeneratePlayer(player);
			g_game.addCreatureHealth(player);
		}
	}

	// update the BedSleepersMap
	Beds::instance().setBedSleeper(NULL, sleeperGUID);

	internalRemoveSleeper();

	BedItem* nextBedItem = getNextBedItem();
	if(nextBedItem){
		nextBedItem->internalRemoveSleeper();
	}

	// change self and partner's appearance
	updateAppearance(NULL);
	if(nextBedItem){
		nextBedItem->updateAppearance(NULL);
	}
}

void BedItem::regeneratePlayer(Player* player) const
{
	// Note: time_t is in seconds
	int32_t sleptTime = int32_t(std::time(NULL) - sleepStart);

	Condition* condition = player->getCondition(CONDITION_REGENERATION, CONDITIONID_DEFAULT, 0);
	if(condition){
	    // regenerate 1 health and 1 mana every 30 seconds that the player had food for
		int32_t regen;

		if(condition->getTicks() != -1){
			regen = std::min((condition->getTicks()/1000), sleptTime) / 30;
			int32_t newRegenTicks = condition->getTicks() - (regen*30000);
			if(newRegenTicks <= 0){
				player->removeCondition(condition);
				condition = NULL;
			}
			else{
				condition->setTicks(newRegenTicks);
			}
		}
		else{
			regen = sleptTime / 30;
		}

		player->changeHealth(regen);
		player->changeMana(regen);
	}

	// regenerate 1 soul every 15 minutes
	int32_t soulRegen = (int32_t)std::max((float)0, ((float)sleptTime/(60*15)));
	player->changeSoul(soulRegen);
}

void BedItem::updateAppearance(const Player* player)
{
	const ItemType& it = Item::items[getID()];
	if(it.type == ITEM_TYPE_BED){
		if(player == NULL) {
			if(it.noSleeperID != 0){
				const ItemType& newType = Item::items[it.noSleeperID];
				if(newType.type == ITEM_TYPE_BED){
					g_game.transformItem(this, it.noSleeperID);
				}
			}
		}
		else if(player->getSex() == PLAYERSEX_FEMALE) {
			if(it.femaleSleeperID != 0){
				const ItemType& newType = Item::items[it.femaleSleeperID];
				if(newType.type == ITEM_TYPE_BED){
					g_game.transformItem(this, it.femaleSleeperID);
				}
			}
		}
		else{
			if(it.maleSleeperID != 0){
				const ItemType& newType = Item::items[it.maleSleeperID];
				if(newType.type == ITEM_TYPE_BED){
					g_game.transformItem(this, it.maleSleeperID);
				}
			}
		}
	}
}

void BedItem::internalSetSleeper(const Player* player)
{
	std::string desc_str = player->getName() + " is sleeping there.";

	setSleeper(player->getGUID());
	setSleepStart(std::time(NULL));
	setSpecialDescription(desc_str);
}

void BedItem::internalRemoveSleeper()
{
	setSleeper(0);
	setSleepStart(0);
	setSpecialDescription("Nobody is sleeping there.");
}

Beds& Beds::instance()
{
	static Beds instance;
	return instance;
}

BedItem* Beds::getBedBySleeper(uint32_t guid)
{
	std::map<uint32_t, BedItem*>::iterator it = BedSleepersMap.find(guid);
	if(it != BedSleepersMap.end()){
    	return it->second;
	}
	return NULL;
}

void Beds::setBedSleeper(BedItem* bed, uint32_t guid)
{
	BedSleepersMap[guid] = bed;
}
