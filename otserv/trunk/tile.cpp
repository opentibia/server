//////////////////////////////////////////////////
// a Tile represents a single field on the map.
// it is a list of Items


#include "definitions.h"

#include <string>
#include <iostream>

#include "tile.h"

#include "creature.h"
#include "item.h"

int Tile::getStackPosItem(){
	int pos=1;
/*	if(size()<=1)
		return 0;
	if(creature && size()>=2)
		pos++;
	Tile::iterator it=end();
	it--;
	if(size()>=3 && (*it)->isAlwaysOnTop())
		pos++;
	std::cout << "stackpos of top item is "<< pos << std::endl; */
	return pos;
}


bool Tile::isBlocking()
{
  ItemVector::iterator iit;
  for (iit = topItems.begin(); iit != topItems.end(); iit++)
    if ((*iit)->isBlocking())
      return true;

  for (iit = downItems.begin(); iit != downItems.end(); iit++)
    if ((*iit)->isBlocking())
      return true;


  return false;
}

int Tile::removeItem(int stack, int type, int count){
	//returns how much items are left
	//TODO
	Item* item=getItemByStack(stack);
	int itemsleft = item->count-count;
	if(itemsleft<=0){
		itemsleft=0;
		removeItemByStack(stack);
	}
	item->count=itemsleft;
	return itemsleft;
}


Item* Tile::getItemByStack(int stack){
/*	if(stack==0)
		return(*getItToPos(0));
	//if theres a creature on this tile...
	if((*(getItToPos(size()-1)))->isAlwaysOnTop()){
		if(stack==1)
			return *(getItToPos(size()-1));
		else{
			if(this->creature)
				if(stack==2)
					return NULL;
				else
					return *(getItToPos(size()-(stack-1)));
			else
				return *(getItToPos(size()-stack));
		}
	}
	else{
		if(this->creature)
			if(stack==1){
				return NULL;
			}
			else{
				return *(getItToPos(size()-(stack-1)));
			}
		else{
			return *(getItToPos(size()-stack));
		}
	}
	return NULL;
  */
  return NULL;
}

int Tile::removeItemByStack(int stack){
/*	if(stack==0)
		erase(getItToPos(0));
	//if theres a creature on this tile...
	else if((*(getItToPos(size()-1)))->isAlwaysOnTop()){
		if(stack==1)
			erase ((getItToPos(size()-1)));
		else{
			if(this->creature)
				if(stack==2)
					return 0;
				else
					erase((getItToPos(size()-(stack-1))));
			else
				erase( (getItToPos(size()-stack)));
		}
	}
	else{
		if(this->creature)
			if(stack==1){
				return 0;
			}
			else{
				erase((getItToPos(size()-(stack-1))));
			}
		else{
			erase( (getItToPos(size()-stack)));
		}
	}
	return 0;*/

  return 0;
}

int Tile::getCreatureStackPos(Creature *c)
{
  CreatureVector::iterator it;
  for (it = creatures.begin(); it != creatures.end(); it++)
  {
    if ((*it) == c)
      return (int) ((it - creatures.begin()) + 1 + topItems.size());
  }

  /* todo error */
  return 255;
}

int Tile::getThingStackPos(Thing *thing)
{
  int n = 0;

  ItemVector::iterator iit;
  for (iit = topItems.begin(); iit != topItems.end(); iit++)
  {
    n++;
    if ((*iit) == thing)
      return n;
  }

  CreatureVector::iterator cit;
  for (cit = creatures.begin(); cit != creatures.end(); cit++)
  {
    n++;
    if ((*cit) == thing)
      return n;
  }

  for (iit = downItems.begin(); iit != downItems.end(); iit++)
  {
    n++;
    if ((*iit) == thing)
      return n;
  }

  /* todo error */
  return 255;
}

Thing* Tile::getThingByStackPos(int pos)
{
  if (pos == 0)
    return &ground;

  pos--;

  if (pos < topItems.size())
    return topItems[pos];

  pos -= topItems.size();

  if (pos < creatures.size())
    return creatures[pos];

  pos -= creatures.size();

  if (pos < downItems.size())
    return downItems[pos];

  return NULL;
}

int Tile::getThingCount()
{
	return 1 + topItems.size() + 	creatures.size() + downItems.size();
}

std::string Tile::getDescription(){
/*	std::string ret;
	std::cout << "Items: "<< size() << std::endl;
	for(unsigned int i=0; i < size(); i++)
		std::cout << "ID: "<< (*this)[i]->getID() << std::endl;
	Tile::iterator it;
	it=end();
	it--;
	ret=(*it)->getDescription();*/
  std::string ret="You dont know why, but you cant see anything!";
	return ret;
}


bool Tile::removeThing(Thing *thing)
{
	Creature* creature = dynamic_cast<Creature*>(thing);
	if (creature) {
    CreatureVector::iterator it;
    for (it = creatures.begin(); it != creatures.end(); it++)
      if (*it == thing)
      {
        creatures.erase(it);
        return true;
      }
  }
  else
  {
    ItemVector::iterator it;
    Item *item = (Item*)thing;

    if (item->isAlwaysOnTop())
    {
      for (it = topItems.begin(); it != topItems.end(); it++)
        if (*it == item)
        {
          topItems.erase(it);
          return true;
        }
    }
    else
    {
      for (it = downItems.begin(); it != downItems.end(); it++)
        if (*it == item)
        {
          downItems.erase(it);
          return true;
        }
    }
  }

  return false;
}


void Tile::addThing(Thing *thing) {
	Creature* creature = dynamic_cast<Creature*>(thing);
	if (creature) {
    creatures.insert(creatures.begin(), creature);
  }
  else
  {
    Item *item = (Item*)thing;
    if (item->isGroundTile())
    {
      ground = *item;
    }
    else if (item->isAlwaysOnTop())
    {
      topItems.insert(topItems.begin(), item);
    }
    else
    {
      downItems.insert(downItems.begin(), item);
    }
  }
}

bool Tile::isPz()
{
  return pz;
}

void Tile::setPz()
{
  pz = true;
}
