//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Base class for the Account Loader/Saver
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


#ifndef __OTSERV_IOACCOUNT_H__
#define __OTSERV_IOACCOUNT_H__

#include <string>

#include "account.h"
#include "definitions.h"

/** Baseclass for all Player-Loaders */
class IOAccount {
public:
	IOAccount() {}
	~IOAccount() {}

	static IOAccount* instance(){
		static IOAccount instance;
		return &instance;
	}

	Account loadAccount(const std::string& name, bool preLoad = false);
	bool saveAccount(Account account);

	bool getPassword(const std::string& accountname, const std::string& name, std::string& password);
};

#endif
