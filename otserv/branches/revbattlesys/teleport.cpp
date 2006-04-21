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

bool Teleport::unserialize(xmlNodePtr nodeItem)
{
	bool ret = Item::unserialize(nodeItem);

	char* nodeValue;	
	nodeValue = (char*)xmlGetProp(nodeItem, (const xmlChar *) "destx");
	if(nodeValue){
		destPos.x = atoi(nodeValue);
		xmlFreeOTSERV(nodeValue);
	}

	nodeValue = (char*)xmlGetProp(nodeItem, (const xmlChar *) "desty");
	if(nodeValue){
		destPos.y = atoi(nodeValue);
		xmlFreeOTSERV(nodeValue);
	}
	
	nodeValue = (char*)xmlGetProp(nodeItem, (const xmlChar *) "destz");
	if(nodeValue){
		destPos.z = atoi(nodeValue);
		xmlFreeOTSERV(nodeValue);
	}
	
	return ret;
}

xmlNodePtr Teleport::serialize()
{
	xmlNodePtr xmlptr = Item::serialize();

	std::stringstream ss;

	ss.str("");
	ss << (int) destPos.x;
	xmlSetProp(xmlptr, (const xmlChar*)"destx", (const xmlChar*)ss.str().c_str());

	ss.str("");
	ss << (int) destPos.y;
	xmlSetProp(xmlptr, (const xmlChar*)"desty", (const xmlChar*)ss.str().c_str());

	ss.str("");
	ss << (int)destPos.z;
	xmlSetProp(xmlptr, (const xmlChar*)"destz", (const xmlChar*)ss.str().c_str());

	return xmlptr;
}

bool Teleport::readAttr(AttrTypes_t attr, PropStream& propStream)
{
	if(ATTR_TELE_DEST == attr){
		TeleportDest* tele_dest;
		if(!propStream.GET_STRUCT(tele_dest)){
			return false;
		}

		setDestPos(Position(tele_dest->_x, tele_dest->_y, tele_dest->_z));
		return true;
	}
	else
		return Item::readAttr(attr, propStream);
}

bool Teleport::serializeAttr(PropWriteStream& propWriteStream)
{
	bool ret = Item::serializeAttr(propWriteStream);

	propWriteStream.ADD_UCHAR(ATTR_TELE_DEST);

	TeleportDest tele_dest;
	
	tele_dest._x = destPos.x;
	tele_dest._y = destPos.y;
	tele_dest._z = destPos.z;

	propWriteStream.ADD_VALUE(tele_dest);
	
	return ret;
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
			g_game.addMagicEffect(destTile->getPosition(), NM_ME_ENERGY_AREA);
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

void Teleport::postAddNotification(Thing* thing, bool hasOwnership /*= true*/)
{
	getParent()->postAddNotification(thing, false /*hasOwnership*/);
}

void Teleport::postRemoveNotification(Thing* thing, bool isCompleteRemoval, bool hadOwnership /*= true*/)
{
	getParent()->postRemoveNotification(thing, isCompleteRemoval, false /*hadOwnership*/);
}
