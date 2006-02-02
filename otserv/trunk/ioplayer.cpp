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
#include "town.h"

#ifdef __USE_MYSQL__
#include "ioplayersql.h"
#else
#include "ioplayerxml.h"
#endif

#ifdef __USE_MYSQL__
#include "luascript.h"
extern LuaScript g_config;
#endif

IOPlayer* IOPlayer::_instance = NULL;

IOPlayer* IOPlayer::instance()
{
	if(!_instance){
#ifdef __USE_MYSQL__
		_instance = (IOPlayer*)new IOPlayerSQL;
#else
		_instance = (IOPlayer*)new IOPlayerXML;
#endif
	}
    #ifdef __DEBUG__
	printf("%s \n", _instance->getSourceDescription());
	#endif 
	return _instance;
}

bool IOPlayer::loadPlayer(Player* player, std::string name)
{
	return false;
}

bool IOPlayer::savePlayer(Player* player)
{
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

bool IOPlayer::playerExists(std::string name)
{
	return false;
}

void IOPlayer::initPlayer(Player* player)
{
	//create depots that does not exist in the player
	Depot* depot = NULL;
	for(TownMap::const_iterator it = Towns::getInstance().getTownBegin(); it != Towns::getInstance().getTownEnd(); ++it){
		depot = player->getDepot(it->second->getTownID());

		//depot does not yet exist?
		if(depot == NULL){
			//create a new depot
			Item* tmpDepot = Item::CreateItem(ITEM_LOCKER1);
			if(tmpDepot->getContainer()){
				if(depot = tmpDepot->getContainer()->getDepot()){
					tmpDepot = NULL;
					Item* depotChest = Item::CreateItem(ITEM_DEPOT);
					depot->__internalAddThing(depotChest);

					player->addDepot(depot, it->second->getTownID());
				}
			}

			if(tmpDepot){
				std::cout << "Failure: Creating a new depot with id: "<< depot->getDepotId() <<
					", for player: " << player->getName() << std::endl;

				tmpDepot->releaseThing2();
			}
		}
	}
}
