


#ifndef __THING_H__
#define __THING_H__

#include "definitions.h"

#include "position.h"


class Tile;


class Thing
{
public:
  Thing();
  virtual ~Thing();

  bool CanMovedTo(Tile *tile);

  int ThrowRange;

  Position pos;

};


#endif // #ifndef __THING_H__
