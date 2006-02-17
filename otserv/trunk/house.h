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

#ifndef __HOUSE_H__
#define __HOUSE_H__

#include <string>
#include <list>
#include <map>

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
	bool addPlayer(const std::string& name);
	bool addGuild(const std::string& guildName, const std::string& rank);
	bool addExpression(const std::string& expression);
	
	bool isInList(const Player* player);
	
	void getList(std::string& _list);
	
private:
	typedef std::list<uint32_t> PlayerList;
	typedef std::list<uint32_t> GuildList; //TODO: include ranks
	typedef std::list<std::string> ExpressionList;
	std::string list;
	PlayerList playerList;
	GuildList guildList;
	ExpressionList expressionList;
};

class Door : public Item
{
public:
	Door(uint16_t _type);
	virtual ~Door();
	
	virtual Door* getDoor() {return this;};
	virtual const Door* getDoor() const {return this;};
	
	House* getHouse(){return house;};
	
	void setDoorId(unsigned long _doorId){ doorId = _doorId;};
	unsigned long getDoorId() const{ return doorId;};
	
	bool canUse(const Player* player);
	
	void setAccessList(const std::string& textlist);
	bool getAccessList(std::string& list);
	
	int unserialize(xmlNodePtr p);

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


class House
{
public:
	House();
	~House();
	
	void addTile(HouseTile* tile);
	//ReturnValue addGuest(const Player* player);
	//ReturnValue addGuest(const std::string& name);
	//ReturnValue removeGuest(const std::string& name);
	
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

	void addDoor(Door* door);
	Door* getDoorByNumber(unsigned long doorId);

	Door* getDoorByPosition(const Position& pos);

private:
	
	typedef std::list<HouseTile*> HouseTileList;
	typedef std::list<Door*> HouseDoorList;
	
	uint32_t houseOwner;
	HouseTileList houseTiles;
	HouseDoorList doorList;
	AccessList guestList;
	AccessList subOwnerList;
	std::string houseName;
	Position posEntry;
};

typedef std::map<uint32_t, House*> HouseMap;

class Houses
{
public:
	static Houses& getInstance(){
		static Houses instance;
		return instance;
	}

	House* getHouse(uint32_t houseid)
	{
		HouseMap::iterator it = houseMap.find(houseid);
		
		if(it != houseMap.end()){
			return it->second;
		}

		House* house = new House();
		houseMap[houseid] = house;
		return house;
	}

	bool loadHousesXML(std::string filename);

private:
	HouseMap houseMap;
};

#endif
