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

#include <iomanip>
#include "position.h"

Position::Position()
	: x(0)
	, y(0)
	, z(0)
{

}

Position::Position(int32_t _x, int32_t _y, int32_t _z)
	: x(_x)
	, y(_y)
	, z(_z)
{

}

Position::~Position()
{

}

bool Position::operator<(const Position& p) const {
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

bool Position::operator>(const Position& p) const {
	return !(*this < p);
}

bool Position::operator==(const Position& p)  const {
	if(p.x==x && p.y==y && p.z ==z){
		return true;
	}

	return false;
}

bool Position::operator!=(const Position& p)  const {
	if(p.x==x && p.y==y && p.z ==z){
		return false;
	}

	return true;
}

Position Position::operator-(const Position& p1){
	return Position(x-p1.x, y-p1.y,z-p1.z);
}

std::ostream& operator<<(std::ostream& os, const Position& pos) {
	os << "( " << std::setw(5) << std::setfill('0') << pos.x;
	os << " / " << std::setw(5) << std::setfill('0') << pos.y;
	os << " / " << std::setw(3) << std::setfill('0') << pos.z;
	os << " )";

	return os;
}

PositionEx::PositionEx(int32_t _x, int32_t _y, int32_t _z, int32_t _stackpos)
	: Position(_x, _y, _z)
	, stackpos(_stackpos)
{

}

PositionEx::PositionEx(const Position& p)
	: Position(p.x, p.y, p.z)
	, stackpos(0)
{

}

PositionEx::PositionEx(const PositionEx& p)
	: Position(p.x, p.y, p.z)
	, stackpos(p.stackpos)
{

}

PositionEx::PositionEx(const Position& p, int32_t _stackpos)
	: Position(p.x,p.y,p.z)
	, stackpos(_stackpos)
{

}

PositionEx::~PositionEx()
{

}

bool PositionEx::operator==(const PositionEx& p) const
{
	if(p.x==x && p.y==y && p.z ==z && p.stackpos == stackpos){
		return true;
	}

	return false;
}

bool PositionEx::operator!=(const PositionEx& p) const
{
	if(p.x==x && p.y==y && p.z ==z && p.stackpos != stackpos){
		return false;
	}

	return true;
}
