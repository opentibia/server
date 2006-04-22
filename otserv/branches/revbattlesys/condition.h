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
	CONDITION_NONE,
	CONDITION_POISON,		   //Damage
	CONDITION_FIRE,			   //Damage
	CONDITION_ENERGY,		   //Damage
	CONDITION_HASTE,		   //Speed
	CONDITION_PARALYZE,		 //Speed
	CONDITION_OUTFIT,		   //Outfit
	CONDITION_LIGHT,		   //Light   -- Player only
	CONDITION_MANASHIELD,  //Generic -- Player only
	//CONDITION_PZLOCK,		   //Generic -- Player only
	CONDITION_INFIGHT,		 //Generic -- Player only
	CONDITION_DRUNK,		   //Generic -- Player only
	CONDITION_EXHAUSTED,	 //Generic
	CONDITION_FOOD,			   //Generic -- Player only
};

enum EndCondition_t {
	REASON_ENDTICKS,
	REASON_ABORT,
};

class Condition{
public:
	Condition();
	virtual ~Condition(){};
	
	virtual bool startCondition(Creature* creature) = 0;
	virtual void executeCondition(Creature* creature, int32_t interval) = 0;
	virtual void endCondition(Creature* creature, EndCondition_t reason) = 0;	
	virtual void addCondition(Creature* creature, const Condition* condition) = 0;
	virtual uint8_t getIcons() const = 0;

	virtual Condition* clone() const = 0;

	ConditionType_t getType() const { return conditionType;}
	int32_t getTicks() const { return ticks; }
	void setTicks(int32_t newTicks) { ticks = newTicks; }

	static Condition* createCondition(ConditionType_t _type, int32_t ticks, int32_t param);

protected:
	int32_t ticks;
	ConditionType_t conditionType;
};

class ConditionGeneric: public Condition
{
public:
	ConditionGeneric(ConditionType_t _type, int32_t _ticks);
	virtual ~ConditionGeneric(){};
	
	virtual bool startCondition(Creature* creature);
	virtual void executeCondition(Creature* creature, int32_t interval);
	virtual void endCondition(Creature* creature, EndCondition_t reason);
	virtual void addCondition(Creature* creature, const Condition* condition);
	virtual uint8_t getIcons() const;
	
	virtual ConditionGeneric* clone()  const { return new ConditionGeneric(*this); }
};

class ConditionDamage: public Condition
{
public:
	ConditionDamage(ConditionType_t _type, int32_t _ticks, int32_t _damagetype);
	virtual ~ConditionDamage(){};
	
	virtual bool startCondition(Creature* creature);
	virtual void executeCondition(Creature* creature, int32_t interval);
	virtual void endCondition(Creature* creature, EndCondition_t reason);	
	virtual void addCondition(Creature* creature, const Condition* condition);	
	virtual uint8_t getIcons() const;	

	virtual ConditionDamage* clone()  const { return new ConditionDamage(*this); }

protected:
	uint32_t owner;
	typedef std::pair<int32_t, int32_t> DamagePair;
	std::list<DamagePair> damageList;

	bool getNextDamage(int32_t& damage);
	void doDamage(Creature* creature, int32_t damage);
};

class ConditionSpeed: public Condition
{
public:
	ConditionSpeed(ConditionType_t _type, int32_t _ticks, int32_t changeSpeed);
	virtual ~ConditionSpeed(){};
	
	virtual bool startCondition(Creature* creature);
	virtual void executeCondition(Creature* creature, int32_t interval);
	virtual void endCondition(Creature* creature, EndCondition_t reason);	
	virtual void addCondition(Creature* creature, const Condition* condition);	
	virtual uint8_t getIcons() const;

	virtual ConditionSpeed* clone()  const { return new ConditionSpeed(*this); }
	
protected:
	int32_t speedDelta;
};

class ConditionOutfit: public Condition
{
public:
	ConditionOutfit(int32_t _ticks, int32_t new_outfit);
	virtual ~ConditionOutfit(){};
	
	virtual bool startCondition(Creature* creature);
	virtual void executeCondition(Creature* creature, int32_t interval);
	virtual void endCondition(Creature* creature, EndCondition_t reason);	
	virtual void addCondition(Creature* creature, const Condition* condition);	
	virtual uint8_t getIcons() const;

	virtual ConditionOutfit* clone()  const { return new ConditionOutfit(*this); }
};

class ConditionLight: public Condition
{
public:
	ConditionLight(int32_t _ticks, int32_t _lightlevel, int32_t _lightcolor);
	virtual ~ConditionLight(){};
	
	virtual bool startCondition(Creature* creature);
	virtual void executeCondition(Creature* creature, int32_t interval);
	virtual void endCondition(Creature* creature, EndCondition_t reason);	
	virtual void addCondition(Creature* creature, const Condition* condition);	
	virtual uint8_t getIcons() const;

	virtual ConditionLight* clone()  const { return new ConditionLight(*this); }
};


#endif
