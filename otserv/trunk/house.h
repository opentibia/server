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

typedef std::list<uint32_t> InviteList;
typedef std::list<HouseTile*> HouseTileList;

class House
{
public:
	House();
	~House();
	
	void addTile(HouseTile* tile);
	ReturnValue addGuest(const Player* player);
	ReturnValue addGuest(const std::string& name);
	ReturnValue removeGuest(const std::string& name);

	bool isInvited(uint32_t guid);
	
	void setEntryPos(const Position& pos) {posEntry = pos;};
	const Position& getEntryPosition() const {return posEntry;};

	void setName(const std::string& _houseName) {houseName = _houseName;};
	const std::string& getName() const {return houseName;};

	void setHouseOwner(uint32_t guid);
	uint32_t getHouseOwner() const {return houseOwner;};

private:
	uint32_t houseOwner;
	HouseTileList houseTiles;
	InviteList guestList;
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

private:
	HouseMap houseMap;
};

#endif
