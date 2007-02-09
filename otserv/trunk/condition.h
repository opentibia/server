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
#include "fileloader.h"
#include "enums.h"

#include <list>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

class Creature;
class Player;
class PropStream;

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
	CONDITION_REGENERATION	= 8192,
	CONDITION_SOUL          = 16384,
	CONDITION_DROWN         = 32768,
	CONDITION_MUTED         = 65536
};

enum ConditionEnd_t{
	CONDITIONEND_CLEANUP,
	CONDITIONEND_DIE,
	CONDITIONEND_TICKS,
	CONDITIONEND_ABORT
};

enum ConditionAttr_t{
	CONDITIONATTR_TYPE = 1,
	CONDITIONATTR_ID = 2,
	CONDITIONATTR_TICKS = 3,
	CONDITIONATTR_HEALTHTICKS = 4,
	CONDITIONATTR_HEALTHGAIN = 5,
	CONDITIONATTR_MANATICKS = 6,
	CONDITIONATTR_MANAGAIN = 7,
	CONDITIONATTR_DELAYED = 8,
	CONDITIONATTR_OWNER = 9,
	CONDITIONATTR_INTERVALDATA = 10,
	CONDITIONATTR_SPEEDDELTA = 11,
	CONDITIONATTR_FORMULA_MINA = 12,
	CONDITIONATTR_FORMULA_MINB = 13,
	CONDITIONATTR_FORMULA_MAXA = 14,
	CONDITIONATTR_FORMULA_MAXB = 15,
	CONDITIONATTR_LIGHTCOLOR = 16,
	CONDITIONATTR_LIGHTLEVEL = 17,
	CONDITIONATTR_LIGHTTICKS = 18,
	CONDITIONATTR_LIGHTINTERVAL = 19,
	CONDITIONATTR_SOULTICKS = 20,
	CONDITIONATTR_SOULGAIN = 21,

	//reserved for serialization
	CONDITIONATTR_END      = 254
};

struct IntervalInfo{
	int32_t timeLeft;
	int32_t value;
	int32_t interval;
};

class Condition{
public:
	Condition(ConditionId_t _id, ConditionType_t _type, int32_t _ticks);
	virtual ~Condition(){};
	
	virtual bool startCondition(Creature* creature) = 0;
	virtual bool executeCondition(Creature* creature, int32_t interval);
	virtual void endCondition(Creature* creature, ConditionEnd_t reason) = 0;
	virtual void addCondition(Creature* creature, const Condition* condition) = 0;
	virtual uint32_t getIcons() const = 0;
	virtual ConditionId_t getId() const {return id;}

	virtual Condition* clone() const = 0;

	ConditionType_t getType() const { return conditionType;}
	int32_t getTicks() const { return ticks; }
	void setTicks(int32_t newTicks) { ticks = newTicks; }

	static Condition* createCondition(ConditionId_t _id, ConditionType_t _type, int32_t ticks, int32_t param);
	static Condition* createCondition(PropStream& propStream);

	virtual bool setParam(ConditionParam_t param, int32_t value);

	//serialization
	virtual xmlNodePtr serialize();
	virtual bool unserialize(xmlNodePtr p);

	bool unserialize(PropStream& propStream);
	virtual bool serialize(PropWriteStream& propWriteStream);
	virtual bool unserializeProp(ConditionAttr_t attr, PropStream& propStream);

	bool isPersistent() const;

protected:
	ConditionId_t id;
	int32_t ticks;
	ConditionType_t conditionType;
};

class ConditionGeneric: public Condition
{
public:
	ConditionGeneric(ConditionId_t _id, ConditionType_t _type, int32_t _ticks);
	virtual ~ConditionGeneric(){};
	
	virtual bool startCondition(Creature* creature);
	virtual bool executeCondition(Creature* creature, int32_t interval);
	virtual void endCondition(Creature* creature, ConditionEnd_t reason);
	virtual void addCondition(Creature* creature, const Condition* condition);
	virtual uint32_t getIcons() const;
	
