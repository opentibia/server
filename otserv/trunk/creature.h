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
#include <boost/any.hpp>
#include "templates.h"
#include "map.h"
#include "position.h"
#include "condition.h"
#include "const.h"
#include "tile.h"
#include "enums.h"
#include "creatureevent.h"
#include <list>

typedef std::list<Condition*> ConditionList;
typedef std::list<CreatureEvent*> CreatureEventList;

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

	// Special slot, covers two, not a real slot
	SLOT_HAND = 12,
	SLOT_TWO_HAND = SLOT_HAND, // alias

	// Last real slot is depot
	SLOT_LAST = SLOT_DEPOT
};

struct FindPathParams{
	bool fullPathSearch;
	bool clearSight;
	bool allowDiagonal;
	bool keepDistance;
	int32_t maxSearchDist;
	int32_t minTargetDist;
	int32_t maxTargetDist;

	FindPathParams()
	{
		clearSight = true;
		fullPathSearch = true;
		allowDiagonal = true;
		keepDistance = false;
		maxSearchDist = -1;
		minTargetDist = -1;
		maxTargetDist = -1;
	}
};

// Used for death entries
struct DeathLessThan;
struct DeathEntry{
	// Death can be either a name (for fields) or a creature (for anything substantial)
	// Fields are only counted if they are the final hit killer
	DeathEntry(std::string name, int32_t dmg) : data(name), damage(dmg), isUnjust(false) {}
	DeathEntry(Creature* killer, int32_t dmg, bool unjust) : data(killer), damage(dmg), isUnjust(unjust) {}

	bool isCreatureKill() const {return data.type() == typeid(Creature*);}
	bool isNameKill() const {return !isCreatureKill();}
	bool isUnjustKill() const {return isUnjust;}

	Creature* getKillerCreature() const {return boost::any_cast<Creature*>(data);}
	std::string getKillerName() const {return boost::any_cast<std::string>(data);}

protected:
	boost::any data;
	int32_t damage;
	bool isUnjust;

	friend struct DeathLessThan;
};

struct DeathLessThan{
	bool operator()(const DeathEntry& d1, const DeathEntry& d2){
		// Sort descending
		return d1.damage > d2.damage;
	}
};

typedef std::vector<DeathEntry> DeathList;
typedef std::map<std::string, std::string> ParametersMap;

class Map;
class Thing;
class Container;
class Player;
class Monster;
class Npc;
class Item;
class Tile;

#define EVENT_CREATURECOUNT 10
#define EVENT_CREATURE_THINK_INTERVAL 1000
#define EVENT_CHECK_CREATURE_INTERVAL (EVENT_CREATURE_THINK_INTERVAL / EVENT_CREATURECOUNT)

class FrozenPathingConditionCall {
public:
	FrozenPathingConditionCall(const Position& _targetPos);
	virtual ~FrozenPathingConditionCall() {}

	virtual bool operator()(const Position& startPos, const Position& testPos,
		const FindPathParams& fpp, int32_t& bestMatchDist) const;

	bool isInRange(const Position& startPos, const Position& testPos,
		const FindPathParams& fpp) const;

protected:
	Position targetPos;
};

//////////////////////////////////////////////////////////////////////
// Defines the Base class for all creatures and base functions which
// every creature has

class Creature : public AutoID, virtual public Thing
{
protected:
	Creature();
public:
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
	virtual std::string getXRayDescription() const;
	virtual std::string getDescription(int32_t lookDistance) const;

	void setID(){this->id = auto_id | this->idRange();}
	void setRemoved() {isInternalRemoved = true;}
	virtual bool getWalkBit(Player *viewer) const;

	virtual uint32_t idRange() = 0;
	uint32_t getID() const { return id; }
	virtual void removeList() = 0;
	virtual void addList() = 0;
	virtual void onRemoved();

	virtual bool canSee(const Position& pos) const;
	virtual bool canSeeCreature(const Creature* creature) const;
	virtual bool canBeSeen(const Creature* viewer, bool checkVisibility = true) const;
	virtual bool canWalkthrough(const Creature* creature) const;

	virtual RaceType_t getRace() const {return RACE_NONE;}
	Direction getDirection() const { return direction;}
	void setDirection(Direction dir) { direction = dir;}

	const Position& getMasterPos() const { return masterPos; }
	virtual void setMasterPos(const Position& pos, uint32_t radius = 1) {
		masterPos = pos;
		masterRadius = radius;
	}

