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

#include <vector>

class Condition;
class Creature;
class Position;
class Item;
class CombatHealth;
class CombatMana;
class CombatCondition;
class CombatField;

class Combat{
public:
	Combat(uint8_t _impactEffect = NM_ME_NONE);
	virtual ~Combat() = 0;

	virtual CombatHealth* getCombatHealth() {return NULL;};
	virtual const CombatHealth* getCombatHealth() const {return NULL;};

	virtual CombatMana* getCombatMana() {return NULL;};
	virtual const CombatMana* getCombatMana() const {return NULL;};

	virtual CombatCondition* getCombatCondition() {return NULL;};
	virtual const CombatCondition* getCombatCondition() const {return NULL;};

	virtual CombatField* getCombatField() {return NULL;};
	virtual const CombatField* getCombatField() const {return NULL;};

	virtual void doCombat(Creature* attacker, Creature* target) const = 0;
	virtual void doCombat(Creature* attacker, const Position& pos) const = 0;

	virtual bool setParam(CombatParam_t param, uint32_t value);

protected:
	void addImpactEffect(const Position& pos) const;

	//configureable
	uint8_t impactEffect;
};

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
};

class CombatCondition : public Combat{
public:
	CombatCondition(Condition* _condition, uint8_t _impactEffect);
	CombatCondition(ConditionType_t _removeType, uint8_t _impactEffect);
	~CombatCondition();

	virtual CombatCondition* getCombatCondition() {return this;};
	virtual const CombatCondition* getCombatCondition() const {return this;};

protected:
	Condition* condition;
	ConditionType_t removeType;
};

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

