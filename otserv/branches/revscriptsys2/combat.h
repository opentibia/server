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

#include "classes.h"
#include "map.h"
#include "otsystem.h"

//for luascript callback
class CombatDamageCalculationCallback {
public:
	CombatDamageCalculationCallback() : type(FORMULA_UNDEFINED) {}
	CombatDamageCalculationCallback(FormulaType type) : type(type) {}
	CombatDamageCalculationCallback(const CombatDamageCalculationCallback& c) : type(c.type) {}
	CombatDamageCalculationCallback& operator=(const CombatDamageCalculationCallback& rhs) {type = rhs.type; return *this;}

	void operator()(Creature* creature, int32_t& min, int32_t& max, bool useCharges) const;
	operator bool() const {return type != FORMULA_UNDEFINED;}
protected:
	FormulaType type;
};

/*
class TileCallback : public CallBack{
public:
	TileCallback() {};
	void onTileCombat(Creature* creature, Tile* tile) const;

protected:
	FormulaType type;
};

class TargetCallback : public CallBack{
public:
	TargetCallback() {};
	void onTargetCombat(Creature* creature, Creature* target) const;

protected:
	FormulaType type;
};
*/
struct CombatParams{
	CombatParams() {
		combatType = COMBAT_NONE;
		blockedByArmor = false;
		blockedByShield = false;
		targetCasterOrTopMost = false;
		isAggressive = true;
		itemId = 0;
		hitEffect = NM_ME_UNK;
		hitTextColor = TEXTCOLOR_UNK;
		impactEffect = NM_ME_NONE;
		distanceEffect = NM_ME_NONE;
		dispelType = CONDITION_NONE;
		useCharges = false;
	}

	std::list<const Condition*> conditionList;
	ConditionType dispelType;
	CombatType combatType;
	bool blockedByArmor;
	bool blockedByShield;
	bool targetCasterOrTopMost;
	bool isAggressive;
	uint32_t itemId;
	MagicEffectClasses hitEffect;
	TextColor_t hitTextColor;
	uint8_t impactEffect;
	uint8_t distanceEffect;
	bool useCharges;

	CombatDamageCalculationCallback damageCallback;
};

typedef bool (*COMBATFUNC)(Creature*, Creature*, const CombatParams&, void*);

struct Combat2Var{
	int32_t minChange;
	int32_t maxChange;
};

class MatrixArea
{
public:
	MatrixArea(uint32_t _rows, uint32_t _cols)
	{
		centerX = 0;
		centerY = 0;

		rows = _rows;
		cols = _cols;

		data_ = new bool*[rows];

		for(uint32_t row = 0; row < rows; ++row){
			data_[row] = new bool[cols];

			for(uint32_t col = 0; col < cols; ++col){
				data_[row][col] = 0;
			}
		}
	}

	MatrixArea(const MatrixArea& rhs)
	{
		centerX = rhs.centerX;
		centerY = rhs.centerY;
		rows = rhs.rows;
		cols = rhs.cols;

		data_ = new bool*[rows];

		for(uint32_t row = 0; row < rows; ++row){
			data_[row] = new bool[cols];

			for(uint32_t col = 0; col < cols; ++col){
				data_[row][col] = rhs.data_[row][col];
			}
		}
	}

	~MatrixArea()
	{
		for(uint32_t row = 0; row < rows; ++row){
			delete[] data_[row];
		}

		delete[] data_;
	}

	void setValue(uint32_t row, uint32_t col, bool value) const {data_[row][col] = value;}
	bool getValue(uint32_t row, uint32_t col) const {return data_[row][col];}

	void setCenter(uint32_t y, uint32_t x) {centerX = x; centerY = y;}
	void getCenter(uint32_t& y, uint32_t& x) const {x = centerX; y = centerY;}

	size_t getRows() const {return rows;}
	size_t getCols() const {return cols;}

	inline const bool* operator[](uint32_t i) const { return data_[i]; }
	inline bool* operator[](uint32_t i) { return data_[i]; }

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
	AreaCombat() {hasExtArea = false;}
	~AreaCombat() {clear();}

	AreaCombat(const AreaCombat& rhs);

	ReturnValue doCombat(Creature* attacker, const Position& pos, const Combat& combat) const;
	bool getList(const Position& centerPos, const Position& targetPos, std::list<Tile*>& list) const;

