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


#ifndef __OTSERV_ACTOR_H__
#define __OTSERV_ACTOR_H__

#include "creature_type.h"

class Creature;
class Game;
class Spawn;

typedef std::list<Creature*> CreatureList;

enum TargetSearchType_t{
	TARGETSEARCH_DEFAULT,
	TARGETSEARCH_RANDOM,
	TARGETSEARCH_ATTACKRANGE
};

class Actor : public Creature
{
private:
	Actor(CreatureType mtype);

public:
#ifdef __ENABLE_SERVER_DIAGNOSTIC__
	static uint32_t monsterCount;
#endif

	static Actor* create();
	static Actor* create(CreatureType cType);
	static Actor* create(const std::string& name);
	static int32_t despawnRange;
	static int32_t despawnRadius;

	virtual ~Actor();

	virtual Actor* getActor() {return this;}
	virtual const Actor* getActor() const {return this;}

	CreatureType& getType();

	bool shouldReload() const {return shouldReload_;}
	void shouldReload(bool b) {shouldReload_ = b;}
	bool alwaysThink() const {return alwaysThink_;}
	void alwaysThink(bool b);

	virtual uint32_t idRange(){ return 0x40000000;}
	static AutoList<Actor> listMonster;
	void removeList(){listMonster.removeList(getID());}
	void addList() {listMonster.addList(this);}
	
	virtual const std::string& getName() const {return cType.name();}
	virtual const std::string& getNameDescription() const {return cType.nameDescription();}
	virtual std::string getDescription(int32_t lookDistance) const {return strDescription + '.';}
	virtual void updateBaseSpeed();
	virtual void updateMaxHealth();
	virtual void updateLightLevel();
	virtual void updateLightColor();
	virtual void updateNameDescription();

	virtual RaceType getRace() const { return cType.race(); }
	virtual int32_t getArmor() const { return cType.armor(); }
	virtual int32_t getDefense() const { return cType.defense(); }
	virtual bool isPushable() const { return cType.pushable() && (baseSpeed > 0); }
	virtual bool isAttackable() const { return cType.isAttackable();}

	bool canPushItems() const {return cType.canPushItems();}
	bool canPushCreatures() const {return cType.canPushCreatures();}
	bool isHostile() const { return cType.isHostile();}
	virtual bool canSeeInvisibility() const { return Creature::isImmune(CONDITION_INVISIBLE);}
	uint32_t getManaCost() const {return cType.manaCost();}	
	void setSpawn(Spawn* _spawn) {spawn = _spawn;}

	virtual void onAttackedCreatureDissapear(bool isLogout);
	virtual void onFollowCreatureDissapear(bool isLogout);

	virtual void onAttackedCreature(Creature* target);
	virtual void onAttackedCreatureDrainHealth(Creature* target, int32_t points);
	virtual void onAttackedCreatureDrainMana(Creature* target, int32_t points);

	virtual void onCreatureAppear(const Creature* creature, bool isLogin);
	virtual void onCreatureDisappear(const Creature* creature, bool isLogout);
	virtual void onCreatureMove(const Creature* creature, const Tile* newTile, const Position& newPos,
		const Tile* oldTile, const Position& oldPos, bool teleport);

	virtual void drainHealth(Creature* attacker, CombatType combatType, int32_t damage, bool showeffect);
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

	bool isTarget(Creature* creature);
	bool isFleeing() const {return getHealth() <= cType.fleeHealth();}

	bool isImmune(CombatType type) const;
	BlockType blockHit(Creature* attacker, CombatType combatType, int32_t& damage,
		bool checkDefense = false, bool checkArmor = false);

private:
	CreatureList targetList;
	CreatureList friendList;

	CreatureType cType;

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
	bool isMasterInRange;

	Spawn* spawn;
	bool shouldReload_;

	bool alwaysThink_;

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
	bool getIdleStatus() const {return isIdle;}

	virtual void onAddCondition(ConditionType type, bool hadCondition);
	virtual void onEndCondition(ConditionType type, bool lastCondition);
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

	virtual uint64_t getLostExperience() const { return ((skillLoss ? cType.experience(): 0)); }
	virtual void dropLoot(Container* corpse);
	virtual CombatType getDamageImmunities() const { return cType.damageImmunities(); }
	virtual ConditionType getConditionImmunities() const { return cType.conditionImmunities(); }
	virtual uint16_t getCorpseId() const { return cType.corpseId(); }
	virtual void getPathSearchParams(const Creature* creature, FindPathParams& fpp) const;
	virtual bool useCacheMap() const {return true;}
};

#endif
