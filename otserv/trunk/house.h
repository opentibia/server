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
	typedef OTSERV_HASH_SET<uint32_t> GuildList; //TODO: include ranks

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
	virtual bool unserialize(xmlNodePtr p);
	virtual xmlNodePtr serialize();

	virtual bool readAttr(AttrTypes_t attr, PropStream& propStream);
	virtual bool serializeAttr(PropWriteStream& propWriteStream);

	void setDoorId(unsigned long _doorId){ doorId = _doorId;};
	unsigned long getDoorId() const{ return doorId;};
	
	bool canUse(const Player* player);
	
	void setAccessList(const std::string& textlist);
	bool getAccessList(std::string& list) const;

protected:
	void setHouse(House* _house);
	
private:
	unsigned long doorId;
	House* house;
	AccessList* accessList;
	friend class House;
};

enum AccessList_t{
	GUEST_LIST = 0x100,
	SUBOWNER_LIST = 0x101,
};

enum AccessHouseLevel_t{
	HOUSE_NO_INVITED = 0,
	HOUSE_GUEST = 1,
	HOUSE_SUBOWNER = 2,
	HOUSE_OWNER = 3,
};

typedef std::list<HouseTile*> HouseTileList;
typedef std::list<Door*> HouseDoorList;


class HouseTransferItem : public Item
{
public:
	static HouseTransferItem* createHouseTransferItem(House* house);
	
	HouseTransferItem(House* _house){house = _house;};
	virtual ~HouseTransferItem(){};
	
	virtual bool onTradeEvent(TradeEvents_t event, Player* owner);

	House* getHouse(){return house;};
	
protected:
	House* house;
};

class House
{
public:
	House(uint32_t _houseid);
	~House();
	
	void addTile(HouseTile* tile);
	
	bool canEditAccessList(unsigned long listId, const Player* player);
	// listId special values:
	//	GUEST_LIST     guest list
	//  SUBOWNER_LIST subowner list
	void setAccessList(unsigned long listId, const std::string& textlist);
	bool getAccessList(unsigned long listId, std::string& list);

	bool isInvited(const Player* player);
	
	AccessHouseLevel_t getHouseAccessLevel(const Player* player);
	bool kickPlayer(Player* player, const std::string& name);
	
	void setEntryPos(const Position& pos) {posEntry = pos;};
	const Position& getEntryPosition() const {return posEntry;};

	void setName(const std::string& _houseName) {houseName = _houseName;};
	const std::string& getName() const {return houseName;};

	void setHouseOwner(uint32_t guid);
	uint32_t getHouseOwner() const {return houseOwner;};

	void setPaidUntil(uint64_t paid){paidUntil = paid;};
	uint64_t getPaidUntil() const {return paidUntil;};

	void setRent(uint32_t _rent){rent = _rent;};
	uint32_t getRent() const {return rent;};
	
	void setPayRentWarnings(uint32_t warnings) {rentWarnings = warnings;};
	uint32_t getPayRentWarnings() const {return rentWarnings;};

	void setTownId(uint32_t _town){townid = _town;};
	uint32_t getTownId() const {return townid;};

	uint32_t getHouseId() const {return houseid;};

	void addDoor(Door* door);
	Door* getDoorByNumber(unsigned long doorId);
	Door* getDoorByPosition(const Position& pos);
	
	HouseTransferItem* getTransferItem();
	void resetTransferItem();
	bool executeTransfer(HouseTransferItem* item, Player* player);
	
	HouseTileList::iterator getHouseTileBegin() {return houseTiles.begin();}
	HouseTileList::iterator getHouseTileEnd() {return houseTiles.end();}

	HouseDoorList::iterator getHouseDoorBegin() {return doorList.begin();}
	HouseDoorList::iterator getHouseDoorEnd() {return doorList.end();}

private:
	bool transferToDepot();

	bool isLoaded;
	uint32_t houseid;
	uint32_t houseOwner;
	HouseTileList houseTiles;
	HouseDoorList doorList;
	AccessList guestList;
	AccessList subOwnerList;
	std::string houseName;
	Position posEntry;
	uint64_t paidUntil;
	//uint64_t lastWarning;
	uint32_t rentWarnings;
	uint32_t rent;
	uint32_t townid;
	
	HouseTransferItem* transferItem;
	Container transfer_container;
};

typedef std::map<uint32_t, House*> HouseMap;

enum RentPerioid_t{
	RENTPERIOD_DAILY,
	RENTPERIOD_WEEKLY,
	RENTPERIOD_MONTHLY,
	RENTPERIOD_YEARLY,
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

	bool loadHousesXML(std::string filename);
	
	bool payHouses();

	HouseMap::iterator getHouseBegin() {return houseMap.begin();}
	HouseMap::iterator getHouseEnd() {return houseMap.end();}

private:
	RentPerioid_t rentPeriod;
	HouseMap houseMap;
};

#endif
