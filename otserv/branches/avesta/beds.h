//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
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

#ifndef __OTS_BEDS_H__
#define __OTS_BEDS_H__

#include "item.h"
#include "position.h"
#include "definitions.h"

#include <ctime>
#include <list>

class House;
class Player;


class BedItem : public Item
{
public:
	BedItem(uint16_t id);
	virtual ~BedItem();

	virtual BedItem* getBed(){ return this; }
	virtual const BedItem* getBed() const { return this; }

	//serialization
	// leave blank -- it's not needed, xml serialization is for containers
	virtual bool unserialize(xmlNodePtr p){ return true; }
	// same as above.
	virtual xmlNodePtr serialize(){ return xmlNewNode(NULL,(const xmlChar*)"item"); }

    virtual bool readAttr(AttrTypes_t attr, PropStream& propStream);
	virtual bool serializeAttr(PropWriteStream& propWriteStream);

	//override
	virtual bool canRemove() const {return (house == NULL); }


	// mutator / accessor for sleeperGUID
	uint32_t getSleeper() const { return sleeperGUID; }
	void setSleeper(uint32_t guid){ sleeperGUID = guid; }

	// mutator / accessor for sleepStart
	time_t getSleepStart() const { return sleepStart; }
	void setSleepStart(time_t now){ sleepStart = now; }

	// mutator / accessor for house
	House* getHouse() const { return house; }
	void setHouse(House* h){ house = h; }

	// can a player even use the bed? :o
	bool canUse(Player* player);

	// player is sleeping in the bed
	void sleep(Player* player);
	// player is waking up or being kicked because the house is sold.
	void wakeUp(Player* player);

	// find the partner bed item
	bool findPartner();

	// should I make a partner accessor/mutator?

protected:
	// change itemid when a player sleeps in this bed
	void updateAppearance(const Player* player);
	// regenerate player
	void regeneratePlayer(Player* player) const;
	// set all sleeper information
	void internalSetSleeper(const Player* player);
	// remove all sleeper information
	void internalRemoveSleeper();

	// GUID of the player sleeping in bed
	uint32_t sleeperGUID;
	// when he fell asleep
	time_t sleepStart;
	// House the bed belongs to
	House* house;
	// Partner Item
	BedItem* partner;
};


class Beds
{
public:
	~Beds(){}

	static Beds& instance();

	BedItem* getBedBySleeper(uint32_t guid);
	void setBedSleeper(BedItem* bed, uint32_t guid);

protected:
	Beds(){ BedSleepersMap.clear(); }

	std::map<uint32_t, BedItem*> BedSleepersMap;
};

#endif
