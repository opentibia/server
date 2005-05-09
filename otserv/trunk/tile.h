

#ifndef __TILE_H__
#define __TILE_H__


#include "item.h"
#include "container.h"
#include "magic.h"

#include "definitions.h"
class Creature;


typedef std::vector<Item*> ItemVector;
typedef std::vector<Creature*> CreatureVector;


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

  //Item           ground;
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
	bool isBlocking() const;

  bool isPz() const;
  void setPz();
  
  bool floorChange();
  bool floorChange(Direction direction);
  
  std::string getDescription() const;

protected:
  bool pz;
};


#endif // #ifndef __TILE_H__

