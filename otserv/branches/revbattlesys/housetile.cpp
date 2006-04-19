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

#include "housetile.h"
#include "house.h"
#include "game.h"

extern Game g_game;

HouseTile::HouseTile(int x, int y, int z, House* _house) :
	Tile(x, y, z)
{
	house = _house;
	setFlag(TILESTATE_HOUSE);
}

HouseTile::~HouseTile()
{
	//
}

ReturnValue HouseTile::__queryAdd(int32_t index, const Thing* thing, uint32_t count,
	uint32_t flags) const
{
	if(const Creature* creature = thing->getCreature()){
		if(const Player* player = creature->getPlayer()){
			if(player->getAccessLevel() < 3 && !house->isInvited(player))
				return RET_PLAYERISNOTINVITED;
		}
	}

	return Tile::__queryAdd(index, thing, count, flags);
}

Cylinder* HouseTile::__queryDestination(int32_t& index, const Thing* thing, Item** destItem,
	uint32_t& flags)
{
	if(const Creature* creature = thing->getCreature()){
		if(const Player* player = creature->getPlayer()){
			if(!house->isInvited(player)){
				const Position& EntryPos = house->getEntryPosition();
				Tile* destTile = g_game.getTile(EntryPos.x, EntryPos.y, EntryPos.z);
				assert(destTile != NULL);

				index = -1;
				*destItem = NULL;
				return destTile;
			}
		}
	}

	return Tile::__queryDestination(index, thing, destItem, flags);
}
