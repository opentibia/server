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
#include <vector>

class Creature;
class Player;
class PropStream;

enum ConditionType_t
{
	CONDITION_NONE           = 0,
	CONDITION_POISON         = 1 << 0,
	CONDITION_FIRE           = 1 << 1,
	CONDITION_ENERGY         = 1 << 2,
	CONDITION_LIFEDRAIN      = 1 << 3,
	CONDITION_HASTE          = 1 << 4,
	CONDITION_PARALYZE	     = 1 << 5,
	CONDITION_OUTFIT         = 1 << 6,
	CONDITION_INVISIBLE      = 1 << 7,
	CONDITION_LIGHT          = 1 << 8,
	CONDITION_MANASHIELD     = 1 << 9,
	CONDITION_INFIGHT        = 1 << 10,
	CONDITION_DRUNK          = 1 << 11,
	CONDITION_EXHAUST_YELL   = 1 << 12,
	CONDITION_REGENERATION   = 1 << 13,
	CONDITION_SOUL           = 1 << 14,
	CONDITION_DROWN          = 1 << 15,
	CONDITION_MUTED          = 1 << 16,
	CONDITION_ATTRIBUTES     = 1 << 17,
	CONDITION_FREEZING       = 1 << 18,
	CONDITION_DAZZLED        = 1 << 19,
	CONDITION_CURSED         = 1 << 20,
	CONDITION_EXHAUST_COMBAT = 1 << 21,
	CONDITION_EXHAUST_HEAL   = 1 << 22,
	CONDITION_PACIFIED       = 1 << 23, // Cannot attack anything
	CONDITION_HUNTING        = 1 << 24, // Killing monsters
	CONDITION_TRADE_MUTED    = 1 << 25, // Cannot talk on trade channels
	CONDITION_EXHAUST_OTHERS = 1 << 26
};

enum ConditionEnd_t
{
	CONDITIONEND_CLEANUP,
	CONDITIONEND_DIE,
	CONDITIONEND_TICKS,
	CONDITIONEND_ABORT
};

enum ConditionAttr_t
{
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
	CONDITIONATTR_SKILLS = 22,
	CONDITIONATTR_STATS = 23,
	CONDITIONATTR_OUTFIT = 24,
	CONDITIONATTR_PERIODDAMAGE = 25,
	CONDITIONATTR_SKILLSPERCENT = 26,
	CONDITIONATTR_ISBUFF = 27,
	CONDITIONATTR_SUBID = 28,

	//reserved for serialization
	CONDITIONATTR_END      = 254
};

struct IntervalInfo
{
	int32_t timeLeft;
	int32_t value;
	int32_t interval;
};

class Condition
{
public:
	Condition(const ConditionId_t& _id, const ConditionType_t& _type, const int32_t& _ticks);
	virtual ~Condition();

	virtual bool startCondition(Creature* creature);
	virtual bool executeCondition(Creature* creature, const int32_t& interval);
	virtual void endCondition(Creature* creature, const ConditionEnd_t& reason) = 0;
	virtual void addCondition(Creature* creature, const Condition* condition) = 0;
	virtual uint16_t getIcons() const;
	const ConditionId_t& getId() const;
	const uint32_t& getSubId() const;

	virtual Condition* clone() const = 0;

	const ConditionType_t& getType() const;
	const int64_t& getEndTime() const;
	const int32_t& getTicks() const;
	void setTicks(const int32_t& newTicks);

	static bool canBeAggressive(const ConditionType_t& type);
	static Condition* createCondition(const ConditionId_t& _id, const ConditionType_t& _type,
	                                  const int32_t& ticks, const int32_t& param = 0);
	static Condition* createCondition(PropStream& propStream);

	virtual bool setParam(const ConditionParam_t& param, const int32_t& value);

	//serialization
	bool unserialize(PropStream& propStream);
	virtual bool serialize(PropWriteStream& propWriteStream);
	virtual bool unserializeProp(const ConditionAttr_t& attr, PropStream& propStream);

