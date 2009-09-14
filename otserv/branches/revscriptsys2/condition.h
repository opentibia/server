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

#include "condition_attributes.h"
#include "outfit.h"

struct LightInfo{
	uint32_t level;
	uint32_t color;
	LightInfo(){
		level = 0;
		color = 0;
	};
	LightInfo(uint32_t _level, uint32_t _color){
		level = _level;
		color = _color;
	};
};

struct IntervalInfo{
	int32_t timeLeft;
	int32_t value;
	int32_t interval;
};

class Condition{
public:
	Condition(ConditionID _id, ConditionType _type, int32_t _ticks);
	virtual ~Condition(){};

	virtual bool startCondition(Creature* creature);
	virtual bool executeCondition(Creature* creature, int32_t interval);
	virtual void endCondition(Creature* creature, ConditionEnd reason) = 0;
	virtual void addCondition(Creature* creature, const Condition* condition) = 0;
	virtual uint16_t getIcons() const;
	ConditionID getId() const {return id;}
	uint32_t getSubId() const {return subId;}

	virtual Condition* clone() const = 0;

	ConditionType getType() const { return conditionType;}
	int64_t getEndTime() const {return ticks == -1? 0 : endTime;}
	int32_t getTicks() const { return ticks; }
	void setTicks(int32_t newTicks);

	static Condition* createCondition(ConditionID _id, ConditionType _type, int32_t ticks, int32_t param = 0);
	static Condition* createCondition(PropStream& propStream);

	virtual bool setParam(ConditionParam param, int32_t value);

	//serialization
	bool unserialize(PropStream& propStream);
	virtual bool serialize(PropWriteStream& propWriteStream);
	virtual bool unserializeProp(ConditionAttribute attr, PropStream& propStream);

	bool isPersistent() const;

protected:
	ConditionID id;
	uint32_t subId;
	int32_t ticks;
	int64_t endTime;
	ConditionType conditionType;
	bool isBuff;

	virtual bool updateCondition(const Condition* addCondition);
};

class ConditionGeneric: public Condition
{
public:
	ConditionGeneric(ConditionID _id, ConditionType _type, int32_t _ticks);
	virtual ~ConditionGeneric(){};

	virtual bool startCondition(Creature* creature);
	virtual bool executeCondition(Creature* creature, int32_t interval);
	virtual void endCondition(Creature* creature, ConditionEnd reason);
	virtual void addCondition(Creature* creature, const Condition* condition);
	virtual uint16_t getIcons() const;

	virtual ConditionGeneric* clone()  const { return new ConditionGeneric(*this); }
};

class ConditionManaShield : public ConditionGeneric
{
public:
	ConditionManaShield(ConditionID _id, ConditionType _type, int32_t _ticks) : ConditionGeneric(_id, _type, _ticks) {};
	virtual ~ConditionManaShield(){};

	virtual ConditionManaShield* clone()  const { return new ConditionManaShield(*this); }
};

class ConditionAttributes : public ConditionGeneric
{
public:
	ConditionAttributes(ConditionID _id, ConditionType _type, int32_t _ticks);
	virtual ~ConditionAttributes(){};

	virtual bool startCondition(Creature* creature);
	virtual bool executeCondition(Creature* creature, int32_t interval);
	virtual void endCondition(Creature* creature, ConditionEnd reason);
	virtual void addCondition(Creature* creature, const Condition* condition);

	virtual ConditionAttributes* clone()  const { return new ConditionAttributes(*this); }

	virtual bool setParam(ConditionParam param, int32_t value);

	//serialization
	virtual bool serialize(PropWriteStream& propWriteStream);
	virtual bool unserializeProp(ConditionAttribute attr, PropStream& propStream);

protected:
	int32_t skills[SkillType::size];
	int32_t skillsPercent[SkillType::size];
	int32_t stats[PlayerStatType::size];
	int32_t statsPercent[PlayerStatType::size];
	int32_t currentSkill;
	int32_t currentStat;

	void updatePercentStats(Player* player);
	void updateStats(Player* player);
	void updatePercentSkills(Player* player);
	void updateSkills(Player* player);
};

class ConditionRegeneration : public ConditionGeneric
{
public:
	ConditionRegeneration(ConditionID _id, ConditionType _type, int32_t _ticks);
	virtual ~ConditionRegeneration(){};
	virtual void addCondition(Creature* creature, const Condition* addCondition);
	virtual bool executeCondition(Creature* creature, int32_t interval);

	virtual ConditionRegeneration* clone()  const { return new ConditionRegeneration(*this); }

	virtual bool setParam(ConditionParam param, int32_t value);

	//serialization
	virtual bool serialize(PropWriteStream& propWriteStream);
	virtual bool unserializeProp(ConditionAttribute attr, PropStream& propStream);

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
	ConditionSoul(ConditionID _id, ConditionType _type, int32_t _ticks);
	virtual ~ConditionSoul(){};
	virtual void addCondition(Creature* creature, const Condition* addCondition);
	virtual bool executeCondition(Creature* creature, int32_t interval);

