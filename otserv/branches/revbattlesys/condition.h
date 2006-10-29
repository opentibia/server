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

#ifndef __OTSERV_CONDITION_H__
#define __OTSERV_CONDITION_H__

#include "definitions.h"
#include "enums.h"

#include <list>

class Creature;
class Player;

enum ConditionType_t {
	CONDITION_NONE					= 0,
	CONDITION_POISON				= 1,
	CONDITION_FIRE					= 2,
	CONDITION_ENERGY				= 4,
	CONDITION_LIFEDRAIN			= 8,
	CONDITION_HASTE					= 16,
	CONDITION_PARALYZE			= 32,
	CONDITION_OUTFIT				= 64,
	CONDITION_INVISIBLE			= 128,
	CONDITION_LIGHT					= 256,
	CONDITION_MANASHIELD		= 512,
	CONDITION_INFIGHT				= 1024,
	CONDITION_DRUNK					= 2048,
	CONDITION_EXHAUSTED			= 4096,
	CONDITION_REGENERATION	= 8192
};

enum EndCondition_t {
	REASON_ENDTICKS,
	REASON_ABORT,
};

class Condition{
public:
	Condition(int32_t _id, ConditionType_t _type, int32_t _ticks);
	virtual ~Condition(){};
	
	virtual bool startCondition(Creature* creature) = 0;
	virtual bool executeCondition(Creature* creature, int32_t interval);
	virtual void endCondition(Creature* creature, EndCondition_t reason) = 0;	
	virtual void addCondition(Creature* creature, const Condition* condition) = 0;
	virtual uint8_t getIcons() const = 0;
	//virtual bool stackable() const = 0;
	virtual int32_t getId() const {return id;}

	virtual Condition* clone() const = 0;

	ConditionType_t getType() const { return conditionType;}
	int32_t getTicks() const { return ticks; }
	void setTicks(int32_t newTicks) { ticks = newTicks; }
	bool reduceTicks(int32_t interval);

	static Condition* createCondition(ConditionType_t _type, int32_t ticks, int32_t param, int32_t _id /*= 0*/);

	virtual bool setParam(ConditionParam_t param, int32_t value);
	void setFormulaVars(double _mina, double _minb, double _maxa, double _maxb);

protected:
	void Condition::getFormulaValues(int32_t var, int32_t& min, int32_t& max) const;

	int32_t id;
	int32_t ticks;
	ConditionType_t conditionType;

	//formula variables
	double mina;
	double minb;
	double maxa;
	double maxb;
};

class ConditionGeneric: public Condition
{
public:
	ConditionGeneric(int32_t _id, ConditionType_t _type, int32_t _ticks);
	virtual ~ConditionGeneric(){};
	
	virtual bool startCondition(Creature* creature);
	virtual bool executeCondition(Creature* creature, int32_t interval);
	virtual void endCondition(Creature* creature, EndCondition_t reason);
	virtual void addCondition(Creature* creature, const Condition* condition);
	virtual uint8_t getIcons() const;
	//virtual bool stackable() const {return false;};
	
	virtual ConditionGeneric* clone()  const { return new ConditionGeneric(*this); }
};

class ConditionManaShield : public ConditionGeneric
{
public:
	ConditionManaShield(int32_t _id, ConditionType_t _type, int32_t _ticks) : ConditionGeneric(_id, _type, _ticks) {};
	virtual ~ConditionManaShield(){};
	//virtual bool stackable() const {return true;};

	virtual ConditionManaShield* clone()  const { return new ConditionManaShield(*this); }
};

class ConditionRegeneration : public ConditionGeneric
{
public:
	ConditionRegeneration(int32_t _id, ConditionType_t _type, int32_t _ticks);
	virtual ~ConditionRegeneration(){};
	virtual void addCondition(Creature* creature, const Condition* addCondition);
	virtual bool executeCondition(Creature* creature, int32_t interval);
	//virtual bool stackable() const {return true;};

	virtual ConditionRegeneration* clone()  const { return new ConditionRegeneration(*this); }

