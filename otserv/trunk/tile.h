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


#ifndef __TILE_H__
#define __TILE_H__


#include "item.h"
#include "container.h"
#include "magic.h"

#include "definitions.h"
#include "templates.h"
#include "scheduler.h"

class Creature;

typedef std::vector<Item*> ItemVector;
typedef std::vector<Creature*> CreatureVector;

enum EventType
{
	EVENT_CREATURE_ENTER,
	EVENT_CREATURE_LEAVE,
	EVENT_ITEM_ADD,
	EVENT_ITEM_REMOVE
};

class Event {
public:
	virtual void onCreatureEnter(const Creature *creature, const Position &pos) {};
	virtual void onCreatureLeave(const Creature *creature, const Position &pos) {};
};

class Tile
{
public:
  Creature* getCreature() const{
		if(creatures.size())
    		return creatures[0];
    	else
    		return NULL;
  }

  Tile()
  {
    pz               = false;
    splash           = NULL;
		ground           = NULL;
  }

  Item*           ground;
  Item*          splash;
  ItemVector     topItems;
  CreatureVector creatures;
  ItemVector     downItems;

  bool removeThing(Thing *thing);
  void addThing(Thing *thing);
	bool insertThing(Thing *thing, int stackpos);
	MagicEffectItem* getFieldItem();
	Teleport* getTeleportItem() const;

	Thing* getTopMoveableThing();
	Creature* getTopCreature();
	Item* getTopTopItem();
	Item* getTopDownItem();
	Item* getMoveableBlockingItem();
	
  int getCreatureStackPos(Creature *c) const;
  int getThingStackPos(const Thing *thing) const;
	int getThingCount() const;

  Thing* getTopThing();
	Thing* getThingByStackPos(int pos);

  bool isBlockingProjectile() const;
	bool isBlocking(bool ispickupable = false, bool ignoreMoveableBlocking = false) const;

  bool isPz() const;
  void setPz();
  
  bool floorChange();
  bool floorChange(Direction direction);
  
  std::string getDescription() const;

	/*
	void RegisterTrigger(enum EventType et, Event* event)
	{
		event_map.insert(event_pair(et, event));
	}
	*/

protected:
	typedef std::multimap<enum EventType, Event*, std::less<enum EventType> > EventMap;
	typedef EventMap::value_type event_pair;
	EventMap event_map;

  bool pz;
};


#endif // #ifndef __TILE_H__