	virtual int getThrowRange() const {return 1;};
	virtual bool isPushable() const {return (getWalkDelay() <= 0);};
	virtual bool canBePushedBy(const Player *player) const;
	virtual bool isRemoved() const {return isInternalRemoved;};
	virtual bool canSeeInvisibility() const { return false;}

	int32_t getWalkDelay(Direction dir) const;
	int32_t getWalkDelay() const;
	int64_t getTimeSinceLastMove() const;

	int64_t getEventStepTicks(bool onlyDelay = false) const;
	int32_t getStepDuration(Direction dir) const;
	int32_t getStepDuration() const;
	virtual int32_t getStepSpeed() const {return getSpeed();}
	int32_t getSpeed() const {return getBaseSpeed() + varSpeed;}
	void setSpeed(int32_t varSpeedDelta)
	{
		int32_t oldSpeed = getSpeed();
		varSpeed = varSpeedDelta;
		if(getSpeed() <= 0){
			stopEventWalk();
			cancelNextWalk = true;
		}
		else if(oldSpeed <= 0 && !listWalkDir.empty()){
			addEventWalk();
		}
	}

	void setBaseSpeed(uint32_t newBaseSpeed) {baseSpeed = newBaseSpeed;}
	uint32_t getBaseSpeed() const {return baseSpeed;}

	virtual int32_t getHealth() const {
		if(health < 0){
			return 0;
		}
		return health;
	}
	virtual int32_t getMaxHealth() const {
		if(healthMax < 0){
			return 0;
		}
		return healthMax;
	}
	virtual int32_t getMana() const
	{
		if(mana < 0){
			return 0;
		}
		return mana;
	}
	virtual int32_t getMaxMana() const
	{
		if(manaMax < 0){
			return 0;
		}
		return manaMax;
	}
	virtual bool hasHiddenHealth() const { return false; }

	const Outfit_t getCurrentOutfit() const {return currentOutfit;}
	const void setCurrentOutfit(Outfit_t outfit) {currentOutfit = outfit;}
	const Outfit_t getDefaultOutfit() const {return defaultOutfit;}
	bool isInvisible() const {return hasCondition(CONDITION_INVISIBLE, false);}
	ZoneType_t getZone() const {return getTile()->getZone();}

	//walk functions
	bool startAutoWalk(std::list<Direction>& listDir);
	void addEventWalk(bool firstStep = false);
	void stopEventWalk();
	void goToFollowCreature();

	//walk events
	virtual void onWalk(Direction& dir);
	virtual void onWalkAborted() {};
	virtual void onWalkComplete() {};

	//follow functions
	virtual Creature* getFollowCreature() const { return followCreature; };
	virtual bool setFollowCreature(Creature* creature, bool fullPathSearch = false);

	//follow events
	virtual void onFollowCreature(const Creature* creature) {};
	virtual void onFollowCreatureComplete(const Creature* creature) {};

	//combat functions
	Creature* getAttackedCreature() { return attackedCreature; }
	virtual bool setAttackedCreature(Creature* creature);
	virtual BlockType_t blockHit(Creature* attacker, CombatType_t combatType, int32_t& damage,
		bool checkDefense = false, bool checkArmor = false);

	void setMaster(Creature* creature) {master = creature;}
	Creature* getMaster() {return master;}
	bool isSummon() const {return master != NULL;}
	bool isPlayerSummon() const {return master != NULL && master->getPlayer() != NULL;}
	Player* getPlayerMaster() const {return (isPlayerSummon() ? master->getPlayer() : NULL);}
	Player* getPlayerInCharge() { return (getPlayer() ? getPlayer() : getPlayerMaster());}
	const Player* getPlayerInCharge() const { return (getPlayer() ? getPlayer() : getPlayerMaster());}
	const Creature* getMaster() const {return master;}

	virtual void addSummon(Creature* creature);
	virtual void removeSummon(const Creature* creature);
	void destroySummons();
	const std::list<Creature*>& getSummons() {return summons;}

	virtual int32_t getArmor() const {return 0;}
	virtual int32_t getDefense() const {return 0;}
	virtual float getAttackFactor() const {return 1.0f;}
	virtual float getDefenseFactor() const {return 1.0f;}

