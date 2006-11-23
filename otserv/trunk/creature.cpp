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
#include "otpch.h"

#include "creature.h"
#include "game.h"
#include "otsystem.h"
#include "player.h"
#include "npc.h"
#include "monster.h"
#include "container.h"
#include "condition.h"
#include "combat.h"

#include <string>
#include <sstream>
#include <algorithm>
#include "configmanager.h"

#include <vector>

OTSYS_THREAD_LOCKVAR AutoID::autoIDLock;
unsigned long AutoID::count = 1000;
AutoID::list_type AutoID::list;

extern Game g_game;
extern ConfigManager g_config;

Creature::Creature() :
  isInternalRemoved(false)
{
	direction  = NORTH;
	master = NULL;

	health     = 1000;
	healthMax  = 1000;
	mana = 0;
	manaMax = 0;
	
	lastMove = 0;
	lastStepCost = 1;
	baseSpeed = 220;
	varSpeed = 0;

	followCreature = NULL;
	eventWalk = 0;
	internalUpdateFollow = false;
	//followDistance = 1;

	eventCheck = 0;
	//eventCheckAttacking = 0;
	attackedCreature = NULL;
	lastHitCreature = 0;
	internalDefense = true;
	internalArmor = true;
	blockTicks = 0;
}

Creature::~Creature()
{
	std::list<Creature*>::iterator cit;
	for(cit = summons.begin(); cit != summons.end(); ++cit) {
		(*cit)->setAttackedCreature(NULL);
		(*cit)->setMaster(NULL);
		(*cit)->releaseThing2();
	}

	summons.clear();

	for(ConditionList::iterator it = conditions.begin(); it != conditions.end(); ++it){
		(*it)->endCondition(this, REASON_ABORT);
		delete *it;
	}

	conditions.clear();

	attackedCreature = NULL;

	//std::cout << "Creature destructor " << this->getID() << std::endl;
}

void Creature::setRemoved()
{
	isInternalRemoved = true;
}

bool Creature::canSee(const Position& pos) const
{
	const Position& myPos = getPosition();

	if(myPos.z <= 7){
		//we are on ground level or above (7 -> 0)
		//view is from 7 -> 0
		if(pos.z > 7){
			return false;
		}
	}
	else if(myPos.z >= 8){
		//we are underground (8 -> 15)
		//view is +/- 2 from the floor we stand on
		if(std::abs(myPos.z - pos.z) > 2){
			return false;
		}
	}

	int offsetz = myPos.z - pos.z;

	if ((pos.x >= myPos.x - 8 + offsetz) && (pos.x <= myPos.x + 9 + offsetz) &&
		(pos.y >= myPos.y - 6 + offsetz) && (pos.y <= myPos.y + 7 + offsetz))
		return true;

	return false;
}

void Creature::addEventThink()
{
	if(eventCheck == 0){
		eventCheck = g_game.addEvent(makeTask(500, boost::bind(&Game::checkCreature, &g_game, getID(), 500)));
		//onStartThink();
	}
}

void Creature::stopEventThink()
{
	if(eventCheck != 0){
		g_game.stopEvent(eventCheck);
		eventCheck = 0;
		//onStopThink();
	}
}

void Creature::onThink(uint32_t interval)
{
	if(internalUpdateFollow && followCreature){
		internalUpdateFollow = false;
		setFollowCreature(followCreature);
		//internalFollowCreature(followCreature);
	}

	blockTicks += interval;

	if(blockTicks >= 1500){
		blockTicks = 0;
		internalDefense = true;
		internalArmor = true;
	}

	if(attackedCreature){
		if(g_game.canThrowObjectTo(getPosition(), attackedCreature->getPosition())){
			doAttacking(interval);
		}

		attackedCreature->onAttacked();
	}

	if(eventCheck != 0){
		eventCheck = 0;
		addEventThink();
	}

	//eventCheck = g_game.addEvent(makeTask(interval, boost::bind(&Game::checkCreature,
	//	&g_game, getID(), interval)));
}

