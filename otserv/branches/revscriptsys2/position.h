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

#include "enums.h"

class Position{
public:

  	Position() : x(0), y(0), z(0) { };
	
	Position(int32_t _x, int32_t _y, int32_t _z)
	: x(_x), y(_y), z(_z) {};

	virtual ~Position() {};

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

	int32_t x;
	int32_t y;
	int32_t z;

	bool operator<(const Position& p) const {
		if(z < p.z){
			return true;
		}
		if(z > p.z){
			return false;
		}

		if(y < p.y){
			return true;
		}
		if(y > p.y){
			return false;
		}

		if(x < p.x){
			return true;
		}
		if(x > p.x){
			return false;
		}

		return false;
	}

	bool operator>(const Position& p) const {
		return !(*this < p);
	}

	bool operator==(const Position& p)  const {
		if(p.x==x && p.y==y && p.z ==z){
			return true;
		}

		return false;
	}

	bool operator!=(const Position& p)  const {
		if(p.x==x && p.y==y && p.z ==z){
			return false;
		}

		return true;
	}

	Position operator-(const Position& p1){
		return Position(x-p1.x, y-p1.y,z-p1.z);
	}
};

std::ostream& operator<<(std::ostream&, const Position&);
std::ostream& operator<<(std::ostream&, const Direction&);

class PositionEx : public Position{
public:
	~PositionEx(){};

	PositionEx(int32_t _x, int32_t _y, int32_t _z, int32_t _stackpos = 0)
	: Position(_x, _y, _z), stackpos(_stackpos) {};

	PositionEx(const Position& p)
	: Position(p.x, p.y, p.z), stackpos(0) {};

	PositionEx(const PositionEx& p)
	: Position(p.x, p.y, p.z), stackpos(p.stackpos) {};

	PositionEx(const Position& p, int32_t _stackpos)
	: Position(p.x,p.y,p.z), stackpos(_stackpos) {};

	int32_t stackpos;

	bool operator==(const PositionEx& p)  const {
		if(p.x==x && p.y==y && p.z ==z && p.stackpos == stackpos){
			return true;
		}
		
		return false;
	}

	bool operator!=(const PositionEx& p)  const {
		if(p.x==x && p.y==y && p.z ==z && p.stackpos != stackpos){
			return false;
		}
		
		return true;
	}
};


#endif
