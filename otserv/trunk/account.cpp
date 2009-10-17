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

#include <cmath>
#include <algorithm>
#include <functional>
#include <iostream>

#include "account.h"

Account::Account()
{
	number = warnings = 0;
	premEnd = 0;
}

Account::~Account()
{
	charList.clear();
}

uint16_t Account::getPremiumDaysLeft(uint32_t _premEnd)
{
	if(_premEnd < (uint32_t)time(NULL)){
		return 0;
	}

	return (uint16_t)std::ceil((double)(_premEnd - time(NULL)) / 86400);
}
