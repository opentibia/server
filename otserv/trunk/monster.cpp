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

#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

#include "monster.h"
#include "monsters.h"
#include "game.h"
#include "spells.h"
#include "combat.h"
#include "spawn.h"
#include "configmanager.h"
#include "party.h"

extern Game g_game;
extern ConfigManager g_config;
extern Monsters g_monsters;

AutoList<Monster>Monster::listMonster;

int32_t Monster::despawnRange;
int32_t Monster::despawnRadius;

#ifdef __ENABLE_SERVER_DIAGNOSTIC__
uint32_t Monster::monsterCount = 0;
#endif

Monster* Monster::createMonster(MonsterType* mType)
{
	return new Monster(mType);
}

Monster* Monster::createMonster(const std::string& name)
{
	MonsterType* mType = g_monsters.getMonsterType(name);
	if(!mType){
		return NULL;
	}

	return createMonster(mType);
}

Monster::Monster(MonsterType* _mtype) :
Creature()
{
	isIdle = true;
	isMasterInRange = false;
	mType = _mtype;
	spawn = NULL;
	defaultOutfit = mType->outfit;
	currentOutfit = mType->outfit;

	health     = mType->health;
	healthMax  = mType->health_max;
	baseSpeed = mType->base_speed;
	internalLight.level = mType->lightLevel;
	internalLight.color = mType->lightColor;

	minCombatValue = 0;
	maxCombatValue = 0;

	targetTicks = 0;
	targetChangeTicks = 0;
	targetChangeCooldown = 0;
	attackTicks = 0;
	defenseTicks = 0;
	yellTicks = 0;
	extraMeleeAttack = false;

	strDescription = mType->nameDescription;
	toLowerCaseString(strDescription);

	// register creature events
	MonsterScriptList::iterator it;
	for(it = mType->scriptList.begin(); it != mType->scriptList.end(); ++it){
		if(!registerCreatureEvent(*it)){
			std::cout << "Warning: [Monster::Monster]. Unknown event name - " << *it << std::endl;
		}
	}


#ifdef __ENABLE_SERVER_DIAGNOSTIC__
	monsterCount++;
#endif
}

Monster::~Monster()
{
	clearTargetList();
	clearFriendList();

#ifdef __ENABLE_SERVER_DIAGNOSTIC__
	monsterCount--;
#endif
}

void Monster::onAttackedCreatureDissapear(bool isLogout)
{
#ifdef __DEBUG__
	std::cout << "Attacked creature dissapeared." << std::endl;
#endif

	attackTicks = 0;
	extraMeleeAttack = true;
}

void Monster::onFollowCreatureDissapear(bool isLogout)
{
#ifdef __DEBUG__
	std::cout << "Follow creature dissapeared." << std::endl;
#endif
}

void Monster::onAttackedCreature(Creature* target)
{
	Creature::onAttackedCreature(target);

	if(isSummon()){
		getMaster()->onSummonAttackedCreature(this, target);
	}
}

void Monster::onAttackedCreatureDrainHealth(Creature* target, int32_t points)
{
	Creature::onAttackedCreatureDrainHealth(target, points);
	
	if(isSummon()){
		getMaster()->onSummonAttackedCreatureDrainHealth(this, target, points);
	}
}

void Monster::onAttackedCreatureDrainMana(Creature* target, int32_t points)
{
	Creature::onAttackedCreatureDrainMana(target, points);

	if(isSummon()){
		getMaster()->onSummonAttackedCreatureDrainMana(this, target, points);
	}
}

void Monster::onCreatureAppear(const Creature* creature, bool isLogin)
{
	Creature::onCreatureAppear(creature, isLogin);

	if(creature == this){
		//We just spawned lets look around to see who is there.
		if(isSummon()){
			isMasterInRange = canSee(getMaster()->getPosition());
		}
		updateTargetList();
		updateIdleStatus();
	}
	else{
		onCreatureEnter(const_cast<Creature*>(creature));
	}
}

void Monster::onCreatureDisappear(const Creature* creature, bool isLogout)
{
	Creature::onCreatureDisappear(creature, isLogout);

	if(creature == this){
		if(spawn){
			spawn->startSpawnCheck();
		}

		setIdle(true);
	}
	else{
		onCreatureLeave(const_cast<Creature*>(creature));
	}
}

void Monster::onCreatureMove(const Creature* creature, const Tile* newTile, const Position& newPos,
	const Tile* oldTile, const Position& oldPos, bool teleport)
{
	Creature::onCreatureMove(creature, newTile, newPos, oldTile, oldPos, teleport);

	if(creature == this){
		if(isSummon()){
			isMasterInRange = canSee(getMaster()->getPosition());
		}

		updateTargetList();
		updateIdleStatus();

		/*
		TODO: Optimizations here
		if(teleport){
			//do a full update of the friend/target list
		}
		else{
			//partial update of the friend/target list
		}
		*/
	}
	else{
		bool canSeeNewPos = canSee(newPos);
		bool canSeeOldPos = canSee(oldPos);

		if(canSeeNewPos && !canSeeOldPos){
			onCreatureEnter(const_cast<Creature*>(creature));
		}
		else if(!canSeeNewPos && canSeeOldPos){
			onCreatureLeave(const_cast<Creature*>(creature));
		}

		if(isSummon() && getMaster() == creature){
			if(canSeeNewPos){
				//Turn the summon on again
				isMasterInRange = true;
				updateIdleStatus();
			}
		}

		if(!followCreature && !isSummon()){
			//we have no target lets try pick this one
			if(isOpponent(creature)){
				selectTarget(const_cast<Creature*>(creature));
			}
		}
	}
}

