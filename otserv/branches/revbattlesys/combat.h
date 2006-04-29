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
	Combat(uint8_t _impactEffect = NM_ME_NONE);
	virtual ~Combat() = 0;

	virtual bool execute(Creature* attacker, Creature* target) const;
	virtual bool execute(Creature* attacker, const Position& pos) const;

protected:
	//configureable
	uint8_t impactEffect;
};

class CombatHealth : public Combat{
public:
	CombatHealth(DamageType_t _damageType, uint32_t _minChange, uint32_t _maxChange, uint8_t _impactEffect);
	~CombatHealth();

	virtual bool execute(Creature* attacker, Creature* target) const;
	virtual bool execute(Creature* attacker, const Position& pos) const;

protected:
	DamageType_t damageType;

	//configureable
	uint32_t minChange;
	uint32_t maxChange;
};

class CombatMana : public Combat{
public:
	CombatMana(uint32_t _minChange, uint32_t _maxChange, uint8_t _impactEffect);
	~CombatMana();

	virtual bool execute(Creature* attacker, Creature* target) const;
	virtual bool execute(Creature* attacker, const Position& pos) const;

protected:
	uint32_t minChange;
	uint32_t maxChange;
};

class MagicField;

class CombatField : public Combat{
public:
	CombatField(MagicField* _field);
	~CombatField();

	virtual bool execute(Creature* attacker, Creature* target) const;
	virtual bool execute(Creature* attacker, const Position& pos) const;

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
