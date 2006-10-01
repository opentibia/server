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
#include "thing.h"
#include "item.h"
#include "position.h"
#include "condition.h"
#include "const76.h"
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

enum fight_t {
	FIGHT_MELEE,
	FIGHT_DIST,
	FIGHT_MAGICDIST
};

enum playerLooks
{
	PLAYER_MALE_1=0x80,
	PLAYER_MALE_2=0x81,
	PLAYER_MALE_3=0x82,
	PLAYER_MALE_4=0x83,
	PLAYER_MALE_5=0x84,
	PLAYER_MALE_6=0x85,
	PLAYER_MALE_7=0x86,
	PLAYER_FEMALE_1=0x88,
	PLAYER_FEMALE_2=0x89,
	PLAYER_FEMALE_3=0x8A,
	PLAYER_FEMALE_4=0x8B,
	PLAYER_FEMALE_5=0x8C,
	PLAYER_FEMALE_6=0x8D,
	PLAYER_FEMALE_7=0x8E,
};

class Map;
class Thing;
class Container;
class Player;
class Monster;
class Npc;

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

	virtual unsigned long idRange() = 0;
	unsigned long getID() const { return id; }
	virtual void removeList() = 0;
	virtual void addList() = 0;

	virtual bool canSee(const Position& pos) const = 0;
	virtual bool isInRange(const Position& pos) const = 0;

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
	virtual bool isPushable() const {return true;};
	virtual bool isRemoved() const {return isInternalRemoved;};
		
	int64_t getSleepTicks() const;
	int64_t getEventStepTicks() const;
	int getStepDuration() const;
	
	uint32_t getSpeed() const {return speed;};
	void setSpeed(uint32_t newSpeed){ speed = newSpeed; }
	
	void setNormalSpeed() {speed = 220 + (2* (level - 1));}	
	int getNormalSpeed() {return 220 + (2* (level - 1));}
	
	int32_t getHealth() const {return health;}
	int32_t getMaxHealth() const {return healthMax;}
	int32_t getMana() const {return mana;}

	uint8_t getLookType() const { return lookType; }
	uint16_t getLookTypeEx() const { return lookTypeEx; }
	uint8_t getLookHead() const { return lookHead; }
	uint8_t getLookBody() const { return lookBody; }
	uint8_t getLookLegs() const { return lookLegs; }
	uint8_t getLookFeet() const { return lookFeet; }

	//walk functions
	bool startAutoWalk(std::list<Direction>& listDir);
	bool addEventWalk();
	bool stopAutoWalk();

	//walk events
	//virtual void onWalkQuery(bool& continueWalk) {};
	virtual void onWalkAborted() {};

	//follow functions
	virtual const Creature* getFollowCreature() { return followCreature; };
	virtual void setFollowCreature(const Creature* creature);
	virtual bool internalFollowCreature(const Creature* creature);

	//follow events
	virtual void onFollowCreature(const Creature* creature) {};

	//combat functions
	Creature* getAttackedCreature() { return attackedCreature; }
	virtual void setAttackedCreature(Creature* creature);
	virtual BlockType_t blockHit(Creature* attacker, DamageType_t damageType, int32_t& damage,
		bool checkDefense = false, bool checkArmor = false);
	virtual void doAttacking() {};

	void setMaster(Creature* creature);
	Creature* getMaster() {return master;}
	const Creature* getMaster() const {return master;}
	
	virtual void addSummon(Creature* creature);
	virtual void removeSummon(Creature* creature);

	virtual int getArmor() const {return 0;}
	virtual int getDefense() const {return 0;}

	bool addCondition(Condition* condition);
	void removeCondition(ConditionType_t type);
	void executeConditions(int32_t newticks);
	Condition* getCondition(ConditionType_t type);
	bool hasCondition(ConditionType_t type) const;
	virtual bool isImmune(DamageType_t type) const;
	virtual uint32_t getImmunities() const { return 0; }
	virtual bool isAttackable() const { return true;};

	virtual void changeHealth(int32_t healthChange);
	virtual void changeMana(int32_t manaChange);

	virtual void drainHealth(Creature* attacker, DamageType_t damageType, int32_t damage);
	virtual void drainMana(Creature* attacker, int32_t manaLoss);

	virtual void die();
	virtual Item* getCorpse();
	virtual int32_t getGainedExperience(Creature* attacker) const;
	virtual bool addDamagePoints(Creature* attacker, int32_t damagePoints);

	//combat event functions
	virtual void onAddCondition(ConditionType_t type);
	virtual void onEndCondition(ConditionType_t type);
	virtual void onTickCondition(ConditionType_t type, bool& bRemove);
	virtual void onAttackedCreature(Creature* target);
	virtual void onAttacked();
	virtual void onAttackedCreatureDrainHealth(Creature* target, int32_t points);
	virtual void onAttackedCreatureKilled(Creature* target);
	virtual void onKilledCreature(Creature* target);
	virtual void onGainExperience(int32_t gainExperience);
	virtual void onAttackedCreatureBlockHit(Creature* target, BlockType_t blockType);
	//virtual void onTargetCreatureDisappear();
	
	virtual void getCreatureLight(LightInfo& light) const;
	virtual void setNormalCreatureLight();
	void setCreatureLight(LightInfo& light);
	
	virtual void onThink(uint32_t interval);
	virtual void onWalk();
	virtual bool getNextStep(Direction& dir);
	void addWalk(std::list<Direction>& listDir);
	void addWalkEvent();
	void stopWalkEvent();

	virtual void onAddTileItem(const Position& pos, const Item* item);
	virtual void onUpdateTileItem(const Position& pos, uint32_t stackpos, const Item* oldItem, const Item* newItem);
	virtual void onRemoveTileItem(const Position& pos, uint32_t stackpos, const Item* item);
	virtual void onUpdateTile(const Position& pos);

	virtual void onCreatureAppear(const Creature* creature, bool isLogin);
	virtual void onCreatureDisappear(const Creature* creature);
	virtual void onCreatureDisappear(const Creature* creature, uint32_t stackpos, bool isLogout);
	virtual void onCreatureMove(const Creature* creature, const Position& oldPos, uint32_t oldStackPos, bool teleport);

	virtual void onCreatureTurn(const Creature* creature, uint32_t stackPos) { };
	virtual void onCreatureSay(const Creature* creature, SpeakClasses type, const std::string& text) { };
	
	virtual void onCreatureChangeOutfit(const Creature* creature) { };