void Monster::updateTargetList()
{
	CreatureList::iterator it;
	for(it = friendList.begin(); it != friendList.end();){
		if((*it)->getHealth() <= 0 || !canSee((*it)->getPosition())){
			(*it)->releaseThing2();
			it = friendList.erase(it);
		}
		else
			++it;
	}

	for(it = targetList.begin(); it != targetList.end();){
		if((*it)->getHealth() <= 0 || !canSee((*it)->getPosition())){
			(*it)->releaseThing2();
			it = targetList.erase(it);
		}
		else
			++it;
	}

	const SpectatorVec& list = g_game.getSpectators(getPosition());
	for(SpectatorVec::const_iterator it = list.begin(); it != list.end(); ++it){
		if((*it) != this && canSee((*it)->getPosition())){
			onCreatureFound(*it);
		}
	}
}

void Monster::clearTargetList()
{
	for(CreatureList::iterator it = targetList.begin(); it != targetList.end(); ++it){
		(*it)->releaseThing2();
	}
	targetList.clear();
}

void Monster::clearFriendList()
{
	for(CreatureList::iterator it = friendList.begin(); it != friendList.end(); ++it){
		(*it)->releaseThing2();
	}
	friendList.clear();
}

void Monster::onCreatureFound(Creature* creature, bool pushFront /*= false*/)
{
	if(isFriend(creature)){
		assert(creature != this);
		if(std::find(friendList.begin(), friendList.end(), creature) == friendList.end()){
			creature->useThing2();
			friendList.push_back(creature);
		}
	}

	if(isOpponent(creature)){
		assert(creature != this);
		if(std::find(targetList.begin(), targetList.end(), creature) == targetList.end()){
			creature->useThing2();
			if(pushFront){
				targetList.push_front(creature);
			}
			else{
				targetList.push_back(creature);
			}

			updateIdleStatus();
		}
	}
}

void Monster::onCreatureEnter(Creature* creature)
{
	//std::cout << "onCreatureEnter - " << creature->getName() << std::endl;

	if(getMaster() == creature){
		//Turn the summon on again
		isMasterInRange = true;
		updateIdleStatus();
	}

	onCreatureFound(creature, true);
}


bool Monster::isFriend(const Creature* creature)
{
	if(isPlayerSummon()){
		const Player* masterPlayer = getPlayerMaster();
		const Player* tmpPlayer = NULL;
		if(creature->getPlayer()){
			tmpPlayer = creature->getPlayer();
		}
		else if(creature->isPlayerSummon()){
			tmpPlayer = creature->getPlayerMaster();
		}

		if(tmpPlayer && (tmpPlayer == getMaster() || masterPlayer->isPartner(tmpPlayer)) ){
			return true;
		}
	}
	else{
		if(creature->getMonster() && !creature->isSummon()){
			return true;
		}
	}

	return false;
}

bool Monster::isOpponent(const Creature* creature)
{
	if(isPlayerSummon()){
		if(creature != getMaster()){
			return true;
		}
	}
	else{
		if( (creature->getPlayer() && !creature->getPlayer()->hasFlag(PlayerFlag_IgnoredByMonsters)) ||
			(creature->isPlayerSummon()) ) {
			return true;
		}
	}

	return false;
}

void Monster::onCreatureLeave(Creature* creature)
{
	//std::cout << "onCreatureLeave - " << creature->getName() << std::endl;

	if(getMaster() == creature){
		//Turn the monster off until its master comes back
		isMasterInRange = false;
		updateIdleStatus();
	}

	//update friendList
	if(isFriend(creature)){
		CreatureList::iterator it = std::find(friendList.begin(), friendList.end(), creature);
		if(it != friendList.end()){
			(*it)->releaseThing2();
			friendList.erase(it);
		}
#ifdef __DEBUG__
		else{
			std::cout << "Monster: " << creature->getName() << " not found in the friendList." << std::endl;
		}
#endif
	}

	//update targetList
	if(isOpponent(creature)){
		CreatureList::iterator it = std::find(targetList.begin(), targetList.end(), creature);
		if(it != targetList.end()){
			(*it)->releaseThing2();
			targetList.erase(it);
			if(targetList.empty()){
				updateIdleStatus();
			}
		}
#ifdef __DEBUG__
		else{
			std::cout << "Player: " << creature->getName() << " not found in the targetList." << std::endl;
		}
#endif
	}
}

