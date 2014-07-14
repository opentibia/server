//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
//
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
#include "otpch.h"

#include "beds.h"
#include "scheduler.h"
#include "player.h"
#include "tile.h"
#include "house.h"
#include "ioplayer.h"
#include "game.h"
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
	Beds::instance().setBedSleeper(NULL, sleeperGUID);
}

BedItem* BedItem::getBed()
{
	return this;
}

const BedItem* BedItem::getBed() const
{
	return this;
}

Attr_ReadValue BedItem::readAttr(AttrTypes_t attr, PropStream& propStream)
{
	switch(attr){
		case ATTR_SLEEPERGUID:
		{
			uint32_t _guid;
			if(!propStream.GET_ULONG(_guid)){
				return ATTR_READ_ERROR;
			}

			if(_guid != 0){
				std::string name;
				if(IOPlayer::instance()->getNameByGuid(_guid, name)){
					setSpecialDescription(name + " is sleeping there.");
					Beds::instance().setBedSleeper(this, _guid);
				}
			}

			sleeperGUID = _guid;
			return ATTR_READ_CONTINUE;
		}

		case ATTR_SLEEPSTART:
		{
			uint32_t sleep_start;
			if(!propStream.GET_ULONG(sleep_start)){
				return ATTR_READ_ERROR;
			}
			sleepStart = (time_t)sleep_start;
			return ATTR_READ_CONTINUE;
		}

		default:
			break;
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

bool BedItem::canRemove() const
{
	return (house == NULL);
}

uint32_t BedItem::getSleeper() const
{
	return sleeperGUID;
}

void BedItem::setSleeper(uint32_t guid)
{
	sleeperGUID = guid;
}

time_t BedItem::getSleepStart() const
{
	return sleepStart;
}

void BedItem::setSleepStart(time_t now)
{
	sleepStart = now;
}

House* BedItem::getHouse() const
{
	return house;
}

void BedItem::setHouse(House* h)
{
	house = h;
}

BedItem* BedItem::getNextBedItem()
{
	Direction dir = Item::items[getID()].bedPartnerDirection;
	Position targetPos = getPosition();
	switch(dir.value()){
		case enums::NORTH: targetPos.y--; break;
		case enums::SOUTH: targetPos.y++; break;
		case enums::EAST: targetPos.x++; break;
		case enums::WEST: targetPos.x--; break;
		default: break;
	}

	Tile* tile = g_game.getMap()->getParentTile(targetPos);
	if(tile != NULL) {
		return tile->getBedItem();
	}

	return NULL;
}

bool BedItem::canUse(Player* player)
{
	if(house == NULL || (g_config.getNumber(ConfigManager::PREMIUM_ONLY_BEDS) &&
		!player->isPremium())){
		return false;
	}
	else if(sleeperGUID != 0){
		if(house->getHouseAccessLevel(player) != HOUSE_OWNER){

			Player* sleeper = g_game.getPlayerByGuidEx(sleeperGUID);

			bool result = false;
			// compares house access of the kicker (player) to the sleeper
			// kicker can only kick if he has greater or equal access to the house
			// IE: Guest cannot kick sub-owner, sub-owner can kick guest; sub-owner cannot kick owner, owner can kick sub-owner
			if(house->getHouseAccessLevel(sleeper) <= house->getHouseAccessLevel(player)){
				result = true;
			}

			if(sleeper->isOffline()){
				delete sleeper;
			}

			return result;
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
		g_game.addMagicEffect(this->getPosition(), MAGIC_EFFECT_PUFF);
		wakeUp();
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
		player->getParentTile()->moveCreature(NULL, player, getParentTile());
		g_scheduler.addEvent(createSchedulerTask(SCHEDULER_MINTICKS, boost::bind(&Game::kickPlayer, &g_game, player->getID())));

		// change self and partner's appearance
		updateAppearance(player);
		if(nextBedItem){
			nextBedItem->updateAppearance(player);
		}
	}
}

void BedItem::wakeUp()
{
	if(house == NULL){
		return;
	}

	if(sleeperGUID != 0){
		Player* player = g_game.getPlayerByGuidEx(sleeperGUID);

		if(player){
			regeneratePlayer(player);

			if(player->isOffline()){
				IOPlayer::instance()->savePlayer(player);
				delete player;
				player = NULL;
			}
			else{
				g_game.addCreatureHealth(player);
			}
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
	int32_t sleepDuration = int32_t(std::time(NULL) - sleepStart);

	Condition* condition = player->getCondition(CONDITION_REGENERATION);
	int32_t regenAmount = 0;
	if(condition){
		// regenerate 1 health and 1 mana every 30 seconds that the player had food for
		int32_t regenDuration = std::min(player->getFood(), sleepDuration);
		regenAmount = regenDuration / 30;

		int32_t newTicks = condition->getTicks() - regenDuration * 1000;
		if(newTicks <= 0){
			player->removeCondition(condition);
		}
		else{
			condition->setTicks(newTicks);
		}
	}
	else{
		regenAmount = sleepDuration / 30;
	}

	player->changeHealth(regenAmount);
	player->changeMana(regenAmount);

	// regenerate 1 soul every 15 minutes
	int32_t soulRegen = (int32_t)( ((float)sleepDuration) /60*15);
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
					g_game.transformItem(NULL, this, it.noSleeperID);
				}
			}
		}
		else if(player->isFemale()){
			if(it.femaleSleeperID != 0){
				const ItemType& newType = Item::items[it.femaleSleeperID];
				if(newType.type == ITEM_TYPE_BED){
					g_game.transformItem(NULL, this, it.femaleSleeperID);
				}
			}
		}
		else{
			if(it.maleSleeperID != 0){
				const ItemType& newType = Item::items[it.maleSleeperID];
				if(newType.type == ITEM_TYPE_BED){
					g_game.transformItem(NULL, this, it.maleSleeperID);
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

Beds::Beds()
{

}

Beds::~Beds()
{

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
