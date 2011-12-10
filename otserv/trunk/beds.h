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

#ifndef __OTSERV_BEDS_H__
#define __OTSERV_BEDS_H__

#include "definitions.h"
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
	BedItem(const uint16_t& id);
	virtual ~BedItem();

	virtual BedItem* getBed();
	virtual const BedItem* getBed() const;

	//serialization
	virtual Attr_ReadValue readAttr(AttrTypes_t attr, PropStream& propStream);
	virtual bool serializeAttr(PropWriteStream& propWriteStream) const;

	//override
	virtual bool canRemove() const;

	const uint32_t& getSleeper() const;
	void setSleeper(const uint32_t& guid);

	const time_t& getSleepStart() const;
	void setSleepStart(const time_t& now);

	House* getHouse() const;
	void setHouse(House* h);

	bool canUse(Player* player);
	void sleep(Player* player);
	void wakeUp();
	BedItem* getNextBedItem();

protected:
	void updateAppearance(const Player* player);
	void regeneratePlayer(Player* player) const;
	void internalSetSleeper(const Player* player);
	void internalRemoveSleeper();

	uint32_t sleeperGUID;
	time_t sleepStart;
	House* house;
};


class Beds
{
	Beds();

public:
	static Beds& instance();

	BedItem* getBedBySleeper(const uint32_t& guid);
	void setBedSleeper(BedItem* bed, const uint32_t& guid);

protected:
	std::map<uint32_t, BedItem*> BedSleepersMap;
};

#endif // __OTSERV_BEDS_H__
