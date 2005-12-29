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

#include "depot.h"

Depot::Depot(uint16_t _type) :
Container(_type)
{
	depotId = 0;
	maxSize = 30;
	maxDepotLimit = 1500;
}

Depot::~Depot()
{
	//
}

ReturnValue Depot::__queryAdd(int32_t index, const Thing* thing, uint32_t count,
	bool childIsOwner /*= false*/) const
{
	const Item* item = thing->getItem();
	if(item == NULL){
		return RET_NOTPOSSIBLE;
	}

	int addCount = 0;

	if((item->isStackable() && item->getItemCount() != count)){
		addCount = 1;
	}

	if(item->getTopParent() != this){	
		if(const Container* container = item->getContainer()){
			addCount = container->getItemHoldingCount() + 1;
		}
		else
			addCount = 1;
	}

	if(getItemHoldingCount() + addCount >= maxDepotLimit){
		return RET_DEPOTISFULL;
	}

	return Container::__queryAdd(index, thing, count, childIsOwner);
}
