//////////////////////////////////////////////////
// a Tile represents a single field on the map.
// it is a list of Items


#include "definitions.h"

#include <string>
#include <iostream>

#include "tile.h"

#include "creature.h"

bool Tile::isBlockingProjectile() const
{
	if(ground && ground->isBlockingProjectile() == true)
    return true;
  
  ItemVector::const_iterator iit;
  for (iit = topItems.begin(); iit != topItems.end(); ++iit)
    if ((*iit)->isBlockingProjectile())
      return true;

  for (iit = downItems.begin(); iit != downItems.end(); ++iit)
    if ((*iit)->isBlockingProjectile())
      return true;


  return false;
}

bool Tile::isBlocking() const
{
  if(ground && ground->isBlocking() == true)
    return true;
  
	ItemVector::const_iterator iit;
  for (iit = topItems.begin(); iit != topItems.end(); ++iit)
    if ((*iit)->isBlocking())
      return true;

  for (iit = downItems.begin(); iit != downItems.end(); ++iit)
    if ((*iit)->isBlocking())
      return true;


  return false;
}
bool Tile::floorChange(){
  ItemVector::iterator iit;
  if(ground && !(ground->noFloorChange() == true))
    return true;
  for (iit = topItems.begin(); iit != topItems.end(); ++iit){  
         if ((*iit)->floorChangeNorth() || (*iit)->floorChangeSouth() || (*iit)->floorChangeEast() || (*iit)->floorChangeWest())
         return true;      
      }

  for (iit = downItems.begin(); iit != downItems.end(); ++iit){ 
         if ((*iit)->floorChangeNorth() || (*iit)->floorChangeSouth() || (*iit)->floorChangeEast() || (*iit)->floorChangeWest())
         return true;
      }


  return false;
     }
bool Tile::floorChange(Direction direction)
{
  
  ItemVector::iterator iit;
  for (iit = topItems.begin(); iit != topItems.end(); ++iit){
    if(direction == NORTH){  
         if ((*iit)->floorChangeNorth())
         return true;
      }
    else if(direction == SOUTH){
         if ((*iit)->floorChangeSouth())
         return true;
         }
    else if(direction == EAST){
         if ((*iit)->floorChangeEast())
         return true;
         }
    else if(direction == WEST){
         if ((*iit)->floorChangeWest())
         return true;
         }           
      }

  for (iit = downItems.begin(); iit != downItems.end(); ++iit){
    if(direction == NORTH){  
         if ((*iit)->floorChangeNorth())
         return true;
      }
    else if(direction == SOUTH){
         if ((*iit)->floorChangeSouth())
         return true;
         }
    else if(direction == EAST){
         if ((*iit)->floorChangeEast())
         return true;
         }
    else if(direction == WEST){
         if ((*iit)->floorChangeWest())
         return true;
         } 
      }


  return false;
}

int Tile::getCreatureStackPos(Creature *c) const
{
  CreatureVector::const_iterator it;
  for (it = creatures.begin(); it != creatures.end(); ++it)
  {
    if ((*it) == c)
      return (int) ((it - creatures.begin()) + 1 + topItems.size()) + (splash ? 1 : 0);
  }

  /* todo error */
  return 255;
}

int Tile::getThingStackPos(const Thing *thing) const
{
  int n = 0;

  if (splash)
  {
    if (thing == splash)
      return 1;
    n++;
  }
  
  ItemVector::const_iterator iit;
  for (iit = topItems.begin(); iit != topItems.end(); ++iit)
  {
    n++;
    if ((*iit) == thing)
      return n;
  }

  CreatureVector::const_iterator cit;
  for (cit = creatures.begin(); cit != creatures.end(); ++cit)
  {
    n++;
    if ((*cit) == thing)
      return n;
  }

  for (iit = downItems.begin(); iit != downItems.end(); ++iit)
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
    return ground;

  pos--;

  if (splash)
  {
    if (pos == 0)
      return splash;
      //return NULL;
    pos--;
  }

  if ((unsigned) pos < topItems.size())
    return topItems[pos];

  pos -= (uint32_t)topItems.size();

  if ((unsigned) pos < creatures.size())
    return creatures[pos];

  pos -= (uint32_t)creatures.size();

  if ((unsigned) pos < downItems.size())
    return downItems[pos];

  return NULL;
}

int Tile::getThingCount() const
{
  return (uint32_t) 1 + (splash ? 1 : 0) + topItems.size() +	creatures.size() + downItems.size();
}

std::string Tile::getDescription() const
{
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

bool Tile::insertThing(Thing *thing, int stackpos)
{
	Item *item = (Item*)thing;
	int pos = stackpos;

	if (pos == 0) {
    //ground = item;
		return false;
	}

  pos--;

  if(splash)
  {
		if (pos == 0) {
      //splash = item;
			return false;
		}

    pos--;
  }

	if ((unsigned) pos < topItems.size()) {		
		ItemVector::iterator it = topItems.begin();
		while(pos > 0){
			pos--;
			++it;
		}
		topItems.insert(it, item);
		return true;
	}

  pos -= (uint32_t)topItems.size();

	if ((unsigned) pos < creatures.size()) {
		//creatures.insert(creatures[pos], item);
		return false;
	}

  pos -= (uint32_t)creatures.size();

	if ((unsigned) pos < downItems.size()) {
    	ItemVector::iterator it = downItems.begin();
		while(pos > 0){	
			pos--;
			++it;
		}
		downItems.insert(it, item);
		return true;
	}
	else {
		//Add it to the end.
		downItems.insert(downItems.end(), item);
		return true;
	}

  return false;
}

bool Tile::removeThing(Thing *thing)
{
	Creature* creature = dynamic_cast<Creature*>(thing);
	if (creature) {
    CreatureVector::iterator it;
    for (it = creatures.begin(); it != creatures.end(); ++it)
      if (*it == thing)
      {
        creatures.erase(it);
        return true;
      }
  }
  else if (thing == splash)
  {
    splash = NULL;
    return true;
  }
  else
  {
    ItemVector::iterator it;
    Item *item = (Item*)thing;

    if (item->isAlwaysOnTop())
    {
      for (it = topItems.begin(); it != topItems.end(); ++it)
        if (*it == item)
        {
          topItems.erase(it);
          return true;
        }
    }
    else
    {
      for (it = downItems.begin(); it != downItems.end(); ++it)
        if (*it == item)
        {
          downItems.erase(it);
          return true;
        }
    }
  }

  return false;
}

Teleport* Tile::getTeleportItem()
{
  Teleport* teleport = NULL;
  for (ItemVector::const_iterator iit = topItems.begin(); iit != topItems.end(); ++iit)
  {
		teleport = dynamic_cast<Teleport*>(*iit);
		if (teleport)
      return teleport;
  }

	return NULL;
}

MagicEffectItem* Tile::getFieldItem()
{
  MagicEffectItem* fieldItem = NULL;
  for (ItemVector::const_iterator iit = downItems.begin(); iit != downItems.end(); ++iit)
  {
		fieldItem = dynamic_cast<MagicEffectItem*>(*iit);
		if (fieldItem)
      return fieldItem;
  }

	return NULL;
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
      ground = item;
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

bool Tile::isPz() const
{
  return pz;
}

void Tile::setPz()
{
  pz = true;
}
