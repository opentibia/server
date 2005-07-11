//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Special Tasks which require more arguments than possible
// with STL functions...
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


#ifndef __OTSERV_TASKS_H
#define __OTSERV_TASKS_H

#include "scheduler.h"
#include "position.h"
#include "player.h"
#include "game.h"

/*
class MovePlayer : public std::binary_function<Game*, Direction, int> {
public:
	MovePlayer(unsigned long playerid) : _pid(playerid) {}

	virtual result_type operator()(const first_argument_type& game, const second_argument_type& dir) const
	{
		OTSYS_THREAD_LOCK(game->gameLock)
		// get the player we want to move...
		Creature* creature = game->getCreatureByID(_pid);

		Player* player = dynamic_cast<Player*>(creature);
		if (!player) { // player is not available anymore it seems...
			OTSYS_THREAD_UNLOCK(game->gameLock)
			return 0;
		}

		if(player->cancelMove) {
	    OTSYS_THREAD_UNLOCK(game->gameLock)
		  return 1;
    }
		       
		Position pos = player->pos;
		switch (dir) {
			case NORTH:
					pos.y--;
					break;
			case EAST:
					pos.x++;
					break;
			case SOUTH:
					pos.y++;
					break;
			case WEST:
					pos.x--;
					break;
			case NORTHEAST:
					pos.x++;
					pos.y--;
				break;
			case NORTHWEST:
					pos.x--;
					pos.y--;
				break;
			case SOUTHWEST:
					pos.x--;
					pos.y++;
				break;
			case SOUTHEAST:
					pos.x++;
					pos.y++;
				break;
		}

#ifdef __DEBUG__
		std::cout << "move to: " << dir << std::endl;
#endif

		game->thingMove(player, player, pos.x, pos.y, pos.z, 1);

		game->flushSendBuffers();
		OTSYS_THREAD_UNLOCK(game->gameLock)

		if(player->pos != pos) {
			return 0;
		}
		else
			return 1;
	}

protected:
	unsigned long _pid;
};

class StopMovePlayer : public std::unary_function<Game*, bool> {
public:
		StopMovePlayer(unsigned long playerid) : _pid(playerid) { }
		
		virtual result_type operator()(const argument_type& game) const {
			// get the player we want to move...
			Creature* creature = game->getCreatureByID(_pid);
			Player* player = dynamic_cast<Player*>(creature);
			if (!player) { // player is not available anymore it seems...
				return false;
			}
			else {
				if(player->cancelMove) {
					player->cancelMove = false;
					player->sendCancelWalk("");
					return true;
				}
			}

			return false;
		}

protected:
	unsigned long _pid;

};
*/
#endif