protected:	
	
	int32_t health, healthMax;
	int32_t mana, manaMax;
	int32_t level;
	int32_t magLevel;

	uint8_t lookType, lookHead, lookBody, lookLegs, lookFeet, lookMaster;
	uint16_t lookTypeEx;

	Position masterPos;
	uint64_t lastMove;
	uint32_t speed;

	Direction direction;

	unsigned long eventCheck;
	unsigned long eventCheckAttacking;
	
	Creature* master;
	std::list<Creature*> summons;
	
	typedef std::list<Condition*> ConditionList;
	ConditionList conditions;

	//follow variables
	const Creature* followCreature;
	unsigned long eventWalk;
	std::list<Direction> listWalkDir;
	bool internalUpdateFollow;
	uint32_t followDistance;

	//combat variables
	Creature* attackedCreature;
	typedef std::map<uint32_t, int32_t> DamageMap;
	DamageMap damageMap;
	uint32_t lastHitCreature;
	bool internalDefense;
	bool internalArmor;

	LightInfo internalLight;
	void validateWalkPath();
	virtual int32_t getLostExperience() const { return 0; };
	virtual double getDamageRatio(Creature* attacker) const;
	bool getKillers(Creature** lastHitCreature, Creature** mostDamageCreature);
	virtual void dropLoot(Container* corpse) {};
	virtual uint16_t getLookCorpse() const { return 0; }

	friend class Game;
	friend class Map;
	friend class Commands;
	friend class LuaScriptInterface;
	
	unsigned long id;
	bool isInternalRemoved;
};


#endif
