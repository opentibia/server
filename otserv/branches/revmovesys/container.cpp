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

#include "container.h"
#include "game.h"
#include "player.h"

extern Game g_game;

Container::Container(const uint16_t _type) : Item(_type)
{
	//std::cout << "Container constructor " << this << std::endl;
	maxSize = items[this->getID()].maxItems;
	depotId = 0;
}

Container::~Container()
{
	//std::cout << "Container destructor " << this << std::endl;
	for(ItemList::iterator cit = itemlist.begin(); cit != itemlist.end(); ++cit){
		
		(*cit)->setParent(NULL);
		(*cit)->releaseThing2();
	}
    
	itemlist.clear();
}

double Container::getWeight() const
{
	double weight = items[id].weight;
	std::list<const Container*> stack;

	ItemList::const_iterator it;
	stack.push_back(this);
	
	while(stack.size() > 0) {
		const Container* container = stack.front();
		stack.pop_front();

		for (it = container->getItems(); it != container->getEnd(); ++it) {
			Container* container = dynamic_cast<Container*>(*it);
			if(container) {
				stack.push_back(container);
				weight += items[container->getID()].weight;
			}
			else
				weight += (*it)->getWeight();
		}
	}

	return weight;
}

ItemList::const_iterator Container::getItems() const
{
	return itemlist.begin();
}

ItemList::const_iterator Container::getEnd() const
{
	return itemlist.end();
}

Item* Container::getItem(uint32_t index)
{
	size_t n = 0;			
	for (ItemList::const_iterator cit = getItems(); cit != getEnd(); ++cit) {
		if(n == index)
			return *cit;
		else
			++n;
	}

	return NULL;
}

uint32_t Container::getItemHoldingCount() const
{
	uint32_t holdcount = 0;

	std::list<const Container*> stack;
	stack.push_back(this);
	
	ItemList::const_iterator it;

	while(stack.size() > 0) {
		const Container *container = stack.front();
		stack.pop_front();

		for (it = container->getItems(); it != container->getEnd(); ++it) {
			Container *container = dynamic_cast<Container*>(*it);
			if(container) {
				stack.push_back(container);
			}

			++holdcount;
		}
	}

	return holdcount;
}

bool Container::isHoldingItem(const Item* item) const
{
	std::list<const Container*> stack;
	stack.push_back(this);
	
	ItemList::const_iterator it;

	while(stack.size() > 0) {
		const Container *container = stack.front();
		stack.pop_front();

		for (it = container->getItems(); it != container->getEnd(); ++it) {

			if(*it == item) {
				return true;
			}

			Container *containerIt = dynamic_cast<Container*>(*it);
			if(containerIt){
				stack.push_back(containerIt);
			}
		}
	}

	return false;
}

ReturnValue Container::__moveThingTo(Creature* creature, Cylinder* toCylinder, int32_t index, Thing* thing, uint32_t count)
{
	Item* item = dynamic_cast<Item*>(thing);
	if(item == NULL)
		return RET_NOTPOSSIBLE;

	Cylinder* subCylinder = dynamic_cast<Cylinder*>(toCylinder->__getThing(index));
	if(subCylinder){
		toCylinder = subCylinder;
		index = -1;
	}

	//check constraints before moving
	//
	//
	//
	
	uint32_t maxQueryCount = 0;
	ReturnValue ret = toCylinder->__queryMaxCount(index, item, count, maxQueryCount);
	if(ret != RET_NOERROR){
		return ret;
	}

	uint32_t m = std::min(count, maxQueryCount);
	if(m == 0){
		return RET_NOTENOUGHROOM;
	}

	if(item->isStackable()) {
		//Special condition if we are moving a stackable item in the same container
		//index will not be correct after removing "fromItem" if the fromIndex < index
		if(this == toCylinder && m == item->getItemCountOrSubtype()){
			int32_t fromIndex = __getIndexOfThing(item);
			if(fromIndex < index)
				index = index - 1;
		}

		ret = __removeThing(item, m);
		if(ret != RET_NOERROR)
			return ret;

		Item* toItem = dynamic_cast<Item*>(toCylinder->__getThing(index));
		if(toItem){
			if(toItem->getID() == item->getID()){
				uint32_t n = std::min((uint32_t)100 - toItem->getItemCountOrSubtype(), m);
				ret = toCylinder->__updateThing(toItem, toItem->getItemCountOrSubtype() + n);
				
				if(m - n > 0){
					Item* moveItem = Item::CreateItem(item->getID(), m - n);
					ret = toCylinder->__addThing(0, moveItem);
				}

				//todo: Fix leak here

				if(item->getItemCountOrSubtype() == 0){
					g_game.FreeThing(item);
				}
			}
			else{
				Item* moveItem = Item::CreateItem(item->getID(), m);

				if(index == -1)
					ret = toCylinder->__addThing(moveItem);
				else
					ret = toCylinder->__addThing(index, moveItem);
			}
		}
		else{
			//todo: check if exchange item if destination is a inventory cylinder

			Item* moveItem = Item::CreateItem(item->getID(), m);

			if(index == -1)
				ret = toCylinder->__addThing(moveItem);
			else
				ret = toCylinder->__addThing(index, moveItem);
		}

		if(ret != RET_NOERROR)
			return ret;
	}
	else{
		//todo: check if exchange item if destination is a inventory cylinder

		ReturnValue ret = __removeThing(item);

		if(ret != RET_NOERROR)
			return ret;

		if(index == -1)
			ret = toCylinder->__addThing(item);
		else
			ret = toCylinder->__addThing(index, item);

		if(ret != RET_NOERROR)
			return ret;
	}

	return RET_NOERROR;
}