bool Monster::searchTarget(TargetSearchType_t searchType /*= TARGETSEARCH_DEFAULT*/)
{
#ifdef __DEBUG__
	std::cout << "Searching target... " << std::endl;
#endif

	std::list<Creature*> resultList;
	const Position& myPos = getPosition();
	for(CreatureList::iterator it = targetList.begin(); it != targetList.end(); ++it){
		if(followCreature != (*it) && isTarget(*it)){
			if(searchType == TARGETSEARCH_RANDOM || canUseAttack(myPos, *it)){
				resultList.push_back(*it);
			}
		}
	}

	if(!resultList.empty()){
		uint32_t index = random_range(0, resultList.size() - 1);
		CreatureList::iterator it = resultList.begin();
		std::advance(it, index);
#ifdef __DEBUG__
		std::cout << "Selecting target " << (*it)->getName() << std::endl;
#endif
		return selectTarget(*it);
	}

	if(searchType == TARGETSEARCH_ATTACKRANGE){
		return false;
	}

	//lets just pick the first target in the list
	for(CreatureList::iterator it = targetList.begin(); it != targetList.end(); ++it){
		if(followCreature != (*it) && selectTarget(*it)){
#ifdef __DEBUG__
			std::cout << "Selecting target " << (*it)->getName() << std::endl;
#endif
			return true;
		}
	}

	return false;
}

void Monster::onFollowCreatureComplete(const Creature* creature)
{
	if(creature){
		CreatureList::iterator it = std::find(targetList.begin(), targetList.end(), creature);
		Creature* target;
		if(it != targetList.end()){
			target = (*it);
			targetList.erase(it);

			if(hasFollowPath){
				//push target we have found a path to the front
				targetList.push_front(target);
			}
			else if(!isSummon()){
				//push target we have not found a path to the back
				targetList.push_back(target);
			}
			else{
				//Since we removed the creature from the targetList (and not put it back) we have to release it too
				target->releaseThing2();
			}
		}
	}
}

BlockType_t Monster::blockHit(Creature* attacker, CombatType_t combatType, int32_t& damage,
	bool checkDefense /* = false*/, bool checkArmor /* = false*/)
{
	BlockType_t blockType = Creature::blockHit(attacker, combatType, damage, checkDefense, checkArmor);

	if(damage != 0){
		int32_t elementMod = 0;
		ElementMap::iterator it = mType->elementMap.find(combatType);
		if(it != mType->elementMap.end()){
			elementMod = it->second;
		}

		if(elementMod != 0)
			damage = (int32_t)std::ceil(damage * ((float)(100 - elementMod) / 100));
			if(damage <= 0){
				damage = 0;
				blockType = BLOCK_DEFENSE;
			}
	}

	return blockType;
}

bool Monster::isTarget(Creature* creature)
{
	if( creature->isRemoved() ||
		!creature->isAttackable() ||
		creature->getZone() == ZONE_PROTECTION ||
		!canSeeCreature(creature)){
		return false;
	}

	if(creature->getPosition().z != getPosition().z){
		return false;
	}

	return true;
}

bool Monster::selectTarget(Creature* creature)
{
#ifdef __DEBUG__
	std::cout << "Selecting target... " << std::endl;
#endif

	if(!isTarget(creature)){
		return false;
	}

	CreatureList::iterator it = std::find(targetList.begin(), targetList.end(), creature);
	if(it == targetList.end()){
		//Target not found in our target list.
#ifdef __DEBUG__
		std::cout << "Target not found in targetList." << std::endl;
#endif
		return false;
	}

	if(isHostile() || isSummon()){
		if(setAttackedCreature(creature) && !isSummon()){
			g_dispatcher.addTask(createTask(
				boost::bind(&Game::checkCreatureAttack, &g_game, getID())));
		}
	}

	return setFollowCreature(creature, true);
}

void Monster::setIdle(bool _idle)
{
	isIdle = _idle;

	if(!isIdle){
		g_game.addCreatureCheck(this);
	}
	else{
		onIdleStatus();
		g_game.removeCreatureCheck(this);
	}
}

void Monster::updateIdleStatus()
{
	bool idle = false;

	if(conditions.empty()){
		if(isSummon()){
			if(!isMasterInRange){
				idle = true;
			}
			else if(getMaster()->getMonster() && getMaster()->getMonster()->getIdleStatus()){
				idle = true;
			}
		}
		else{
			if(targetList.empty()){
				idle = true;
			}
		}
	}

	setIdle(idle);
}

void Monster::onAddCondition(ConditionType_t type, bool hadCondition)
{
	Creature::onAddCondition(type, hadCondition);

	//the walkCache need to be updated if the monster becomes "resistent" to the damage, see Tile::__queryAdd()
	if(type == CONDITION_FIRE || type == CONDITION_ENERGY || type == CONDITION_POISON){
		updateMapCache();
	}

	updateIdleStatus();
}

void Monster::onEndCondition(ConditionType_t type, bool lastCondition)
{
	Creature::onEndCondition(type, lastCondition);

	//the walkCache need to be updated if the monster loose the "resistent" to the damage, see Tile::__queryAdd()
	if(type == CONDITION_FIRE || type == CONDITION_ENERGY || type == CONDITION_POISON){
		updateMapCache();
	}

	updateIdleStatus();
}

