

#include "definitions.h"

#include <string>
#include <sstream>

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
	lookmaster = 0;
	looktype   = PLAYER_MALE_1;
	pzLocked = false;

	lookcorpse = 3128;

  health     = 1000;//150;
  healthmax  = 1000;//150;
  experience = 100000;
  lastmove=0;

  lastDamage = 0;

	inFightTicks = 0;
	inFightTicks = 0;
  manaShieldTicks = 0;
  hasteTicks = 0;
	paralyzeTicks = 0;
	exhaustedTicks  = 0;
	pzLocked = false;
	
	burningTicks = 0;
	energizedTicks = 0;
	poisonedTicks = 0;

	curburningTicks = 0;
	curenergizedTicks = 0;
	curpoisonedTicks = 0;

  attackedCreature = 0;
  speed = 220;
}


void Creature::drainHealth(int damage)
{
  lastDamage = min(health, damage);

  health -= lastDamage;
}

void Creature::drainMana(int damage)
{
  lastDamage = min(mana, damage);

  mana -= lastDamage;
}

void Creature::setAttackedCreature(unsigned long id)
{
  attackedCreature = id;
}

void Creature::addMagicDamage(const MagicDamageContainer& dmgContainer, bool skipfirst /*= true*/)
{
	if(dmgContainer.getMagicType() == magicNone || dmgContainer.empty())
		return;

	MagicDamageType mt = dmgContainer.getMagicType();
	MagicDamageMap[mt] = dmgContainer;
	MagicDamageVec& vec = MagicDamageMap[mt];
	
	//has already been handled
	if(skipfirst && vec.size() > 0) {
		damageInfo& di = vec[0];
		di.first.second--;

		if(di.first.second <=0) {
			vec.erase(vec.begin());
		}
	}

	long ticks = 0;
	long curticks = 0;

	for(MagicDamageVec::iterator mdi = vec.begin(); mdi != vec.end(); ++mdi) {
		if(vec.begin() == mdi)
			curticks = mdi->first.first;

		ticks += mdi->first.first * mdi->first.second;
	}
	
	//inFightTicks += ticks;

	switch(mt) {
	case magicFire:
		burningTicks = ticks;
		curburningTicks = curticks;
		break;
		
	case magicEnergy:
		energizedTicks = ticks;
		curenergizedTicks = curticks;
		break;

	case magicPoison:
		poisonedTicks = ticks;
		curpoisonedTicks = curticks;
		break;
	case magicNone:
		// nothing to do
		break;
	}
}

MagicDamageVec* Creature::getMagicDamageVec(MagicDamageType md)
{
	if(MagicDamageMap.find(md) != MagicDamageMap.end()) {
		return &MagicDamageMap[md];
	}

	return NULL;
}


bool Creature::canMovedTo(const Tile *tile) const
{
  if(tile){   
  if (tile->creatures.size())
    return false;

  return Thing::canMovedTo(tile);
  }
  else return false;
}

std::string Creature::getDescription(bool self) const {
    std::stringstream s;
	std::string str;	
	s << "You see a " << name << ".";
	str = s.str();
	return str;
            }