	bool addCondition(Condition* condition);
	bool addCombatCondition(Condition* condition);
	void removeCondition(ConditionType_t type, ConditionId_t id);
	void removeCondition(ConditionType_t type);
	void removeCondition(Condition* condition);
	void removeCondition(const Creature* attacker, ConditionType_t type);
	Condition* getCondition(ConditionType_t type, ConditionId_t id, uint32_t subId) const;
	void executeConditions(uint32_t interval);
	bool hasCondition(ConditionType_t type, bool checkTime = true) const;
	virtual bool isImmune(ConditionType_t type) const;
	virtual bool isImmune(CombatType_t type) const;
	virtual bool isSuppress(ConditionType_t type) const;
	virtual uint32_t getDamageImmunities() const { return 0; }
	virtual uint32_t getConditionImmunities() const { return 0; }
	virtual uint32_t getConditionSuppressions() const { return 0; }
	virtual bool isAttackable() const { return true;}
	virtual void changeHealth(int32_t healthChange);
	virtual void changeMana(int32_t manaChange);

	virtual void gainHealth(Creature* caster, int32_t healthGain);
	virtual void drainHealth(Creature* attacker, CombatType_t combatType, int32_t damage);
	virtual void drainMana(Creature* attacker, int32_t points);

	virtual bool challengeCreature(Creature* creature) {return false;};
	virtual bool convinceCreature(Creature* creature) {return false;};

	virtual void onDie();
	virtual void die();

	virtual uint64_t getGainedExperience(Creature* attacker) const;
	void addDamagePoints(Creature* attacker, int32_t damagePoints);
	void addHealPoints(Creature* caster, int32_t healthPoints);
	bool hasBeenAttacked(uint32_t attackerId) const;

	//combat event functions
	virtual void onAddCondition(ConditionType_t type, bool hadCondition);
	virtual void onAddCombatCondition(ConditionType_t type, bool hadCondition);
	virtual void onEndCondition(ConditionType_t type, bool lastCondition);
	virtual void onTickCondition(ConditionType_t type, int32_t interval, bool& bRemove);
	virtual void onCombatRemoveCondition(const Creature* attacker, Condition* condition);
	virtual void onAttackedCreature(Creature* target) {};
	virtual void onSummonAttackedCreature(Creature* summon, Creature* target) {};
	virtual void onAttacked() {};
	virtual void onAttackedCreatureDrainHealth(Creature* target, int32_t points);
	virtual void onSummonAttackedCreatureDrainHealth(Creature* summon, Creature* target, int32_t points) {};
	virtual void onAttackedCreatureDrainMana(Creature* target, int32_t points) {};
	virtual void onSummonAttackedCreatureDrainMana(Creature* summon, Creature* target, int32_t points) {};
	virtual void onTargetCreatureGainHealth(Creature* target, int32_t points);
	virtual void onAttackedCreatureKilled(Creature* target);
	virtual void onKilledCreature(Creature* target, bool lastHit);
	virtual void onGainExperience(uint64_t gainExp, bool fromMonster);
	virtual void onGainSharedExperience(uint64_t gainExp, bool fromMonster);
	virtual void onAttackedCreatureBlockHit(Creature* target, BlockType_t blockType);
	virtual void onBlockHit(BlockType_t blockType);
	virtual void onChangeZone(ZoneType_t zone);
	virtual void onAttackedCreatureChangeZone(ZoneType_t zone);
	virtual void onIdleStatus();

	virtual void getCreatureLight(LightInfo& light) const;
	virtual void setNormalCreatureLight();
	void setCreatureLight(LightInfo& light) {internalLight = light;}

	virtual void onThink(uint32_t interval);
	virtual void onAttacking(uint32_t interval);
	virtual void onWalk();
	virtual bool getNextStep(Direction& dir, uint32_t& flags);

	virtual void onAddTileItem(const Tile* tile, const Position& pos, const Item* item);
	virtual void onUpdateTileItem(const Tile* tile, const Position& pos,
		const Item* oldItem, const ItemType& oldType, const Item* newItem, const ItemType& newType);
	virtual void onRemoveTileItem(const Tile* tile, const Position& pos,
		const ItemType& iType, const Item* item);
	virtual void onUpdateTile(const Tile* tile, const Position& pos);

	virtual void onCreatureAppear(const Creature* creature, bool isLogin);
	virtual void onCreatureDisappear(const Creature* creature, bool isLogout);
	virtual void onCreatureMove(const Creature* creature, const Tile* newTile, const Position& newPos,
		const Tile* oldTile, const Position& oldPos, bool teleport);

	virtual void onAttackedCreatureDissapear(bool isLogout) {};
	virtual void onFollowCreatureDissapear(bool isLogout) {};

	virtual void onCreatureTurn(const Creature* creature) { };
	virtual void onCreatureSay(const Creature* creature, SpeakClasses type, const std::string& text) { };

