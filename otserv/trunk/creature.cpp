

#include "definitions.h"

#include <string>

using namespace std;

#include "creature.h"


static unsigned int idcount = 0x4711;


Creature::Creature(const char *name)
{
  idcount++;

  id         = idcount;
  direction  = NORTH;

  this->name = name;

  lookhead   = rand() % 133;
	lookbody   = rand() % 133;
	looklegs   = rand() % 133;
	lookfeet   = rand() % 133;
	looktype   = 0x88;

  health     = 1000;//150;
  healthmax  = 1000;//150;

  lastDamage = 0;

  speed      = 220;
}


void Creature::drainHealth(int damage)
{
  lastDamage = min(health, damage);

  health -= lastDamage;
}


