

#include "definitions.h"

#include <string>

#include "creature.h"
#include "tile.h"

using namespace std;

static unsigned int idcount = 0x4711;


Creature::Creature(const char *name) : access(0)
{
  idcount++;

  id         = idcount;

  direction  = NORTH;

  this->name = name;

  lookhead   = 0;
	lookbody   = 0;
	looklegs   = 0;
	lookfeet   = 0;
	looktype   = PLAYER_MALE_1;

	lookcorpse = 2276;

  health     = 1000;//150;
  healthmax  = 1000;//150;

  lastDamage = 0;
  pzLockedTicks = 0;

  attackedCreature = 0;
}


void Creature::drainHealth(int damage)
{
  lastDamage = min(health, damage);

  health -= lastDamage;
}


void Creature::setAttackedCreature(unsigned long id)
{
  attackedCreature = id;
}

bool Creature::canMovedTo(Tile *tile)
{
  if (tile->creatures.size())
    return false;

  return Thing::canMovedTo(tile);
}