	virtual ConditionSoul* clone()  const { return new ConditionSoul(*this); }

	virtual bool setParam(ConditionParam param, int32_t value);

	//serialization
	virtual bool serialize(PropWriteStream& propWriteStream);
	virtual bool unserializeProp(ConditionAttribute attr, PropStream& propStream);

protected:
	uint32_t internalSoulTicks;
	uint32_t soulTicks;
	uint32_t soulGain;
};

class ConditionInvisible: public ConditionGeneric
{
public:
	ConditionInvisible(ConditionID _id, ConditionType _type, int32_t _ticks);
	virtual ~ConditionInvisible(){};

	virtual bool startCondition(Creature* creature);
	virtual void endCondition(Creature* creature, ConditionEnd reason);

	virtual ConditionInvisible* clone()  const { return new ConditionInvisible(*this); }
};

class ConditionDamage: public Condition
{
public:
	ConditionDamage(ConditionID _id, ConditionType _type);
	virtual ~ConditionDamage(){};

	static void generateDamageList(int32_t amount, int32_t start, std::list<int32_t>& list);

	virtual bool startCondition(Creature* creature);
	virtual bool executeCondition(Creature* creature, int32_t interval);
	virtual void endCondition(Creature* creature, ConditionEnd reason);
	virtual void addCondition(Creature* creature, const Condition* condition);
	virtual uint16_t getIcons() const;

	virtual ConditionDamage* clone()  const { return new ConditionDamage(*this); }

	virtual bool setParam(ConditionParam param, int32_t value);

	bool addDamage(int32_t rounds, int32_t time, int32_t value);
	bool doForceUpdate() const { return forceUpdate;}
	int32_t getTotalDamage() const;

	//serialization
	virtual bool serialize(PropWriteStream& propWriteStream);
	virtual bool unserializeProp(ConditionAttribute attr, PropStream& propStream);

protected:
	int32_t maxDamage;
	int32_t minDamage;
	int32_t startDamage;
	int32_t periodDamage;
	int32_t periodDamageTick;
	int32_t tickInterval;

	bool forceUpdate;
	bool delayed;
	uint32_t owner;

	bool init();

	typedef std::list<IntervalInfo> DamageList;
	DamageList damageList;

	bool getNextDamage(int32_t& damage);
	bool doDamage(Creature* creature, int32_t damage);
	bool updateCondition(const ConditionDamage* addCondition);
};

class ConditionSpeed: public Condition
{
public:
	ConditionSpeed(ConditionID _id, ConditionType _type, int32_t _ticks, int32_t changeSpeed);
	virtual ~ConditionSpeed(){};

	virtual bool startCondition(Creature* creature);
	virtual bool executeCondition(Creature* creature, int32_t interval);
	virtual void endCondition(Creature* creature, ConditionEnd reason);
	virtual void addCondition(Creature* creature, const Condition* condition);
	virtual uint16_t getIcons() const;

	virtual ConditionSpeed* clone()  const { return new ConditionSpeed(*this); }

	virtual bool setParam(ConditionParam param, int32_t value);

	void setFormulaVars(float _mina, float _minb, float _maxa, float _maxb);

	//serialization
	virtual bool serialize(PropWriteStream& propWriteStream);
	virtual bool unserializeProp(ConditionAttribute attr, PropStream& propStream);

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
	ConditionOutfit(ConditionID _id, ConditionType _type, int32_t _ticks);
	virtual ~ConditionOutfit(){};

	virtual bool startCondition(Creature* creature);
	virtual bool executeCondition(Creature* creature, int32_t interval);
	virtual void endCondition(Creature* creature, ConditionEnd reason);
	virtual void addCondition(Creature* creature, const Condition* condition);

	virtual ConditionOutfit* clone()  const { return new ConditionOutfit(*this); }

	void addOutfit(OutfitType outfit);

	//serialization
	virtual bool serialize(PropWriteStream& propWriteStream);
	virtual bool unserializeProp(ConditionAttribute attr, PropStream& propStream);

protected:
	std::vector<OutfitType> outfits;

	void changeOutfit(Creature* creature, int32_t index = -1);
};

class ConditionLight: public Condition
{
public:
	ConditionLight(ConditionID _id, ConditionType _type, int32_t _ticks, int32_t _lightlevel, int32_t _lightcolor);
	virtual ~ConditionLight(){};

	virtual bool startCondition(Creature* creature);
	virtual bool executeCondition(Creature* creature, int32_t interval);
	virtual void endCondition(Creature* creature, ConditionEnd reason);
	virtual void addCondition(Creature* creature, const Condition* addCondition);

	virtual ConditionLight* clone()  const { return new ConditionLight(*this); }

	virtual bool setParam(ConditionParam param, int32_t value);

	//serialization
	virtual bool serialize(PropWriteStream& propWriteStream);
	virtual bool unserializeProp(ConditionAttribute attr, PropStream& propStream);

protected:
	LightInfo lightInfo;
	uint32_t internalLightTicks;
	uint32_t lightChangeInterval;
};

#endif
