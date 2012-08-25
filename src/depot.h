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

#ifndef __OTSERV_DEPOT_H__
#define __OTSERV_DEPOT_H__

#include "container.h"

class Position;

class Depot : public Container{
public:
	Depot(uint16_t _type);
	~Depot();

	virtual Depot* getDepot();
	virtual const Depot* getDepot() const;

	//serialization
	virtual Attr_ReadValue readAttr(AttrTypes_t attr, PropStream& propStream);

	uint32_t getDepotId() const;
	void setMaxDepotLimit(uint32_t maxitems);
	void setDepotId(uint32_t id);

	//cylinder implementations
	virtual Cylinder* getParent();
	virtual const Cylinder* getParent() const;
	virtual bool isRemoved() const;
	virtual Position getPosition() const;
	virtual Tile* getParentTile();
	virtual const Tile* getParentTile() const;
	virtual Item* getItem();
	virtual const Item* getItem() const;
	virtual Creature* getCreature();
	virtual const Creature* getCreature() const;
	virtual ReturnValue __queryAdd(int32_t index, const Thing* thing, uint32_t count,
		uint32_t flags) const;

	virtual ReturnValue __queryMaxCount(int32_t index, const Thing* thing, uint32_t count,
		uint32_t& maxQueryCount, uint32_t flags) const;

	virtual void postAddNotification(Creature* actor, Thing* thing, const Cylinder* oldParent, int32_t index, cylinderlink_t link = LINK_OWNER);
	virtual void postRemoveNotification(Creature* actor, Thing* thing, const Cylinder* newParent, int32_t index, bool isCompleteRemoval, cylinderlink_t link = LINK_OWNER);

	//overrides
	virtual bool canRemove() const;

private:
	uint32_t maxDepotLimit;
	uint32_t depotId;
};

#endif

