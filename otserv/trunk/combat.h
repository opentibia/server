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

#ifndef __OTSERV_COMBAT_H__
#define __OTSERV_COMBAT_H__

#include "definitions.h"
#include "thing.h"
#include "condition.h"
#include "map.h"
#include "baseevents.h"
#include "otsystem.h"
#include <vector>

class Condition;
class Creature;
class Position;
class Item;

//for luascript callback
class ValueCallback : public CallBack{
public:
	ValueCallback(const formulaType_t& _type);
	virtual ~ValueCallback();
	void getMinMaxValues(Player* player, int32_t& min, int32_t& max, bool useCharges) const;

protected:
	formulaType_t type;
};

class TileCallback : public CallBack{
public:
	virtual ~TileCallback();
	void onTileCombat(Creature* creature, Tile* tile) const;

protected:
	formulaType_t type;
};

class TargetCallback : public CallBack{
public:
	virtual ~TargetCallback();
	void onTargetCombat(Creature* creature, Creature* target) const;

protected:
	formulaType_t type;
};

struct CombatParams{
	CombatParams() {
		combatType = COMBAT_NONE;
		blockedByArmor = false;
		blockedByShield = false;
		targetCasterOrTopMost = false;
		isAggressive = true;
		pzBlock = false;
		itemId = 0;
		hitEffect = NM_ME_UNK;
		hitTextColor = TEXTCOLOR_UNK;
		impactEffect = NM_ME_NONE;
		distanceEffect = NM_ME_NONE;
		dispelType = CONDITION_NONE;
		useCharges = false;

		valueCallback = NULL;
		tileCallback = NULL;
		targetCallback = NULL;
	}

	std::list<const Condition*> conditionList;
	ConditionType_t dispelType;
	CombatType_t combatType;
	bool blockedByArmor;
	bool blockedByShield;
	bool targetCasterOrTopMost;
	bool isAggressive;
	bool pzBlock;
	uint32_t itemId;
	MagicEffectClasses hitEffect;
	TextColor_t hitTextColor;
	uint8_t impactEffect;
	uint8_t distanceEffect;
	bool useCharges;

	ValueCallback* valueCallback;
	TileCallback* tileCallback;
	TargetCallback* targetCallback;
};

typedef bool (*COMBATFUNC)(Creature*, Creature*, const CombatParams&, void*);

struct Combat2Var{
	int32_t minChange;
	int32_t maxChange;
};

class MatrixArea {
public:
	MatrixArea(const uint32_t& _rows, const uint32_t& _cols);
	MatrixArea(const MatrixArea& rhs);
	~MatrixArea();

	void setValue(const uint32_t& row, const uint32_t& col, bool value) const;
	bool getValue(const uint32_t& row, const uint32_t& col) const;

	void setCenter(const uint32_t& y, const uint32_t& x);
	void getCenter(uint32_t& y, uint32_t& x) const;

	const size_t& getRows() const;
	const size_t& getCols() const;

	const bool* operator[](const uint32_t& i) const;
	bool* operator[](const uint32_t& i);

protected:
	uint32_t centerX;
	uint32_t centerY;

	uint32_t rows;
	uint32_t cols;
	bool** data_;
};

typedef std::map<Direction, MatrixArea* > AreaCombatMap;

class AreaCombat{
public:
	AreaCombat();
	~AreaCombat();

	AreaCombat(const AreaCombat& rhs);

	ReturnValue doCombat(Creature* attacker, const Position& pos, const Combat& combat) const;
	bool getList(const Position& centerPos, const Position& targetPos, std::list<Tile*>& list) const;

	void setupArea(const std::list<uint32_t>& list, const uint32_t& rows);
	void setupArea(const int32_t& length, const int32_t& spread);
	void setupArea(const int32_t& radius);
	void setupExtArea(const std::list<uint32_t>& list, const uint32_t& rows);
	void clear();

protected:
	enum MatrixOperation_t{
		MATRIXOPERATION_COPY,
		MATRIXOPERATION_MIRROR,
		MATRIXOPERATION_FLIP,
		MATRIXOPERATION_ROTATE90,
		MATRIXOPERATION_ROTATE180,
		MATRIXOPERATION_ROTATE270
	};

	MatrixArea* createArea(const std::list<uint32_t>& list, const uint32_t& rows);
	void copyArea(const MatrixArea* input, MatrixArea* output, const MatrixOperation_t& op) const;

	MatrixArea* getArea(const Position& centerPos, const Position& targetPos) const;
	int32_t round(const float& v) const;

	AreaCombatMap areas;
	bool hasExtArea;
};

class Combat{
public:
	Combat();
	~Combat();

	static void doCombatHealth(Creature* caster, Creature* target,
		const int32_t& minChange, const int32_t& maxChange, const CombatParams& params);
	static void doCombatHealth(Creature* caster, const Position& pos,
		const AreaCombat* area, const int32_t& minChange, const int32_t& maxChange, const CombatParams& params);

	static void doCombatMana(Creature* caster, Creature* target,
		const int32_t& minChange, const int32_t& maxChange, const CombatParams& params);
	static void doCombatMana(Creature* caster, const Position& pos,
		const AreaCombat* area, const int32_t& minChange, const int32_t& maxChange, const CombatParams& params);