	bool isPersistent() const;

protected:
	ConditionId_t id;
	uint32_t subId;
	int32_t ticks;
	int64_t endTime;
	ConditionType_t conditionType;
	bool isBuff;

	virtual bool updateCondition(const Condition* addCondition);
};

class ConditionGeneric: public Condition
{
public:
	ConditionGeneric(const ConditionId_t& _id, const ConditionType_t& _type, const int32_t& _ticks);
	virtual ~ConditionGeneric();

	virtual void endCondition(Creature* creature, const ConditionEnd_t& reason);
	virtual void addCondition(Creature* creature, const Condition* condition);
	virtual uint16_t getIcons() const;

	virtual ConditionGeneric* clone() const;
};

class ConditionManaShield : public ConditionGeneric
{
public:
	ConditionManaShield(const ConditionId_t& _id, const ConditionType_t& _type, const int32_t& _ticks);
	virtual ~ConditionManaShield();

	virtual ConditionManaShield* clone() const;
};

class ConditionAttributes : public ConditionGeneric
{
public:
	ConditionAttributes(const ConditionId_t& _id, const ConditionType_t& _type, const int32_t& _ticks);
	virtual ~ConditionAttributes() {};

	virtual bool startCondition(Creature* creature);
	virtual bool executeCondition(Creature* creature, const int32_t& interval);
	virtual void endCondition(Creature* creature, const ConditionEnd_t& reason);
	virtual void addCondition(Creature* creature, const Condition* condition);

	virtual ConditionAttributes* clone() const;

	virtual bool setParam(const ConditionParam_t& param, const int32_t& value);

	//serialization
	virtual bool serialize(PropWriteStream& propWriteStream);
	virtual bool unserializeProp(const ConditionAttr_t& attr, PropStream& propStream);

protected:
	int32_t skills[SKILL_LAST + 1];
	int32_t skillsPercent[SKILL_LAST + 1];
	int32_t stats[STAT_LAST + 1];
	int32_t statsPercent[STAT_LAST + 1];
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
	ConditionRegeneration(const ConditionId_t& _id, const ConditionType_t& _type, const int32_t& _ticks);
	virtual ~ConditionRegeneration();
	virtual void addCondition(Creature* creature, const Condition* addCondition);
	virtual bool executeCondition(Creature* creature, const int32_t& interval);

	virtual ConditionRegeneration* clone() const;

	virtual bool setParam(const ConditionParam_t& param, const int32_t& value);

	//serialization
	virtual bool serialize(PropWriteStream& propWriteStream);
	virtual bool unserializeProp(const ConditionAttr_t& attr, PropStream& propStream);

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
	ConditionSoul(const ConditionId_t& _id, const ConditionType_t& _type, const int32_t& _ticks);
	virtual ~ConditionSoul();
	virtual void addCondition(Creature* creature, const Condition* addCondition);
	virtual bool executeCondition(Creature* creature, const int32_t& interval);

	virtual ConditionSoul* clone() const;

	virtual bool setParam(const ConditionParam_t& param, const int32_t& value);

	//serialization
	virtual bool serialize(PropWriteStream& propWriteStream);
	virtual bool unserializeProp(const ConditionAttr_t& attr, PropStream& propStream);

protected:
	uint32_t internalSoulTicks;
	uint32_t soulTicks;
	uint32_t soulGain;
};

class ConditionInvisible: public ConditionGeneric
{
public:
	ConditionInvisible(const ConditionId_t& _id, const ConditionType_t& _type, const int32_t& _ticks);
	virtual ~ConditionInvisible();

	virtual bool startCondition(Creature* creature);
	virtual void endCondition(Creature* creature, const ConditionEnd_t& reason);

	virtual ConditionInvisible* clone() const;
};

class ConditionDamage: public Condition
{
public:
	ConditionDamage(const ConditionId_t& _id, const ConditionType_t& _type);
	virtual ~ConditionDamage();