	virtual ConditionGeneric* clone()  const { return new ConditionGeneric(*this); }
};

class ConditionManaShield : public ConditionGeneric
{
public:
	ConditionManaShield(ConditionId_t _id, ConditionType_t _type, int32_t _ticks) : ConditionGeneric(_id, _type, _ticks) {};
	virtual ~ConditionManaShield(){};

	virtual ConditionManaShield* clone()  const { return new ConditionManaShield(*this); }
};

class ConditionRegeneration : public ConditionGeneric
{
public:
	ConditionRegeneration(ConditionId_t _id, ConditionType_t _type, int32_t _ticks);
	virtual ~ConditionRegeneration(){};
	virtual void addCondition(Creature* creature, const Condition* addCondition);
	virtual bool executeCondition(Creature* creature, int32_t interval);

	virtual ConditionRegeneration* clone()  const { return new ConditionRegeneration(*this); }

	virtual bool setParam(ConditionParam_t param, int32_t value);

	//serialization
	virtual xmlNodePtr serialize();
	virtual bool unserialize(xmlNodePtr p);

	virtual bool serialize(PropWriteStream& propWriteStream);
	virtual bool unserializeProp(ConditionAttr_t attr, PropStream& propStream);

protected:
	uint32_t internalHealthTicks;
	uint32_t internalManaTicks;

	uint32_t healthTicks;
	uint32_t manaTicks;
	uint32_t healthGain;
	uint32_t manaGain;
};

class ConditionSoul : public ConditionGeneric
{
public:
	ConditionSoul(ConditionId_t _id, ConditionType_t _type, int32_t _ticks);
	virtual ~ConditionSoul(){};
	virtual void addCondition(Creature* creature, const Condition* addCondition);
	virtual bool executeCondition(Creature* creature, int32_t interval);

	virtual ConditionSoul* clone()  const { return new ConditionSoul(*this); }

	virtual bool setParam(ConditionParam_t param, int32_t value);

	//serialization
	virtual xmlNodePtr serialize();
	virtual bool unserialize(xmlNodePtr p);

	virtual bool serialize(PropWriteStream& propWriteStream);
	virtual bool unserializeProp(ConditionAttr_t attr, PropStream& propStream);

protected:
	uint32_t internalSoulTicks;
	uint32_t soulTicks;
	uint32_t soulGain;
};

class ConditionInvisible: public ConditionGeneric
{
public:
	ConditionInvisible(ConditionId_t _id, ConditionType_t _type, int32_t _ticks);
	virtual ~ConditionInvisible(){};

	virtual bool startCondition(Creature* creature);
	virtual void endCondition(Creature* creature, ConditionEnd_t reason);

	virtual ConditionInvisible* clone()  const { return new ConditionInvisible(*this); }
};

class ConditionDamage: public Condition
{
public:
	ConditionDamage(ConditionId_t _id, ConditionType_t _type);
	virtual ~ConditionDamage(){};
	
	static void generateDamageList(int32_t amount, int32_t start, std::list<int32_t>& list);

	virtual bool startCondition(Creature* creature);
	virtual bool executeCondition(Creature* creature, int32_t interval);
	virtual void endCondition(Creature* creature, ConditionEnd_t reason);
	virtual void addCondition(Creature* creature, const Condition* condition);
	virtual uint32_t getIcons() const;

	virtual ConditionDamage* clone()  const { return new ConditionDamage(*this); }

	virtual bool setParam(ConditionParam_t param, int32_t value);

	void addDamage(uint32_t rounds, uint32_t time, int32_t value);

	//serialization
	virtual xmlNodePtr serialize();
	virtual bool unserialize(xmlNodePtr p);

