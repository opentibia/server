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

class Combat{
public:
	Combat();
	virtual ~Combat();

	ReturnValue doCombat(Creature* attacker, Creature* target, int32_t param) const;
	ReturnValue doCombat(Creature* attacker, const Position& pos, int32_t param) const;
	CombatType_t getCombatType() const {return combatType;}
	DamageType_t getDamageType() const {return damageType;}

	void setCombatType(CombatType_t _combatType, uint32_t param);
	void setEffects(uint8_t _distanceEffect, uint8_t _impactEffect);
	void setCondition(Condition* _condition);

protected:
	CombatType_t combatType;
	DamageType_t damageType;

	//Effects
	uint8_t distanceEffect;
	uint8_t impactEffect;

	//Condition
	Condition* condition;

	bool doChangeHealth(Creature* attacker, Creature* target, int32_t healthChange) const;
	bool doChangeMana(Creature* attacker, Creature* target, int32_t manaChange) const;
	bool doCreateField(Creature* attacker, Creature* target, int32_t param) const;
	void doAddCondition(Creature* attacker, Creature* target, int32_t param) const;
	void doRemoveCondition(Creature* attacker, Creature* target, int32_t param) const;

protected:
	ReturnValue internalCombat(Creature* attacker, Creature* target, int32_t param) const;
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

class AreaCombat : public Combat{
public:
	AreaCombat(bool _needDirection = false);

	void setRow(int row, std::vector<uint8_t> data);

	ReturnValue doCombat(Creature* attacker, const Position& pos, int32_t param) const;
	ReturnValue doCombat(Creature* attacker, Creature* target, int32_t param) const;

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