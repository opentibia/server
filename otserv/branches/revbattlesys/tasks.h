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


#ifndef __OTSERV_TASKS_H__
#define __OTSERV_TASKS_H__

#include <boost/function.hpp>
#include <functional>

#include "player.h"
#include "game.h"
#include "position.h"

//class MovePlayer : public std::binary_function<Game*, Direction, int>{
class MovePlayerTask : public boost::function<bool(Game*, Direction)>{
public:
	MovePlayerTask(Player* player, Game* game, Direction dir) :
		//boost::function2<bool, Game*, Direction>(game, dir),
		_player(player)
	{
		_player->useThing2();
	}

	virtual ~MovePlayerTask()
	{
		_player->releaseThing2();
	}

	//virtual bool operator()(Game* game, Direction dir)
	result_type operator()(const first_argument_type& game, const second_argument_type& dir) const
	{
		OTSYS_THREAD_LOCK_CLASS lockClass(game->gameLock, "MovePlayerTask");

		if(_player->isRemoved()){
			return false;
		}

#ifdef __DEBUG__
		std::cout << "MovePlayerTask: " << dir << std::endl;
#endif

		//_player->lastmove = OTSYS_TIME();
		ReturnValue ret = game->internalMoveCreature(_player, dir);

		game->flushSendBuffers();

		return (ret == RET_NOERROR);
	}

private:
	Player* _player;
};

class Task{
public:
	Task(boost::function1<bool, Game*> f) :
		_f(f)
	{
		//
	}

	virtual ~Task()
	{
		//
	}

	virtual void operator()(Game* arg)
	{
		_f(arg);
	}

protected:
	boost::function1<bool, Game*> _f;
};

#endif
