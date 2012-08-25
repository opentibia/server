//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Classes for Account & Account Character
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

#include <string>
#include <list>
#include <stdint.h>

struct AccountCharacter
{
	AccountCharacter() : ip(0), port(7172) {}

	std::string name;
	std::string world;
	uint32_t ip;
	uint16_t port;
};

struct Account
{
	Account() : number(0), premiumEnd(0), warnings(0) {}
	~Account() {};

	std::string name;
	uint32_t number;
	std::string password;
	time_t premiumEnd;
	uint32_t warnings;
	std::list<AccountCharacter> charList;
};

#endif

