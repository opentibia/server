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

#ifndef __OTSERV_POS_H
#define __OTSERV_POS_H
//////////////////////////////////////////////////
// represents a map position
// for now just a 3d point
#include <stdlib.h>
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
	NORTHEAST = 7,
};

class Position{
public:

	// for now we just initialise the position to a startpoint
	//Position() : x(247), y(218), z(7) { };
  	Position() : x(31), y(31), z(7) { };
	~Position() {};

	template<int deltax, int deltay, int deltaz>
	inline static bool areInRange(const Position& p1, const Position& p2){
		if(std::abs(p1.x - p2.x) > deltax ||
			std::abs(p1.y - p2.y) > deltay ||
			std::abs(p1.z - p2.z) > deltaz){
			return false;
		}
		return true;
	}
	
	template<int deltax, int deltay>
	inline static bool areInRange(const Position& p1, const Position& p2){
		if(std::abs(p1.x - p2.x) > deltax ||
			std::abs(p1.y - p2.y) > deltay){
			return false;
		}
		return true;
	}
	
	Position(int _x, int _y, int _z)
	: x(_x), y(_y), z(_z) {};

	int x,y,z;

	bool operator<(const Position& p) const {
		if(z < p.z)
			return true;
		if(z > p.z)
			return false;

		if(y < p.y)
			return true;
		if(y > p.y)
			return false;

		if(x < p.x)
			return true;
		if(x > p.x)
			return false;

		return false;
	}

	bool operator>(const Position& p) const {
		return ! (*this < p);
	}

	bool operator==(const Position p)  const {
		if(p.x==x && p.y==y && p.z ==z)
			return true;
		else
			return false;
	}

	bool operator!=(const Position p)  const {
		if(p.x==x && p.y==y && p.z ==z)
			return false;
		else
			return true;
	}

	Position operator-(const Position p1){
		return Position(x-p1.x, y-p1.y,z-p1.z);
	}

	/*
	void dist(){
		x=abs(x);
		y=abs(y);
		z=abs(z);
	}
		
	bool operator==(const position p){
		return (x==p.x && y== p.x && z==p.z);
	}
	*/

};

std::ostream& operator<<(std::ostream&, const Position&);
std::ostream& operator<<(std::ostream&, const Direction&);


class PositionEx : public Position{
public:  
	PositionEx(){ };
	~PositionEx(){};

	PositionEx(int _x, int _y, int _z, int _stackpos)
	: Position(_x,_y,_z), stackpos(_stackpos) {};
	
	PositionEx(int _x, int _y, int _z)
	: Position(_x,_y,_z), stackpos(0) {};

	PositionEx(Position p)
	: Position(p.x,p.y,p.z), stackpos(0) {};
	
	PositionEx(Position p,int _stackpos)
	: Position(p.x,p.y,p.z), stackpos(_stackpos) {};

	int stackpos;

	bool operator==(const PositionEx p)  const {
		if(p.x==x && p.y==y && p.z ==z && p.stackpos == stackpos)
			return true;
		else
			return false;
	}

	bool operator!=(const PositionEx p)  const {
		if(p.x==x && p.y==y && p.z ==z && p.stackpos != stackpos)
			return false;
		else
			return true;
	}

};


#endif
