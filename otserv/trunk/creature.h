//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// base class for every creature
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


#ifndef __OTSERV_CREATURE_H__
#define __OTSERV_CREATURE_H__

#include "definitions.h"

#include "templates.h"
#include "position.h"
#include "condition.h"
#include "const79.h"
#include "tile.h"
#include "enums.h"

#include <list>

enum slots_t {
	SLOT_WHEREEVER = 0,
	SLOT_FIRST = 1,
	SLOT_HEAD = SLOT_FIRST,
	SLOT_NECKLACE = 2,
	SLOT_BACKPACK = 3,
	SLOT_ARMOR = 4,
	SLOT_RIGHT = 5,
	SLOT_LEFT = 6,
	SLOT_LEGS = 7,
	SLOT_FEET = 8,
	SLOT_RING = 9,
	SLOT_AMMO = 10,
	SLOT_DEPOT = 11,
	SLOT_LAST = SLOT_DEPOT
};

struct FindPathParams{
	bool fullPathSearch;
	bool needReachable;
	uint32_t targetDistance;
};

class Map;
class Thing;
class Container;
class Player;
class Monster;
class Npc;
class Item;

//////////////////////////////////////////////////////////////////////
// Defines the Base class for all creatures and base functions which 
// every creature has

class Creature : public AutoID, virtual public Thing
{
public:
	Creature();
	virtual ~Creature();
	
	virtual Creature* getCreature() {return this;};
	virtual const Creature* getCreature()const {return this;};
	virtual Player* getPlayer() {return NULL;};
	virtual const Player* getPlayer() const {return NULL;};
	virtual Npc* getNpc() {return NULL;};
	virtual const Npc* getNpc() const {return NULL;};
	virtual Monster* getMonster() {return NULL;};
	virtual const Monster* getMonster() const {return NULL;};

	virtual const std::string& getName() const = 0;
	virtual const std::string& getNameDescription() const = 0;
	virtual std::string getDescription(int32_t lookDistance) const;

	void setID(){this->id = auto_id | this->idRange();}
	void setRemoved();

	virtual uint32_t idRange() = 0;
	uint32_t getID() const { return id; }
	virtual void removeList() = 0;
	virtual void addList() = 0;

	virtual bool canSee(const Position& pos) const;

	unsigned long getExpForLv(const int& lv) const
	{ 
		return (int)((50*lv*lv*lv)/3 - 100 * lv * lv + (850*lv) / 3 - 200);
	}

	virtual RaceType_t getRace() const {return RACE_NONE;}
	Direction getDirection() const { return direction;}
	void setDirection(Direction dir) { direction = dir;}

	const Position& getMasterPos() const { return masterPos;}
	void setMasterPos(const Position& pos) { masterPos = pos;}

	virtual int getThrowRange() const {return 1;};
	virtual bool isPushable() const {return (getSleepTicks() <= 0);};
	virtual bool isRemoved() const {return isInternalRemoved;};
	virtual bool canSeeInvisibility() const { return false;}
		
	int64_t getSleepTicks() const;
	int64_t getEventStepTicks() const;
	int getStepDuration() const;

	uint32_t getSpeed() const {int32_t n = baseSpeed + varSpeed; return std::max(n, (int32_t)1);}
	void setSpeed(int32_t varSpeedDelta){ varSpeed = varSpeedDelta; }
	
	void setBaseSpeed(uint32_t newBaseSpeed) {baseSpeed = newBaseSpeed;}
	int getBaseSpeed() {return baseSpeed;}

	int32_t getHealth() const {return health;}
	int32_t getMaxHealth() const {return healthMax;}
	int32_t getMana() const {return mana;}
	int32_t getMaxMana() const {return manaMax;}

	const Outfit_t getCurrentOutfit() const {return currentOutfit;}
	const void setCurrentOutfit(Outfit_t outfit) {currentOutfit = outfit;}
	const Outfit_t getDefaultOutfit() const {return defaultOutfit;}
	bool isInvisible() const {return hasCondition(CONDITION_INVISIBLE);}
	bool isInPz() const {return getTile()->hasProperty(PROTECTIONZONE);}

	//walk functions
	bool startAutoWalk(std::list<Direction>& listDir);
	void addEventWalk();
	void stopEventWalk();

	//walk events
	void onWalk(Direction& dir);
	virtual void onWalkAborted() {};

	//follow functions
	virtual const Creature* getFollowCreature() { return followCreature; };
	virtual bool setFollowCreature(Creature* creature);

	//follow events
	virtual void onFollowCreature(const Creature* creature) {};

	//combat functions
	Creature* getAttackedCreature() { return attackedCreature; }
	virtual bool setAttackedCreature(Creature* creature);
	virtual BlockType_t blockHit(Creature* attacker, CombatType_t combatType, int32_t& damage,
		bool checkDefense = false, bool checkArmor = false);

	void setMaster(Creature* creature) {master = creature;}
	Creature* getMaster() {return master;}
	const Creature* getMaster() const {return master;}
	
	virtual void addSummon(Creature* creature);
	virtual void removeSummon(const Creature* creature);

	virtual int32_t getArmor() const {return 0;}
	virtual int32_t getDefense() const {return 0;}

