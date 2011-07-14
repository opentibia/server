//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Account class
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


#ifndef __OTSERV_ACCOUNT_H__
#define __OTSERV_ACCOUNT_H__

#include "definitions.h"
#include <list>
#include <string>
#include <ctime>
#include <cmath>

class Account
{
public:
	Account() : number(0), warnings(0), premEnd(0){}
	~Account(){}

	static uint16_t getPremiumDaysLeft(int32_t _premEnd)
	{
		if(_premEnd < time(NULL)){
			return 0;
		}
		return (uint16_t)std::ceil(((double)(_premEnd - time(NULL))) / 86400.);
	}

	uint32_t number;
	uint32_t warnings;
	std::string name;
	std::string	password;

	time_t premEnd; // < current time is none, (time_t)(-1) is infinite.
	std::list<std::string> charList;
};

#endif  // #ifndef __ACCOUNT_H__