ReturnValue Container::__queryMaxCount(int32_t index, const Thing* thing, uint32_t count, uint32_t& maxQueryCount)
{
	const Item* item = dynamic_cast<const Item*>(thing);
	if(item == NULL){
		maxQueryCount = 0;
		return RET_NOTPOSSIBLE;
	}

	uint32_t freeSlots = (capacity() - size());

	if(index != -1){
		if(item->isStackable()){
			Item* toItem = dynamic_cast<Item*>(__getThing(index));

			uint32_t n = 0;
			if(toItem && toItem->getID() == item->getID()){
				n = 100 - toItem->getItemCountOrSubtype();
			}

			maxQueryCount = freeSlots * 100 + n;

			if(maxQueryCount == 0)
				return RET_NOTENOUGHROOM;
			else 
				return RET_NOERROR;
		}
	}

	maxQueryCount = freeSlots;

	if(maxQueryCount == 0)
		return RET_NOTENOUGHROOM;
	else 
		return RET_NOERROR;
}

ReturnValue Container::__addThing(Thing* thing)
{
	return __addThing(0, thing);
}

ReturnValue Container::__addThing(uint32_t index, Thing* thing)
{
	if(index < 0 || index >= capacity())
		return RET_NOTPOSSIBLE;

	Item* item = dynamic_cast<Item*>(thing);
	if(item == NULL)
		return RET_NOTPOSSIBLE;

	if(itemlist.size() >= capacity())
		return RET_NOTENOUGHROOM;

	const Position& cylinderMapPos = getPosition();

	SpectatorVec list;
	SpectatorVec::iterator it;
	g_game.getSpectators(Range(cylinderMapPos, 2, 2, 2, 2, false), list);

	item->setParent(this);
	itemlist.push_front(item);

	//send to client
	for(it = list.begin(); it != list.end(); ++it) {
		Player* spectator = dynamic_cast<Player*>(*it);
		if(spectator){
			spectator->sendAddContainerItem(this, item);
		}
	}

	return RET_NOERROR;
}

ReturnValue Container::__updateThing(Thing* thing, uint32_t count)
{
	int32_t index = __getIndexOfThing(thing);
	if(index == -1){
#ifdef __DEBUG__
		std::cout << "Failure: [Container::__updateThing] item == NULL" << std::endl;
#endif
		return RET_NOTPOSSIBLE;
	}

	Item* item = dynamic_cast<Item*>(thing);
	if(item == NULL){
		return RET_NOTPOSSIBLE;
	}

	item->setItemCountOrSubtype(count);

	const Position& cylinderMapPos = getPosition();

	SpectatorVec list;
	SpectatorVec::iterator it;
	g_game.getSpectators(Range(cylinderMapPos, 2, 2, 2, 2, false), list);

	//send to client
	for(it = list.begin(); it != list.end(); ++it) {
		Player* spectator = dynamic_cast<Player*>(*it);
		if(spectator){
			spectator->sendUpdateContainerItem(this, index, item);
		}
	}

	return RET_NOERROR;
}

ReturnValue Container::__updateThing(uint32_t index, Thing* thing)
{
	Item* item = dynamic_cast<Item*>(thing);
	if(item == NULL){
		return RET_NOTPOSSIBLE;
	}

	ItemList::iterator cit = std::find(itemlist.begin(), itemlist.end(), thing);
	if(cit == itemlist.end())
		return RET_NOTPOSSIBLE;
	
	itemlist.insert(cit, item);
	item->setParent(this);

	(*cit)->releaseThing2(); //<- should release the memory?
	(*cit)->setParent(NULL);
	itemlist.erase(cit);

	const Position& cylinderMapPos = getPosition();

	SpectatorVec list;
	SpectatorVec::iterator it;
	g_game.getSpectators(Range(cylinderMapPos, 2, 2, 2, 2, false), list);

	//send to client
	for(it = list.begin(); it != list.end(); ++it) {
		Player* spectator = dynamic_cast<Player*>(*it);
		if(spectator){
			spectator->sendUpdateContainerItem(this, index, item);
		}
	}

	return RET_NOERROR;
}

