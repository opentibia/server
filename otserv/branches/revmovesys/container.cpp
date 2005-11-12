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

Container::Container(const unsigned short _type) : Item(_type)
{
	//std::cout << "Container constructor " << this << std::endl;
	maxitems = items[this->getID()].maxItems;
	actualitems = 0;
	parent = NULL;
	depot = 0;
	useCount = 0;
}

Container::~Container()
{
	//std::cout << "Container destructor " << this << std::endl;
	for(ContainerList::iterator cit = lcontained.begin(); cit != lcontained.end(); ++cit)
	{
    	Container* container = dynamic_cast<Container*>(*cit);
    	if(container)
    		container->setParent(NULL);
    	(*cit)->releaseThing();
  	}
    
	lcontained.clear();
}

bool Container::addItem(Item *newitem) {
	//first check if we are a container, there is an item to be added and if we can add more items...
	//if (!iscontainer) throw TE_NoContainer();
	if (newitem == NULL)
		return false;

	// seems we should add the item...
	// new items just get placed in front of the items we already have...
	if(lcontained.size() < maxitems) {
		newitem->pos.x = 0xFFFF;

		//FIXME: is this correct? i dont get what it does. tliff
		Container* container = dynamic_cast<Container*>(newitem); 
		if(container) {
			container->setParent(this);
		}

		lcontained.push_front(newitem);

		// increase the itemcount
		++actualitems;
		return true;
	}

	return false;
}

bool Container::removeItem(Item* item)
{
	for (ContainerList::iterator cit = lcontained.begin(); cit != lcontained.end(); cit++) {
		if((*cit) == item) {

			Container* container = dynamic_cast<Container*>(*cit); 
			if(container) {
				container->setParent(NULL);
			}

			lcontained.erase(cit);
			--actualitems;
			return true;
		}
	}

	return false;
}

void Container::moveItem(unsigned char from_slot, unsigned char to_slot)
{
	int n = 0;
	for (ContainerList::iterator cit = lcontained.begin(); cit != lcontained.end(); ++cit) {
		if(n == from_slot) {
			Item *item = (*cit);
			lcontained.erase(cit);
			lcontained.push_front(item);
			break;
		}
		++n;
	}
}

Item* Container::getItem(unsigned long slot_num)
{
	size_t n = 0;			
	for (ContainerList::const_iterator cit = getItems(); cit != getEnd(); ++cit) {
		if(n == slot_num)
			return *cit;
		else
			++n;
	}

	return NULL;
}

const Item* Container::getItem(unsigned long slot_num) const
{
	size_t n = 0;			
	for (ContainerList::const_iterator cit = getItems(); cit != getEnd(); ++cit) {
		if(n == slot_num)
			return *cit;
		else
			++n;
	}

	return NULL;
}

unsigned char Container::getSlotNumberByItem(const Item* item) const
{
	unsigned char n = 0;			
	for (ContainerList::const_iterator cit = getItems(); cit != getEnd(); ++cit) {
		if(*cit == item)
			return n;
		else
			++n;
	}

	return 0xFF;
}

long Container::getItemHoldingCount() const
{
	int holdcount = 0;

	std::list<const Container*> stack;
	stack.push_back(this);
	
	ContainerList::const_iterator it;

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
	
	ContainerList::const_iterator it;

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

double Container::getWeight() const
{
	double weight = items[id].weight;
	std::list<const Container*> stack;

	ContainerList::const_iterator it;
	stack.push_back(this);
	
	while(stack.size() > 0) {
		const Container *container = stack.front();
		stack.pop_front();

		for (it = container->getItems(); it != container->getEnd(); ++it) {
			Container *container = dynamic_cast<Container*>(*it);
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

ContainerList::const_iterator Container::getItems() const {
	return lcontained.begin();
}

ContainerList::const_iterator Container::getEnd() const {
	return lcontained.end();
}

Container *Container::getTopParent()
{
	if(getParent() == NULL)
		return this;

	Container *aux = this->getParent();
	while(aux->getParent() != NULL) {
		aux = aux->getParent();
	}
	return aux;
}

const Container *Container::getTopParent() const
{
	if(getParent() == NULL)
		return this;

	Container *aux = this->getParent();

	while(aux->getParent() != NULL) {
		aux = aux->getParent();
	}
	return aux;
}