	virtual bool setParam(ConditionParam_t param, int32_t value);

protected:
	uint32_t internalHealthTicks;
	uint32_t internalManaTicks;

	uint32_t healthTicks;
	uint32_t manaTicks;

	uint32_t healthGain;
	uint32_t manaGain;
};

class ConditionInvisible: public ConditionGeneric
{
public:
	ConditionInvisible(int32_t _id, ConditionType_t _type, int32_t _ticks);
	virtual ~ConditionInvisible(){};

	virtual bool startCondition(Creature* creature);
	virtual void endCondition(Creature* creature, EndCondition_t reason);
	//virtual bool stackable() const {return true;};

	virtual ConditionInvisible* clone()  const { return new ConditionInvisible(*this); }
};

class ConditionDamage: public Condition
{
public:
	ConditionDamage(int32_t _id, ConditionType_t _type);
	virtual ~ConditionDamage(){};
	
	virtual bool startCondition(Creature* creature);
	virtual bool executeCondition(Creature* creature, int32_t interval);
	virtual void endCondition(Creature* creature, EndCondition_t reason);
	virtual void addCondition(Creature* creature, const Condition* condition);
	virtual uint8_t getIcons() const;
	//virtual bool stackable() const {return false;};

	virtual ConditionDamage* clone()  const { return new ConditionDamage(*this); }

	virtual bool setParam(ConditionParam_t param, int32_t value);

	void addDamage(uint32_t rounds, uint32_t time, int32_t value);

protected:
	bool delayed;
	uint32_t owner;

	struct DamageInfo{
		int32_t timeLeft;
		int32_t damage;
		int32_t interval;
	};

	std::list<DamageInfo> damageList;

	bool getNextDamage(int32_t& damage);
	bool doDamage(Creature* creature, int32_t damage);
};

class ConditionSpeed: public Condition
{
public:
	ConditionSpeed(int32_t _id, ConditionType_t _type, int32_t _ticks, int32_t changeSpeed);
	virtual ~ConditionSpeed(){};
	
	virtual bool startCondition(Creature* creature);
	virtual bool executeCondition(Creature* creature, int32_t interval);
	virtual void endCondition(Creature* creature, EndCondition_t reason);	
	virtual void addCondition(Creature* creature, const Condition* condition);	
	virtual uint8_t getIcons() const;	
	//virtual bool stackable() const {return false;};

	virtual ConditionSpeed* clone()  const { return new ConditionSpeed(*this); }

	virtual bool setParam(ConditionParam_t param, int32_t value);
	
protected:
	int32_t speedDelta;
};

class ConditionOutfit: public Condition
{
public:
	ConditionOutfit(int32_t _id, ConditionType_t _type, int32_t _ticks);
	virtual ~ConditionOutfit(){};
	
	virtual bool startCondition(Creature* creature);
	virtual bool executeCondition(Creature* creature, int32_t interval);
	virtual void endCondition(Creature* creature, EndCondition_t reason);	
	virtual void addCondition(Creature* creature, const Condition* condition);	
	virtual uint8_t getIcons() const;
	//virtual bool stackable() const {return false;};

	virtual ConditionOutfit* clone()  const { return new ConditionOutfit(*this); }

	void addOutfit(Outfit_t outfit);

protected:
	std::vector<Outfit_t> outfits;

	void changeOutfit(Creature* creature, int32_t index = -1);
};

class ConditionLight: public Condition
{
public:
	ConditionLight(int32_t _id, int32_t _ticks, int32_t _lightlevel, int32_t _lightcolor);
	virtual ~ConditionLight(){};
	
	virtual bool startCondition(Creature* creature);
	virtual bool executeCondition(Creature* creature, int32_t interval);
	virtual void endCondition(Creature* creature, EndCondition_t reason);	
	virtual void addCondition(Creature* creature, const Condition* condition);	
	virtual uint8_t getIcons() const;
	//virtual bool stackable() const {return false;};

	virtual ConditionLight* clone()  const { return new ConditionLight(*this); }
};


#endif