void Creature::onWalk()
{
	if(getSleepTicks() <= 0){
		Direction dir;
		if(getNextStep(dir)){
			ReturnValue ret = g_game.internalMoveCreature(this, dir);

			if(ret != RET_NOERROR){
				internalUpdateFollow = true;
			}
		}
	}

	if(eventWalk != 0){
		eventWalk = 0;
		addEventWalk();
	}
}

void Creature::onWalk(Direction& dir)
{
	if(hasCondition(CONDITION_DRUNK)){
		uint32_t r = random_range(0, 16);

		if(r <= 4){
			switch(r){
				case 0: dir = NORTH; break;
				case 1: dir = WEST;  break;
				case 3: dir = SOUTH; break;
				case 4: dir = EAST;  break;

				default:
					break;
			}

			g_game.internalCreatureSay(this, SPEAK_SAY, "Hicks!");
		}
	}
}

bool Creature::getNextStep(Direction& dir)
{
	if(!listWalkDir.empty()){
		Position pos = getPosition();
		dir = listWalkDir.front();
		listWalkDir.pop_front();
		onWalk(dir);

		return true;
	}

	return false;
}

bool Creature::startAutoWalk(std::list<Direction>& listDir)
{
	listWalkDir = listDir;
	addEventWalk();
	return true;
}

void Creature::addEventWalk()
{
	if(eventWalk == 0){
		//std::cout << "addEventWalk()" << std::endl;

		int64_t ticks = getEventStepTicks();
		eventWalk = g_game.addEvent(makeTask(ticks, std::bind2nd(std::mem_fun(&Game::checkWalk), getID())));
	}
}

void Creature::stopEventWalk()
{
	if(eventWalk != 0){
		g_game.stopEvent(eventWalk);
		eventWalk = 0;

		if(!listWalkDir.empty()){
			listWalkDir.clear();
			onWalkAborted();
		}
	}
}

void Creature::validateWalkPath()
{
	if(!internalUpdateFollow && followCreature){
		if(listWalkDir.empty() || !g_game.isPathValid(this, listWalkDir, followCreature->getPosition())){
			internalUpdateFollow = true;
		}
	}
}

void Creature::onAddTileItem(const Position& pos, const Item* item)
{
	validateWalkPath();
}

void Creature::onUpdateTileItem(const Position& pos, uint32_t stackpos, const Item* oldItem, const Item* newItem)
{
	validateWalkPath();
}

void Creature::onRemoveTileItem(const Position& pos, uint32_t stackpos, const Item* item)
{
	validateWalkPath();
}

void Creature::onUpdateTile(const Position& pos)
{
	validateWalkPath();
}

void Creature::onCreatureAppear(const Creature* creature, bool isLogin)
{
	validateWalkPath();
}

void Creature::onCreatureDisappear(const Creature* creature, bool isLogout)
{
	if(attackedCreature == creature){
		setAttackedCreature(NULL);
		onAttackedCreatureDissapear(isLogout);
	}

	if(followCreature == creature){
		setFollowCreature(NULL);
		onFollowCreatureDissapear(isLogout);
	}
}

void Creature::onCreatureDisappear(const Creature* creature, uint32_t stackpos, bool isLogout)
{
	validateWalkPath();

	onCreatureDisappear(creature, true);
}

void Creature::onCreatureMove(const Creature* creature, const Position& newPos, const Position& oldPos,
	uint32_t oldStackPos, bool teleport)
{
	if(creature == this){
		lastMove = OTSYS_TIME();
		
		lastStepCost = 1;

		if(!teleport){
			if(oldPos.z != newPos.z){
				//floor change extra cost
				lastStepCost = 2;
			}
			else if(std::abs(newPos.x - oldPos.x) >=1 && std::abs(newPos.y - oldPos.y) >= 1){
				//diagonal extra cost
				lastStepCost = 2;
			}
		}
	}

	if(followCreature == creature || (creature == this && followCreature)){
		if(newPos.z != oldPos.z || !canSee(followCreature->getPosition())){
			onCreatureDisappear(followCreature, false);
		}
		
		validateWalkPath();
	}

	if(attackedCreature == creature || (creature == this && attackedCreature)){
		if(newPos.z != oldPos.z || !canSee(attackedCreature->getPosition())){
			onCreatureDisappear(attackedCreature, false);
		}
		else if(attackedCreature->isInPz() || isInPz()){
			onCreatureDisappear(attackedCreature, false);
		}
	}
}

