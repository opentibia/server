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

class Monster : public Creature
{
private:
	Monster(MonsterType* mtype);
	//const Monster& operator=(const Monster& rhs);

public:
	static Monster* createMonster(const std::string& name);
	virtual ~Monster();

	virtual Monster* getMonster() {return this;};
	virtual const Monster* getMonster() const {return this;};

	virtual uint32_t idRange(){ return 0x40000000;}
	static AutoList<Monster> listMonster;
	void removeList() {listMonster.removeList(getID());}
	void addList() {listMonster.addList(this);}
	
	virtual const std::string& getName() const {return mType->name;}
	virtual const std::string& getNameDescription() const {return mType->nameDescription;}
	virtual std::string getDescription(int32_t lookDistance) const;

	virtual bool canSee(const Position& pos) const;

	virtual RaceType_t getRace() const { return mType->race; }

	virtual int32_t getArmor() const { return mType->armor; }
	virtual int32_t getDefense() const { return mType->defense; }

	virtual bool isPushable() const { return mType->pushable; }
	virtual bool isAttackable() const { return mType->isAttackable;}
	virtual void doAttacking(uint32_t interval);

	virtual void drainHealth(Creature* attacker, CombatType_t combatType, int32_t damage);

	virtual bool challengeCreature(Creature* creature);
	virtual bool convinceCreature(Creature* creature);

	virtual void setNormalCreatureLight();

	virtual void onAttackedCreatureDissapear(bool isLogout);
	virtual void onFollowCreatureDissapear(bool isLogout);

	virtual void onAddTileItem(const Position& pos, const Item* item);
	virtual void onUpdateTileItem(const Position& pos, uint32_t stackpos, const Item* oldItem, const Item* newItem);
	virtual void onRemoveTileItem(const Position& pos, uint32_t stackpos, const Item* item);
	virtual void onUpdateTile(const Position& pos);

	virtual void onCreatureAppear(const Creature* creature, bool isLogin);
	virtual void onCreatureDisappear(const Creature* creature, uint32_t stackpos, bool isLogout);
	virtual void onCreatureMove(const Creature* creature, const Position& newPos, const Position& oldPos,
		uint32_t oldStackPos, bool teleport);

	virtual void onThink(uint32_t interval);
	virtual void onWalk();
	virtual bool getNextStep(Direction& dir);

	bool canPushItems() const {return mType->canPushItems;}
	bool isSummon() const { return getMaster() != NULL; }
	bool isHostile() const { return mType->isHostile;}
	virtual bool canSeeInvisibility() const { return isImmune(CONDITION_INVISIBLE);}

	virtual void getCombatValues(int32_t& min, int32_t& max);
	
	uint32_t getManaCost() const {return mType->manaCost;}

private:
	int32_t thinkTicks;
	uint32_t yellTicks;
	uint32_t attackTicks;
	uint32_t defenseTicks;
	int32_t changeTargetTicks;
	std::string strDescription;
	bool internalUpdateTargetList;
	bool isActive;
	bool isWalkActive;
	bool spellBonusAttack;

	int32_t minCombatValue;
	int32_t maxCombatValue;

	typedef std::list<Creature*> TargetList;
	TargetList targetList;
	MonsterType* mType;

	void searchTarget();
	bool selectTarget(Creature* creature);

	void startThink();
	void stopThink();
	void onThinkYell(uint32_t interval);
	void onDefending(uint32_t interval);
	void onThinkChangeTarget(uint32_t interval);

	void onCreatureEnter(const Creature* creature);
	void onCreatureLeave(const Creature* creature);
	void onCreatureFound(const Creature* creature);

	void updateLookDirection();
	void updateTargetList();
	void clearTargetList();

	bool isInSpawnZone(const Position& pos);
	bool getRandomStep(const Position& creaturePos, const Position& centerPos, Direction& dir);

	virtual int32_t getLostExperience() const { return (isSummon() ? 0 : mType->experience); }
	virtual int getLookCorpse() { return mType->lookcorpse; }
	virtual void dropLoot(Container* corpse);
	virtual uint32_t getDamageImmunities() const { return mType->damageImmunities; }
	virtual uint32_t getConditionImmunities() const { return mType->conditionImmunities; }
	virtual uint16_t getLookCorpse() const { return mType->lookcorpse; }
	virtual void getPathSearchParams(const Creature* creature, FindPathParams& fpp) const;
};

#endif
