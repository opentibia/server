//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// protocoll chooser
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
#include "tile.h"
#include "otsystem.h"

#include <string>

#include "protocol.h"

class Player;

extern Map gmap;


Protocol::Protocol()
{
}


Protocol::~Protocol()
{
}


void Protocol::setPlayer(Player* p)
{
	player = p;
  map    = &gmap;
}

void Protocol::sleepTillMove(){
	int ground =	map->getTile(	player->pos.x,
									player->pos.y,
									player->pos.z)->ground.getID();
	long long delay = ((long long)player->lastmove + (long long)player->getStepDuration(Item::items[ground].speed)) -
				((long long)OTSYS_TIME());

	if(delay > 0){
		std::cout << "Delaying "<< player->getName() << " --- " << delay << std::endl;
		OTSYS_SLEEP((uint32_t)delay);
	}
	player->lastmove = OTSYS_TIME();
}