void Creature::onCreatureChangeVisible(const Creature* creature, bool visible)
{
	if(!visible && !canSeeInvisibility() && getMaster() != creature){
		if(followCreature == creature){
			onCreatureDisappear(followCreature, false);
		}

		if(attackedCreature == creature){
			onCreatureDisappear(attackedCreature, false);
		}
	}
}

void Creature::die()
{
	Creature* lastHitCreature = NULL;
	Creature* mostDamageCreature = NULL;

	if(getKillers(&lastHitCreature, &mostDamageCreature)){
		if(lastHitCreature){
			lastHitCreature->onKilledCreature(this);
		}

		if(mostDamageCreature && mostDamageCreature != lastHitCreature){
			mostDamageCreature->onKilledCreature(this);
		}
	}

	for(DamageMap::iterator it = damageMap.begin(); it != damageMap.end(); ++it){
		if(Creature* attacker = g_game.getCreatureByID((*it).first)){
			attacker->onAttackedCreatureKilled(this);
		}
	}

	if(getMaster()){
		getMaster()->removeSummon(this);
	}
}

bool Creature::getKillers(Creature** _lastHitCreature, Creature** _mostDamageCreature)
{
	*_lastHitCreature = g_game.getCreatureByID(lastHitCreature);

	int32_t mostDamage = 0;
	for(DamageMap::iterator it = damageMap.begin(); it != damageMap.end(); ++it){
		if((*it).second > mostDamage){
			if(*_mostDamageCreature = g_game.getCreatureByID((*it).first)){
				mostDamage = (*it).second;
			}
		}
	}

	return (*_lastHitCreature || *_mostDamageCreature);
}

Item* Creature::getCorpse()
{
	Item* corpse = Item::CreateItem(getLookCorpse());

	/*
	if(corpse){
		if(Container* corpseContainer = corpse->getContainer()){
			dropLoot(corpseContainer);
		}
	}
	*/

	return corpse;
}

void Creature::changeHealth(int32_t healthChange)
{
	if(healthChange > 0){
		health += std::min(healthChange, healthMax - health);
	}
	else{
		health = std::max((int32_t)0, health + healthChange);
	}

	g_game.addCreatureHealth(this);
}

void Creature::changeMana(int32_t manaChange)
{
	if(manaChange > 0){
		mana += std::min(manaChange, manaMax - mana);
	}
	else{
		mana = std::max((int32_t)0, mana + manaChange);
	}
}

void Creature::drainHealth(Creature* attacker, CombatType_t combatType, int32_t damage)
{
	changeHealth(-damage);

	if(attacker){
		attacker->onAttackedCreatureDrainHealth(this, damage);
	}
}

void Creature::drainMana(Creature* attacker, int32_t manaLoss)
{
	onAttacked();
	changeMana(-manaLoss);
}

BlockType_t Creature::blockHit(Creature* attacker, CombatType_t combatType, int32_t& damage,
	bool checkDefense /* = false */, bool checkArmor /* = false */)
{
	BlockType_t blockType = BLOCK_NONE;

	if(blockType == BLOCK_NONE && isImmune(combatType)){
		damage = 0;
		blockType = BLOCK_IMMUNITY;
	}

	if(blockType == BLOCK_NONE && internalDefense && checkDefense){
		int32_t defense = getDefense();

		int32_t attackValue = (rand() % 80) + (damage / 16);
		if(attackValue < defense){
			damage = 0;
			blockType = BLOCK_DEFENSE;
			internalDefense = false;
			onDefenseBlock(true);
		}
		else{
			onDefenseBlock(false);
		}
	}
	
	if(blockType == BLOCK_NONE && internalArmor && checkArmor){
		internalArmor = false;
		int32_t armor = getArmor();

		int32_t armor2 = armor*armor;
		int32_t max = armor2/8 + 2*armor + 8;
		int32_t min = -armor2/32 + (5*armor)/2;
		if(min < 0){
			min = 0;
		}
		min = random_range(min/2, min);
		int32_t reduceDamage = random_range(min, max);

		if(reduceDamage >= damage){
			damage = 0;
			blockType = BLOCK_ARMOR;
			onArmorBlock(true);
		}
		else{
			damage -= reduceDamage;
			onArmorBlock(false);
		}
	}

	if(attacker){
		attacker->onAttackedCreature(this);
		attacker->onAttackedCreatureBlockHit(this, blockType);
	}

	onAttacked();

	return blockType;
}