void Monster::onThink(uint32_t interval)
{
	Creature::onThink(interval);

	if(despawn()){
		g_game.removeCreature(this, true);
		setIdle(true);
	}
	else{
		updateIdleStatus();
		if(!isIdle){
			addEventWalk();

			if(isSummon()){
				if(!attackedCreature){
					if(getMaster() && getMaster()->getAttackedCreature()){
						///This happens if the monster is summoned during combat
						selectTarget(getMaster()->getAttackedCreature());
					}
					else if(getMaster() != followCreature){
						//Our master has not ordered us to attack anything, lets follow him around instead.
						setFollowCreature(getMaster());
					}
				}
				else if(attackedCreature == this){
					setFollowCreature(NULL);
				}
				else if(followCreature != attackedCreature){
					//This happens just after a master orders an attack, so lets follow it aswell.
					setFollowCreature(attackedCreature);
				}
			}
			else if(!targetList.empty()){
				if(!followCreature || !hasFollowPath){
					searchTarget();
				}
				else if(isFleeing()){
					if(attackedCreature && !canUseAttack(getPosition(), attackedCreature)){
						searchTarget(TARGETSEARCH_ATTACKRANGE);
					}
				}
			}

			onThinkTarget(interval);
			onThinkYell(interval);
			onThinkDefense(interval);
		}
	}
}

void Monster::doAttacking(uint32_t interval)
{
	if(!attackedCreature || (isSummon() && attackedCreature == this)){
		return;
	}

	bool updateLook = true;
	bool outOfRange = true;

	resetTicks = interval != 0;
	attackTicks += interval;

	const Position& myPos = getPosition();
	const Position& targetPos = attackedCreature->getPosition();

	for(SpellList::iterator it = mType->spellAttackList.begin(); it != mType->spellAttackList.end(); ++it){
		bool inRange = false;
		if(canUseSpell(myPos, targetPos, *it, interval, inRange)){
			if(it->chance >= (uint32_t)random_range(1, 100)){
				if(updateLook){
					updateLookDirection();
					updateLook = false;
				}

				minCombatValue = it->minCombatValue;
				maxCombatValue = it->maxCombatValue;
				it->spell->castSpell(this, attackedCreature);
				if(it->isMelee){
					extraMeleeAttack = false;
				}

#ifdef __DEBUG__
				static uint64_t prevTicks = OTSYS_TIME();
				std::cout << "doAttacking ticks: " << OTSYS_TIME() - prevTicks << std::endl;
				prevTicks = OTSYS_TIME();
#endif
			}
		}

		if(inRange){
			outOfRange = false;
		}
		else if(it->isMelee){
			//melee swing out of reach
			extraMeleeAttack = true;
		}
	}

	if(updateLook){
		updateLookDirection();
	}

	if(resetTicks){
		attackTicks = 0;
	}
}

bool Monster::canUseAttack(const Position& pos, const Creature* target) const
{
	if(isHostile()){
		const Position& targetPos = target->getPosition();
		for(SpellList::iterator it = mType->spellAttackList.begin(); it != mType->spellAttackList.end(); ++it){
			if((*it).range != 0 && std::max(std::abs(pos.x - targetPos.x), std::abs(pos.y - targetPos.y)) <= (int32_t)(*it).range){
				return g_game.isSightClear(pos, targetPos, true);
			}
		}

		return false;
	}

	return true;
}

bool Monster::canUseSpell(const Position& pos, const Position& targetPos,
	const spellBlock_t& sb, uint32_t interval, bool& inRange)
{
	inRange = true;

	if(!sb.isMelee || !extraMeleeAttack){
		if(sb.speed > attackTicks){
			resetTicks = false;
			return false;
		}

		if(attackTicks % sb.speed >= interval){
			//already used this spell for this round
			return false;
		}
	}

	if(sb.range != 0 && std::max(std::abs(pos.x - targetPos.x), std::abs(pos.y - targetPos.y)) > (int32_t)sb.range){
		inRange = false;
		return false;
	}

	return true;
}

void Monster::onThinkTarget(uint32_t interval)
{
	if(!isSummon()){
		if(mType->changeTargetSpeed > 0){
			bool canChangeTarget = true;
			if(targetChangeCooldown > 0){
				targetChangeCooldown -= interval;
				if(targetChangeCooldown <= 0){
					targetChangeCooldown = 0;
					targetChangeTicks = (uint32_t)mType->changeTargetSpeed;
				}
				else{
					canChangeTarget = false;
				}
			}

			if(canChangeTarget){
				targetChangeTicks += interval;

				if(targetChangeTicks >= (uint32_t)mType->changeTargetSpeed){
					targetChangeTicks = 0;
					targetChangeCooldown = (uint32_t)mType->changeTargetSpeed;

					if(mType->changeTargetChance >= random_range(1, 100)){
						searchTarget(TARGETSEARCH_RANDOM);
					}
				}
			}
		}
	}
}