	bool addCondition(Condition* condition);
	void removeCondition(ConditionType_t type, ConditionId_t id);
	void removeCondition(ConditionType_t type);
	void removeCondition(Condition* condition);
	void removeCondition(const Creature* attacker, ConditionType_t type);
	Condition* getCondition(ConditionType_t type, ConditionId_t id) const;
	void executeConditions(int32_t newticks);
	bool hasCondition(ConditionType_t type) const;
	virtual bool isImmune(ConditionType_t type) const;
	virtual bool isImmune(CombatType_t type) const;
	virtual bool isSuppress(ConditionType_t type) const;
	virtual uint32_t getDamageImmunities() const { return 0; }
	virtual uint32_t getConditionImmunities() const { return 0; }
	virtual uint32_t getConditionSuppressions() const { return 0; }
	virtual bool isAttackable() const { return true;};

	virtual void changeHealth(int32_t healthChange);
	virtual void changeMana(int32_t manaChange);

	virtual void drainHealth(Creature* attacker, CombatType_t combatType, int32_t damage);
	virtual void drainMana(Creature* attacker, int32_t manaLoss);

	virtual bool challengeCreature(Creature* creature) {return false;};
	virtual bool convinceCreature(Creature* creature) {return false;};

	virtual void die();
	virtual Item* getCorpse();
	virtual int32_t getGainedExperience(Creature* attacker) const;
	virtual bool addDamagePoints(Creature* attacker, int32_t damagePoints);

	//combat event functions
	virtual void onAddCondition(ConditionType_t type);
	virtual void onEndCondition(ConditionType_t type);
	virtual void onTickCondition(ConditionType_t type, bool& bRemove);
	virtual void onCombatRemoveCondition(const Creature* attacker, Condition* condition);
	virtual void onAttackedCreature(Creature* target);
	virtual void onAttacked();
	virtual void onAttackedCreatureDrainHealth(Creature* target, int32_t points);
	virtual void onAttackedCreatureKilled(Creature* target);
	virtual void onKilledCreature(Creature* target);
	virtual void onGainExperience(int32_t gainExperience);
	virtual void onAttackedCreatureBlockHit(Creature* target, BlockType_t blockType);
	//virtual void onDefenseBlock(bool blockedHit) {};
	//virtual void onArmorBlock(bool blockedHit) {};

	virtual void getCreatureLight(LightInfo& light) const;
	virtual void setNormalCreatureLight();
	void setCreatureLight(LightInfo& light) {internalLight = light;}
	
	void addEventThink();
	void stopEventThink();
	virtual void onThink(uint32_t interval);
	virtual void onAttacking(uint32_t interval);
	virtual void onWalk();
	virtual bool getNextStep(Direction& dir);

	virtual void onAddTileItem(const Position& pos, const Item* item);
	virtual void onUpdateTileItem(const Position& pos, uint32_t stackpos, const Item* oldItem, const Item* newItem);
	virtual void onRemoveTileItem(const Position& pos, uint32_t stackpos, const Item* item);
	virtual void onUpdateTile(const Position& pos);

	virtual void onCreatureAppear(const Creature* creature, bool isLogin);
	virtual void onCreatureDisappear(const Creature* creature, uint32_t stackpos, bool isLogout);
	virtual void onCreatureMove(const Creature* creature, const Position& newPos, const Position& oldPos,
		uint32_t oldStackPos, bool teleport);

	virtual void onAttackedCreatureDissapear(bool isLogout) {};
	virtual void onFollowCreatureDissapear(bool isLogout) {};

	virtual void onCreatureTurn(const Creature* creature, uint32_t stackPos) { };
	virtual void onCreatureSay(const Creature* creature, SpeakClasses type, const std::string& text) { };
	
	virtual void onCreatureChangeOutfit(const Creature* creature, const Outfit_t& outfit) { };
	virtual void onCreatureChangeVisible(const Creature* creature, bool visible);

	virtual void getCombatValues(int32_t& min, int32_t& max) {};
	int32_t getAttackStrength() const {return attackStrength;}
	int32_t getDefenseStrength() const {return defenseStrength;}

	uint32_t getSummonCount() const {return summons.size();}

protected:
	
	int32_t health, healthMax;
	int32_t mana, manaMax;
	int32_t attackStrength;
	int32_t defenseStrength;

	Outfit_t currentOutfit;
	Outfit_t defaultOutfit;

	Position masterPos;
	uint64_t lastMove;
	uint32_t lastStepCost;
	uint32_t baseSpeed;
	int32_t varSpeed;

	Direction direction;

	unsigned long eventCheck;
	
	Creature* master;
	std::list<Creature*> summons;
	
	typedef std::list<Condition*> ConditionList;
	ConditionList conditions;

	//follow variables
	Creature* followCreature;
	unsigned long eventWalk;
	std::list<Direction> listWalkDir;
	bool internalUpdateFollow;

	//combat variables
	Creature* attackedCreature;
	typedef std::map<uint32_t, int32_t> DamageMap;
	DamageMap damageMap;
	uint32_t lastHitCreature;
	uint32_t blockCount;
	uint32_t blockTicks;

	//bool internalDefense;
	//bool internalArmor;

	void onCreatureDisappear(const Creature* creature, bool isLogout);
	virtual void doAttacking(uint32_t interval) {};

	LightInfo internalLight;
	void validateWalkPath();
	virtual int32_t getLostExperience() const { return 0; };
	virtual double getDamageRatio(Creature* attacker) const;
	bool getKillers(Creature** lastHitCreature, Creature** mostDamageCreature);
	virtual void dropLoot(Container* corpse) {};
	virtual uint16_t getLookCorpse() const { return 0; }
	virtual void getPathSearchParams(const Creature* creature, FindPathParams& fpp) const;

	friend class Game;
	friend class Map;
	friend class Commands;
	friend class LuaScriptInterface;
	
	uint32_t id;
	bool isInternalRemoved;
};


#endif
