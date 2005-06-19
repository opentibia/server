
#include "thing.h"
#include "tile.h"


Thing::Thing()
{
  throwRange = 1;
}


Thing::~Thing()
{
}


bool Thing::canMovedTo(const Tile *tile) const
{
  return !tile->isBlocking();
}