void Monster::onThinkDefense(uint32_t interval)
{
	resetTicks = true;
	defenseTicks += interval;

	for(SpellList::iterator it = mType->spellDefenseList.begin(); it != mType->spellDefenseList.end(); ++it){
		if(it->speed > defenseTicks){
			resetTicks = false;
			continue;
		}

		if(defenseTicks % it->speed >= interval){
			//already used this spell for this round
			continue;
		}

		if((it->chance >= (uint32_t)random_range(1, 100))){
			minCombatValue = it->minCombatValue;
			maxCombatValue = it->maxCombatValue;
			it->spell->castSpell(this, this);
		}
	}

	if(!isSummon() && (int32_t)summons.size() < mType->maxSummons){
		for(SummonList::iterator it = mType->summonList.begin(); it != mType->summonList.end(); ++it){
			if(it->speed > defenseTicks){
				resetTicks = false;
				continue;
			}

			if((int32_t)summons.size() >= mType->maxSummons){
				continue;
			}

			if(defenseTicks % it->speed >= interval){
				//already used this spell for this round
				continue;
			}

			if((it->chance >= (uint32_t)random_range(1, 100))){
				Monster* summon = Monster::createMonster(it->name);
				if(summon){
					const Position& summonPos = getPosition();

					addSummon(summon);
					if(g_game.placeCreature(summon, summonPos)){
						g_game.addMagicEffect(summon->getPosition(), NM_ME_TELEPORT);
						g_game.addMagicEffect(getPosition(), NM_ME_MAGIC_ENERGY);
					}
					else{
						removeSummon(summon);
					}
				}
			}
		}
	}

	if(resetTicks){
		defenseTicks = 0;
	}
}

void Monster::onThinkYell(uint32_t interval)
{
	if(mType->yellSpeedTicks > 0){
		yellTicks += interval;

		if(yellTicks >= mType->yellSpeedTicks){
			yellTicks = 0;

			if(!mType->voiceVector.empty() && (mType->yellChance >= (uint32_t)random_range(1, 100))){
				uint32_t index = random_range(0, mType->voiceVector.size() - 1);
				const voiceBlock_t& vb = mType->voiceVector[index];

				if(vb.yellText){
					g_game.internalCreatureSay(this, SPEAK_MONSTER_YELL, vb.text);
				}
				else{
					g_game.internalCreatureSay(this, SPEAK_MONSTER_SAY, vb.text);
				}
			}
		}
	}
}

void Monster::onWalk()
{
	Creature::onWalk();
}

bool Monster::pushItem(Item* item, int32_t radius)
{
	const Position& centerPos = item->getPosition();

	typedef std::pair<int32_t, int32_t> relPair;
	std::vector<relPair> relList;
	relList.push_back(relPair(-1, -1));
	relList.push_back(relPair(-1, 0));
	relList.push_back(relPair(-1, 1));
	relList.push_back(relPair(0, -1));
	relList.push_back(relPair(0, 1));
	relList.push_back(relPair(1, -1));
	relList.push_back(relPair(1, 0));
	relList.push_back(relPair(1, 1));

	std::random_shuffle(relList.begin(), relList.end());

	Position tryPos;
	for(int32_t n = 1; n <= radius; ++n){
		for(std::vector<relPair>::iterator it = relList.begin(); it != relList.end(); ++it){
			int32_t dx = it->first * n;
			int32_t dy = it->second * n;

			tryPos = centerPos;
			tryPos.x = tryPos.x + dx;
			tryPos.y = tryPos.y + dy;

			Tile* tile = g_game.getTile(tryPos.x, tryPos.y, tryPos.z);
			if(tile && g_game.canThrowObjectTo(centerPos, tryPos)){
				if(g_game.internalMoveItem(item->getParent(), tile,
					INDEX_WHEREEVER, item, item->getItemCount(), NULL) == RET_NOERROR){
					return true;
				}
			}
		}
	}

	return false;
}

void Monster::pushItems(Tile* tile)
{
	//We can not use iterators here since we can push the item to another tile
	//which will invalidate the iterator.
	//start from the end to minimize the amount of traffic
	if(TileItemVector* items = tile->getItemList()){
		uint32_t moveCount = 0;
		uint32_t removeCount = 0;
		int32_t downItemSize = tile->getDownItemCount();

		for(int32_t i = downItemSize - 1; i >= 0; --i){
			assert(i >= 0 && i < downItemSize);
			Item* item = items->at(i);
			if(item && item->hasProperty(MOVEABLE) && (item->hasProperty(BLOCKPATH)
				|| item->hasProperty(BLOCKSOLID))){
					if(moveCount < 20 && pushItem(item, 1)){
						moveCount++;
					}
					else if(g_game.internalRemoveItem(item) == RET_NOERROR){
						++removeCount;
					}
			}
		}

		if(removeCount > 0){
			g_game.addMagicEffect(tile->getPosition(), NM_ME_PUFF);
		}
	}
}

