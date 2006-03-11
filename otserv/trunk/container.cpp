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

Container::Container(uint16_t _type) : Item(_type)
{
	//std::cout << "Container constructor " << this << std::endl;
	maxSize = items[this->getID()].maxItems;
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
			Container* container = (*it)->getContainer();
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
	uint32_t counter = 0;

	std::list<const Container*> stack;
	stack.push_back(this);
	
	ItemList::const_iterator it;

	while(stack.size() > 0){
		const Container* container = stack.front();
		stack.pop_front();

		for(it = container->getItems(); it != container->getEnd(); ++it) {
			Container* container = (*it)->getContainer();
			if(container) {
				stack.push_back(container);
			}

			++counter;
		}
	}

	return counter;
}

bool Container::isHoldingItem(const Item* item) const
{
	std::list<const Container*> stack;
	stack.push_back(this);
	
	ItemList::const_iterator it;

	while(stack.size() > 0){
		const Container* container = stack.front();
		stack.pop_front();

		for(it = container->getItems(); it != container->getEnd(); ++it){
			if(*it == item){
				return true;
			}

			Container* containerIt = (*it)->getContainer();
			if(containerIt){
				stack.push_back(containerIt);
			}
		}
	}

	return false;
}

void Container::onAddContainerItem(Item* item)
{
	const Position& cylinderMapPos = getPosition();

	SpectatorVec list;
	SpectatorVec::iterator it;
	g_game.getSpectators(Range(cylinderMapPos, 2, 2, 2, 2, false), list);

	//send to client
	Player* player = NULL;
	for(it = list.begin(); it != list.end(); ++it) {
		if(player = (*it)->getPlayer()){
			player->onAddContainerItem(this, item);
		}
	}
}

void Container::onUpdateContainerItem(uint32_t index, Item* olditem, Item* newitem)
{
	const Position& cylinderMapPos = getPosition();

	SpectatorVec list;
	SpectatorVec::iterator it;
	g_game.getSpectators(Range(cylinderMapPos, 2, 2, 2, 2, false), list);

	//send to client
	Player* player = NULL;
	for(it = list.begin(); it != list.end(); ++it) {
		if(player = (*it)->getPlayer()){
			player->onUpdateContainerItem(this, index, olditem, newitem);
		}
	}
}

void Container::onRemoveContainerItem(uint32_t index, Item* item)
{
	const Position& cylinderMapPos = getPosition();

	SpectatorVec list;
	SpectatorVec::iterator it;
	g_game.getSpectators(Range(cylinderMapPos, 2, 2, 2, 2, false), list);

	//send change to client
	Player* player = NULL;
	for(it = list.begin(); it != list.end(); ++it) {
		if(player = (*it)->getPlayer()){
			player->onRemoveContainerItem(this, index, item);
		}
	}
}

ReturnValue Container::__queryAdd(int32_t index, const Thing* thing, uint32_t count,
	uint32_t flags) const
{
	bool childIsOwner = ((flags & FLAG_CHILDISOWNER) == FLAG_CHILDISOWNER);
	if(childIsOwner){
		//a child container is querying, since we are the top container (not carried by a player)
		//just return with no error.
		return RET_NOERROR;
	}

	const Item* item = thing->getItem();
	if(item == NULL){
		return RET_NOTPOSSIBLE;
	}

	if(!item->isPickupable()){
		return RET_CANNOTPICKUP;
	}

	if(item == this){
		return RET_THISISIMPOSSIBLE;
	}

	const Cylinder* cylinder = getParent();
	while(cylinder){
		if(cylinder == thing){
			return RET_THISISIMPOSSIBLE;
		}
		cylinder = cylinder->getParent();
	}
	
	bool skipLimit = ((flags & FLAG_NOLIMIT) == FLAG_NOLIMIT);

	if(index == INDEX_WHEREEVER && !skipLimit){
		if(size() >= capacity())
			return RET_CONTAINERNOTENOUGHROOM;
	}

	const Cylinder* topParent = getTopParent();
	if(topParent != this){
		return topParent->__queryAdd(INDEX_WHEREEVER, item, count, flags | FLAG_CHILDISOWNER);
	}
	else
		return RET_NOERROR;
}

