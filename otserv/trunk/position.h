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

#ifndef __OTSERV_POSITION_H__
#define __OTSERV_POSITION_H__

#include "definitions.h"
#include <cmath>
#include <iostream>

enum Direction {
	NORTH = 0,
	EAST = 1,
	SOUTH = 2,
	WEST = 3,
	SOUTHWEST = 4,
	SOUTHEAST = 5,
	NORTHWEST = 6,
	NORTHEAST = 7
};

class Position{
public:
	int32_t x;
	int32_t y;
	int32_t z;

	Position(const int32_t& _x, const int32_t& _y, const int32_t& _z);
	Position();
	virtual ~Position();

	template<int32_t deltax, int32_t deltay, int32_t deltaz>
	inline static bool areInRange(const Position& p1, const Position& p2){
		if(std::abs(int32_t(p1.x - p2.x)) > deltax ||
			std::abs(int32_t(p1.y - p2.y)) > deltay ||
			std::abs(int32_t(p1.z - p2.z)) > deltaz){
			return false;
		}
		return true;
	}

	template<int32_t deltax, int32_t deltay>
	inline static bool areInRange(const Position& p1, const Position& p2){
		if(std::abs(int32_t(p1.x - p2.x)) > deltax ||
			std::abs(int32_t(p1.y - p2.y)) > deltay){
			return false;
		}
		return true;
	}

	bool operator<(const Position& p) const;
	bool operator>(const Position& p) const;
	bool operator==(const Position& p)  const;
	bool operator!=(const Position& p)  const;
	Position operator-(const Position& p1);
};

std::ostream& operator<<(std::ostream&, const Position&);
std::ostream& operator<<(std::ostream&, const Direction&);

class PositionEx : public Position {
public:
	int32_t stackpos;

	PositionEx();	
	PositionEx(const int32_t& _x, const int32_t& _y, const int32_t& _z, const int32_t& _stackpos);
	PositionEx(const int32_t& _x, const int32_t& _y, const int32_t& _z);
	PositionEx(const Position& p);
	PositionEx(const PositionEx& p);
	PositionEx(const Position& p, const int32_t& _stackpos);	
	virtual ~PositionEx();

	bool operator==(const PositionEx& p) const;
	bool operator!=(const PositionEx& p) const;
};

#endif // __OTSERV_POSITION_H__
