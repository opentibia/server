
#include "thing.h"
#include "tile.h"


Thing::Thing()
{
  throwRange = 1;
}


Thing::~Thing()
{
}


bool Thing::canMovedTo(Tile *tile)
{
  return !tile->isBlocking();
}