	static void doCombatCondition(Creature* caster, Creature* target,
		const CombatParams& params);
	static void doCombatCondition(Creature* caster, const Position& pos,
		const AreaCombat* area, const CombatParams& params);

	static void doCombatDispel(Creature* caster, Creature* target,
		const CombatParams& params);
	static void doCombatDispel(Creature* caster, const Position& pos,
		const AreaCombat* area, const CombatParams& params);

	static void getCombatArea(const Position& centerPos, const Position& targetPos,
		const AreaCombat* area, std::list<Tile*>& list);

	static bool isInPvpZone(const Creature* attacker, const Creature* target);
	static bool isUnjustKill(const Creature* attacker, const Creature* target);
	static bool isPlayerCombat(const Creature* target);
	static CombatType_t ConditionToDamageType(const ConditionType_t& type);
	static ConditionType_t DamageToConditionType(const CombatType_t& type);
	static ReturnValue canTargetCreature(const Player* attacker, const Creature* target);
	static ReturnValue checkPVPExtraRestrictions(const Creature* attacker, const Creature* target, bool isWalking);
	static ReturnValue canDoCombat(const Creature* caster, const Tile* tile, bool isAggressive);
	static ReturnValue canDoCombat(const Creature* attacker, const Creature* target);
	static void doPVPDamageReduction(int32_t& healthChange, const Player* target);
	static void checkPVPDamageReduction(const Creature* attacker, const Creature* target, int32_t& healthChange);
	static void postCombatEffects(Creature* caster, const Position& pos, const CombatParams& params);

	static void addDistanceEffect(Creature* caster, const Position& fromPos,
		const Position& toPos, const uint8_t& effect);

	void doCombat(Creature* caster, Creature* target) const;
	void doCombat(Creature* caster, const Position& pos) const;

	bool setCallback(const CallBackParam_t& key);
	CallBack* getCallback(const CallBackParam_t& key);

	bool setParam(const CombatParam_t& param, const uint32_t& value);
	void setArea(AreaCombat* _area);
	bool hasArea() const;
	void setCondition(const Condition* _condition);
	void setPlayerCombatValues(const formulaType_t& _type, const double& _mina,
		const double& _minb, const double& _maxa, const double& _maxb);
	void postCombatEffects(Creature* caster, const Position& pos) const;

protected:
	static void doCombatDefault(Creature* caster, Creature* target, const CombatParams& params);

	static void onCreatureDoCombat(Creature* caster, Creature* target, bool isAggressive);

	static void CombatFunc(Creature* caster, const Position& pos,
		const AreaCombat* area, const CombatParams& params, COMBATFUNC func, void* data);

	static bool CombatHealthFunc(Creature* caster, Creature* target, const CombatParams& params, void* data);
	static bool CombatManaFunc(Creature* caster, Creature* target, const CombatParams& params, void* data);
	static bool CombatConditionFunc(Creature* caster, Creature* target, const CombatParams& params, void* data);
	static bool CombatDispelFunc(Creature* caster, Creature* target, const CombatParams& params, void* data);
	static bool CombatNullFunc(Creature* caster, Creature* target, const CombatParams& params, void* data);

	static void combatTileEffects(const SpectatorVec& list, Creature* caster, Tile* tile, const CombatParams& params);
	bool getMinMaxValues(Creature* creature, Creature* target, int32_t& min, int32_t& max) const;

	//configureable
	CombatParams params;

	//formula variables
	formulaType_t formulaType;
	double mina;
	double minb;
	double maxa;
	double maxb;

	AreaCombat* area;
};

class MagicField : public Item {
public:
	MagicField(const uint16_t& _type);
	virtual ~MagicField();

	virtual MagicField* getMagicField();
	virtual const MagicField* getMagicField() const;

	virtual bool isBlocking(const Creature* creature) const;

	bool isReplaceable() const;
	CombatType_t getCombatType() const;
	void onStepInField(Creature* creature, bool purposeful = true);
	bool canOwnerHarm(const Creature* target) const;

private:
	int64_t createTime;
};

inline std::string CombatTypeName(const CombatType_t& combat) {
	switch(combat){
		case COMBAT_NONE: return "unknown";
		case COMBAT_PHYSICALDAMAGE: return "physical";
		case COMBAT_ENERGYDAMAGE: return "energy";
		case COMBAT_EARTHDAMAGE: return "earth";
		case COMBAT_FIREDAMAGE: return "fire";
		case COMBAT_UNDEFINEDDAMAGE: return "undefined";
		case COMBAT_LIFEDRAIN: return "life drain";
		case COMBAT_MANADRAIN: return "mana drain";
		case COMBAT_HEALING: return "healing";
		case COMBAT_DROWNDAMAGE: return "drown";
		case COMBAT_ICEDAMAGE: return "ice";
		case COMBAT_HOLYDAMAGE: return "holy";
		case COMBAT_DEATHDAMAGE: return "death";
	}
	return "none";
}

inline int32_t CombatTypeToIndex(const CombatType_t& combat) {
	if(combat == COMBAT_NONE) return 0;
	for(int32_t c = 0; c < COMBAT_COUNT; ++c){
		if(combat & (1 << c))
			return c+1;
	}
	return 0;
}

inline CombatType_t CombatIndexToType(const int& combatindex) {
	if(combatindex == 0) return COMBAT_NONE;
	return (CombatType_t)(1 << (combatindex-1));
}

#endif // __OTSERV_COMBAT_H__
