

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

  //lastDamage = 0;

	inFightTicks = 0;
	inFightTicks = 0;
  manaShieldTicks = 0;
  hasteTicks = 0;
	paralyzeTicks = 0;
	exhaustedTicks  = 0;
	pzLocked = false;
	
	/*
	burningTicks = 0;
	energizedTicks = 0;
	poisonedTicks = 0;
	*/

	immunities = 0;

	/*
	curburningTicks = 0;
	curenergizedTicks = 0;
	curpoisonedTicks = 0;
	*/

  attackedCreature = 0;
  speed = 220;
}


void Creature::drainHealth(int damage)
{
  //lastDamage = min(health, damage);

  health -= min(health, damage);
}

void Creature::drainMana(int damage)
{
  //lastDamage = min(mana, damage);

  mana -= min(mana, damage);
}

void Creature::setAttackedCreature(unsigned long id)
{
  attackedCreature = id;
}

void Creature::addCondition(const CreatureCondition& condition, bool refresh)
{
	if(condition.getCondition()->attackType == ATTACK_NONE)
		return;

	ConditionVec &condVec = conditions[condition.getCondition()->attackType];
	
	if(refresh) {
		condVec.clear();
	}

	condVec.push_back(condition);
}

void Creature::addDamage(Creature* attacker, int damage)
{
	if(damage <= 0)
		return;

	unsigned long id = 0;
	if(attacker) {
		id = attacker->getID();
	}

	totaldamagelist[id].push_back(make_pair(OTSYS_TIME(), damage));
}

int Creature::getLostExperience() {
	return (int)std::floor(((double)experience * 0.1));
}

int Creature::getInflicatedDamage(unsigned long id)
{
	int ret = 0;
	std::map<long, DamageList >::const_iterator tdIt = totaldamagelist.find(id);
	if(tdIt != totaldamagelist.end()) {
		for(DamageList::const_iterator dlIt = tdIt->second.begin(); dlIt != tdIt->second.end(); ++dlIt) {
			ret += dlIt->second;
		}
	}

	return ret;
}

int Creature::getInflicatedDamage(Creature* attacker)
{
	unsigned long id = 0;
	if(attacker) {
		id = attacker->getID();
	}

	return getInflicatedDamage(id);
}

int Creature::getTotalInflictedDamage()
{
	int ret = 0;
	std::map<long, DamageList >::const_iterator tdIt;
	for(tdIt = totaldamagelist.begin(); tdIt != totaldamagelist.end(); ++tdIt) {
		ret += getInflicatedDamage(tdIt->first);
	}

	return ret;
}

int Creature::getGainedExperience(Creature* attacker)
{
	int totaldamage = getTotalInflictedDamage();
	int attackerdamage = getInflicatedDamage(attacker);
	int lostexperience = getLostExperience();
	int gainexperience = 0;
	
	if(attackerdamage > 0 && totaldamage > 0) {
		gainexperience = (int)std::floor(((double)attackerdamage / totaldamage) * lostexperience);
	}

	return gainexperience;
}

std::vector<long> Creature::getInflicatedDamageCreatureList()
{
	std::vector<long> list;

	std::map<long, DamageList >::const_iterator tdIt;
	for(tdIt = totaldamagelist.begin(); tdIt != totaldamagelist.end(); ++tdIt) {
		list.push_back(tdIt->first);
	}

	return list;
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

std::string Creature::getDescription(bool self) const
{
  std::stringstream s;
	std::string str;	
	s << "You see a " << name << ".";
	str = s.str();
	return str;
}
