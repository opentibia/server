
#include "thing.h"



Thing::Thing()
{
  ThrowRange = 1;
}


Thing::~Thing()
{
}


bool Thing::CanMovedTo(Tile *tile)
{
  return true;
}


