//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// a Tile represents a single field on the map.
// it is a list of Items
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


#include "definitions.h"

#include <string>
#include <iostream>

#include "tile.h"

#include "creature.h"

ReturnValue Tile::isBlocking(int objectstate, bool ignoreCreature /* = false*/, bool ignoreMoveableBlocking /*=false*/) const
{
	if(isPz() && ((objectstate & BLOCK_PZ) == BLOCK_PZ)) {
		return RET_PROTECTIONZONE;
	}

	if(((objectstate & BLOCK_PATHFIND) == BLOCK_PATHFIND) && (floorChange() || getTeleportItem())) {
		return RET_THEREISNOWAY;
	}
		
	if(ground) {
		const ItemType& groundType = Item::items[ground->getID()];

		if(((objectstate & BLOCK_PROJECTILE) == BLOCK_PROJECTILE) &&
			groundType.blockProjectile)
			return RET_CANNOTTHROW;

		/*
		if((groundType.blockPathFind || groundType.blockSolid) && ((objectstate & BLOCK_PATHFIND) == BLOCK_PATHFIND))
			return RET_THEREISNOWAY;
		*/

		if(((objectstate & BLOCK_PICKUPABLE) == BLOCK_PICKUPABLE)) {			
			if(groundType.blockSolid && (!groundType.hasHeight || groundType.pickupable))
				return RET_NOTENOUGHROOM;
		}

		if(((objectstate & BLOCK_SOLID) == BLOCK_SOLID) && groundType.blockSolid)
			return RET_NOTENOUGHROOM;
	}
	else if( !((objectstate & BLOCK_PROJECTILE) == BLOCK_PROJECTILE)) {
		return RET_NOTILE;
	}

	if(!ignoreCreature && !creatures.empty() && ((objectstate & BLOCK_SOLID) == BLOCK_SOLID))
		return RET_CREATUREBLOCK;

	ItemVector::const_iterator iit;
	for (iit = topItems.begin(); iit != topItems.end(); ++iit) {
		const ItemType& iiType = Item::items[(*iit)->getID()];

		if(((objectstate & BLOCK_PROJECTILE) == BLOCK_PROJECTILE)) {
			if(iiType.blockProjectile)
				return RET_CANNOTTHROW;
			/*else
				continue;*/
		}

		/*
		if((iiType.blockPathFind || iiType.blockSolid) && ((objectstate & BLOCK_PATHFIND) == BLOCK_PATHFIND) &&
			!(ignoreMoveableBlocking && iiType.moveable))
			return RET_THEREISNOWAY;
			*/

		if(((objectstate & BLOCK_PICKUPABLE) == BLOCK_PICKUPABLE)) {			
			if(iiType.blockSolid && (!iiType.hasHeight || iiType.pickupable))
				return RET_NOTENOUGHROOM;
		}

		if(((objectstate & BLOCK_SOLID) == BLOCK_SOLID) && iiType.blockSolid &&
			!(ignoreMoveableBlocking && iiType.moveable))
			return RET_NOTENOUGHROOM;
	}
	
	for (iit = downItems.begin(); iit != downItems.end(); ++iit) {
		const ItemType& iiType = Item::items[(*iit)->getID()];

		if(((objectstate & BLOCK_PROJECTILE) == BLOCK_PROJECTILE)) {
			if(iiType.blockProjectile)
				return RET_CANNOTTHROW;
			/*else
				continue;*/
		}

		/*
		if((iiType.blockPathFind || iiType.blockSolid) && ((objectstate & BLOCK_PATHFIND) == BLOCK_PATHFIND) &&
			!(ignoreMoveableBlocking && iiType.moveable))
			return RET_THEREISNOWAY;
		*/

		if(((objectstate & BLOCK_PICKUPABLE) == BLOCK_PICKUPABLE)) {
			if(iiType.blockSolid && (!iiType.hasHeight || iiType.pickupable))
				return RET_NOTENOUGHROOM;
		}

		if(((objectstate & BLOCK_SOLID) == BLOCK_SOLID) && iiType.blockSolid &&
			!(ignoreMoveableBlocking && iiType.moveable))
			return RET_NOTENOUGHROOM;
	}

	//return false;
	return RET_NOERROR;
}

