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

#include "teleport.h"
#include "game.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <sstream>

extern Game g_game;

Teleport::Teleport(uint16_t _type) : Item(_type)
{
	destPos.x = 0;
	destPos.y = 0;
	destPos.z = 0;
}

Teleport::~Teleport()
{
	//
}

int Teleport::unserialize(xmlNodePtr p)
{
	Item::unserialize(p);
	char *tmp = (char*)xmlGetProp(p, (const xmlChar *) "destx");
	if(tmp){
		destPos.x = atoi(tmp);
		xmlFreeOTSERV(tmp);
	}
	tmp = (char*)xmlGetProp(p, (const xmlChar *) "desty");
	if(tmp){
		destPos.y = atoi(tmp);
		xmlFreeOTSERV(tmp);
	}
	tmp = (char*)xmlGetProp(p, (const xmlChar *) "destz");
	if(tmp){
		destPos.z = atoi(tmp);
		xmlFreeOTSERV(tmp);
	}
	
	return 0;
}

xmlNodePtr Teleport::serialize()
{
	xmlNodePtr xmlptr = Item::serialize();

	std::stringstream s;

	s.str(""); //empty the stringstream
	s << (int) destPos.x;
	xmlSetProp(xmlptr, (const xmlChar*)"destx", (const xmlChar*)s.str().c_str());

	s.str(""); //empty the stringstream
	s << (int) destPos.y;
	xmlSetProp(xmlptr, (const xmlChar*)"desty", (const xmlChar*)s.str().c_str());

	s.str(""); //empty the stringstream
	s << (int)destPos.z;
	xmlSetProp(xmlptr, (const xmlChar*)"destz", (const xmlChar*)s.str().c_str());

	return xmlptr;
}

ReturnValue Teleport::__queryAdd(int32_t index, const Thing* thing, uint32_t count,
	uint32_t flags) const
{
	return RET_NOTPOSSIBLE;
}

ReturnValue Teleport::__queryMaxCount(int32_t index, const Thing* thing, uint32_t count,
	uint32_t& maxQueryCount, uint32_t flags) const
{
	return RET_NOTPOSSIBLE;
}

ReturnValue Teleport::__queryRemove(const Thing* thing, uint32_t count) const
{
	return RET_NOERROR;
}

Cylinder* Teleport::__queryDestination(int32_t& index, const Thing* thing, Item** destItem,
	uint32_t& flags)
{
	return this;
}

void Teleport::__addThing(Thing* thing)
{
	return __addThing(0, thing);
}

void Teleport::__addThing(int32_t index, Thing* thing)
{
	Tile* destTile = g_game.getTile(getDestPos().x, getDestPos().y, getDestPos().z);
	if(destTile){
		if(Creature* creature = thing->getCreature()){
			getTile()->moveCreature(creature, destTile, true);
			g_game.AddMagicEffectAt(destTile->getPosition(), NM_ME_ENERGY_AREA);
		}
		else if(Item* item = thing->getItem()){
			g_game.internalMoveItem(getTile(), destTile, INDEX_WHEREEVER, item, item->getItemCount());
		}
	}
}

void Teleport::__updateThing(Thing* thing, uint32_t count)
{
	//
}

void Teleport::__replaceThing(uint32_t index, Thing* thing)
{
	//
}

void Teleport::__removeThing(Thing* thing, uint32_t count)
{
	//
}

int32_t Teleport::__getIndexOfThing(const Thing* thing) const
{
	return -1;
}

Thing* Teleport::__getThing(uint32_t index) const
{
	return NULL;
}

void Teleport::postAddNotification(Thing* thing, bool hasOwnership /*= true*/)
{
	getParent()->postAddNotification(thing, false /*hasOwnership*/);
}

void Teleport::postRemoveNotification(Thing* thing, bool isCompleteRemoval, bool hadOwnership /*= true*/)
{
	getParent()->postRemoveNotification(thing, isCompleteRemoval, false /*hadOwnership*/);
}

void Teleport::__internalAddThing(Thing* thing)
{
	//
}

void Teleport::__internalAddThing(uint32_t index, Thing* thing)
{
	//
}
