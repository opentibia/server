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

#include "actor.h"
#include "tasks.h"
#include "player.h"
#include "game.h"
#include "combat.h"
#include "spawn.h"
#include "container.h"
#include "configmanager.h"
#include "party.h"
#include "creature_manager.h"

extern Game g_game;
extern ConfigManager g_config;
extern CreatureManager g_creature_types;

AutoList<Actor>Actor::listMonster;

int32_t Actor::despawnRange;
int32_t Actor::despawnRadius;

#ifdef __ENABLE_SERVER_DIAGNOSTIC__
uint32_t Actor::monsterCount = 0;
#endif

Actor* Actor::create()
{
	CreatureType t;
	return new Actor(t);
}

Actor* Actor::create(CreatureType cType)
{
	return new Actor(cType);
}

Actor* Actor::create(const std::string& name)
{
	CreatureType* cType = g_creature_types.getMonsterType(name);
	if(!cType)
		return NULL;
	return create(*cType);
}

Actor::Actor(CreatureType _cType) : Creature(), cType(_cType)
{
	isIdle = false;
	isMasterInRange = false;

	spawn = NULL;
	shouldReload_ = false;
	alwaysThink_ = false;

	defaultOutfit = cType.outfit();
	currentOutfit = cType.outfit();

	health     = cType.health();
	healthMax  = cType.health_max();
	baseSpeed = cType.base_speed();
	internalLight.level = cType.lightLevel();
	internalLight.color = cType.lightColor();

	minCombatValue = 0;
	maxCombatValue = 0;

	targetTicks = 0;
	targetChangeTicks = 0;
	targetChangeCooldown = 0;
	attackTicks = 0;
	defenseTicks = 0;
	yellTicks = 0;
	extraMeleeAttack = false;

	updateNameDescription();

#ifdef __ENABLE_SERVER_DIAGNOSTIC__
	monsterCount++;
#endif
}

Actor::~Actor()
{
	clearTargetList();
	clearFriendList();

#ifdef __ENABLE_SERVER_DIAGNOSTIC__
	monsterCount--;
#endif
}

CreatureType& Actor::getType()
{
	return cType;
}

void Actor::updateBaseSpeed()
{
	baseSpeed = cType.base_speed();
}

void Actor::updateMaxHealth()
{
	healthMax = cType.health_max();
}

void Actor::updateLightColor()
{
	internalLight.color = cType.lightColor();
}

void Actor::updateLightLevel()
{
	internalLight.level = cType.lightLevel();
}

void Actor::updateNameDescription()
{
	strDescription = cType.nameDescription();
	toLowerCaseString(strDescription);
}

void Actor::onAttackedCreature(Creature* target)
{
	Creature::onAttackedCreature(target);

	if(isSummon()){
		getMaster()->onSummonAttackedCreature(this, target);
	}
}

void Actor::onAttackedCreatureDrainHealth(Creature* target, int32_t points)
{
	Creature::onAttackedCreatureDrainHealth(target, points);

	if(isSummon()){
		getMaster()->onSummonAttackedCreatureDrainHealth(this, target, points);
	}
}

void Actor::onAttackedCreatureDrainMana(Creature* target, int32_t points)
{
	Creature::onAttackedCreatureDrainMana(target, points);

	if(isSummon()){
		getMaster()->onSummonAttackedCreatureDrainMana(this, target, points);
	}
}

void Actor::onAttackedCreatureDissapear(bool isLogout)
{
#ifdef __DEBUG__
	std::cout << "Attacked creature dissapeared." << std::endl;
#endif

	attackTicks = 0;
	extraMeleeAttack = true;
}

void Actor::onFollowCreatureDissapear(bool isLogout)
{
#ifdef __DEBUG__
	std::cout << "Follow creature dissapeared." << std::endl;
#endif
}

void Actor::onCreatureAppear(const Creature* creature, bool isLogin)
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

void Actor::onCreatureDisappear(const Creature* creature, bool isLogout)
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

void Actor::onCreatureMove(const Creature* creature, const Tile* newTile, const Position& newPos,
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
			}
		}

		updateIdleStatus();

		if(!followCreature && !isSummon()){
			//we have no target lets try pick this one
			if(isOpponent(creature)){
				selectTarget(const_cast<Creature*>(creature));
			}
		}
	}
}

