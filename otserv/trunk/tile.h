

#ifndef __TILE_H__
#define __TILE_H__


#include "item.h"


class Creature;


typedef std::vector<Item*> ItemVector;
typedef std::vector<Creature*> CreatureVector;


class Tile
{
public:
  Creature* getCreature(){
    return creatures[0];
  }

  Tile()
  {
    pz = false;
  }

  Item           ground;
  ItemVector     topItems;
  CreatureVector creatures;
  ItemVector     downItems;

  bool removeThing(Thing *thing);
  void addThing(Thing *thing);

  int getCreatureStackPos(Creature *c);
  int getThingStackPos(Thing *thing);
	int getThingCount();

  Thing* getThingByStackPos(int pos);




  int getStackPosItem();
  
  int removeItem(int stack, int type, int count);
  
  int removeItemByStack(int stack);
  
  Item* getItemByStack(int stack);
  
  bool isBlocking();

  bool isPz();
  void setPz();
  
  std::string getDescription();
protected:
  bool pz;
};


#endif // #ifndef __TILE_H__

