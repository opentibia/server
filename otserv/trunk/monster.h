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

class Monster : public Creature {
private:
	Monster(MonsterType* mtype);

public:
#ifdef __ENABLE_SERVER_DIAGNOSTIC__
	static uint32_t monsterCount;
#endif

	static Monster* createMonster(MonsterType* mType);
	static Monster* createMonster(const std::string& name);

	virtual ~Monster();

	virtual Monster* getMonster();
	virtual const Monster* getMonster() const;

	virtual uint32_t idRange();
	static AutoList<Monster> listMonster;
	void removeList();
	void addList();

	virtual const std::string& getName() const;
	virtual const std::string& getNameDescription() const;
	virtual std::string getDescription(const int32_t& lookDistance) const;

	virtual RaceType_t getRace() const;
	virtual int32_t getArmor() const;
	virtual int32_t getDefense() const;
	virtual bool isPushable() const;
	virtual bool isAttackable() const;
	virtual bool isImmune(CombatType_t type) const;

	bool canPushItems() const;
	bool canPushCreatures() const;
	bool isHostile() const;
	virtual bool canSeeInvisibility() const;
	uint32_t getManaCost() const;
	void setSpawn(Spawn* _spawn);

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
	virtual bool hasExtraSwing();

	bool searchTarget(TargetSearchType_t searchType = TARGETSEARCH_DEFAULT);
	bool selectTarget(Creature* creature);

	const CreatureList& getTargetList();
	const CreatureList& getFriendList();

	void updateHadRecentBattleVar();
	bool hadRecentBattle() const;
	bool isTarget(Creature* creature);
	bool getIdleStatus() const;
	bool isFleeing() const;

	virtual bool hasHiddenHealth() const;

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

	virtual uint64_t getLostExperience() const;
	virtual uint16_t getLookCorpse();
	virtual void dropLoot(Container* corpse);
	virtual const uint32_t& getDamageImmunities() const;
	virtual const uint32_t& getConditionImmunities() const;
	virtual const uint16_t& getLookCorpse() const;
	virtual void getPathSearchParams(const Creature* creature, FindPathParams& fpp) const;
	virtual bool useCacheMap() const;
};

#endif