void Actor::updateTargetList()
{
	CreatureList::iterator it;
	for(it = friendList.begin(); it != friendList.end();){
		if((*it)->getHealth() <= 0 || !canSee((*it)->getPosition())){
			(*it)->unRef();
			it = friendList.erase(it);
		}
		else
			++it;
	}

	for(it = targetList.begin(); it != targetList.end();){
		if((*it)->getHealth() <= 0 || !canSee((*it)->getPosition())){
			(*it)->unRef();
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

void Actor::clearTargetList()
{
	for(CreatureList::iterator it = targetList.begin(); it != targetList.end(); ++it){
		(*it)->unRef();
	}
	targetList.clear();
}

void Actor::clearFriendList()
{
	for(CreatureList::iterator it = friendList.begin(); it != friendList.end(); ++it){
		(*it)->unRef();
	}
	friendList.clear();
}

void Actor::onCreatureFound(Creature* creature, bool pushFront /*= false*/)
{
	if(isFriend(creature)){
		assert(creature != this);
		if(std::find(friendList.begin(), friendList.end(), creature) == friendList.end()){
			creature->addRef();
			friendList.push_back(creature);
		}
	}

	if(isOpponent(creature)){
		assert(creature != this);
		if(std::find(targetList.begin(), targetList.end(), creature) == targetList.end()){
			creature->addRef();
			if(pushFront){
				targetList.push_front(creature);
			}
			else{
				targetList.push_back(creature);
			}
		}
	}

	updateIdleStatus();
}

void Actor::onCreatureEnter(Creature* creature)
{
	//std::cout << "onCreatureEnter - " << creature->getName() << std::endl;

	if(getMaster() == creature){
		//Turn the summon on again
		isMasterInRange = true;
	}

	onCreatureFound(creature, true);
}


bool Actor::isFriend(const Creature* creature)
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
		if(creature->getActor() && !creature->isSummon()){
			return true;
		}
	}

	return false;
}

bool Actor::isOpponent(const Creature* creature)
{
	if(isPlayerSummon()){
		if(creature != getMaster()){
			return true;
		}
	}
	else{
		if( (creature->getPlayer() && !creature->getPlayer()->hasFlag(PlayerFlag_IgnoredByMonsters)) ||
			isPlayerSummon()) {
			return true;
		}
	}

	return false;
}

void Actor::onCreatureLeave(Creature* creature)
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
			(*it)->unRef();
			friendList.erase(it);
		}
#ifdef __DEBUG__
		else{
			std::cout << "Actor: " << creature->getName() << " not found in the friendList." << std::endl;
		}
#endif
	}

	//update targetList
	if(isOpponent(creature)){
		CreatureList::iterator it = std::find(targetList.begin(), targetList.end(), creature);
		if(it != targetList.end()){
			(*it)->unRef();
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

bool Actor::searchTarget(TargetSearchType_t searchType /*= TARGETSEARCH_DEFAULT*/)
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

void Actor::onFollowCreatureComplete(const Creature* creature)
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
				target->unRef();
			}
		}
	}
}

