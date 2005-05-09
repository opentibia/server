


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

  virtual bool canMovedTo(const Tile *tile) const;
  virtual void useThing() = 0;
  virtual void releaseThing() = 0;

  int throwRange;

  Position pos;

};


#endif // #ifndef __THING_H__