	virtual bool serialize(PropWriteStream& propWriteStream);
	virtual bool unserializeProp(ConditionAttr_t attr, PropStream& propStream);

protected:
	int32_t maxDamage;
	int32_t minDamage;
	int32_t startDamage;
	int32_t tickInterval;

	bool delayed;
	uint32_t owner;

	bool init();
	int32_t getTotalDamage() const;

	typedef std::list<IntervalInfo> DamageList;
	DamageList damageList;

	bool getNextDamage(int32_t& damage);
	bool doDamage(Creature* creature, int32_t damage);
};

class ConditionSpeed: public Condition
{
public:
	ConditionSpeed(ConditionId_t _id, ConditionType_t _type, int32_t _ticks, int32_t changeSpeed);
	virtual ~ConditionSpeed(){};
	
	virtual bool startCondition(Creature* creature);
	virtual bool executeCondition(Creature* creature, int32_t interval);
	virtual void endCondition(Creature* creature, ConditionEnd_t reason);
	virtual void addCondition(Creature* creature, const Condition* condition);
	virtual uint32_t getIcons() const;	

	virtual ConditionSpeed* clone()  const { return new ConditionSpeed(*this); }

	virtual bool setParam(ConditionParam_t param, int32_t value);
	
	void setFormulaVars(float _mina, float _minb, float _maxa, float _maxb);

	//serialization
	virtual xmlNodePtr serialize();
	virtual bool unserialize(xmlNodePtr p);

	virtual bool serialize(PropWriteStream& propWriteStream);
	virtual bool unserializeProp(ConditionAttr_t attr, PropStream& propStream);

protected:	
	void getFormulaValues(int32_t var, int32_t& min, int32_t& max) const;
	
	int32_t speedDelta;
	
	//formula variables
	float mina;
	float minb;
	float maxa;
	float maxb;
};

class ConditionOutfit: public Condition
{
public:
	ConditionOutfit(ConditionId_t _id, ConditionType_t _type, int32_t _ticks);
	virtual ~ConditionOutfit(){};
	
	virtual bool startCondition(Creature* creature);
	virtual bool executeCondition(Creature* creature, int32_t interval);
	virtual void endCondition(Creature* creature, ConditionEnd_t reason);
	virtual void addCondition(Creature* creature, const Condition* condition);
	virtual uint32_t getIcons() const;

	virtual ConditionOutfit* clone()  const { return new ConditionOutfit(*this); }

	void addOutfit(Outfit_t outfit);

	//serialization
	virtual xmlNodePtr serialize();
	virtual bool unserialize(xmlNodePtr p);

	virtual bool serialize(PropWriteStream& propWriteStream);
	virtual bool unserializeProp(ConditionAttr_t attr, PropStream& propStream);

protected:
	std::vector<Outfit_t> outfits;

	void changeOutfit(Creature* creature, int32_t index = -1);
};

class ConditionLight: public Condition
{
public:
	ConditionLight(ConditionId_t _id, ConditionType_t _type, int32_t _ticks, int32_t _lightlevel, int32_t _lightcolor);
	virtual ~ConditionLight(){};
	
	virtual bool startCondition(Creature* creature);
	virtual bool executeCondition(Creature* creature, int32_t interval);
	virtual void endCondition(Creature* creature, ConditionEnd_t reason);
	virtual void addCondition(Creature* creature, const Condition* addCondition);
	virtual uint32_t getIcons() const;

	virtual ConditionLight* clone()  const { return new ConditionLight(*this); }
	
	virtual bool setParam(ConditionParam_t param, int32_t value);

	//serialization
	virtual xmlNodePtr serialize();
	virtual bool unserialize(xmlNodePtr p);

	virtual bool serialize(PropWriteStream& propWriteStream);
	virtual bool unserializeProp(ConditionAttr_t attr, PropStream& propStream);

protected:
	LightInfo lightInfo;
	uint32_t internalLightTicks;
	uint32_t lightChangeInterval;
};

#endif
