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


#ifndef __OTSERV_MONSTER_H__
#define __OTSERV_MONSTER_H__

#include "definitions.h"
#include "tile.h"
#include "monsters.h"
#include "otsystem.h"

class Creature;
class Game;
class Spawn;

typedef std::list<Creature*> CreatureList;

enum TargetSearchType_t{
	TARGETSEARCH_DEFAULT,
	TARGETSEARCH_RANDOM,
	TARGETSEARCH_ATTACKRANGE,
	TARGETSEARCH_NEAREAST
};

class Monster : public Creature
{
private:
	Monster(MonsterType* mtype);
	//const Monster& operator=(const Monster& rhs);

public:
#ifdef __ENABLE_SERVER_DIAGNOSTIC__
	static uint32_t monsterCount;
#endif

	static Monster* createMonster(MonsterType* mType);
	static Monster* createMonster(const std::string& name);

	virtual ~Monster();

	virtual Monster* getMonster() {return this;};
	virtual const Monster* getMonster() const {return this;};

	virtual uint32_t idRange(){ return MONSTER_ID_RANGE;}
	static AutoList<Monster> listMonster;
	void removeList(){listMonster.removeList(getID());}
	void addList() {listMonster.addList(this);}

	virtual const std::string& getName() const {return mType->name;}
	virtual const std::string& getNameDescription() const {return mType->nameDescription;}
	virtual std::string getDescription(int32_t lookDistance) const {return strDescription + '.';}

	virtual RaceType_t getRace() const { return mType->race; }
	virtual int32_t getArmor() const { return mType->armor; }
	virtual int32_t getDefense() const { return mType->defense; }
	virtual bool isPushable() const { return mType->pushable && (baseSpeed > 0); }
	virtual bool isAttackable() const { return mType->isAttackable;}
	virtual bool isImmune(CombatType_t type) const;

	bool canPushItems() const {return mType->canPushItems;}
	bool canPushCreatures() const {return mType->canPushCreatures;}
	bool isHostile() const { return mType->isHostile;}
	virtual bool canSeeInvisibility() const {return Creature::isImmune(CONDITION_INVISIBLE);}
	uint32_t getManaCost() const {return mType->manaCost;}
	void setSpawn(Spawn* _spawn) {spawn = _spawn;};

	virtual void onAttackedCreatureDissapear(bool isLogout);
	virtual void onFollowCreatureDissapear(bool isLogout);
	virtual void onAttackedCreature(Creature* target);
	virtual void onAttackedCreatureDrainHealth(Creature* target, int32_t points);
	virtual void onAttackedCreatureDrainMana(Creature* target, int32_t points);

	virtual void onCreatureAppear(const Creature* creature, bool isLogin);
	virtual void onCreatureDisappear(const Creature* creature, bool isLogout);
	virtual void onCreatureMove(const Creature* creature, const Tile* newTile, const Position& newPos,
		const Tile* oldTile, const Position& oldPos, bool teleport);

	virtual void drainHealth(Creature* attacker, CombatType_t combatType, int32_t damage);
	virtual void changeHealth(int32_t healthChange);

	virtual void onWalk();
	virtual bool getNextStep(Direction& dir, uint32_t& flags);
	virtual void onFollowCreatureComplete(const Creature* creature);

	virtual void onThink(uint32_t interval);

	virtual bool challengeCreature(Creature* creature);
	virtual bool convinceCreature(Creature* creature);

	virtual void setNormalCreatureLight();
	virtual bool getCombatValues(int32_t& min, int32_t& max);

	virtual void doAttacking(uint32_t interval);
	virtual bool hasExtraSwing() {return extraMeleeAttack;}

	bool searchTarget(TargetSearchType_t searchType = TARGETSEARCH_DEFAULT);
	bool selectTarget(Creature* creature);

	const CreatureList& getTargetList() {return targetList;}
	const CreatureList& getFriendList() {return friendList;}

	void updateHadRecentBattleVar();
	bool hadRecentBattle() const { return hadRecentBattleVar; }
	bool isTarget(Creature* creature);
	bool getIdleStatus() const { return isIdle;}
	bool isFleeing() const {return getHealth() <= mType->runAwayHealth;}

	virtual bool hasHiddenHealth() const { return mType->hideHealth; }

	virtual BlockType_t blockHit(Creature* attacker, CombatType_t combatType, int32_t& damage,
		bool checkDefense = false, bool checkArmor = false);

private:
	CreatureList targetList;
	CreatureList friendList;

	MonsterType* mType;

	int32_t minCombatValue;
	int32_t maxCombatValue;
	uint32_t attackTicks;
	uint32_t targetTicks;
	uint32_t targetChangeTicks;
	uint32_t defenseTicks;
	uint32_t yellTicks;
	int32_t targetChangeCooldown;
	bool resetTicks;
	bool isIdle;
	bool extraMeleeAttack;
	bool semiIdle;
	bool hadRecentBattleVar;
	int64_t timeOfLastHit;

	Spawn* spawn;
	bool isMasterInRange;

	std::string strDescription;

	virtual void onCreatureEnter(Creature* creature);
	virtual void onCreatureLeave(Creature* creature);
	void onCreatureFound(Creature* creature, bool pushFront = false);

	void updateLookDirection();

	void updateTargetList();
	void clearTargetList();
	void clearFriendList();

	void die();
	Item* createCorpse();
	bool despawn();
	bool inDespawnRange(const Position& pos);

	void setIdle(bool _idle);
	void updateIdleStatus();

	virtual void onAddCondition(ConditionType_t type, bool hadCondition);
	virtual void onEndCondition(ConditionType_t type, bool lastCondition);
	virtual void onCreatureConvinced(const Creature* convincer, const Creature* creature);

	bool canUseAttack(const Position& pos, const Creature* target) const;
	bool canUseSpell(const Position& pos, const Position& targetPos,
		const spellBlock_t& sb, uint32_t interval, bool& inRange);
	bool getRandomStep(const Position& creaturePos, Direction& dir);
	bool getDanceStep(const Position& creaturePos, Direction& dir,
		bool keepAttack = true, bool keepDistance = true);
	bool isInSpawnRange(const Position& toPos);
	bool canWalkTo(Position pos, Direction dir);

	bool pushItem(Item* item, int32_t radius);
	void pushItems(Tile* tile);
	bool pushCreature(Creature* creature);
	void pushCreatures(Tile* tile);

	void onThinkTarget(uint32_t interval);
	void onThinkYell(uint32_t interval);
	void onThinkDefense(uint32_t interval);

	bool isFriend(const Creature* creature);
	bool isOpponent(const Creature* creature);

	virtual uint64_t getLostExperience() const { return ((skillLoss ? mType->experience : 0)); }
	virtual uint16_t getLookCorpse() { return mType->lookCorpse; }
	virtual void dropLoot(Container* corpse);
	virtual uint32_t getDamageImmunities() const { return mType->damageImmunities; }
	virtual uint32_t getConditionImmunities() const { return mType->conditionImmunities; }
	virtual uint16_t getLookCorpse() const { return mType->lookCorpse; }
	virtual void getPathSearchParams(const Creature* creature, FindPathParams& fpp) const;
	virtual bool useCacheMap() const {return true;}
};

#endif
