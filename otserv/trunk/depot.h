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
#ifndef __DEPOT_H__
#define __DEPOT_H__

#include "container.h"

class Depot : public Container{
public:
	Depot(uint16_t _type);
	~Depot();

	uint32_t getDepotId() {return depotId;};
	void setMaxDepotLimit(uint32_t maxitems) {maxDepotLimit = maxitems;};
	void setDepotId(uint32_t id) {depotId = id;};
	virtual Depot* getDepot() {return this;};
	virtual const Depot* getDepot() const {return this;};

	virtual ReturnValue __queryAdd(int32_t index, const Thing* thing, uint32_t count,
		bool childIsOwner = false) const;

private:
	uint32_t maxDepotLimit;
	uint32_t depotId;
};

#endif

