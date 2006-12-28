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

#include "thing.h"
#include "definitions.h"
#include "enums.h"
#include "map.h"
#include "baseevents.h"

#include <vector>

class Condition;
class Creature;
class Position;
class Item;

//for luascript callback
class ValueCallback : public CallBack{
public:
	ValueCallback(formulaType_t _type);
	void getMinMaxValues(Player* player, int32_t& min, int32_t& max) const;

protected:
	formulaType_t type;
};

class TileCallback : public CallBack{
public:
	TileCallback() {};
	void onTileCombat(Creature* creature, Tile* tile) const;

protected:
	formulaType_t type;
};	

class TargetCallback : public CallBack{
public:
	TargetCallback() {};
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
		itemId = 0;
		impactEffect = NM_ME_NONE;
		distanceEffect = NM_ME_NONE;
		condition = NULL;
		dispelType = CONDITION_NONE;

		valueCallback = NULL;
		tileCallback = NULL;
		targetCallback = NULL;
	}

	const Condition* condition;
	ConditionType_t dispelType;
	CombatType_t combatType;
	bool blockedByArmor;
	bool blockedByShield;
	bool targetCasterOrTopMost;
	bool isAggressive;
	int32_t itemId;
	uint8_t impactEffect;
	uint8_t distanceEffect;

	ValueCallback* valueCallback;
	TileCallback* tileCallback;
	TargetCallback* targetCallback;
};

typedef bool (*COMBATFUNC)(Creature*, Creature*, const CombatParams&, void*);

struct Combat2Var{
	int32_t minChange;
	int32_t maxChange;
};

class Combat{
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
	static ReturnValue canDoCombat(const Creature* caster, const Tile* tile, bool isAggressive);
	static ReturnValue canDoCombat(Creature* attacker, Creature* target);
	static void postCombatEffects(Creature* caster, const Position& pos, const CombatParams& params);
	static ConditionType_t CombatTypeToCondition(CombatType_t type);

	void doCombat(Creature* caster, Creature* target) const;
	void doCombat(Creature* caster, const Position& pos) const;

	bool setCallback(CallBackParam_t key);
	CallBack* getCallback(CallBackParam_t key);

	bool setParam(CombatParam_t param, uint32_t value);
	void setArea(const AreaCombat* _area);
	bool hasArea() const {return area != NULL;}
	void setCondition(const Condition* _condition);
	void setPlayerCombatValues(formulaType_t _type, double _mina, double _minb, double _maxa, double _maxb);
	void postCombatEffects(Creature* caster, const Position& pos) const;

protected:
	static void doCombatDefault(Creature* caster, Creature* target, const CombatParams& params);

	static void CombatFunc(Creature* caster, const Position& pos,
		const AreaCombat* area, const CombatParams& params, COMBATFUNC func, void* data);

	static bool CombatHealthFunc(Creature* caster, Creature* target, const CombatParams& params, void* data);
	static bool CombatManaFunc(Creature* caster, Creature* target, const CombatParams& params, void* data);
	static bool CombatConditionFunc(Creature* caster, Creature* target, const CombatParams& params, void* data);
	static bool CombatDispelFunc(Creature* caster, Creature* target, const CombatParams& params, void* data);
	static bool CombatNullFunc(Creature* caster, Creature* target, const CombatParams& params, void* data);

	static void combatTileEffects(Creature* caster, Tile* tile, const CombatParams& params);
	void getMinMaxValues(Creature* creature, int32_t& min, int32_t& max) const;

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

template <typename T>
class Matrix
{
public:
	//Matrix() {}
  Matrix(uint32_t _rows, uint32_t _cols)
  {
		centerX = 0;
		centerY = 0;

		rows = _rows;
		cols = _cols;

		data_ = new T*[rows];

		for(uint32_t row = 0; row < rows; ++row){
			data_[row] = new T[cols];
			
			for(uint32_t col = 0; col < cols; ++col){
				data_[row][col] = 0;
			}
		}
  }

	~Matrix()
	{
		for(uint32_t row = 0; row < rows; ++row){
			delete[] data_[row];
		}

		delete[] data_;
	}

	void setValue(uint32_t row, uint32_t col, T value) const {data_[row][col] = value;}
	T getValue(uint32_t row, uint32_t col) const {return data_[row][col];}

	void setCenter(uint32_t y, uint32_t x) {centerX = x; centerY = y;}
	void getCenter(uint32_t& y, uint32_t& x) const {x = centerX; y = centerY;}

	size_t getRows() const {return rows;}
	size_t getCols() const {return cols;}

	inline const T* operator[](uint32_t i) const { return data_[i]; }
	inline T* operator[](uint32_t i) { return data_[i]; }

private:
	uint32_t centerX;
	uint32_t centerY;

	uint32_t rows;
	uint32_t cols;
	T** data_;
};

typedef Matrix<bool> MatrixArea;
typedef std::map<Direction, MatrixArea* > AreaCombatMap;

class AreaCombat{
public:
	AreaCombat();
	~AreaCombat();

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
		MATRIXOPERATION_ROTATE270,
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

class MagicField : public Item{
public:
	MagicField(uint16_t _type);
	~MagicField();

	virtual MagicField* getMagicField() {return this;};
	virtual const MagicField* getMagicField() const {return this;};

	CombatType_t getCombatType() const;
	void onStepInField(Creature* creature);
};

#endif

