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

#include "definitions.h"
#include "ioaccount.h"

#if defined USE_SQL_ENGINE
#include "ioaccountsql.h"
#else
#include "ioaccountxml.h"
#endif

IOAccount* IOAccount::_instance = NULL;

IOAccount* IOAccount::instance(){
	if(!_instance){
#if defined USE_SQL_ENGINE
		_instance = (IOAccount*)new IOAccountSQL;
#else
		_instance = (IOAccount*)new IOAccountXML;
#endif
	}
	#ifdef __DEBUG__
	printf("%s \n", _instance->getSourceDescription());
	#endif
	return _instance;
}

Account IOAccount::loadAccount(unsigned long accno){
	Account acc;
	return acc;
}

bool IOAccount::getPassword(unsigned long accno, const std::string &name, std::string &password)
{
	return false;
}

