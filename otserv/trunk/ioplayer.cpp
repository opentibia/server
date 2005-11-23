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

#include "ioplayer.h"

#ifdef __USE_MYSQL__
	#include "ioplayersql.h"
#endif
#include "ioplayerxml.h"

#ifdef __USE_MYSQL__
#include "luascript.h"
extern LuaScript g_config;
#endif

IOPlayer* IOPlayer::_instance = NULL;

IOPlayer* IOPlayer::instance(){
	if(!_instance){
#ifdef __USE_MYSQL__
        if(g_config.getGlobalString("sourcedata") == "SQL") 
		_instance = (IOPlayer*)new IOPlayerSQL;
		else // if(g_config.getGlobalString("sourcedata") == "XML") //fallback to xml
#endif
		_instance = (IOPlayer*)new IOPlayerXML;

	}
    #ifdef __DEBUG__
	printf("%s \n", _instance->getSourceDescription());
	#endif 
	return _instance;
}

bool IOPlayer::loadPlayer(Player* player, std::string name){
	return false;
}

bool IOPlayer::savePlayer(Player* player){
	return false;
}

bool IOPlayer::getGuidByName(unsigned long &guid, unsigned long &alvl, std::string &name)
{
	return false;
}

bool IOPlayer::getNameByGuid(unsigned long guid, std::string &name)
{
	return false;
}
