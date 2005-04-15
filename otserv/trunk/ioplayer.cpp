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

#ifdef USE_SQL
	#include "ioplayersql.h"
#else
	#include "ioplayerxml.h"
#endif

IOPlayer* IOPlayer::_instance = NULL;

IOPlayer* IOPlayer::instance(){
	if(!_instance){
#ifdef USE_SQL
 _instance = (IOPlayer*)new IOPlayerSQL;
#else
	_instance = (IOPlayer*)new IOPlayerXML;
#endif
	}

	return _instance;
}

bool IOPlayer::loadPlayer(Player* player, std::string name){
	return false;
}

bool IOPlayer::savePlayer(Player* player){
	return false;
}