bool Creature::setAttackedCreature(Creature* creature)
{
	if(creature){
		const Position& creaturePos = creature->getPosition();
		if(creaturePos.z != getPosition().z || !canSee(creaturePos)){
			attackedCreature = NULL;
			return false;
		}
	}

	attackedCreature = creature;

	if(attackedCreature){
		onAttackedCreature(attackedCreature);
		attackedCreature->onAttacked();
	}

	std::list<Creature*>::iterator cit;
	for(cit = summons.begin(); cit != summons.end(); ++cit) {
		(*cit)->setAttackedCreature(creature);
	}

	return true;
}

void Creature::getPathSearchParams(const Creature* creature, FindPathParams& fpp) const
{
	fpp.fullPathSearch = false;
	fpp.needReachable = true;
	fpp.targetDistance = 1;

	bool fullPathSearch = false;
	if(followCreature != creature || !g_game.canThrowObjectTo(getPosition(), creature->getPosition())){
		fpp.fullPathSearch = true;
	}
}

bool Creature::setFollowCreature(Creature* creature)
{
	if(creature){
		const Position& creaturePos = creature->getPosition();
		if(creaturePos.z != getPosition().z || !canSee(creaturePos)){
			followCreature = NULL;
			return false;
		}
	}

	if(creature){
		FindPathParams fpp;
		getPathSearchParams(creature, fpp);

		listWalkDir.clear();
		//uint32_t maxDistance = getFollowDistance();
		//bool fullPathSearch = getFullPathSearch();
		//bool reachable = getFollowReachable();
		if(!g_game.getPathToEx(this, creature->getPosition(), 1, fpp.targetDistance, fpp.fullPathSearch, fpp.needReachable, listWalkDir)){
			followCreature = NULL;
			return false;
		}

		startAutoWalk(listWalkDir);
	}

	followCreature = creature;
	onFollowCreature(creature);
	return true;
}

/*
void Creature::setFollowCreature(const Creature* creature)
{
	if(creature && !canSee(creature->getPosition())){
		std::cout << "Creature::setFollowCreature - can not see creature" << std::endl;
	}

	if(followCreature != creature){
		followCreature = creature;

		onFollowCreature(creature);
	}
}

bool Creature::internalFollowCreature(const Creature* creature)
{
	if(getBaseSpeed() <= 0){
		setFollowCreature(NULL);
		return false;
	}

	if(creature && !canSee(creature->getPosition())){
		setFollowCreature(NULL);
		return false;
	}
	
	setFollowCreature(creature);

	if(creature){
		listWalkDir.clear();
		uint32_t maxDistance = getFollowDistance();
		bool fullPath = getFullPathSearch();
		bool reachable = getFollowReachable();
		if(!g_game.getPathToEx(this, creature->getPosition(), 1, maxDistance, fullPath, reachable, listWalkDir)){
			setFollowCreature(NULL);
			return false;
		}

		startAutoWalk(listWalkDir);
	}
	
	//setFollowCreature(creature);
	return true;
}
*/

double Creature::getDamageRatio(Creature* attacker) const
{
	int32_t totalDamage = 0;
	int32_t attackerDamage = 0;

	for(DamageMap::const_iterator it = damageMap.begin(); it != damageMap.end(); ++it){
		totalDamage += (*it).second;

		if((*it).first == attacker->getID()){
			attackerDamage += (*it).second;
		}
	}

	return ((double)attackerDamage / totalDamage);
}

int32_t Creature::getGainedExperience(Creature* attacker) const
{
	int32_t lostExperience = getLostExperience();
	return (int32_t)std::floor(getDamageRatio(attacker) * lostExperience);
}

