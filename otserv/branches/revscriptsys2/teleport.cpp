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
#include "otpch.h"

#include "teleport.h"
#include "game.h"

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

Attr_ReadValue Teleport::readAttr(AttrTypes_t attr, PropStream& propStream)
{
	if(ATTR_TELE_DEST == attr){
		TeleportDest* tele_dest;
		if(!propStream.GET_STRUCT(tele_dest)){
			return ATTR_READ_ERROR;
		}

		setDestPos(Position(tele_dest->_x, tele_dest->_y, tele_dest->_z));
		return ATTR_READ_CONTINUE;
	}
	else
		return Item::readAttr(attr, propStream);
}

bool Teleport::serializeAttr(PropWriteStream& propWriteStream) const
{
	bool ret = Item::serializeAttr(propWriteStream);

	propWriteStream.ADD_UCHAR(ATTR_TELE_DEST);

	TeleportDest tele_dest;

	tele_dest._x = destPos.x;
	tele_dest._y = destPos.y;
	tele_dest._z = (uint8_t)destPos.z;

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

ReturnValue Teleport::__queryRemove(const Thing* thing, uint32_t count, uint32_t flags) const
{
	return RET_NOERROR;
}

Cylinder* Teleport::__queryDestination(int32_t& index, const Thing* thing, Item** destItem,
	uint32_t& flags)
{
	return this;
}

void Teleport::__addThing(Creature* actor, Thing* thing)
{
	return __addThing(actor, 0, thing);
}

void Teleport::__addThing(Creature* actor, int32_t index, Thing* thing)
{
	Tile* destTile = g_game.getParentTile(getDestPos().x, getDestPos().y, getDestPos().z);
	if(destTile){
		if(Creature* creature = thing->getCreature()){
			getParentTile()->moveCreature(actor, creature, destTile, true);
			g_game.addMagicEffect(destTile->getPosition(), MAGIC_EFFECT_TELEPORT);
		}
		else if(Item* item = thing->getItem()){
			g_game.internalMoveItem(actor, getParentTile(), destTile, INDEX_WHEREEVER, item, item->getItemCount(), NULL);
		}
	}
}

void Teleport::__updateThing(Creature* actor, Thing* thing, uint16_t itemId, uint32_t count)
{
	//
}

void Teleport::__replaceThing(Creature* actor, uint32_t index, Thing* thing)
{
	//
}

void Teleport::__removeThing(Creature* actor, Thing* thing, uint32_t count)
{
	//
}

void Teleport::postAddNotification(Creature* actor, Thing* thing, const Cylinder* oldParent, int32_t index, cylinderlink_t link /*= LINK_OWNER*/)
{
	getParent()->postAddNotification(actor, thing, oldParent, index, LINK_PARENT);
}

void Teleport::postRemoveNotification(Creature* actor, Thing* thing, const Cylinder* newParent, int32_t index, bool isCompleteRemoval, cylinderlink_t link /*= LINK_OWNER*/)
{
	getParent()->postRemoveNotification(actor, thing, newParent, index, isCompleteRemoval, LINK_PARENT);
}

