

#include "definitions.h"

#include <string>
#include <sstream>
#include <algorithm>

#include "creature.h"
#include "tile.h"
#include "otsystem.h"

using namespace std;

template<class Creature> typename AutoList<Creature>::list_type AutoList<Creature>::list;

Creature::Creature(const char *name, unsigned long _id) :
 AutoList<Creature>(_id)
 ,access(0)
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
	looktype_ex = 0;

	lookcorpse = 3128;

  health     = 1000;
  healthmax  = 1000;
  experience = 100000;
  lastmove = 0;

	immunities = 0;

	lastDamage = 0;
	lastManaDamage = 0;

  attackedCreature = 0;
  speed = 220;
//	addspeed = 0;
	isInvisible = false;
}

Creature::~Creature()
{
	std::vector<Creature*>::iterator cit;
	for(cit = summons.begin(); cit != summons.end(); ++cit) {
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

void Creature::setAttackedCreature(unsigned long id)
{
  attackedCreature = id;
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
	std::vector<Creature*>::iterator cit = std::find(summons.begin(), summons.end(), creature);
	if(cit != summons.end()) {
		(*cit)->setMaster(NULL);
		(*cit)->releaseThing();
		summons.erase(cit);
	}
}

Item* Creature::getCorpse(Creature *attacker)
{
	Item *corpseitem = Item::CreateItem(this->getLookCorpse());
	corpseitem->pos = this->pos;

	//Add eventual loot
	Container *container = dynamic_cast<Container*>(corpseitem);
	if(container) {
		this->dropLoot(container);
	}

	return corpseitem;
}

/*
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
*/

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


void Creature::addCondition(Condition *condition){
	if(!condition){
		std::cout << "null condition" << std::endl;
		return;
	}
	conditiontype_t cond_type = condition->getType();
	Condition *prev_cond = getCondition(cond_type);
	if(prev_cond){
		prev_cond->addCondition(condition);
		delete condition;
	}
	else{
		if(condition->startCondition(this)){
			conditions.push_back(condition);
		}
	}
}

void Creature::removeCondition(conditiontype_t c_type){
	ConditionList::iterator it;
	for(it = conditions.begin(); it != conditions.end(); ++it){
		if((*it)->getType() == c_type){
			(*it)->endCondition(REASON_ABORT);
			delete *it;
			conditions.erase(it);
		}
	}
}

void Creature::executeConditions(int newticks){
	ConditionList::iterator it;
	for(it = conditions.begin(); it != conditions.end();){
		(*it)->ticks -= newticks;
		if((*it)->ticks <= 0){
			(*it)->endCondition(REASON_ENDTICKS);
			delete *it;
			it = conditions.erase(it);
		}
		else{
			(*it)->executeCondition(newticks);
			++it;
		}
	}
}

Condition* Creature::getCondition(conditiontype_t c_type){
	ConditionList::iterator it;
	if(conditions.size() == 0)
		return NULL;
	
	for(it = conditions.begin(); it != conditions.end(); ++it){
		if((*it)->getType() == c_type)
			return *it;
	}
	return NULL;
}

bool Creature::hasCondition(conditiontype_t c_type){
	return (getCondition(c_type) != NULL);
}

void Creature::addExhaustion(long ticks){
	Condition *cond = Condition::createCondition(CONDITION_EXHAUSTED, ticks, 0);
	this->addCondition(cond);
}
