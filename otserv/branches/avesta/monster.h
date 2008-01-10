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

#include "tile.h"
#include "monsters.h"

class Creature;
class Game;
class Spawn;

class Monster : public Creature
{
private:
	Monster(MonsterType* mtype);
	//const Monster& operator=(const Monster& rhs);

public:
	static Monster* createMonster(MonsterType* mType);
	static Monster* createMonster(const std::string& name);
	virtual ~Monster();

	virtual Monster* getMonster() {return this;};
	virtual const Monster* getMonster() const {return this;};

	virtual uint32_t idRange(){ return 0x40000000;}
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

	bool canPushItems() const {return mType->canPushItems;}
	bool canPushCreatures() const {return mType->canPushCreatures;}
	bool isHostile() const { return mType->isHostile;}
	virtual bool canSeeInvisibility() const { return isImmune(CONDITION_INVISIBLE);}
	uint32_t getManaCost() const {return mType->manaCost;}	
	void setSpawn(Spawn* _spawn) {spawn = _spawn;};

	virtual void onAttackedCreatureDissapear(bool isLogout);
	virtual void onFollowCreatureDissapear(bool isLogout);

	virtual void onCreatureAppear(const Creature* creature, bool isLogin);
	virtual void onCreatureDisappear(const Creature* creature, uint32_t stackpos, bool isLogout);
	virtual void onCreatureMove(const Creature* creature, const Position& newPos, const Position& oldPos,
		uint32_t oldStackPos, bool teleport);

	virtual void drainHealth(Creature* attacker, CombatType_t combatType, int32_t damage);
	virtual void changeHealth(int32_t healthChange);

	virtual void onWalk();
	virtual bool getNextStep(Direction& dir);

	virtual void onThink(uint32_t interval);

	virtual bool challengeCreature(Creature* creature);
	virtual bool convinceCreature(Creature* creature);

	virtual void setNormalCreatureLight();
	virtual bool getCombatValues(int32_t& min, int32_t& max);

	virtual void doAttacking(uint32_t interval);

private:
	typedef std::list<Creature*> TargetList;
	typedef std::list<Monster*> MonsterList;
	TargetList targetList;
	MonsterList friendList;

	MonsterType* mType;
	Spawn* spawn;

	int32_t minCombatValue;
	int32_t maxCombatValue;
	uint32_t attackTicks;
	uint32_t targetTicks;
	uint32_t targetChangeTicks;
	uint32_t defenseTicks;
	uint32_t yellTicks;
	bool resetTicks;
	bool isActivated;
	bool extraAttack;

	std::string strDescription;

	virtual void onCreatureEnter(Creature* creature);
	virtual void onCreatureLeave(Creature* creature);
	void onCreatureFound(Creature* creature, bool pushFront = false);

	void updateLookDirection();

	void updateTargetList();
	void clearTargetList();

	void die();
	bool despawn();

	bool activate(bool forced = false);
	bool deactivate();

	virtual void onAddCondition(ConditionType_t type);
	virtual void onEndCondition(ConditionType_t type);

	bool canDoSpell(const Position& pos, const Position& targetPos, const spellBlock_t& sb, uint32_t interval);
	bool searchTarget(bool targetChange = false);
	bool selectTarget(Creature* creature);
	bool getRandomStep(const Position& creaturePos, Direction& dir);
	bool getDanceStep(const Position& creaturePos, const Position& centerPos, Direction& dir);
	bool isInSpawnRange(const Position& toPos);
	bool canWalkTo(Position pos, Direction dir);

	bool pushItem(Item* item, int32_t radius);
	void pushItems(Tile* tile);
	bool pushCreature(Creature* creature);
	void pushCreatures(Tile* tile);

	void onThinkTarget(uint32_t interval);
	void onThinkYell(uint32_t interval);
	void onThinkDefense(uint32_t interval);

	virtual int32_t getLostExperience() const { return (isSummon() ? 0 : mType->experience); }
	virtual int getLookCorpse() { return mType->lookcorpse; }
	virtual void dropLoot(Container* corpse);
	virtual uint32_t getDamageImmunities() const { return mType->damageImmunities; }
	virtual uint32_t getConditionImmunities() const { return mType->conditionImmunities; }
	virtual uint16_t getLookCorpse() const { return mType->lookcorpse; }
	virtual void getPathSearchParams(const Creature* creature, FindPathParams& fpp) const;
};

#endif
