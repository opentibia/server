
#include "thing.h"
#include "tile.h"


Thing::Thing()
{
  throwRange = 1;
  isRemoved = true;
}


Thing::~Thing()
{
}

bool Thing::canMovedTo(const Tile *tile) const
{
	return false;
  //return !tile->isBlocking();
}