	virtual void onCreatureChangeOutfit(const Creature* creature, const Outfit_t& outfit) { };
	virtual void onCreatureConvinced(const Creature* convincer, const Creature* creature) {};
	virtual void onCreatureChangeVisible(const Creature* creature, bool visible);
	virtual void onPlacedCreature() {};
	virtual void onRemovedCreature() {};

	virtual WeaponType_t getWeaponType() {return WEAPON_NONE;}
	virtual bool getCombatValues(int32_t& min, int32_t& max) {return false;}

	size_t getSummonCount() const {return summons.size();}
	void setDropLoot(bool _lootDrop) {lootDrop = _lootDrop;}
	void setLossSkill(bool _skillLoss) {skillLoss = _skillLoss;}

	//creature script events
	bool registerCreatureEvent(const std::string& name);

	virtual void setParent(Cylinder* cylinder){
		_tile = dynamic_cast<Tile*>(cylinder);
		Thing::setParent(cylinder);
	}

	virtual Position getPosition() const {return _tile->getTilePosition();}
	virtual Tile* getTile(){return _tile;}
	virtual const Tile* getTile() const{return _tile;}
	int32_t getWalkCache(const Position& pos) const;

	static bool canSee(const Position& myPos, const Position& pos, uint32_t viewRangeX, uint32_t viewRangeY);

protected:
	static const int32_t mapWalkWidth = Map::maxViewportX * 2 + 1;
	static const int32_t mapWalkHeight = Map::maxViewportY * 2 + 1;
	bool localMapCache[mapWalkHeight][mapWalkWidth];

	virtual bool useCacheMap() const {return false;}

	Tile* _tile;
	uint32_t id;
	bool isInternalRemoved;
	bool isMapLoaded;
	bool isUpdatingPath;
	// The creature onThink event vector this creature belongs to
	// -1 represents that the creature isn't in any vector
	int32_t checkCreatureVectorIndex;
	bool creatureCheck;

	int32_t health, healthMax;
	int32_t mana, manaMax;

	Outfit_t currentOutfit;
	Outfit_t defaultOutfit;

	Position masterPos;
	int32_t masterRadius;
	uint64_t lastStep;
	uint32_t lastStepCost;
	uint32_t baseSpeed;
	int32_t varSpeed;
	bool skillLoss;
	bool lootDrop;
	Direction direction;
	ConditionList conditions;
	LightInfo internalLight;

	//summon variables
	Creature* master;
	std::list<Creature*> summons;

	//follow variables
	Creature* followCreature;
	uint32_t eventWalk;
	bool cancelNextWalk;
	std::list<Direction> listWalkDir;
	uint32_t walkUpdateTicks;
	bool hasFollowPath;
	bool forceUpdateFollowPath;

	//combat variables
	Creature* attackedCreature;

	struct CountBlock_t{
		int32_t total;
		int64_t ticks;
		uint32_t hits;
	};

	typedef std::map<uint32_t, CountBlock_t> CountMap;
	CountMap damageMap;
	CountMap healMap;
	CombatType_t lastDamageSource;
	uint32_t lastHitCreature;
	uint32_t blockCount;
	uint32_t blockTicks;

	//creature script events
	bool hasEventRegistered(CreatureEventType_t event){
		return (0 != (scriptEventsBitField & ((uint32_t)1 << event)));
	}
	uint32_t scriptEventsBitField;
	typedef std::list<CreatureEvent*> CreatureEventList;
	CreatureEventList eventsList;
	CreatureEventList getCreatureEvents(CreatureEventType_t type);
	void onDieEvent(Item* corpse);
	void onKillEvent(Creature* target, bool lastHit);

	void updateMapCache();
#ifdef __DEBUG__
	void validateMapCache();
#endif
	void updateTileCache(const Tile* tile);
	void updateTileCache(const Tile* tile, int32_t dx, int32_t dy);
	void updateTileCache(const Tile* tile, const Position& pos);
	void internalCreatureDisappear(const Creature* creature, bool isLogout);
	virtual void doAttacking(uint32_t interval) {};
	virtual bool hasExtraSwing() {return false;}

	virtual uint64_t getLostExperience() const { return 0; };
	virtual double getDamageRatio(Creature* attacker) const;
	DeathList getKillers(int32_t assist_count = 1);
	virtual void dropLoot(Container* corpse) {};
	virtual uint16_t getLookCorpse() const { return 0; }
	virtual void getPathSearchParams(const Creature* creature, FindPathParams& fpp) const;
	virtual Item* dropCorpse();
	virtual Item* createCorpse();

	friend class Game;
	friend class Map;
	friend class LuaScriptInterface;
};

#endif
