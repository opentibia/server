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

#include "ioaccount.h"

#ifdef __USE_MYSQL__
#include "ioaccountsql.h"
#endif
#include "ioaccountxml.h"

#ifdef __USE_MYSQL__
#include "luascript.h"
extern LuaScript g_config;
#endif

IOAccount* IOAccount::_instance = NULL;

IOAccount* IOAccount::instance(){
	if(!_instance){
#ifdef __USE_MYSQL__
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

bool IOAccount::getPassword(unsigned long accno, const std::string& name, std::string& password)
{
	return false;
}

