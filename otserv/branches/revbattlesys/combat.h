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
class CombatCallBack : public CallBack{
public:
	void getMinMaxValues(Player* player, int32_t& min, int32_t& max) const;
};

struct CombatParams{
	CombatParams() {
		damageType = DAMAGE_NONE;
		blockedByArmor = false;
		blockedByShield = false;
		targetCasterOrTopMost = false;
		impactEffect = NM_ME_NONE;
		distanceEffect = NM_ME_NONE;
	}

	DamageType_t damageType;
	bool blockedByArmor;
	bool blockedByShield;
	bool targetCasterOrTopMost;

	uint8_t impactEffect;
	uint8_t distanceEffect;
};

class Combat{
public:
	Combat(CombatType_t _type);
	~Combat();

	static bool doCombatHealth(Creature* caster, Creature* target, 
		int32_t minChange, int32_t maxChange, const CombatParams& params);
	static void doCombatHealth(Creature* caster, Creature* target,
		int32_t minChange, int32_t maxChange, const CombatParams& params,
		const Condition* condition);
	static void doCombatHealth(Creature* caster, const Position& pos,
		const AreaCombat* area, int32_t minChange, int32_t maxChange, const CombatParams& params,
		const Condition* condition);

	static bool doCombatMana(Creature* caster, Creature* target,
		int32_t minChange, int32_t maxChange, const CombatParams& params);
	static void doCombatMana(Creature* caster, Creature* target,
		int32_t minChange, int32_t maxChange, const CombatParams& params,
		const Condition* condition);
	static void doCombatMana(Creature* caster, const Position& pos,
		const AreaCombat* area, int32_t minChange, int32_t maxChange, const CombatParams& params,
		const Condition* condition);

	static bool doCombatCondition(Creature* caster, Creature* target,
		const Condition* condition);
	static void doCombatCondition(Creature* caster, Creature* target,
		const Condition* condition, const CombatParams& params);
	static void doCombatCondition(Creature* caster, const Position& pos,
		const AreaCombat* area, const Condition* condition, const CombatParams& params);

	static void getCombatArea(Creature* caster, const Position& pos, const AreaCombat* area, std::list<Tile*>& list);
	static bool canDoCombat(const Creature* caster, const Tile* tile);

	void doCombat(Creature* caster, Creature* target) const;
	void doCombat(Creature* caster, const Position& pos) const;

	bool setCallback(CombatParam_t key);
	CallBack* getCallback();

	virtual bool setParam(CombatParam_t param, uint32_t value);
	void setArea(const AreaCombat* _area);
	void setCondition(const Condition* _condition);

protected:
	void getMinMaxValues(Creature* creature, int32_t& min, int32_t& max) const;

	//configureable
	CombatType_t combatType;

	CombatParams params;

	AreaCombat* area;
	Condition* condition;
	CombatCallBack* callback;
};

/*
class MagicField;

class CombatField : public Combat{
public:
	CombatField(MagicField* _field);
	~CombatField();

	virtual CombatField* getCombatField() {return this;};
	virtual const CombatField* getCombatField() const {return this;};
	
	void doCombat(Creature* attacker, Creature* target) const;
	void doCombat(Creature* attacker, const Position& pos) const;

protected:
	MagicField* field;
};
*/

template <typename T>
class Matrix
{
public:
  Matrix(){};
  Matrix(int rows, int cols)
  {
    for(int i = 0; i < rows; ++i){
      data_.push_back(std::vector<T>(cols));
		}
  }

	size_t getRows() const {return data_.size();}
	size_t getCols() const {return data_[0].size();}
  inline std::vector<T> & operator[](int i) { return data_[i]; }

  inline const std::vector<T> & operator[] (int i) const { return data_[i]; }

  void resize(int rows, int cols)
  {
    data_.resize(rows);
		for(int i = 0; i < rows; ++i){
      data_[i].resize(cols);
		}
  }

private:
  std::vector<std::vector<T> > data_; 
};

class AreaCombat{
public:
	AreaCombat();
	~AreaCombat();

	ReturnValue doCombat(Creature* attacker, const Position& pos, const Combat& combat) const;
	void setRow(int row, std::vector<uint8_t> data);

	bool getList(const Position& pos, Direction rotatedir, std::list<Tile*>& list) const;

protected:
	Matrix<uint8_t> area;
	bool needDirection;
};

class MagicField : public Item{
public:
	MagicField(uint16_t _type);
	~MagicField();

	virtual MagicField* getMagicField() {return this;};
	virtual const MagicField* getMagicField() const {return this;};

	DamageType_t getDamageType() const;
	const Condition* getCondition() const { return condition; }
	void setCondition(Condition* _condition) { condition = _condition; }

protected:
	Condition* condition;
	DamageType_t damageType;
};

#endif

