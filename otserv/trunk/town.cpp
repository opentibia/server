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

#include "otpch.h"
#include "town.h"
#include <boost/algorithm/string/predicate.hpp>

Town::Town(const uint32_t& _townid)
	: townid(_townid)
{}

const Position& Town::getTemplePosition() const
{
	return posTemple;
}

const std::string& Town::getName() const
{
	return townName;
}

void Town::setTemplePos(const Position& pos)
{
	posTemple = pos;
}

void Town::setName(const std::string& _townName)
{
	townName = _townName;
}

const uint32_t& Town::getTownID() const
{
	return townid;
}

Towns::Towns()
{}

bool Towns::addTown(const uint32_t& _townid, Town* town)
{
	TownMap::iterator it = townMap.find(_townid);

	if (it != townMap.end())
	{
		return false;
	}

	townMap[_townid] = town;
	return true;
}

Towns& Towns::getInstance()
{
	static Towns instance;
	return instance;
}

Town* Towns::getTown(const std::string& townname)
{
	for (TownMap::iterator it = townMap.begin(); it != townMap.end(); ++it)
	{
		if (boost::algorithm::iequals(it->second->getName(), townname))
		{
			return it->second;
		}
	}

	return NULL;
}

Town* Towns::getTown(const uint32_t& _townid)
{
	TownMap::iterator it = townMap.find(_townid);

	if (it != townMap.end())
	{
		return it->second;
	}

	return NULL;
}

TownMap::const_iterator Towns::getTownBegin() const
{
	return townMap.begin();
}

TownMap::const_iterator Towns::getTownEnd() const
{
	return townMap.end();
}
