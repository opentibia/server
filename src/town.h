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

#ifndef __OTSERV_TOWN_H__
#define __OTSERV_TOWN_H__

#include <string>
#include <map>
#include <stdint.h>
#include "position.h"

class Town
{
public:
	Town(uint32_t _townid);

	const Position& getTemplePosition() const;
	const std::string& getName() const;

	void setTemplePos(const Position& pos);
	void setName(const std::string& _townName);
	uint32_t getTownID() const;

private:
	uint32_t townid;
	std::string townName;
	Position posTemple;
};

typedef std::map<uint32_t, Town*> TownMap;

class Towns
{
public:
	static Towns* getInstance();

	bool addTown(uint32_t _townid, Town* town);

	Town* getTown(const std::string& townname);
	Town* getTown(uint32_t _townid);

	TownMap::const_iterator getTownBegin() const;
	TownMap::const_iterator getTownEnd() const;

private:
	TownMap townMap;
};

#endif
