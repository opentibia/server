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
#include <sstream>

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

bool Depot::unserialize(xmlNodePtr nodeItem)
{
	bool ret = Container::unserialize(nodeItem);

	char* nodeValue;
	nodeValue = (char*)xmlGetProp(nodeItem, (const xmlChar *) "depot");
	if(nodeValue){
		setDepotId(atoi(nodeValue));
		xmlFreeOTSERV(nodeValue);
	}
	
	return ret;
}

xmlNodePtr Depot::serialize()
{
	xmlNodePtr xmlptr = Container::serialize();

	std::stringstream ss;
	ss.str("");
	ss << (int) depotId;
	xmlSetProp(xmlptr, (const xmlChar*)"depot", (const xmlChar*)ss.str().c_str());

	return xmlptr;
}

bool Depot::readAttr(AttrTypes_t attr, PropStream& propStream)
{
	if(ATTR_DEPOT_ID == attr){
		unsigned short _depotId;
		if(!propStream.GET_USHORT(_depotId)){
			return false;
		}
		
		setDepotId(_depotId);
		return true;
	}
	else
		return Item::readAttr(attr, propStream);
}

ReturnValue Depot::__queryAdd(int32_t index, const Thing* thing, uint32_t count,
	uint32_t flags) const
{
	const Item* item = thing->getItem();
	if(item == NULL){
		return RET_NOTPOSSIBLE;
	}

	bool skipLimit = ((flags & FLAG_NOLIMIT) == FLAG_NOLIMIT);
	
	if(!skipLimit){
		int addCount = 0;

		if((item->isStackable() && item->getItemCount() != count)){
			addCount = 1;
		}

		if(item->getTopParent() != this){
			if(const Container* container = item->getContainer()){
				addCount = container->getItemHoldingCount() + 1;
			}
			else{
				addCount = 1;
			}
		}

		if(getItemHoldingCount() + addCount > maxDepotLimit){
			return RET_DEPOTISFULL;
		}
	}

	return Container::__queryAdd(index, thing, count, flags);
}

ReturnValue Depot::__queryMaxCount(int32_t index, const Thing* thing, uint32_t count,
	uint32_t& maxQueryCount, uint32_t flags) const
{
	return Container::__queryMaxCount(index, thing, count, maxQueryCount, flags);
}

void Depot::postAddNotification(Thing* thing, int32_t index, cylinderlink_t link /*= LINK_OWNER*/)
{
	if(getParent() != NULL){
		//getParent()->postAddNotification(thing, index, false /*hasOwnership*/);
		getParent()->postAddNotification(thing, index, LINK_PARENT);
	}
}

void Depot::postRemoveNotification(Thing* thing, int32_t index, bool isCompleteRemoval, cylinderlink_t link /*= LINK_OWNER*/)
{
	if(getParent() != NULL){
		//getParent()->postRemoveNotification(thing, index, isCompleteRemoval, false /*hadOwnership*/);
		getParent()->postRemoveNotification(thing, index, isCompleteRemoval, LINK_PARENT);
	}
}
