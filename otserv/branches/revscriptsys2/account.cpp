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

#include "account.h"

Account::Account()
{
	number = 0;
	warnings = 0;
	premEnd = 0;
}

Account::~Account()
{
	charList.clear();
}

uint16_t Account::getPremiumDaysLeft(uint32_t _premEnd)
{
	uint32_t today = (uint32_t)time(NULL) / 86400;
	if((time_t)_premEnd == time_t(-1))
		return 0xFFFF;

	if(uint32_t(_premEnd / 86400) < today)
		return 0;

	if(uint32_t(_premEnd / 86400) - today >= 0xFFFF)
		return 0xFFFF;

	return uint16_t(uint32_t(_premEnd / 86400) - today);
}
