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
#include "otpch.h"

#include "definitions.h"
#include "tile.h"
#include "otsystem.h"

#include <string>

#include "protocol.h"
#include "tools.h"

class Player;

extern Game g_game;


Protocol::Protocol()
{
	player = NULL;
	game = NULL;
	pendingLogout = false;
}


Protocol::~Protocol()
{
	if(s) {
		closesocket(s);
		s = 0;
	}

	player = NULL;
	game = NULL;
}

uint32_t Protocol::getIP() const
{	
	return getIPSocket(s);
}

void Protocol::setPlayer(Player* p)
{
	player = p;
	game   = &g_game;
}

void Protocol::sleepTillMove()
{
	long long delay = player->getSleepTicks();
	if(delay > 0 ){
             
#if __DEBUG__     
		std::cout << "Delaying "<< player->getName() << " --- " << delay << std::endl;		
#endif
		
		OTSYS_SLEEP((uint32_t)delay);
	}

	player->lastmove = OTSYS_TIME();
}
