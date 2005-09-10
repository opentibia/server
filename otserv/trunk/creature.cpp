//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////


#include "definitions.h"

#include <string>
#include <sstream>
#include <algorithm>

#include "game.h"
#include "creature.h"
#include "tile.h"
#include "otsystem.h"

using namespace std;

OTSYS_THREAD_LOCKVAR AutoID::autoIDLock;
unsigned long AutoID::count = 1000;
AutoID::list_type AutoID::list;

extern Game g_game;

Creature::Creature(const std::string& name) :
access(0)
{
	direction  = NORTH;
	master = NULL;
	
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
	
	inFightTicks = 0;
	inFightTicks = 0;
	manaShieldTicks = 0;
	hasteTicks = 0;
	paralyzeTicks = 0;
	exhaustedTicks  = 0;
	pzLocked = false;
	immunities = 0;
	eventCheck = 0;
	eventCheckAttacking = 0;
	
	attackedCreature = 0;
	speed = 220;
}

Creature::~Creature()
{
	std::list<Creature*>::iterator cit;
	for(cit = summons.begin(); cit != summons.end(); ++cit) {
		(*cit)->setAttackedCreature(NULL);
		(*cit)->setMaster(NULL);
		(*cit)->releaseThing();
	}
	
	summons.clear();
}

void Creature::drainHealth(int damage)
{
	health -= min(health, damage);
}

void Creature::drainMana(int damage)
{
	mana -= min(mana, damage);
}

//void Creature::setAttackedCreature(unsigned long id)
void Creature::setAttackedCreature(const Creature* creature)
{
	std::list<Creature*>::iterator cit;
	for(cit = summons.begin(); cit != summons.end(); ++cit) {
		(*cit)->setAttackedCreature(creature);
	}
	
	if(creature) {
		attackedCreature = creature->getID();
	}
	else
		attackedCreature = 0;
}

void Creature::setMaster(Creature* creature)
{
	//std::cout << "setMaster: " << this << " master=" << creature << std::endl;
	master = creature;
}

void Creature::addSummon(Creature *creature)
{
	//std::cout << "addSummon: " << this << " summon=" << creature << std::endl;
	creature->setMaster(this);
	creature->useThing();
	summons.push_back(creature);
	
}

void Creature::removeSummon(Creature *creature)
{
	//std::cout << "removeSummon: " << this << " summon=" << creature << std::endl;
	std::list<Creature*>::iterator cit = std::find(summons.begin(), summons.end(), creature);
	if(cit != summons.end()) {
		(*cit)->setMaster(NULL);
		(*cit)->releaseThing();
		summons.erase(cit);
	}
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

void Creature::addInflictedDamage(Creature* attacker, int damage)
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
	std::vector<long> damagelist;	
	std::map<long, DamageList >::const_iterator tdIt;
	for(tdIt = totaldamagelist.begin(); tdIt != totaldamagelist.end(); ++tdIt) {
		damagelist.push_back(tdIt->first);
	}
	
	return damagelist;
}

bool Creature::canMovedTo(const Tile *tile) const
{
	if(tile) {
		return (tile->isBlocking(BLOCK_SOLID) == RET_NOERROR);
	}

	return false;
}

std::string Creature::getDescription(bool self) const
{
	std::stringstream s;
	std::string str;	
	s << "a " << name << ".";
	str = s.str();
	return str;
}

int Creature::getStepDuration() const
{
	int duration = 500;
	Tile *tile = g_game.getTile(pos.x, pos.y, pos.z);
	if(tile && tile->ground){
		int groundid = tile->ground->getID();
		uint8_t stepspeed = Item::items[groundid].speed;
		if(stepspeed != 0) {
			duration =  (1000 * stepspeed) / (getSpeed() != 0 ? getSpeed() : 220);
		}
	}
	return duration;
};

long long Creature::getSleepTicks() const
{
	long long delay = 0;
	int stepDuration = getStepDuration();
	
	if(lastmove != 0) {
		delay = (((long long)(lastmove)) + ((long long)(stepDuration))) - ((long long)(OTSYS_TIME()));
	}
	
	return delay;
}
