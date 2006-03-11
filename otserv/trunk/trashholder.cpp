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

#include "trashholder.h"
#include "game.h"

extern Game g_game;

TrashHolder::TrashHolder(uint16_t _type, MagicEffectClasses _effect /*= NM_ME_NONE*/) : Item(_type)
{
	effect = _effect;
}

TrashHolder::~TrashHolder()
{
	//
}

ReturnValue TrashHolder::__queryAdd(int32_t index, const Thing* thing, uint32_t count,
	uint32_t flags) const
{
	return RET_NOERROR;
}

ReturnValue TrashHolder::__queryMaxCount(int32_t index, const Thing* thing, uint32_t count,
	uint32_t& maxQueryCount, uint32_t flags) const
{
	maxQueryCount = std::max((uint32_t)1, count);
	return RET_NOERROR;
}

ReturnValue TrashHolder::__queryRemove(const Thing* thing, uint32_t count) const
{
	return RET_NOTPOSSIBLE;
}

Cylinder* TrashHolder::__queryDestination(int32_t& index, const Thing* thing, Item** destItem,
	uint32_t& flags)
{
	return this;
}

void TrashHolder::__addThing(Thing* thing)
{
	return __addThing(0, thing);
}

void TrashHolder::__addThing(int32_t index, Thing* thing)
{
	if(Item* item = thing->getItem()){
		if(item != this){
			g_game.internalRemoveItem(item);
			if(effect != NM_ME_NONE){
				g_game.AddMagicEffectAt(getPosition(), effect);
			}
		}
	}
}

void TrashHolder::__updateThing(Thing* thing, uint32_t count)
{
	//
}

void TrashHolder::__replaceThing(uint32_t index, Thing* thing)
{
	//
}

void TrashHolder::__removeThing(Thing* thing, uint32_t count)
{
	//
}

int32_t TrashHolder::__getIndexOfThing(const Thing* thing) const
{
	return -1;
}

Thing* TrashHolder::__getThing(uint32_t index) const
{
	return NULL;
}

void TrashHolder::postAddNotification(Thing* thing, bool hasOwnership /*= true*/)
{
	getParent()->postAddNotification(thing, false /*hasOwnership*/);
}

void TrashHolder::postRemoveNotification(Thing* thing, bool isCompleteRemoval, bool hadOwnership /*= true*/)
{
	getParent()->postRemoveNotification(thing, isCompleteRemoval, false /*hadOwnership*/);
}

void TrashHolder::__internalAddThing(Thing* thing)
{
	//
}

void TrashHolder::__internalAddThing(uint32_t index, Thing* thing)
{
	//
}
