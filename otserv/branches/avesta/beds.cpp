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
#include "ioplayer.h" // getNameByGUID :o
#include "game.h"
#include "player.h"

extern Game g_game;


BedItem::BedItem(uint16_t _id) : Item(_id)
{
	house = NULL;
	partner = NULL;
	internalRemoveSleeper();
}

BedItem::~BedItem(){}

bool BedItem::readAttr(AttrTypes_t attr, PropStream& propStream)
{

	switch(attr)
	{
		case ATTR_SLEEPERGUID:
		{
			uint32_t _guid;
			if(!propStream.GET_ULONG(_guid)) {
				return false;
			}

			if(_guid != 0)
			{
				std::string name;
				if(!IOPlayer::instance()->getNameByGuid(_guid, name)) {
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
			if(!propStream.GET_ULONG(sleep_start)) {
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

bool BedItem::serializeAttr(PropWriteStream& propWriteStream)
{
	propWriteStream.ADD_UCHAR(ATTR_SLEEPERGUID);
	propWriteStream.ADD_ULONG(sleeperGUID);
	propWriteStream.ADD_UCHAR(ATTR_SLEEPSTART);
	propWriteStream.ADD_ULONG((int32_t)sleepStart);

	return true;
}

bool BedItem::findPartner()
{
	Direction dir = Item::items[getID()].bedPartnerDir;
	Position targetPos = getPosition();
	switch(dir)
	{
		case NORTH: targetPos.y--; break;
		case SOUTH: targetPos.y++; break;
		case EAST: targetPos.x++; break;
		case WEST: targetPos.x--; break;
		default: break;           // there are no diagonal beds
	}

	Tile* tile = g_game.getMap()->getTile(targetPos);
	if((tile != NULL))
	{
		partner = tile->getBedItem();

		// if partner's partner hasn't already been set, do so
		// I have it find its partner for support for larger beds - you never know,
		// it may happen in the future.
		if((partner != NULL) && (partner->partner == NULL)) {
			partner->findPartner();
		}

		return (partner != NULL);
	}
	return false;
}

bool BedItem::canUse(Player* player)
{
	if((house == NULL) || (sleeperGUID != 0)) {
		return false;
	}

	//Sometimes the partner of the bed could not be found when loading the map
	//This happens because the tile of the first partner was loaded, but the second wasn't
	//Then let's verify if the bed really has the partner.
	if(partner == NULL){
		return findPartner();
	}

	// todo: prem check?
	return true;
}

void BedItem::sleep(Player* player)
{
	// avoid crashes
	if((house == NULL) || (partner == NULL)) {
		return;
	}

	if((player == NULL) || player->isRemoved()) {
		return;
	}

	internalSetSleeper(player);
	partner->internalSetSleeper(player);

	// update the BedSleepersMap
	Beds::instance().setBedSleeper(this, player->getGUID());

	// make the player walk onto the bed
	player->getTile()->moveCreature(player, getTile());

	// kick player after he sees himself walk onto the bed and it change id
	Scheduler::getScheduler().addEvent(createSchedulerTask(50, boost::bind(&Player::kickPlayer, player)));

	// change self and partner's appearance
	updateAppearance(player);
	partner->updateAppearance(player);
}

void BedItem::wakeUp(Player* player)
{
	// avoid crashes
	if((house == NULL) || (partner == NULL)) {
		return;
	}

	if(sleeperGUID != 0)
	{
		// TODO: Clean up
		std::string name;

		// if player == NULL - most likely the house the player is sleeping in was sold
		if((player == NULL))
		{
			bool ret = IOPlayer::instance()->getNameByGuid(sleeperGUID, name);
			if(ret)
			{
				player = new Player(name, NULL);
				ret = IOPlayer::instance()->loadPlayer(player, name);

				if(ret) {
					regeneratePlayer(player);
					IOPlayer::instance()->savePlayer(player);
				}

				player->releaseThing2();
			}
		} else {
			regeneratePlayer(player);
			g_game.addCreatureHealth(player);
		}
	}

	// update the BedSleepersMap
	Beds::instance().setBedSleeper(NULL, sleeperGUID);

	// unset sleep info
	internalRemoveSleeper();
	partner->internalRemoveSleeper();

	// change self and partner's appearance
	updateAppearance(NULL);
	partner->updateAppearance(NULL);
}

void BedItem::regeneratePlayer(Player* player) const
{
	// Note: time_t is in seconds
	int32_t sleptTime = int32_t(std::time(NULL) - sleepStart);

	Condition* condition = player->getCondition(CONDITION_REGENERATION, CONDITIONID_DEFAULT);
	if(condition)
	{
	    // regenerate 1 health and 1 mana every 30 seconds that the player had food for
		int32_t regen;

		if(condition->getTicks() != -1) {
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
		else {
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
	if(player == NULL) {
		g_game.transformItem(this, it.noSleeperID);
	}
	else if(player->getSex() == PLAYERSEX_FEMALE) {
		g_game.transformItem(this, it.femaleSleeperID);
	}
	else {
		g_game.transformItem(this, it.maleSleeperID);
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
	if(it != BedSleepersMap.end()) {
    	return it->second;
	}
	return NULL;
}

void Beds::setBedSleeper(BedItem* bed, uint32_t guid)
{
	BedSleepersMap[guid] = bed;
}