BlockType Actor::blockHit(Creature* attacker, CombatType combatType, int32_t& damage,
	bool checkDefense /* = false*/, bool checkArmor /* = false*/)
{
	BlockType blockType = Creature::blockHit(attacker, combatType, damage, checkDefense, checkArmor);

	if(damage != 0){
		int32_t elementMod = 0;
		ElementMap::const_iterator it = cType.elementMap().find(combatType);
		if(it != cType.elementMap().end()){
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

bool Actor::isTarget(Creature* creature)
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

bool Actor::selectTarget(Creature* creature)
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

void Actor::alwaysThink(bool b)
{
	alwaysThink_ = b;
	if(alwaysThink())
		setIdle(false);
	else
		updateIdleStatus();
}

void Actor::setIdle(bool _idle)
{
	if(getHealth() <= 0 || isRemoved()){
		return;
	}

	isIdle = _idle;

	if(!isIdle){
		g_game.addCreatureCheck(this);
	}
	else{
		onIdleStatus();
		clearTargetList();
		clearFriendList();
		g_game.removeCreatureCheck(this);
	}
}

void Actor::updateIdleStatus()
{
	bool idle = false;

	if(conditions.empty()){
		if(isSummon()){
			if(!isMasterInRange){
				idle = true;
			}
			else if(getMaster()->getActor() && getMaster()->getActor()->getIdleStatus()){
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

void Actor::onAddCondition(ConditionType type, bool hadCondition)
{
	Creature::onAddCondition(type, hadCondition);

	//the walkCache need to be updated if the monster becomes "resistent" to the damage, see Tile::__queryAdd()
	if(type == CONDITION_FIRE || type == CONDITION_ENERGY || type == CONDITION_POISON){
		updateMapCache();
	}

	updateIdleStatus();
}

void Actor::onEndCondition(ConditionType type, bool lastCondition)
{
	Creature::onEndCondition(type, lastCondition);

	//the walkCache need to be updated if the monster loose the "resistent" to the damage, see Tile::__queryAdd()
	if(type == CONDITION_FIRE || type == CONDITION_ENERGY || type == CONDITION_POISON){
		updateMapCache();
	}

	updateIdleStatus();
}

void Actor::onThink(uint32_t interval)
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

void Actor::doAttacking(uint32_t interval)
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

	for(SpellList::const_iterator it = cType.spellAttackList().begin(), spell_list_end = cType.spellAttackList().end(); it != spell_list_end; ++it){
		bool inRange = false;
		if(canUseSpell(myPos, targetPos, *it, interval, inRange)){
			if(it->chance >= (uint32_t)random_range(1, 100)){
				if(updateLook){
					updateLookDirection();
					updateLook = false;
				}

				minCombatValue = it->minCombatValue;
				maxCombatValue = it->maxCombatValue;
				// REVSCRIPT TODO Monsters should cast spells
				//it->spell->castSpell(this, attackedCreature);
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

bool Actor::canUseAttack(const Position& pos, const Creature* target) const
{
	if(isHostile()){
		const Position& targetPos = target->getPosition();
		for(SpellList::const_iterator it = cType.spellAttackList().begin(), spell_attack_list_end = cType.spellAttackList().end(); it != spell_attack_list_end; ++it){
			if((*it).range != 0 && std::max(std::abs(pos.x - targetPos.x), std::abs(pos.y - targetPos.y)) <= (int32_t)(*it).range){
				return g_game.isSightClear(pos, targetPos, true);
			}
		}

		return false;
	}

	return true;
}

bool Actor::canUseSpell(const Position& pos, const Position& targetPos,
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

void Actor::onThinkTarget(uint32_t interval)
{
	if(!isSummon()){
		if(cType.changeTargetSpeed() > 0){
			bool canChangeTarget = true;
			if(targetChangeCooldown > 0){
				targetChangeCooldown -= interval;
				if(targetChangeCooldown <= 0){
					targetChangeCooldown = 0;
					targetChangeTicks = (uint32_t)cType.changeTargetSpeed();
				}
				else{
					canChangeTarget = false;
				}
			}

			if(canChangeTarget){
				targetChangeTicks += interval;

				if(targetChangeTicks >= (uint32_t)cType.changeTargetSpeed()){
					targetChangeTicks = 0;
					targetChangeCooldown = (uint32_t)cType.changeTargetSpeed();

					if(cType.changeTargetChance() >= random_range(1, 100)){
						searchTarget(TARGETSEARCH_RANDOM);
					}
				}
			}
		}
	}
}

void Actor::onThinkDefense(uint32_t interval)
{
	resetTicks = true;
	defenseTicks += interval;

	for(SpellList::const_iterator it = cType.spellDefenseList().begin(), spell_defense_list_end = cType.spellDefenseList().end(); it != spell_defense_list_end; ++it){
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
			// REVSCRIPT TODO Monsters should cast spells
			// it->spell->castSpell(this, this);
		}
	}

	if(!isSummon() && (int32_t)summons.size() < cType.maxSummons()){
		for(SummonList::const_iterator it = cType.summonList().begin(), summon_list_end = cType.summonList().end(); it != summon_list_end; ++it){
			if(it->speed > defenseTicks){
				resetTicks = false;
				continue;
			}

			if((int32_t)summons.size() >= cType.maxSummons()){
				continue;
			}

			if(defenseTicks % it->speed >= interval){
				//already used this spell for this round
				continue;
			}

			if((it->chance >= (uint32_t)random_range(1, 100))){
				Actor* summon = Actor::create(it->name);
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

void Actor::onThinkYell(uint32_t interval)
{
	if(cType.yellSpeedTicks() > 0){
		yellTicks += interval;

		if(yellTicks >= cType.yellSpeedTicks()){
			yellTicks = 0;

			if(!cType.voiceVector().empty() && (cType.yellChance() >= (uint32_t)random_range(1, 100))){
				uint32_t index = random_range(0, cType.voiceVector().size() - 1);
				const voiceBlock_t& vb = cType.voiceVector()[index];

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

void Actor::onWalk()
{
	Creature::onWalk();
}

bool Actor::pushItem(Item* item, int32_t radius)
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
				if(g_game.internalMoveItem(this, item->getParent(), tile,
					INDEX_WHEREEVER, item, item->getItemCount(), NULL) == RET_NOERROR){
					return true;
				}
			}
		}
	}

	return false;
}

void Actor::pushItems(Tile* tile)
{
	//We can not use iterators here since we can push the item to another tile
	//which will invalidate the iterator.
	//start from the end to minimize the amount of traffic
	uint32_t moveCount = 0;
	uint32_t removeCount = 0;
	int32_t downItemSize = tile->items_downCount();

	for(int32_t i = downItemSize - 1; i >= 0; --i){
		assert(i >= 0 && i < downItemSize);
		Item* item = tile->items_get(i);
		if(item && item->isMoveable() && (item->blockPathFind() || item->blockSolid() ) ){
				if(moveCount < 20 && pushItem(item, 1)){
					moveCount++;
				}
				else if(g_game.internalRemoveItem(this, item) == RET_NOERROR){
					++removeCount;
				}
		}
	}

	if(removeCount > 0){
		g_game.addMagicEffect(tile->getPosition(), NM_ME_PUFF);
	}

	if(removeCount > 0){
		g_game.addMagicEffect(tile->getPosition(), NM_ME_PUFF);
	}
}

bool Actor::pushCreature(Creature* creature)
{
	Position monsterPos = creature->getPosition();

	std::vector<Direction> dirList;
	dirList.push_back(NORTH);
	dirList.push_back(SOUTH);
	dirList.push_back(WEST);
	dirList.push_back(EAST);

	std::random_shuffle(dirList.begin(), dirList.end());

	for(std::vector<Direction>::iterator it = dirList.begin(); it != dirList.end(); ++it){
		const Position& tryPos = Combat::getCasterPosition(creature, *it);
		Tile* toTile = g_game.getTile(tryPos.x, tryPos.y, tryPos.z);

		if(toTile && !toTile->blockPathFind()){
			if(g_game.internalMoveCreature(this, creature, *it) == RET_NOERROR){
				return true;
			}
		}
	}

	return false;
}

void Actor::pushCreatures(Tile* tile)
{
	//We can not use iterators here since we can push a creature to another tile
	//which will invalidate the iterator.
	if(CreatureVector* creatures = tile->getCreatures()){
		uint32_t removeCount = 0;

		for(uint32_t i = 0; i < creatures->size();){
			Actor* actor = creatures->at(i)->getActor();

			if(actor && actor->isPushable()){
				if(pushCreature(actor)){
					continue;
				}
				else{
					actor->changeHealth(-actor->getHealth());
					actor->setDropLoot(false);
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

bool Actor::getNextStep(Direction& dir, uint32_t& flags)
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
				else if(cType.staticAttackChance() < (uint32_t)random_range(1, 100)){
					result = getDanceStep(getPosition(), dir);
				}
			}
		}
	}

	if(result && (canPushItems() || canPushCreatures()) ){
		const Position& pos = Combat::getCasterPosition(this, dir);
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

bool Actor::getRandomStep(const Position& creaturePos, Direction& dir)
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

bool Actor::getDanceStep(const Position& creaturePos, Direction& dir,
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

bool Actor::isInSpawnRange(const Position& toPos)
{
	if(masterRadius == -1){
		//no restrictions
		return true;
	}

	return !inDespawnRange(toPos);
}

bool Actor::canWalkTo(Position pos, Direction dir)
{
	switch(dir.value()){
		case enums::NORTH: pos.y += -1; break;
		case enums::WEST:  pos.x += -1; break;
		case enums::EAST:  pos.x += 1; break;
		case enums::SOUTH: pos.y += 1; break;
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


void Actor::die()
{
	setAttackedCreature(NULL);
	destroySummons();

	clearTargetList();
	clearFriendList();
	onIdleStatus();
	Creature::die();
}

Item* Actor::createCorpse()
{
	Item* corpse = Creature::createCorpse();
	if(corpse){
		DeathList killers = getKillers();
		if(!killers.empty() && (*killers.rbegin()).isCreatureKill() ){
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

bool Actor::inDespawnRange(const Position& pos)
{
	if(spawn && !cType.isLureable()){
		if(Actor::despawnRadius == 0){
			return false;
		}

		if(!Spawns::getInstance()->isInZone(masterPos, Actor::despawnRadius, pos)){
			return true;
		}

		if(Actor::despawnRange == 0){
			return false;
		}

		if(std::abs(pos.z - masterPos.z) > Actor::despawnRange){
			return true;
		}

		return false;
	}

	return false;
}

bool Actor::despawn()
{
	return inDespawnRange(getPosition());
}

bool Actor::getCombatValues(int32_t& min, int32_t& max)
{
	if(minCombatValue == 0 && maxCombatValue == 0){
		return false;
	}

	min = minCombatValue;
	max = maxCombatValue;
	return true;
}

void Actor::updateLookDirection()
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

void Actor::dropLoot(Container* corpse)
{
	if(corpse && lootDrop){
		cType.createLoot(corpse);
		Player* killer = g_game.getPlayerByID(corpse->getCorpseOwner());
		if(killer){
			killer->broadcastLoot(this, corpse->getContainer());
		}
	}
}

bool Actor::isImmune(CombatType type) const
{
	ElementMap::const_iterator it = cType.elementMap().find(type);
	if(it != cType.elementMap().end()){
		if(it->second >=100){
			return true;
		}
	}

	return Creature::isImmune(type);
}

void Actor::setNormalCreatureLight()
{
	internalLight.level = cType.lightLevel();
	internalLight.color = cType.lightColor();
}

void Actor::drainHealth(Creature* attacker, CombatType combatType, int32_t damage, bool showtext)
{
	Creature::drainHealth(attacker, combatType, damage, showtext);

	if(isInvisible()){
		removeCondition(CONDITION_INVISIBLE);
	}
}

void Actor::changeHealth(int32_t healthChange)
{
	//In case a player with ignore flag set attacks the monster
	setIdle(false);

	Creature::changeHealth(healthChange);
}

bool Actor::challengeCreature(Creature* creature)
{
	if(isPlayerSummon()){
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

bool Actor::convinceCreature(Creature* creature)
{
	Player* player = creature->getPlayer();
	if(player && !player->hasFlag(PlayerFlag_CanConvinceAll)){
		if(!cType.isConvinceable()){
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

			//destroy summons
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

void Actor::onCreatureConvinced(const Creature* convincer, const Creature* creature)
{
	if(convincer != this && (isFriend(creature) || isOpponent(creature))){
		updateTargetList();
		updateIdleStatus();
	}
}

void Actor::getPathSearchParams(const Creature* creature, FindPathParams& fpp) const
{
	Creature::getPathSearchParams(creature, fpp);

	fpp.minTargetDist = 1;
	fpp.maxTargetDist = cType.targetDistance();

	if(isSummon()){
		if(getMaster() == creature){
			fpp.maxTargetDist = 2;
			fpp.fullPathSearch = true;
		}
		else{
			if(cType.targetDistance() <= 1){
				fpp.fullPathSearch = true;
			}
			else{
				fpp.fullPathSearch = !canUseAttack(getPosition(), creature);
			}
		}
	}
	else{
		if(isFleeing()){
			//Distance should be higher than the client view range (Map_maxClientViewportX/Map_maxClientViewportY)
			fpp.maxTargetDist = Map_maxViewportX;
			fpp.clearSight = false;
			fpp.keepDistance = true;
			fpp.fullPathSearch = false;
		}
		else{
			if(cType.targetDistance() <= 1){
				fpp.fullPathSearch = true;
			}
			else{
				fpp.fullPathSearch = !canUseAttack(getPosition(), creature);
			}
		}
	}
}
