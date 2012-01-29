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

#include "definitions.h"
#include "position.h"
#include "housetile.h"
#include "player.h"
#include <boost/regex.hpp>
#include <string>
#include <list>
#include <map>

class AccessList
{
public:
	AccessList();
	~AccessList();

	bool parseList(const std::string& _list);
	bool addPlayer(const std::string& name);
	bool addGuild(const std::string& guildName, const std::string& rank);
	bool addExpression(const std::string& expression);

	bool isInList(const Player* player);

	void getList(std::string& _list) const;

private:
	typedef std::unordered_set<uint32_t> PlayerList;
	typedef std::list< std::pair<uint32_t, std::string> > GuildList;

	typedef std::list<std::string> ExpressionList;
	typedef std::list<std::pair<boost::regex, bool> > RegExList;
	std::string list;
	PlayerList playerList;
	GuildList guildList;
	ExpressionList expressionList;
	RegExList regExList;
};

class House;
class BedItem;

class Door : public Item
{
public:
	Door(const uint16_t& _type);
	virtual ~Door();

	virtual Door* getDoor();
	virtual const Door* getDoor() const;

	House* getHouse();

	//serialization
	virtual Attr_ReadValue readAttr(const AttrTypes_t& attr, PropStream& propStream);
	virtual bool serializeAttr(PropWriteStream& propWriteStream) const;

	void setDoorId(const uint32_t& _doorId);
	uint32_t getDoorId() const;

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

enum AccessList_t
{
	GUEST_LIST = 0x100,
	SUBOWNER_LIST = 0x101
};

enum AccessHouseLevel_t
{
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
	HouseTransferItem(House* _house);
	virtual ~HouseTransferItem();

	static HouseTransferItem* createHouseTransferItem(House* house);

	virtual bool onTradeEvent(TradeEvents_t event, Player* owner);

	House* getHouse();
	virtual bool canTransform() const;

protected:
	House* house;
};

class House
{
public:
	enum syncflags_t
	{
		HOUSE_SYNC_TOWNID		= 0,
		HOUSE_SYNC_NAME			= 1 << 0,
		HOUSE_SYNC_PRICE		= 1 << 1,
		HOUSE_SYNC_RENT			= 1 << 2,
		HOUSE_SYNC_GUILDHALL	= 1 << 3
	};

	House(const uint32_t& _id);

	void addTile(HouseTile* tile);

	bool canEditAccessList(const uint32_t& listId, const Player* player);
	// listId special values:
	//	GUEST_LIST     guest list
	//  SUBOWNER_LIST subowner list
	void setAccessList(const uint32_t& listId, const std::string& textlist);
	bool getAccessList(const uint32_t& listId, std::string& list) const;

	bool isInvited(const Player* player);

	AccessHouseLevel_t getHouseAccessLevel(const Player* player);
	bool kickPlayer(Player* player, const std::string& name);

	void setEntryPos(const Position& pos);
	const Position& getEntryPosition() const;

	void setName(const std::string& _name);
	const std::string& getName() const;

	void setOwner(const uint32_t& guid);
	const uint32_t& getOwner() const;

	void setPaidUntil(const time_t& paid);
	const time_t& getPaidUntil() const;

	void setRent(const uint32_t& _rent);
	const uint32_t& getRent() const;

	bool hasSyncFlag(const syncflags_t& flag) const;
	void resetSyncFlag(const syncflags_t& flag);

	void setLastWarning(const time_t& _lastWarning);
	const time_t& getLastWarning() const;

	void setPayRentWarnings(const uint32_t& warnings);
	const uint32_t& getPayRentWarnings() const;

	void setTownId(const uint32_t& _town);
	const uint32_t& getTownId() const;

	void setGuildHall(bool _guildHall);
	bool isGuildHall() const;

	void setPendingDepotTransfer(bool _pendingDepotTransfer);
	bool getPendingDepotTransfer() const;

	const uint32_t& getId() const;

	void addDoor(Door* door);
	void removeDoor(Door* door);
	void addBed(BedItem* bed);

	Door* getDoorByNumber(const uint32_t& doorId);
	const Door* getDoorByNumber(const uint32_t& doorId) const;
	Door* getDoorByPosition(const Position& pos);

	HouseTransferItem* getTransferItem();
	void resetTransferItem();
	bool executeTransfer(HouseTransferItem* item, Player* player);

	HouseTileList::iterator getTileBegin();
	HouseTileList::iterator getTileEnd();
	size_t getTileCount();

	HouseDoorList::iterator getDoorBegin();
	HouseDoorList::iterator getDoorEnd();
	size_t getDoorCount();

	HouseBedItemList::iterator getBedsBegin();
	HouseBedItemList::iterator getBedsEnd();
	size_t getBedTiles();
	uint32_t getBedCount();

	// Transfers all items to depot and clicks all players (useful for map updates, for example)
	void clean();

private:
	void updateDoorDescription();
	bool transferToDepot();

	bool isLoaded;
	uint32_t id;
	uint32_t owner;
	std::string ownerName;
	HouseTileList houseTiles;
	HouseDoorList doorList;
	HouseBedItemList bedsList;
	AccessList guestList;
	AccessList subOwnerList;
	std::string name;
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

enum RentPeriod_t
{
	RENTPERIOD_DAILY,
	RENTPERIOD_WEEKLY,
	RENTPERIOD_MONTHLY,
	RENTPERIOD_YEARLY,
	RENTPERIOD_NEVER
};

class Houses
{
	Houses();

public:
	static Houses& getInstance();

	House* getHouse(const uint32_t& houseid, bool add = false);
	House* getHouseByPlayerId(const uint32_t& playerId);

	bool loadHousesXML(const std::string& filename);
	bool payRent(Player* player, House* house, time_t time = 0);
	bool payHouses();
	void getRentPeriodString(std::string& strPeriod);

	HouseMap::iterator getHouseBegin();
	HouseMap::iterator getHouseEnd();

	bool payHouse(House* house, const time_t& time);

private:
	RentPeriod_t rentPeriod;
	HouseMap houseMap;
	friend class IOMapSerialize;
};

#endif
