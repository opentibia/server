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
//class CombatHealth;
//class CombatMana;
//class CombatCondition;
//class CombatField;

//for luascript callback
class CombatCallBack : public CallBack{
public:
	void getMinMaxValues(Player* player, int32_t& min, int32_t& max) const;
};

class Combat{
public:
	Combat(CombatType_t _type);
	~Combat();

	static bool doCombatHealth(Creature* attacker, DamageType_t damageType, Creature* target,
		int32_t minChange, int32_t maxChange);
	static void doCombatHealth(Creature* attacker, DamageType_t damageType, Creature* target,
		int32_t minChange, int32_t maxChange, uint8_t effect,
		const Condition* condition = NULL);
	static void doCombatHealth(Creature* attacker, DamageType_t damageType, const Position& pos,
		const AreaCombat* area, int32_t minChange, int32_t maxChange, uint8_t effect,
		const Condition* condition = NULL);

	static bool doCombatMana(Creature* attacker, Creature* target,
		int32_t minChange, int32_t maxChange);
	static void doCombatMana(Creature* attacker, Creature* target,
		int32_t minChange, int32_t maxChange, uint8_t effect,
		const Condition* condition = NULL);
	static void doCombatMana(Creature* attacker, const Position& pos,
		const AreaCombat* area, int32_t minChange, int32_t maxChange, uint8_t effect,
		const Condition* condition = NULL);

	static bool doCombatCondition(Creature* attacker, Creature* target,
		const Condition* condition);
	static void doCombatCondition(Creature* attacker, Creature* target,
		const Condition* condition, uint8_t effect);
	static void doCombatCondition(Creature* attacker, const Position& pos,
		const AreaCombat* area, const Condition* condition, uint8_t effect);

	static void getCombatArea(Creature* attacker, const Position& pos, const AreaCombat* area, std::list<Tile*>& list);
	static bool canDoCombat(const Creature* attacker, const Tile* tile);

	void doCombat(Creature* attacker, Creature* target) const;
	void doCombat(Creature* attacker, const Position& pos) const;

	bool setCallback(CombatParam_t key);
	CallBack* getCallback();

	virtual bool setParam(CombatParam_t param, uint32_t value);
	void setArea(const AreaCombat* _area);
	void setCondition(const Condition* _condition);

protected:
	void getMinMaxValues(Creature* creature, int32_t& min, int32_t& max) const;

	//configureable
	CombatType_t combatType;
	DamageType_t damageType;

	AreaCombat* area;
	Condition* condition;
	CombatCallBack* callback;
	uint8_t impactEffect;
};

/*
class CombatHealth : public Combat{
public:
	CombatHealth(DamageType_t _damageType, uint8_t _impactEffect);
	~CombatHealth();

	virtual CombatHealth* getCombatHealth() {return this;};
	virtual const CombatHealth* getCombatHealth() const {return this;};

	void doCombat(Creature* attacker, Creature* target) const;
	void doCombat(Creature* attacker, const Position& pos) const;
	void doCombat(Creature* attacker, Creature* target, int32_t minChange, int32_t maxChange) const;
	void doCombat(Creature* attacker, const Position& pos, int32_t minChange, int32_t maxChange) const;

	virtual bool setParam(CombatParam_t param, uint32_t value);

protected:
	void internalCombat(Creature* attacker, Creature* target, int32_t healthChange) const;
	void internalCombat(Creature* attacker, const Position& pos, int32_t minChange, int32_t maxChange) const;
	bool canDoCombat(const Creature* attacker, const Tile* tile) const;
	DamageType_t damageType;
};

class CombatMana : public Combat{
public:
	CombatMana(uint8_t _impactEffect);
	~CombatMana();

	virtual CombatMana* getCombatMana() {return this;};
	virtual const CombatMana* getCombatMana() const {return this;};

	void doCombat(Creature* attacker, Creature* target) const;
	void doCombat(Creature* attacker, const Position& pos) const;
	void doCombat(Creature* attacker, Creature* target, int32_t minChange, int32_t maxChange) const;
	void doCombat(Creature* attacker, const Position& pos, int32_t minChange, int32_t maxChange) const;

protected:
	void internalCombat(Creature* attacker, Creature* target, int32_t manaChange) const;
	void internalCombat(Creature* attacker, const Position& pos, int32_t minChange, int32_t maxChange) const;
	bool canDoCombat(const Creature* attacker, const Tile* tile) const;
};
*/

/*
class CombatCondition : public Combat{
public:
	CombatCondition(Condition* _condition, uint8_t _impactEffect);
	CombatCondition(ConditionType_t _removeType, uint8_t _impactEffect);
	~CombatCondition();

	virtual CombatCondition* getCombatCondition() {return this;};
	virtual const CombatCondition* getCombatCondition() const {return this;};

	virtual void doCombat(Creature* attacker, Creature* target) const;
	virtual void doCombat(Creature* attacker, const Position& pos) const;

protected:
	void internalDoCombat(Creature* attacker, Creature* target) const;

	Condition* condition;
	ConditionType_t removeType;
};
*/

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