ReturnValue Container::__removeThing(Thing* thing)
{
	int32_t index = __getIndexOfThing(thing);
	if(index == -1){
#ifdef __DEBUG__
		std::cout << "Failure: [Container::__removeThing] item == NULL" << std::endl;
#endif
		return RET_NOTPOSSIBLE;
	}

	ItemList::iterator cit = std::find(itemlist.begin(), itemlist.end(), thing);
	if(cit == itemlist.end())
		return RET_NOTPOSSIBLE;

	(*cit)->setParent(NULL);
	itemlist.erase(cit);

	const Position& cylinderMapPos = getPosition();

	SpectatorVec list;
	SpectatorVec::iterator it;
	g_game.getSpectators(Range(cylinderMapPos, 2, 2, 2, 2, false), list);

	//send change to client
	for(it = list.begin(); it != list.end(); ++it) {
		Player* spectator = dynamic_cast<Player*>(*it);
		if(spectator){
			spectator->sendRemoveContainerItem(this, index);
		}
	}

	return RET_NOERROR;
}

ReturnValue Container::__removeThing(Thing* thing, uint32_t count)
{
	int32_t index = __getIndexOfThing(thing);
	if(index == -1){
#ifdef __DEBUG__
		std::cout << "Failure: [Container::__removeThing] item == NULL" << std::endl;
#endif
		return RET_NOTPOSSIBLE;
	}

	ItemList::iterator cit = std::find(itemlist.begin(), itemlist.end(), thing);
	if(cit == itemlist.end())
		return RET_NOTPOSSIBLE;

	const Position& cylinderMapPos = getPosition();

	SpectatorVec list;
	SpectatorVec::iterator it;
	g_game.getSpectators(Range(cylinderMapPos, 2, 2, 2, 2, false), list);

	Item* item = *cit;
	if(item->isStackable()) {
		if(count == 0 || count > item->getItemCountOrSubtype())
			return RET_NOTPOSSIBLE;

		if(count == item->getItemCountOrSubtype()){
			(*cit)->setParent(NULL);
			itemlist.erase(cit);

			//send change to client
			for(it = list.begin(); it != list.end(); ++it) {
				Player* spectator = dynamic_cast<Player*>(*it);
				if(spectator){
					spectator->sendRemoveContainerItem(this, index);
				}
			}

			return RET_NOERROR;
		}
		else{
			item->setItemCountOrSubtype(item->getItemCountOrSubtype() - count);

			//send change to client
			for(it = list.begin(); it != list.end(); ++it) {
				Player* spectator = dynamic_cast<Player*>(*it);
				if(spectator){
					spectator->sendUpdateContainerItem(this, index, item);
				}
			}

			return RET_NOERROR;
		}
	}
	else{
		(*cit)->setParent(NULL);
		itemlist.erase(cit);

		//send change to client
		for(it = list.begin(); it != list.end(); ++it) {
			Player* spectator = dynamic_cast<Player*>(*it);
			if(spectator){
				spectator->sendRemoveContainerItem(this, index);
			}
		}

		return RET_NOERROR;
	}
	
	return RET_NOTPOSSIBLE;
}

int32_t Container::__getIndexOfThing(const Thing* thing) const
{
	uint32_t index = 0;
	for(ItemList::const_iterator cit = getItems(); cit != getEnd(); ++cit){
		if(*cit == thing)
			return index;
		else
			++index;
	}

	return -1;
}

Thing* Container::__getThing(uint32_t index)
{
	int count = 0;
	for(ItemList::const_iterator cit = itemlist.begin(); cit != itemlist.end(); ++cit) {
		if(count == index)
			return *cit;
		else
			++count;
	}

	return NULL;
}

void Container::__internalAddThing(Thing* thing)
{
	__internalAddThing(0, thing);
}

void Container::__internalAddThing(uint32_t index, Thing* thing)
{
#ifdef __DEBUG__
	std::cout << "[Container::__internalAddThing] index: " << index << std::endl;
#endif

	Item* item = dynamic_cast<Item*>(thing);
	if(item == NULL){
#ifdef __DEBUG__
		std::cout << "Failure: [Container::__internalAddThing] item == NULL" << std::endl;
#endif
		return;
	}

	if(index < 0 || index >= capacity()){
#ifdef __DEBUG__
		std::cout << "Failure: [Container::__internalAddThing] - index is out of range" << std::endl;
#endif
		return;
	}

	itemlist.push_front(item);
	item->setParent(this);
}