ReturnValue Container::__queryMaxCount(int32_t index, const Thing* thing, uint32_t count,
	uint32_t& maxQueryCount, uint32_t flags) const
{
	const Item* item = thing->getItem();
	if(item == NULL){
		maxQueryCount = 0;
		return RET_NOTPOSSIBLE;
	}

	if( ((flags & FLAG_NOLIMIT) == FLAG_NOLIMIT) ){
		maxQueryCount = std::max((uint32_t)1, count);
		return RET_NOERROR;
	}

	uint32_t freeSlots = (capacity() - size());

	if(item->isStackable()){
		uint32_t n = 0;
		
		if(index != INDEX_WHEREEVER){
			const Thing* destThing = __getThing(index);
			const Item* destItem = NULL;
			if(destThing)
				destItem = destThing->getItem();

			if(destItem && destItem->getID() == item->getID()){
				n = 100 - destItem->getItemCount();
			}
		}

		maxQueryCount = freeSlots * 100 + n;

		if(maxQueryCount < count){
			return RET_CONTAINERNOTENOUGHROOM;
		}
	}
	else{
		maxQueryCount = freeSlots;

		if(maxQueryCount == 0){
			return RET_CONTAINERNOTENOUGHROOM;
		}
	}

	return RET_NOERROR;
}

ReturnValue Container::__queryRemove(const Thing* thing, uint32_t count) const
{
	uint32_t index = __getIndexOfThing(thing);

	if(index == -1){
		return RET_NOTPOSSIBLE;
	}

	const Item* item = thing->getItem();
	if(item == NULL){
		return RET_NOTPOSSIBLE;
	}

	if(count == 0 || (item->isStackable() && count > item->getItemCount())){
		return RET_NOTPOSSIBLE;
	}

	if(item->isNotMoveable()){
		return RET_NOTMOVEABLE;
	}

	return RET_NOERROR;
}

Cylinder* Container::__queryDestination(int32_t& index, const Thing* thing, Item** destItem,
	uint32_t& flags)
{
	if(index == 254 /*move up*/){
		index = INDEX_WHEREEVER;
		*destItem = NULL;
		
		Container* parentContainer = dynamic_cast<Container*>(getParent());
		if(parentContainer)
			return parentContainer;
		else
			return this;
	}
	else if(index == 255 /*add wherever*/){
		index = INDEX_WHEREEVER;
		*destItem = NULL;
		return this;
	}
	else{
		if(index >= (int32_t)capacity()){
			/*
			if you have a container, maximize it to show all 20 slots
			then you open a bag that is inside the container you will have a bag with 8 slots
			and a "grey" area where the other 12 slots where from the container
			if you drop the item on that grey area
			the client calculates the slot position as if the bag has 20 slots
			*/

			index = INDEX_WHEREEVER;
		}

		if(index != INDEX_WHEREEVER){
			Thing* destThing = __getThing(index);
			if(destThing)
				*destItem = destThing->getItem();

			Cylinder* subCylinder = dynamic_cast<Cylinder*>(*destItem);

			if(subCylinder){
				index = INDEX_WHEREEVER;
				*destItem = NULL;
				return subCylinder;
			}
		}
	}
	
	return this;
}

void Container::__addThing(Thing* thing)
{
	return __addThing(0, thing);
}

void Container::__addThing(int32_t index, Thing* thing)
{
	if(index >= (int32_t)capacity()){
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Container::__addThing], index:" << index << ", index >= capacity()" << std::endl;
		DEBUG_REPORT
#endif
		return /*RET_NOTPOSSIBLE*/;
	}

	Item* item = thing->getItem();
	if(item == NULL){
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Container::__addThing] item == NULL" << std::endl;
		DEBUG_REPORT
#endif
		return /*RET_NOTPOSSIBLE*/;
	}

#ifdef __DEBUG__MOVESYS__
	if(index != INDEX_WHEREEVER)
		if(size() >= capacity()){
			std::cout << "Failure: [Container::__addThing] size() >= capacity()" << std::endl;
			DEBUG_REPORT
			return /*RET_CONTAINERNOTENOUGHROOM*/;
		}
	}
#endif

	item->setParent(this);
	itemlist.push_front(item);

	//send change to client
	if(getParent()){
		onAddContainerItem(item);
	}
}

void Container::__updateThing(Thing* thing, uint32_t count)
{
	int32_t index = __getIndexOfThing(thing);
	if(index == -1){
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Container::__updateThing] index == -1" << std::endl;
		DEBUG_REPORT
#endif
		return /*RET_NOTPOSSIBLE*/;
	}

	Item* item = thing->getItem();
	if(item == NULL){
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Container::__updateThing] item == NULL" << std::endl;
		DEBUG_REPORT
#endif
		return /*RET_NOTPOSSIBLE*/;
	}

	item->setItemCountOrSubtype(count);

	//send change to client
	if(getParent()){
		onUpdateContainerItem(index, item, item);
	}
}

