

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
  }

  Item           ground;
  ItemVector     topItems;
  CreatureVector creatures;
  ItemVector     downItems;

  bool removeThing(Thing *thing);
  void addThing(Thing *thing);

  int getCreatureStackPos(Creature *c);
  int getThingStackPos(Thing *thing);

  Thing* getThingByStackPos(int pos);




  int getStackPosItem();
  
  int removeItem(int stack, int type, int count);
  
  int removeItemByStack(int stack);
  
  Item* getItemByStack(int stack);
  
  bool isBlocking();
  
  std::string getDescription();
};


#endif // #ifndef __TILE_H__