bool Creature::addDamagePoints(Creature* attacker, int32_t damagePoints)
{
	if(damagePoints > 0){
		uint32_t attackerId = (attacker ? attacker->getID() : 0);

		damageMap[attackerId] += damagePoints;
		lastHitCreature = attackerId;
	}

	return true;
}

void Creature::onAddCondition(ConditionType_t type)
{
	if(type == CONDITION_PARALYZE && hasCondition(CONDITION_HASTE)){
		removeCondition(CONDITION_HASTE);
	}
	else if(type == CONDITION_HASTE && hasCondition(CONDITION_PARALYZE)){
		removeCondition(CONDITION_PARALYZE);
	}
}

void Creature::onEndCondition(ConditionType_t type)
{
	//
}

void Creature::onTickCondition(ConditionType_t type, bool& bRemove)
{
	if(const MagicField* field = getTile()->getFieldItem()){
		switch(type){
			case CONDITION_FIRE: bRemove = (field->getCombatType() != COMBAT_FIREDAMAGE); break;
			case CONDITION_ENERGY: bRemove = (field->getCombatType() != COMBAT_ENERGYDAMAGE); break;
			case CONDITION_POISON: bRemove = (field->getCombatType() != COMBAT_POISONDAMAGE); break;
			default: 
				break;
		}
	}
}

void Creature::onCombatRemoveCondition(const Creature* attacker, Condition* condition)
{
	removeCondition(condition);
}

void Creature::onAttackedCreature(Creature* target)
{
	//
}

void Creature::onAttacked()
{
	//
}

void Creature::onAttackedCreatureDrainHealth(Creature* target, int32_t points)
{
	target->addDamagePoints(this, points);
}

void Creature::onAttackedCreatureKilled(Creature* target)
{
	if(target != this){
		int32_t gainedExperience = target->getGainedExperience(this);
		onGainExperience(gainedExperience);
	}
}

void Creature::onKilledCreature(Creature* target)
{
	//
}

void Creature::onGainExperience(int32_t gainExperience)
{
	if(gainExperience > 0){

		if(getMaster()){
			gainExperience = gainExperience / 2;
			getMaster()->onGainExperience(gainExperience);
		}

		std::stringstream strExp;
		strExp << gainExperience;

		g_game.addAnimatedText(getPosition(), TEXTCOLOR_WHITE_EXP, strExp.str());
	}
}

void Creature::onAttackedCreatureBlockHit(Creature* target, BlockType_t blockType)
{
	//
}

void Creature::addSummon(Creature* creature)
{
	//std::cout << "addSummon: " << this << " summon=" << creature << std::endl;
	creature->setMaster(this);
	creature->useThing2();
	summons.push_back(creature);
}

void Creature::removeSummon(const Creature* creature)
{
	//std::cout << "removeSummon: " << this << " summon=" << creature << std::endl;
	std::list<Creature*>::iterator cit = std::find(summons.begin(), summons.end(), creature);
	if(cit != summons.end()) {
		(*cit)->setMaster(NULL);
		(*cit)->releaseThing2();
		summons.erase(cit);
	}
}

bool Creature::addCondition(Condition* condition)
{
	if(condition == NULL){
		return false;
	}

	Condition* prevCond = getCondition(condition->getType(), condition->getId());

	if(prevCond){
		prevCond->addCondition(this, condition);
		delete condition;
	}
	else{
		if(condition->startCondition(this)){
			conditions.push_back(condition);
			onAddCondition(condition->getType());
		}
		else{
			delete condition;
			return false;
		}
	}

	return true;
}

void Creature::removeCondition(ConditionType_t type)
{
	for(ConditionList::iterator it = conditions.begin(); it != conditions.end();){
		if((*it)->getType() == type){
			Condition* condition = *it;
			it = conditions.erase(it);

			condition->endCondition(this, REASON_ABORT);
			delete condition;

			onEndCondition(type);
		}
		else{
			++it;
		}
	}
}