	static void generateDamageList(const int32_t& amount, const int32_t& start, std::list<int32_t>& list);

	virtual bool startCondition(Creature* creature);
	virtual bool executeCondition(Creature* creature, const int32_t& interval);
	virtual void endCondition(Creature* creature, const ConditionEnd_t& reason);
	virtual void addCondition(Creature* creature, const Condition* condition);
	virtual uint16_t getIcons() const;

	virtual ConditionDamage* clone() const;

	virtual bool setParam(const ConditionParam_t& param, const int32_t& value);

	bool addDamage(const int32_t& rounds, const int32_t& time, const int32_t& value);
	bool doForceUpdate() const;
	int32_t getTotalDamage() const;

	size_t getLength() const;

	IntervalInfo popBackDamage();
	IntervalInfo popFrontDamage();
	void clearDamageList();

	//serialization
	virtual bool serialize(PropWriteStream& propWriteStream);
	virtual bool unserializeProp(const ConditionAttr_t& attr, PropStream& propStream);

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
	bool doDamage(Creature* creature, const int32_t& damage);
	bool updateCondition(const ConditionDamage* addCondition);
};

class ConditionSpeed: public Condition
{
public:
	ConditionSpeed(const ConditionId_t& _id, const ConditionType_t& _type,
	               const int32_t& _ticks, const int32_t& changeSpeed);
	virtual ~ConditionSpeed();

	virtual bool startCondition(Creature* creature);
	virtual bool executeCondition(Creature* creature, const int32_t& interval);
	virtual void endCondition(Creature* creature, const ConditionEnd_t& reason);
	virtual void addCondition(Creature* creature, const Condition* condition);
	virtual uint16_t getIcons() const;

	virtual ConditionSpeed* clone() const;

	virtual bool setParam(const ConditionParam_t& param, const int32_t& value);

	void setFormulaVars(const float& _mina, const float& _minb, const float& _maxa, const float& _maxb);

	//serialization
	virtual bool serialize(PropWriteStream& propWriteStream);
	virtual bool unserializeProp(const ConditionAttr_t& attr, PropStream& propStream);

protected:
	void getFormulaValues(const int32_t& var, int32_t& min, int32_t& max) const;

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
	ConditionOutfit(const ConditionId_t& _id, const ConditionType_t& _type, const int32_t& _ticks);
	virtual ~ConditionOutfit();

	virtual bool startCondition(Creature* creature);
	virtual bool executeCondition(Creature* creature, const int32_t& interval);
	virtual void endCondition(Creature* creature, const ConditionEnd_t& reason);
	virtual void addCondition(Creature* creature, const Condition* condition);

	virtual ConditionOutfit* clone() const;

	void addOutfit(const Outfit_t& outfit);

	//serialization
	virtual bool serialize(PropWriteStream& propWriteStream);
	virtual bool unserializeProp(const ConditionAttr_t& attr, PropStream& propStream);

protected:
	std::vector<Outfit_t> outfits;

	void changeOutfit(Creature* creature, const int32_t& index = -1);
};

class ConditionLight : public Condition
{
public:
	ConditionLight(const ConditionId_t& _id, const ConditionType_t& _type,
	               const int32_t& _ticks, const int32_t& _lightlevel, const int32_t& _lightcolor);
	virtual ~ConditionLight();

	virtual bool startCondition(Creature* creature);
	virtual bool executeCondition(Creature* creature, const int32_t& interval);
	virtual void endCondition(Creature* creature, const ConditionEnd_t& reason);
	virtual void addCondition(Creature* creature, const Condition* addCondition);

	virtual ConditionLight* clone() const;

	virtual bool setParam(const ConditionParam_t& param, const int32_t& value);

	//serialization
	virtual bool serialize(PropWriteStream& propWriteStream);
	virtual bool unserializeProp(const ConditionAttr_t& attr, PropStream& propStream);

protected:
	LightInfo lightInfo;
	uint32_t internalLightTicks;
	uint32_t lightChangeInterval;
};

#endif