bool Monster::pushCreature(Creature* creature)
{
	Position monsterPos = creature->getPosition();

	std::vector<Direction> dirList;
	dirList.push_back(NORTH);
	dirList.push_back(SOUTH);
	dirList.push_back(WEST);
	dirList.push_back(EAST);

	std::random_shuffle(dirList.begin(), dirList.end());

	for(std::vector<Direction>::iterator it = dirList.begin(); it != dirList.end(); ++it){
		const Position& tryPos = Spells::getCasterPosition(creature, *it);
		Tile* toTile = g_game.getTile(tryPos.x, tryPos.y, tryPos.z);

		if(toTile && !toTile->hasProperty(BLOCKPATH)){
			if(g_game.internalMoveCreature(creature, *it) == RET_NOERROR){
				return true;
			}
		}
	}

	return false;
}

void Monster::pushCreatures(Tile* tile)
{
	//We can not use iterators here since we can push a creature to another tile
	//which will invalidate the iterator.
	if(CreatureVector* creatures = tile->getCreatures()){
		uint32_t removeCount = 0;

		for(uint32_t i = 0; i < creatures->size();){
			Monster* monster = creatures->at(i)->getMonster();

			if(monster && monster->isPushable()){
				if(pushCreature(monster)){
					continue;
				}
				else{
					monster->changeHealth(-monster->getHealth());
					monster->setDropLoot(false);
					removeCount++;
				}
			}

			++i;
		}

		if(removeCount > 0){
			g_game.addMagicEffect(tile->getPosition(), NM_ME_BLOCKHIT);
		}
	}
}

bool Monster::getNextStep(Direction& dir, uint32_t& flags)
{
	if(isIdle || getHealth() <= 0){
		//we dont have anyone watching might aswell stop walking
		eventWalk = 0;
		return false;
	}

	bool result = false;
	if((!followCreature || !hasFollowPath) && !isSummon()){
		if(followCreature){
			result = getRandomStep(getPosition(), dir);
		}else{
			if(getTimeSinceLastMove() > 1000){
				//choose a random direction
				result = getRandomStep(getPosition(), dir);
			}
		}
	}
	else if(isSummon() || followCreature){
		result = Creature::getNextStep(dir, flags);
		if(result){
			flags |= FLAG_PATHFINDING;
		}
		else{
			//target dancing
			if(attackedCreature && attackedCreature == followCreature){
				if(isFleeing()){
					result = getDanceStep(getPosition(), dir, false, false);
				}
				else if(mType->staticAttackChance < (uint32_t)random_range(1, 100)){
					result = getDanceStep(getPosition(), dir);
				}
			}
		}
	}

	if(result && (canPushItems() || canPushCreatures()) ){
		const Position& pos = Spells::getCasterPosition(this, dir);
		Tile* tile = g_game.getTile(pos.x, pos.y, pos.z);
		if(tile){
			if(canPushItems()){
				pushItems(tile);
			}

			if(canPushCreatures()){
				pushCreatures(tile);
			}
		}
#ifdef __DEBUG__
		else{
			std::cout << "getNextStep - no tile." << std::endl;
		}
#endif
	}

	return result;
}

bool Monster::getRandomStep(const Position& creaturePos, Direction& dir)
{
	std::vector<Direction> dirList;

	dirList.push_back(NORTH);
	dirList.push_back(SOUTH);
	dirList.push_back(WEST);
	dirList.push_back(EAST);
	std::random_shuffle(dirList.begin(), dirList.end());

	for(std::vector<Direction>::iterator it = dirList.begin(); it != dirList.end(); ++it){
		if(canWalkTo(creaturePos, *it)){
			dir = *it;
			return true;
		}
	}

	return false;
}

bool Monster::getDanceStep(const Position& creaturePos, Direction& dir,
	bool keepAttack /*= true*/, bool keepDistance /*= true*/)
{
	bool canDoAttackNow = canUseAttack(creaturePos, attackedCreature);

	assert(attackedCreature != NULL);
	const Position& centerPos = attackedCreature->getPosition();
	uint32_t centerToDist = std::max(std::abs(creaturePos.x - centerPos.x), std::abs(creaturePos.y - centerPos.y));
	uint32_t tmpDist;

	std::vector<Direction> dirList;

	if(!keepDistance || creaturePos.y - centerPos.y >= 0){
		tmpDist = std::max(std::abs((creaturePos.x) - centerPos.x), std::abs((creaturePos.y - 1) - centerPos.y));
		if(tmpDist == centerToDist && canWalkTo(creaturePos, NORTH)){
			bool result = true;
			if(keepAttack){
				result = (!canDoAttackNow || canUseAttack(Position(creaturePos.x, creaturePos.y - 1, creaturePos.z), attackedCreature));
			}
			if(result){
				dirList.push_back(NORTH);
			}
		}
	}

	if(!keepDistance || creaturePos.y - centerPos.y <= 0){
		tmpDist = std::max(std::abs((creaturePos.x) - centerPos.x), std::abs((creaturePos.y + 1) - centerPos.y));
		if(tmpDist == centerToDist && canWalkTo(creaturePos, SOUTH)){
			bool result = true;
			if(keepAttack){
				result = (!canDoAttackNow || canUseAttack(Position(creaturePos.x, creaturePos.y + 1, creaturePos.z), attackedCreature));
			}
			if(result){
				dirList.push_back(SOUTH);
			}
		}
	}

	if(!keepDistance || creaturePos.x - centerPos.x >= 0){
		tmpDist = std::max(std::abs((creaturePos.x + 1) - centerPos.x), std::abs((creaturePos.y) - centerPos.y));
		if(tmpDist == centerToDist && canWalkTo(creaturePos, EAST)){
			bool result = true;
			if(keepAttack){
				result = (!canDoAttackNow || canUseAttack(Position(creaturePos.x + 1, creaturePos.y, creaturePos.z), attackedCreature));
			}
			if(result){
				dirList.push_back(EAST);
			}
		}
	}

	if(!keepDistance || creaturePos.x - centerPos.x <= 0){
		tmpDist = std::max(std::abs((creaturePos.x - 1) - centerPos.x), std::abs((creaturePos.y) - centerPos.y));
		if(tmpDist == centerToDist && canWalkTo(creaturePos, WEST)){
			bool result = true;
			if(keepAttack){
				result = (!canDoAttackNow || canUseAttack(Position(creaturePos.x - 1, creaturePos.y, creaturePos.z), attackedCreature));
			}
			if(result){
				dirList.push_back(WEST);
			}
		}
	}

	if(!dirList.empty()){
		std::random_shuffle(dirList.begin(), dirList.end());
		dir = dirList[random_range(0, dirList.size() - 1)];
		return true;
	}

	return false;
}