void Container::__replaceThing(uint32_t index, Thing* thing)
{
	Item* item = thing->getItem();
	if(item == NULL){
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Container::__replaceThing] item == NULL" << std::endl;
		DEBUG_REPORT
#endif
		return /*RET_NOTPOSSIBLE*/;
	}

	uint32_t count = 0;
	ItemList::iterator cit = itemlist.end();
	for(cit = itemlist.begin(); cit != itemlist.end(); ++cit){
		if(count == index)
			break;
		else
			++count;
	}

	if(cit == itemlist.end()){
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Container::__updateThing] item not found" << std::endl;
		DEBUG_REPORT
#endif
		return /*RET_NOTPOSSIBLE*/;
	}
	
	itemlist.insert(cit, item);
	item->setParent(this);

	//send change to client
	if(getParent()){
		onUpdateContainerItem(index, *cit, item);
	}

	(*cit)->setParent(NULL);
	itemlist.erase(cit);
}

void Container::__removeThing(Thing* thing, uint32_t count)
{
	Item* item = thing->getItem();
	if(item == NULL){
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Container::__removeThing] item == NULL" << std::endl;
		DEBUG_REPORT
#endif
		return /*RET_NOTPOSSIBLE*/;
	}

	int32_t index = __getIndexOfThing(thing);
	if(index == -1){
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Container::__removeThing] index == -1" << std::endl;
		DEBUG_REPORT
#endif
		return /*RET_NOTPOSSIBLE*/;
	}

	ItemList::iterator cit = std::find(itemlist.begin(), itemlist.end(), thing);
	if(cit == itemlist.end()){
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Container::__removeThing] item not found" << std::endl;
		DEBUG_REPORT
#endif
		return /*RET_NOTPOSSIBLE*/;
	}

	if(item->isStackable() && count != item->getItemCount()){
		int newCount = std::max(0, (int)(item->getItemCount() - count));
		item->setItemCount(newCount);

		//send change to client
		if(getParent()){
			onUpdateContainerItem(index, item, item);
		}
	}
	else{
		//send change to client
		if(getParent()){
			onRemoveContainerItem(index, item);
		}

		item->setParent(NULL);
		itemlist.erase(cit);
	}
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

Thing* Container::__getThing(uint32_t index) const
{
	if(index < 0 || index > size())
		return NULL;

	int count = 0;
	for(ItemList::const_iterator cit = itemlist.begin(); cit != itemlist.end(); ++cit) {
		if(count == index)
			return *cit;
		else
			++count;
	}

	return NULL;
}

void Container::postAddNotification(Thing* thing, bool hasOwnership /*= true*/)
{
	Cylinder* topParent = getTopParent();

	if(topParent->getCreature()){
		topParent->postAddNotification(thing, true /*hasOwnership*/);
	}
	else{
		if(topParent == this){
			//let the tile class notify surrounding players
			topParent->getParent()->postAddNotification(thing, false /*hasOwnership*/);
		}
		else
			topParent->postAddNotification(thing, false /*hasOwnership*/);
	}

	//getParent()->postAddNotification(thing, true /*hasOwnership*/);
}

void Container::postRemoveNotification(Thing* thing, bool isCompleteRemoval, bool hadOwnership /*= true*/)
{
	Cylinder* topParent = getTopParent();

	if(topParent->getCreature()){
		topParent->postRemoveNotification(thing, isCompleteRemoval, true /*hasOwnership*/);
	}
	else{
		if(topParent == this){
			//let the tile class notify surrounding players
			topParent->getParent()->postRemoveNotification(thing, isCompleteRemoval, false /*hasOwnership*/);
		}
		else
			topParent->postRemoveNotification(thing, isCompleteRemoval, false /*hasOwnership*/);
	}

	//getParent()->postRemoveNotification(thing, isCompleteRemoval, false /*hadOwnership*/);
}

void Container::__internalAddThing(Thing* thing)
{
	__internalAddThing(0, thing);
}

void Container::__internalAddThing(uint32_t index, Thing* thing)
{
#ifdef __DEBUG__MOVESYS__NOTICE
	std::cout << "[Container::__internalAddThing] index: " << index << std::endl;
#endif

	Item* item = thing->getItem();
	if(item == NULL){
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Container::__internalAddThing] item == NULL" << std::endl;
#endif
		return;
	}

	/*
	if(index < 0 || index >= capacity()){
#ifdef __DEBUG__
		std::cout << "Failure: [Container::__internalAddThing] - index is out of range" << std::endl;
#endif
		return;
	}
	*/

	itemlist.push_front(item);
	item->setParent(this);
}
