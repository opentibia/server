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

#ifndef __OTSERV_HOUSE_H__
#define __OTSERV_HOUSE_H__

#include <string>
#include <list>
#include <map>

#include "boost/regex.hpp"

#include "definitions.h"
#include "position.h"
#include "housetile.h"
#include "player.h"

class House;
class BedItem;

class AccessList
{
public:
	AccessList();
	~AccessList();

	bool parseList(const std::string& _list);
	bool addPlayer(std::string& name);
	bool addGuild(const std::string& guildName, const std::string& rank);
	bool addExpression(const std::string& expression);

	bool isInList(const Player* player);

	void getList(std::string& _list) const;

private:
	typedef OTSERV_HASH_SET<uint32_t> PlayerList;
	typedef std::list< std::pair<uint32_t, std::string> > GuildList;

	typedef std::list<std::string> ExpressionList;
	typedef std::list<std::pair<boost::regex, bool> > RegExList;
	std::string list;
	PlayerList playerList;
	GuildList guildList;
	ExpressionList expressionList;
	RegExList regExList;
};

class Door : public Item
{
public:
	Door(uint16_t _type);
	virtual ~Door();

	virtual Door* getDoor() {return this;};
	virtual const Door* getDoor() const {return this;};

	House* getHouse(){return house;};

	//serialization
	virtual Attr_ReadValue readAttr(AttrTypes_t attr, PropStream& propStream);
	virtual bool serializeAttr(PropWriteStream& propWriteStream) const;

	void setDoorId(uint32_t _doorId){ setIntAttr(ATTR_ITEM_DOORID, (uint32_t)_doorId);};
	uint32_t getDoorId() const{ return getIntAttr(ATTR_ITEM_DOORID);};

	bool canUse(const Player* player);

	void setAccessList(const std::string& textlist);
	bool getAccessList(std::string& list) const;

	//overrides
	virtual void onRemoved();
	void copyAttributes(Item* item);

protected:
	void setHouse(House* _house);

private:
	House* house;
	AccessList* accessList;
	friend class House;
};

enum AccessList_t{
	GUEST_LIST = 0x100,
	SUBOWNER_LIST = 0x101
};

enum AccessHouseLevel_t{
	HOUSE_NO_INVITED = 0,
	HOUSE_GUEST = 1,
	HOUSE_SUBOWNER = 2,
	HOUSE_OWNER = 3
};

typedef std::list<HouseTile*> HouseTileList;
typedef std::list<Door*> HouseDoorList;
typedef std::list<BedItem*> HouseBedItemList;

class HouseTransferItem : public Item
{
public:
	static HouseTransferItem* createHouseTransferItem(House* house);

	HouseTransferItem(House* _house) : Item(0) {house = _house;}
	virtual ~HouseTransferItem(){}

	virtual bool onTradeEvent(TradeEvents_t event, Player* owner);

	House* getHouse(){return house;}
	virtual bool canTransform() const {return false;}

protected:
	House* house;
};

class House
{
public:
	enum syncflags_t{
		HOUSE_SYNC_TOWNID		= 0,
		HOUSE_SYNC_NAME			= 1 << 0,
		HOUSE_SYNC_PRICE		= 1 << 1,
		HOUSE_SYNC_RENT			= 1 << 2,
		HOUSE_SYNC_GUILDHALL	= 1 << 3
	};

	House(uint32_t _houseid);
	~House();

	void addTile(HouseTile* tile);

	bool canEditAccessList(uint32_t listId, const Player* player);
	// listId special values:
	//	GUEST_LIST     guest list
	//  SUBOWNER_LIST subowner list
	void setAccessList(uint32_t listId, const std::string& textlist);
	bool getAccessList(uint32_t listId, std::string& list) const;

	bool isInvited(const Player* player);

	AccessHouseLevel_t getHouseAccessLevel(const Player* player);
	bool kickPlayer(Player* player, const std::string& name);

	void setEntryPos(const Position& pos) {posEntry = pos;}
	const Position& getEntryPosition() const {return posEntry;}

	void setName(const std::string& _houseName) {houseName = _houseName;}
	const std::string& getName() const {return houseName;}

	void setHouseOwner(uint32_t guid);
	uint32_t getHouseOwner() const {return houseOwner;}

	void setPaidUntil(time_t paid){paidUntil = paid;}
	time_t getPaidUntil() const {return paidUntil;}