	void setupArea(const std::list<uint32_t>& list, uint32_t rows);
	void setupArea(int32_t length, int32_t spread);
	void setupArea(int32_t radius);
	void setupExtArea(const std::list<uint32_t>& list, uint32_t rows);
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

	MatrixArea* createArea(const std::list<uint32_t>& list, uint32_t rows);
	void copyArea(const MatrixArea* input, MatrixArea* output, MatrixOperation_t op) const;

	MatrixArea* getArea(const Position& centerPos, const Position& targetPos) const
	{
		int32_t dx = targetPos.x - centerPos.x;
		int32_t dy = targetPos.y - centerPos.y;

		Direction dir = NORTH;

		if(dx < 0){
			dir = WEST;
		}
		else if(dx > 0){
			dir = EAST;
		}
		else if(dy < 0){
			dir = NORTH;
		}
		else{
			dir = SOUTH;
		}

		if(hasExtArea){
			if(dx < 0 && dy < 0)
				dir = NORTHWEST;
			else if(dx > 0 && dy < 0)
				dir = NORTHEAST;
			else if(dx < 0 && dy > 0)
				dir = SOUTHWEST;
			else if(dx > 0 && dy > 0)
				dir = SOUTHEAST;
		}

		AreaCombatMap::const_iterator it = areas.find(dir);
		if(it != areas.end()){
			return it->second;
		}

		return NULL;
	}

	AreaCombatMap areas;
	bool hasExtArea;
};

class Combat {
public:
	Combat();
	~Combat();

	static void doCombatHealth(Creature* caster, Creature* target,
		int32_t minChange, int32_t maxChange, const CombatParams& params);
	static void doCombatHealth(Creature* caster, const Position& pos,
		const AreaCombat* area, int32_t minChange, int32_t maxChange, const CombatParams& params);

	static void doCombatMana(Creature* caster, Creature* target,
		int32_t minChange, int32_t maxChange, const CombatParams& params);
	static void doCombatMana(Creature* caster, const Position& pos,
		const AreaCombat* area, int32_t minChange, int32_t maxChange, const CombatParams& params);

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

	static Position getCasterPosition(const Creature* creature, Direction dir);
	static bool isInPvpZone(const Creature* attacker, const Creature* target);
	static bool isUnjustKill(const Creature* attacker, const Creature* target);
	static bool isPlayerCombat(const Creature* target);
	static CombatType ConditionToDamageType(ConditionType type);
	static ConditionType DamageToConditionType(CombatType type);
	static ReturnValue canTargetCreature(const Player* attacker, const Creature* target);
	static ReturnValue canDoCombat(const Creature* caster, const Tile* tile, bool isAggressive);
	static ReturnValue canDoCombat(const Creature* attacker, const Creature* target);
	static void postCombatEffects(Creature* caster, const Position& pos, const CombatParams& params);

	static void addDistanceEffect(Creature* caster, const Position& fromPos, const Position& toPos,
	uint8_t effect);

	void doCombat(Creature* caster, Creature* target) const;
	void doCombat(Creature* caster, const Position& pos) const;

	bool setParam(CombatParam param, uint32_t value);
	void setArea(AreaCombat* _area)
	{
		if(area){
			delete area;
		}

		area = _area;
	}
	bool hasArea() const {return area != NULL;}
	void setCondition(const Condition* _condition) {params.conditionList.push_back(_condition);}
	void setPlayerCombatValues(FormulaType _type, double _mina, double _minb, double _maxa, double _maxb);
	void postCombatEffects(Creature* caster, const Position& pos) const {Combat::postCombatEffects(caster, pos, params);}

protected:
	static void doCombatDefault(Creature* caster, Creature* target, const CombatParams& params);

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
	FormulaType formulaType;
	double mina;
	double minb;
	double maxa;
	double maxb;

	AreaCombat* area;
};

class MagicField : public Item{
public:
	MagicField(uint16_t _type) : Item(_type) {createTime = OTSYS_TIME();}
	~MagicField() {}

	virtual MagicField* getMagicField() {return this;}
	virtual const MagicField* getMagicField() const {return this;}

	bool isReplaceable() const {return Item::items[getID()].replaceable;}
	CombatType getCombatType() const {
		const ItemType& it = items[getID()];
		return it.combatType;
	}
	void onStepInField(Creature* creature, bool purposeful = true);

private:
	int64_t createTime;
};

#endif
