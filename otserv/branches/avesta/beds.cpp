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

#include <iostream>

extern Game g_game;


BedItem::BedItem(uint16_t _id) : Item(_id)
{
	// set everything to default [no sleeper]
	sleeperGUID = 0;
	sleepStart = 0;
	house = NULL;
	partner = NULL;
	setSpecialDescription("Nobody is sleeping there.");
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

bool BedItem::canUse(Player* player) const
{
	if((house == NULL) || (partner == NULL) || (sleeperGUID != 0)) {
		return false;
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

	// get sleep info
	std::string desc_str = player->getName() + " is sleeping there.";
	time_t now = std::time(NULL);

	// set sleep info
	setSleeper(player->getGUID());
	setSleepStart(now);
	setSpecialDescription(desc_str);
	// and for partner
	partner->setSleeper(player->getGUID());
	partner->setSleepStart(now);
	partner->setSpecialDescription(desc_str);

	// update the BedSleepersMap
	Beds::instance().setBedSleeper(this, player->getGUID());


	// make the player walk onto the bed
	player->getTile()->moveCreature(player, getTile());

	// kick player after he sees himself walk onto the bed and it change id
	Scheduler::getScheduler().addEvent(createSchedulerTask(50, boost::bind(&Player::kickPlayer, player)));

	// change self and partner's appearance
	g_game.transformItem(this, Item::items[getID()].transformToOnUse);
	g_game.transformItem(partner, Item::items[partner->getID()].transformToOnUse);
}

void BedItem::wakeUp(Player* player)
{
	// avoid crashes
	if((house == NULL) || (partner == NULL)) {
		return;
	}

	if(sleeperGUID != 0)
	{
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

				delete player;
			}
		} else {
			regeneratePlayer(player);
			g_game.addCreatureHealth(player);
		}
	}

	// update the BedSleepersMap
	Beds::instance().setBedSleeper(NULL, sleeperGUID);

	// unset sleep info
	setSleeper(0);
	setSleepStart(0);
	setSpecialDescription("Nobody is sleeping there.");
	// and for partner
	partner->setSleeper(0);
	partner->setSleepStart(0);
	partner->setSpecialDescription("Nobody is sleeping there.");

	// change self and partner's appearance
	g_game.transformItem(this, Item::items[getID()].transformToOnUse);
	g_game.transformItem(partner, Item::items[partner->getID()].transformToOnUse);
}

void BedItem::regeneratePlayer(Player* player)
{
	// Note: time_t is in seconds
	uint32_t sleptTime = int32_t(std::time(NULL) - sleepStart);

	Condition* condition = player->getCondition(CONDITION_REGENERATION, CONDITIONID_DEFAULT);
	if(condition)
	{
	    // regenerate 1 health and 1 mana every 30 seconds that the player had food for
		int32_t regen = std::min(uint32_t(condition->getTicks()/1000), sleptTime) / 30;
		player->changeHealth(regen);
		player->changeMana(regen);
		int32_t newRegenTicks = condition->getTicks() - (regen*30000);
		if(newRegenTicks <= 0){
			player->removeCondition(condition);
			condition = NULL;
		}
		else{
			condition->setTicks(newRegenTicks);
		}
	}

	// regenerate 1 soul every 15 minutes
	uint32_t soulRegen = std::max((uint32_t)0, sleptTime/(60*15));
	player->changeSoul(soulRegen);
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
