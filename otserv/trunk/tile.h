

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
    decaySplashAfter = 0;
  }

  Item*           ground;
  Item*          splash;
  ItemVector     topItems;
  CreatureVector creatures;
  ItemVector     downItems;

  __int64        decaySplashAfter;

  bool removeThing(Thing *thing);
  void addThing(Thing *thing);
	bool insertThing(Thing *thing, int stackpos);
	MagicEffectItem* getFieldItem();
	Teleport* getTeleportItem();

  int getCreatureStackPos(Creature *c) const;
  int getThingStackPos(const Thing *thing) const;
	int getThingCount() const;

  Thing* getThingByStackPos(int pos);

  bool isBlockingProjectile() const;
	bool isBlocking(bool ispickupable = false) const;

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