bool Monster::isInSpawnRange(const Position& toPos)
{
	if(masterRadius == -1){
		//no restrictions
		return true;
	}

	return !inDespawnRange(toPos);
}

bool Monster::canWalkTo(Position pos, Direction dir)
{
	switch(dir){
		case NORTH: pos.y += -1; break;
		case WEST:  pos.x += -1; break;
		case EAST:  pos.x += 1; break;
		case SOUTH: pos.y += 1; break;
		default:
			break;
	}

	if(isInSpawnRange(pos)){
		if(getWalkCache(pos) == 0){
			return false;
		}

		Tile* tile = g_game.getTile(pos.x, pos.y, pos.z);
		if(tile && tile->getTopVisibleCreature(this) == NULL && tile->__queryAdd(0, this, 1, FLAG_PATHFINDING) == RET_NOERROR){
			return true;
		}
	}

	return false;
}


void Monster::die()
{
	setAttackedCreature(NULL);
	destroySummons();
	clearTargetList();
	clearFriendList();
	setIdle(true);
}

Item* Monster::createCorpse()
{
	Item* corpse = Creature::createCorpse();
	if(corpse){
		DeathList killers = getKillers();
		if(!killers.empty() && (*killers.rbegin()).isCreatureKill()){
			Creature* mostDamageCreature = (*killers.rbegin()).getKillerCreature();
			Player* corpseOwner = NULL;

			if(mostDamageCreature->getPlayer()){
				corpseOwner = mostDamageCreature->getPlayer();
			}
			else if(mostDamageCreature->isPlayerSummon()){
				corpseOwner = mostDamageCreature->getPlayerMaster();
			}

			if(corpseOwner != NULL){
				corpse->setCorpseOwner(corpseOwner->getID());
			}
		}
	}

	return corpse;
}

bool Monster::inDespawnRange(const Position& pos)
{
	if(spawn && !mType->isLureable){
		if(Monster::despawnRadius == 0){
			return false;
		}

		if(!Spawns::getInstance()->isInZone(masterPos, Monster::despawnRadius, pos)){
			return true;
		}

		if(Monster::despawnRange == 0){
			return false;
		}

		if(std::abs(pos.z - masterPos.z) > Monster::despawnRange){
			return true;
		}

		return false;
	}

	return false;
}

bool Monster::despawn()
{
	return inDespawnRange(getPosition());
}

bool Monster::getCombatValues(int32_t& min, int32_t& max)
{
	if(minCombatValue == 0 && maxCombatValue == 0){
		return false;
	}

	min = minCombatValue;
	max = maxCombatValue;
	return true;
}

void Monster::updateLookDirection()
{
	Direction newDir = getDirection();

	if(attackedCreature){
		const Position& pos = getPosition();
		const Position& attackedCreaturePos = attackedCreature->getPosition();
		int32_t dx = attackedCreaturePos.x - pos.x;
		int32_t dy = attackedCreaturePos.y - pos.y;

		if(std::abs(dx) > std::abs(dy)){
			//look EAST/WEST
			if(dx < 0){
				newDir = WEST;
			}
			else{
				newDir = EAST;
			}
		}
		else if(std::abs(dx) < std::abs(dy)){
			//look NORTH/SOUTH
			if(dy < 0){
				newDir = NORTH;
			}
			else{
				newDir = SOUTH;
			}
		}
		else{
			if(dx < 0 && dy < 0){
				if(getDirection() == SOUTH){
					newDir = WEST;
				}
				else if(getDirection() == EAST){
					newDir = NORTH;
				}
			}
			else if(dx < 0 && dy > 0){
				if(getDirection() == NORTH){
					newDir = WEST;
				}
				else if(getDirection() == EAST){
					newDir = SOUTH;
				}
			}
			else if(dx > 0 && dy < 0){
				if(getDirection() == SOUTH){
					newDir = EAST;
				}
				else if(getDirection() == WEST){
					newDir = NORTH;
				}
			}
			else{
				if(getDirection() == NORTH){
					newDir = EAST;
				}
				else if(getDirection() == WEST){
					newDir = SOUTH;
				}
			}
		}
	}

	g_game.internalCreatureTurn(this, newDir);
}