bool Tile::floorChange() const
{
  ItemVector::const_iterator iit;
  if(ground && ground->floorChangeDown())
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

bool Tile::floorChangeDown() const
{
	if(ground && ground->floorChangeDown())
		return true;
		
	ItemVector::const_iterator iit;
	for(iit = topItems.begin(); iit != topItems.end(); ++iit){
		if((*iit)->floorChangeDown())
			return true;
	}

  	for(iit = downItems.begin(); iit != downItems.end(); ++iit){
		if((*iit)->floorChangeDown())
			return true;
	}
	
  	return false;
}

bool Tile::floorChange(Direction direction) const
{  
	ItemVector::const_iterator iit;
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
  if(ground){
	if(ground == thing){
		return 0;
	}
  }

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
    Item *item = dynamic_cast<Item*>(thing);
		if(!item)
			return false;

    if (item->isAlwaysOnTop())
    {
      for (it = topItems.begin(); it != topItems.end(); ++it)
        if (*it == item)
        {
          topItems.erase(it);
					return true;
        }
    }
    else {
      for (it = downItems.begin(); it != downItems.end(); ++it)
        if (*it == item) {
					downItems.erase(it);
					return true;
				}
		}
  }

  return false;
}

Thing* Tile::getTopMoveableThing(){	
	if(ground && !ground->isNotMoveable())
    	return ground;
	if (splash && !splash->isNotMoveable())
    	return splash;
    
    for(int i = 0; i < topItems.size(); i++){
		if(topItems[i] && !topItems[i]->isNotMoveable())
			return topItems[i];
	}
	for(int i = 0; i < creatures.size(); i++){
			return creatures[i];
	}
	for(int i = 0; i < downItems.size(); i++){
		if(downItems[i] && !downItems[i]->isNotMoveable())
			return downItems[i];
	}
	return NULL;
}

Teleport* Tile::getTeleportItem() const
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
	if(fieldItem)
		return fieldItem;
  }

	return NULL;
}

Creature* Tile::getTopCreature(){
	if(creatures.begin() != creatures.end()) {
		return *(creatures.begin());
	}

	return NULL;
}

Item* Tile::getTopDownItem()
{
	if(downItems.begin() != downItems.end()) {
		return *(downItems.begin());
	}

	return NULL;
}

Item* Tile::getTopTopItem()
{
	if(!topItems.empty()){
		return topItems.back();
	}

	return NULL;
}

Thing* Tile::getTopThing()
{
	Thing* thing = NULL;
	thing = getTopCreature();
	if(thing != NULL)
		return thing;

	thing = getTopDownItem();
	if(thing != NULL)
		return thing;

	thing = getTopTopItem();
	if(thing != NULL)
		return thing;
	
	if(splash)
		return splash;

	if(ground)
		return ground;

	return NULL;
}

Item* Tile::getMoveableBlockingItem()
{
	for (ItemVector::const_iterator iit = downItems.begin(); iit != downItems.end(); ++iit)
	{
		const ItemType& iiType = Item::items[(*iit)->getID()];
		if((iiType.blockPathFind || iiType.blockSolid) && iiType.moveable)
			return *iit;
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
    Item *item = dynamic_cast<Item*>(thing);
		if(!item)
			return;

    if (item->isGroundTile())
    {
      ground = item;
    }
    else if (item->isSplash()){
		if(splash == NULL){
			splash = item;
		}
		else{
			//Should not add the splash directly
			//use game->addthing method
		}
	}
    else if (item->isAlwaysOnTop())
    {
      //topItems.insert(topItems.begin(), item);
			topItems.push_back(item);
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