void Creature::removeCondition(ConditionType_t type, ConditionId_t id)
{
	for(ConditionList::iterator it = conditions.begin(); it != conditions.end();){
		if((*it)->getType() == type && (*it)->getId() == id){
			Condition* condition = *it;
			it = conditions.erase(it);

			condition->endCondition(this, REASON_ABORT);
			delete condition;

			onEndCondition(type);
		}
		else{
			++it;
		}
	}
}

void Creature::removeCondition(const Creature* attacker, ConditionType_t type)
{
	ConditionList tmpList = conditions;

	for(ConditionList::iterator it = tmpList.begin(); it != tmpList.end(); ++it){
		if((*it)->getType() == type){
			onCombatRemoveCondition(attacker, *it);
		}
	}
}

void Creature::removeCondition(Condition* condition)
{
	ConditionList::iterator it = std::find(conditions.begin(), conditions.end(), condition);

	if(it != conditions.end()){
		Condition* condition = *it;
		it = conditions.erase(it);

		condition->endCondition(this, REASON_ABORT);
		onEndCondition(condition->getType());
		delete condition;
	}
}

Condition* Creature::getCondition(ConditionType_t type, ConditionId_t id) const
{
	for(ConditionList::const_iterator it = conditions.begin(); it != conditions.end(); ++it){
		if((*it)->getType() == type && (*it)->getId() == id){
			return *it;
		}
	}

	return NULL;
}

void Creature::executeConditions(int32_t newticks)
{
	for(ConditionList::iterator it = conditions.begin(); it != conditions.end();){
		//(*it)->executeCondition(this, newticks);
		//if((*it)->getTicks() <= 0){

		if(!(*it)->executeCondition(this, newticks)){
			ConditionType_t type = (*it)->getType();

			Condition* condition = *it;
			it = conditions.erase(it);

			condition->endCondition(this, REASON_ENDTICKS);
			delete condition;

			onEndCondition(type);
		}
		else{
			++it;
		}
	}
}

bool Creature::hasCondition(ConditionType_t type) const
{
	if(isSuppress(type)){
		return false;
	}

	for(ConditionList::const_iterator it = conditions.begin(); it != conditions.end(); ++it){
		if((*it)->getType() == type){
			return true;
		}
	}

	return false;
}

bool Creature::isImmune(CombatType_t type) const
{
	return ((getDamageImmunities() & (uint32_t)type) == (uint32_t)type);
}

bool Creature::isImmune(ConditionType_t type) const
{
	return ((getConditionImmunities() & (uint32_t)type) == (uint32_t)type);
}

bool Creature::isSuppress(ConditionType_t type) const
{
	return ((getConditionSuppressions() & (uint32_t)type) == (uint32_t)type);
}

std::string Creature::getDescription(int32_t lookDistance) const
{
	std::string str = "a creature";
	return str;
}

int Creature::getStepDuration() const
{
	OTSYS_THREAD_LOCK_CLASS lockClass(g_game.gameLock, "Creature::getStepDuration()");

	int32_t duration = 0;

	if(isRemoved()){
		return duration;
	}

	const Position& tilePos = getPosition();
	Tile* tile = g_game.getTile(tilePos.x, tilePos.y, tilePos.z);
	if(tile && tile->ground){
		uint32_t groundId = tile->ground->getID();
		uint16_t stepSpeed = Item::items[groundId].speed;

		if(stepSpeed != 0){
			duration =  (1000 * stepSpeed) / getSpeed();
		}
	}

	return duration * lastStepCost;
};

int64_t Creature::getSleepTicks() const
{
	int64_t delay = 0;
	int stepDuration = getStepDuration();
	
	if(lastMove != 0) {
		delay = (((int64_t)(lastMove)) + ((int64_t)(stepDuration))) - ((int64_t)(OTSYS_TIME()));
	}
	
	return delay;
}

int64_t Creature::getEventStepTicks() const
{
	int64_t ret = getSleepTicks();

	if(ret <= 0){
		ret = getStepDuration();
	}

	return ret;
}

void Creature::getCreatureLight(LightInfo& light) const
{
	light = internalLight;
}

void Creature::setNormalCreatureLight()
{
	internalLight.level = 0;
	internalLight.color = 0;
}