void Monster::dropLoot(Container* corpse)
{
	if(corpse && lootDrop){
		mType->createLoot(corpse);
		Player* killer = g_game.getPlayerByID(corpse->getCorpseOwner());
		if(killer)
			killer->broadcastLoot(this, corpse->getContainer());
	}
}

bool Monster::isImmune(CombatType_t type) const
{
	ElementMap::iterator it = mType->elementMap.find(type);
	if(it != mType->elementMap.end()){
		if(it->second >= 100){
			return true;
		}
	}

	return Creature::isImmune(type);
}

void Monster::setNormalCreatureLight()
{
	internalLight.level = mType->lightLevel;
	internalLight.color = mType->lightColor;
}

void Monster::drainHealth(Creature* attacker, CombatType_t combatType, int32_t damage)
{
	Creature::drainHealth(attacker, combatType, damage);

	if(isInvisible()){
		removeCondition(CONDITION_INVISIBLE);
	}
}

void Monster::changeHealth(int32_t healthChange)
{
	Creature::changeHealth(healthChange);
	//In case a player with ignore flag set attacks the monster
	setIdle(false);
}

bool Monster::challengeCreature(Creature* creature)
{
	if(isSummon()){
		return false;
	}
	else{
		bool result = selectTarget(creature);
		if(result){
			targetChangeCooldown = 8000;
			targetChangeTicks = 0;
		}

		return result;
	}

	return false;
}

bool Monster::convinceCreature(Creature* creature)
{
	Player* player = creature->getPlayer();
	if(player && !player->hasFlag(PlayerFlag_CanConvinceAll)){
		if(!mType->isConvinceable){
			return false;
		}
	}
	
	if(isPlayerSummon()){
		return false;
	}
	else if(isSummon()){
		if(getMaster() != creature){
			Creature* oldMaster = getMaster();
			oldMaster->removeSummon(this);
			creature->addSummon(this);

			setFollowCreature(NULL);
			setAttackedCreature(NULL);
			destroySummons();

			isMasterInRange = true;
			updateTargetList();
			updateIdleStatus();

			//Notify surrounding about the change
			SpectatorVec list;
			g_game.getSpectators(list, getPosition(), false, true);
			g_game.getSpectators(list, creature->getPosition(), true, true);

			for(SpectatorVec::iterator it = list.begin(); it != list.end(); ++it){
				(*it)->onCreatureConvinced(creature, this);
			}

			if(spawn){
				spawn->removeMonster(this);
				spawn = NULL;
				masterRadius = -1;
			}

			return true;
		}
	}
	else{
		creature->addSummon(this);
		setFollowCreature(NULL);
		setAttackedCreature(NULL);
		destroySummons();

		isMasterInRange = true;
		updateTargetList();
		updateIdleStatus();

		//Notify surrounding about the change
		SpectatorVec list;
		g_game.getSpectators(list, getPosition(), false, true);
		g_game.getSpectators(list, creature->getPosition(), true, true);

		for(SpectatorVec::iterator it = list.begin(); it != list.end(); ++it){
			(*it)->onCreatureConvinced(creature, this);
		}

		if(spawn){
			spawn->removeMonster(this);
			spawn = NULL;
			masterRadius = -1;
		}

		return true;
	}

	return false;
}

void Monster::onCreatureConvinced(const Creature* convincer, const Creature* creature)
{
	if(convincer != this && (isFriend(creature) || isOpponent(creature))){
		updateTargetList();
		updateIdleStatus();
	}
}

void Monster::getPathSearchParams(const Creature* creature, FindPathParams& fpp) const
{
	Creature::getPathSearchParams(creature, fpp);

	fpp.minTargetDist = 1;
	fpp.maxTargetDist = mType->targetDistance;

	if(isSummon()){
		if(getMaster() == creature){
			fpp.maxTargetDist = 2;
			fpp.fullPathSearch = true;
		}
		else{
			if(mType->targetDistance <= 1){
				fpp.fullPathSearch = true;
			}
			else{
				fpp.fullPathSearch = !canUseAttack(getPosition(), creature);
			}
		}
	}
	else{
		if(isFleeing()){
			//Distance should be higher than the client view range (Map::maxClientViewportX/Map::maxClientViewportY)
			fpp.maxTargetDist = Map::maxViewportX;
			fpp.clearSight = false;
			fpp.keepDistance = true;
			fpp.fullPathSearch = false;
		}
		else{
			if(mType->targetDistance <= 1){
				fpp.fullPathSearch = true;
			}
			else{
				fpp.fullPathSearch = !canUseAttack(getPosition(), creature);
			}
		}
	}
}