	void setRent(uint32_t _rent){rent = _rent;}
	uint32_t getRent() const {return rent;}

	bool hasSyncFlag(syncflags_t flag) const {return ((syncFlags & (uint32_t)flag) == (uint32_t)flag);}
	void resetSyncFlag(syncflags_t flag) {syncFlags &= ~(uint32_t)flag;}

	void setLastWarning(time_t _lastWarning) {lastWarning = _lastWarning;}
	time_t getLastWarning() {return lastWarning;}

	void setPayRentWarnings(uint32_t warnings) {rentWarnings = warnings;}
	uint32_t getPayRentWarnings() const {return rentWarnings;}

	void setTownId(uint32_t _town){townid = _town;}
	uint32_t getTownId() const {return townid;}

	void setGuildHall(bool _guildHall) {guildHall = _guildHall;}
	bool isGuildHall() const {return guildHall;}

	void setPendingDepotTransfer(bool _pendingDepotTransfer) {pendingDepotTransfer = _pendingDepotTransfer;}
	bool getPendingDepotTransfer() const {return pendingDepotTransfer;}

	uint32_t getHouseId() const {return houseid;}

	void addDoor(Door* door);
	void removeDoor(Door* door);
	void addBed(BedItem* bed);

	Door* getDoorByNumber(uint32_t doorId);
	Door* getDoorByNumber(uint32_t doorId) const;
	Door* getDoorByPosition(const Position& pos);

	HouseTransferItem* getTransferItem();
	void resetTransferItem();
	bool executeTransfer(HouseTransferItem* item, Player* player);

	HouseTileList::iterator getTileBegin() {return houseTiles.begin();}
	HouseTileList::iterator getTileEnd() {return houseTiles.end();}
	size_t getTileCount() {return houseTiles.size();}

	HouseDoorList::iterator getDoorBegin() {return doorList.begin();}
	HouseDoorList::iterator getDoorEnd() {return doorList.end();}
	size_t getDoorCount() {return doorList.size();}

	HouseBedItemList::iterator getBedsBegin() {return bedsList.begin();}
	HouseBedItemList::iterator getBedsEnd() {return bedsList.end();}
	size_t getBedCount() {return bedsList.size();}

	// Transfers all items to depot and clicks all players (useful for map updates, for example)
	void cleanHouse();

private:
	void updateDoorDescription();
	bool transferToDepot();

	bool isLoaded;
	uint32_t houseid;
	uint32_t houseOwner;
	std::string houseOwnerName;
	HouseTileList houseTiles;
	HouseDoorList doorList;
	HouseBedItemList bedsList;
	AccessList guestList;
	AccessList subOwnerList;
	std::string houseName;
	Position posEntry;
	time_t paidUntil;
	uint32_t rentWarnings;
	time_t lastWarning;
	uint32_t rent;
	uint32_t townid;
	bool guildHall;
	uint32_t syncFlags;
	bool pendingDepotTransfer;

	HouseTransferItem* transferItem;
	Container transfer_container;
};

typedef std::map<uint32_t, House*> HouseMap;

enum RentPeriod_t{
	RENTPERIOD_DAILY,
	RENTPERIOD_WEEKLY,
	RENTPERIOD_MONTHLY,
	RENTPERIOD_YEARLY,
	RENTPERIOD_NEVER
};

class Houses
{
	Houses();
	~Houses();

public:
	static Houses& getInstance(){
		static Houses instance;
		return instance;
	}

	House* getHouse(uint32_t houseid, bool add = false)
	{
		HouseMap::iterator it = houseMap.find(houseid);

		if(it != houseMap.end()){
			return it->second;
		}
		if(add){
			House* house = new House(houseid);
			houseMap[houseid] = house;
			return house;
		}
		else{
			return NULL;
		}

	}

	House* getHouseByPlayerId(uint32_t playerId);

	bool loadHousesXML(std::string filename);
	bool payRent(Player* player, House* house, time_t time = 0);
	bool payHouses();
	void getRentPeriodString(std::string& strPeriod);

	HouseMap::iterator getHouseBegin() {return houseMap.begin();}
	HouseMap::iterator getHouseEnd() {return houseMap.end();}

	bool payHouse(House* house, time_t time);

private:
	RentPeriod_t rentPeriod;
	HouseMap houseMap;
	friend class IOMapSerialize;

	friend IOMapSerialize;
};

#endif
