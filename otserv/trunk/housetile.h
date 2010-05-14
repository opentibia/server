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

#ifndef __HOUSETILE_H__
#define __HOUSETILE_H__

#include "definitions.h"
#include "tile.h"

class House;

// House tiles are almost always dynamic
class HouseTile : public DynamicTile
{
public:
	HouseTile(int x, int y, int z, House* _house);
	~HouseTile();

	//cylinder implementations
	virtual ReturnValue __queryAdd(int32_t index, const Thing* thing, uint32_t count,
		uint32_t flags) const;
	
	virtual Cylinder* __queryDestination(int32_t& index, const Thing* thing, Item** destItem,
		uint32_t& flags);
	
	virtual void __addThing(int32_t index, Thing* thing);
	virtual void __internalAddThing(uint32_t index, Thing* thing);

	House* getHouse() {return house;};

private:
	void updateHouse(Item* item);

	House* house;
};

#endif
