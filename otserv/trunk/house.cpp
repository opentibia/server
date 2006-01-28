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

#include <sstream>

#include "house.h"
#include "ioplayer.h"
#include "game.h"

extern Game g_game;

House::House()
{
	houseName = "OTServ headquarter (Flat 1, Area 42)";
	houseOwner = 0;
	posEntry.x = 0;
	posEntry.y = 0;
	posEntry.z = 0;
}

House::~House()
{
	//
}

void House::addTile(HouseTile* tile)
{
	houseTiles.push_back(tile);
}

void House::setHouseOwner(uint32_t guid)
{
	houseOwner = guid;
	Item* iiItem = NULL;
	
	std::stringstream houseDescription;
	houseDescription << "It belongs to house '" << houseName << "'. " << std::endl;

	std::string name;
	if(guid != 0 && IOPlayer::instance()->getNameByGuid(guid, name)){
		houseDescription << name;
	}
	else{
		houseDescription << "Nobody";
	}
	
	houseDescription << " owns this house." << std::endl;

	for(HouseTileList::iterator it = houseTiles.begin(); it != houseTiles.end(); ++it){
		for(uint32_t i = 0; i < (*it)->getThingCount(); ++i){
			iiItem = (*it)->__getThing(i)->getItem();

			if(iiItem && iiItem->isDoor()){
				iiItem->setSpecialDescription(houseDescription.str());
			}
		}
	}
}

ReturnValue House::addGuest(const Player* player)
{
	if(!isInvited(player->getGUID())){
		guestList.push_back(player->getGUID());
		return RET_NOERROR;
	}

	return RET_NOTPOSSIBLE;
}

ReturnValue House::addGuest(const std::string& name)
{
	unsigned long guid = 0;
	unsigned long access = 0;
	std::string dbName = name;

	if(IOPlayer::instance()->getGuidByName(guid, access, dbName)){
		if(!isInvited(guid)){
			guestList.push_back(guid);
			return RET_NOERROR;
		}
	}

	return RET_NOTPOSSIBLE;
}

ReturnValue House::removeGuest(const std::string& name)
{
	unsigned long guid = 0;
	unsigned long access = 0;
	std::string dbName = name;

	if(IOPlayer::instance()->getGuidByName(guid, access, dbName)){
		if(isInvited(guid)){
			InviteList::iterator it = std::find(guestList.begin(), guestList.end(), guid);
			guestList.erase(it);

			Player* uninvitedPlayer = g_game.getPlayerByName(dbName);
			if(uninvitedPlayer){
				HouseTile* houseTile = dynamic_cast<HouseTile*>(uninvitedPlayer->getTile());

				if(houseTile && houseTile->getHouse() == this){
					if(g_game.internalTeleport(uninvitedPlayer, getEntryPosition()) == RET_NOERROR){
						g_game.AddMagicEffectAt(getEntryPosition(), NM_ME_ENERGY_AREA);
					}
				}
			}

			return RET_NOERROR;
		}
	}

	return RET_NOTPOSSIBLE;
}

bool House::isInvited(uint32_t guid)
{
	if(guid == houseOwner)
		return true;

	InviteList::iterator it = std::find(guestList.begin(), guestList.end(), guid);
	return (it != guestList.end());
}
