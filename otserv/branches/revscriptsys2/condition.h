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

#include "classes.h"
#include "combat.h"
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

struct EffectModPeriodicDamage{
	EffectModPeriodicDamage() {}
	EffectModPeriodicDamage(CombatType type, int32_t total, int32_t percent, int32_t value, int32_t rounds) :
		type(type),
		total(total),
		percent(percent),
		value(value),
		rounds(rounds),
		sum(0),
		roundCompleted(0) {}

	CombatType type;
	int32_t total;
	int32_t percent;
	int32_t value;
	int32_t rounds;

	//
	int32_t sum;
	int32_t roundCompleted;
};

struct EffectModPeriodicStamina{
	EffectModPeriodicStamina() {}
	EffectModPeriodicStamina(int32_t value) :
		value(value) {}

	int32_t value;
};

struct EffectModStat{
	EffectModStat() {}
	EffectModStat(PlayerStatType type, int32_t percent, int32_t value) :
		type(type),
		percent(percent),
		value(value),
		delta(0) {}
	PlayerStatType type;
	int32_t percent;
	int32_t value;

	//
	int32_t delta;
};

struct EffectModSkill{
	EffectModSkill() {}
	EffectModSkill(SkillType type, int32_t percent, int32_t value) :
		type(type),
		percent(percent),
		value(value),
		delta(0) {}
	SkillType type;
	int32_t percent;
	int32_t value;

	//
	int32_t delta;
};

struct EffectModSpeed{
	EffectModSpeed() {}
	EffectModSpeed(int32_t percent, int32_t value) :
		percent(percent),
		value(value),
		delta(0) {}

	int32_t percent;
	int32_t value;

	//
	int32_t delta;
};

struct EffectModRegen{
	EffectModRegen() {}
	EffectModRegen(int32_t percent, int32_t value) :
		percent(percent),
		value(value) {}
	EffectModRegen(PlayerStatType type, int32_t percent, int32_t value) :
		type(type),
		percent(percent),
		value(value) {}

	PlayerStatType type;
	int32_t percent;
	int32_t value;
};

struct EffectModLight{
	EffectModLight() {}
	EffectModLight(int32_t level, int32_t color) :
		level(level),
		color(color) {}

	int32_t level;
	int32_t color;
};

struct EffectModShapeShift{
	EffectModShapeShift() {}
	EffectModShapeShift(uint32_t lookType, uint32_t lookTypeEx, uint32_t lookHead,
		uint32_t lookBody, uint32_t lookLegs, uint32_t lookFeet, uint32_t lookAddons) :
		lookType(lookType),
		lookTypeEx(lookTypeEx),
		lookHead(lookHead),
		lookBody(lookBody),
		lookLegs(lookLegs),
		lookFeet(lookFeet),
		lookAddons(lookAddons) {}

	uint32_t lookType;
	uint32_t lookTypeEx;
	uint32_t lookHead;
	uint32_t lookBody;
	uint32_t lookLegs;
	uint32_t lookFeet;
	uint32_t lookAddons;
};

struct EffectModDispel{
	std::string name;
};

class Condition{
public:
	static Condition* createPeriodDamageCondition(ConditionId id, uint32_t interval,
		int32_t damage, uint32_t rounds);

	static Condition* createPeriodAverageDamageCondition(ConditionId id, uint32_t interval,
		int32_t startDamage, int32_t total);

	static Condition* createCondition(ConditionId id, uint32_t ticks, uint32_t sourceId = 0, uint32_t flags = 0);

	static Condition* createCondition(const std::string& name, uint32_t ticks,
		MechanicType mechanicType = MECHANIC_NONE, CombatType combatType = COMBAT_NONE,
		uint32_t sourceId = 0, uint32_t flags = 0);

	static Condition* createCondition(PropStream& propStream);

	enum Flag{
		FLAG_INFIGHT		= 1,
		FLAG_SLOW			= 2,
		FLAG_HASTE			= 4,
		FLAG_STRENGTHENED	= 8,
		FLAG_MANASHIELD		= 16,
		FLAG_DRUNK			= 32
	};

	class Effect;
	Condition(const std::string& name,
			CombatType combatType,
			MechanicType mechanicType,
			uint32_t sourceId,
			uint32_t ticks,
			uint32_t flags) :
		name(name),
		combatType(combatType),
		mechanicType(mechanicType),
		sourceId(sourceId),
		ticks(ticks),
		flags(flags)
		{}
	Condition(const Condition& rhs);
	~Condition();

	uint16_t getIcon() const;
	MechanicType getMechanicType() const { return mechanicType;}
	CombatType getCombatType() const { return combatType;}
	uint32_t getSourceId() const { return sourceId;}
	const std::string& getName() const {return name;}
	uint32_t getTicks() const {return ticks;}
	void setTicks(uint32_t newTicks) {ticks = newTicks;}
	void setSource(const CombatSource& _combatSource) {combatSource = _combatSource;}

	bool onBegin(Creature* creature);
	void onEnd(Creature* creature, ConditionEnd reason);
	bool onUpdate(Creature* creature, const Condition* addCondition);
	bool onTick(Creature* creature, uint32_t ticks);

	void addEffect(Condition::Effect* effect);

	Condition* clone()  const { return new Condition(*this); }

	//serialization
	bool isPersistent() const;
	bool unserialize(PropStream& propStream);
	bool serialize(PropWriteStream& propWriteStream);

	//class Effect
	class Effect{
	public:
		enum Type{
			//periodic
			PERIODIC_HEAL,
			PERIODIC_DAMAGE,
			PERIODIC_MOD_STAMINA,
			REGEN_HEALTH,
			REGEN_MANA,
			REGEN_SOUL,

			//start/end
			MOD_SPEED,
			MOD_STAT,
			MOD_SKILL,
			SHAPESHIFT,
			LIGHT,
			DISPEL,

			SCRIPT
		};

		static Effect* createRegenHealth(uint32_t interval, int32_t percent, int32_t value)
		{
			EffectModRegen mod(percent, value);
			return new Effect(REGEN_HEALTH, interval, mod);
		}

		static Effect* createRegenMana(uint32_t interval, int32_t percent, int32_t value)
		{
			EffectModRegen mod(percent, value);
			return new Effect(REGEN_MANA, interval, mod);
		}

		static Effect* createRegenSoul(uint32_t interval, int32_t percent, int32_t value)
		{
			EffectModRegen mod(percent, value);
			return new Effect(REGEN_SOUL, interval, mod);
		}

		static Effect* createModStamina(uint32_t interval, int32_t value)
		{
			EffectModPeriodicStamina mod(value);
			return new Effect(PERIODIC_MOD_STAMINA, interval, mod);
		}

		Effect(Effect::Type type, uint32_t interval, boost::any data = boost::any()) :
			type(type),
			interval(interval),
			data(data),
			tickCount(0),
			owner_condition(NULL)
			{}

		~Effect(){};
		bool onBegin(Creature* creature);
		bool onEnd(Creature* creature, ConditionEnd reason);
		bool onUpdate(Creature* creature, const Condition::Effect* addEffect);

		bool onTick(Creature* creature, uint32_t ticks);
		void setOwner(Condition* condition) {owner_condition = condition;}

		Effect::Type getEffectType() const {return type;}
		template<typename T> T& getModEffect() {return boost::any_cast<T&>(data);}

	protected:
		int32_t getStatValue(Creature* creature, PlayerStatType statType, int32_t percent, int32_t value);
		int32_t getSkillValue(Creature* creature, SkillType skillType, int32_t percent, int32_t value);

		Effect::Type type;
		uint32_t interval;
		boost::any data;

		//variables that should not be serialized
		uint32_t tickCount;
		Condition* owner_condition;

		friend class Condition;
	};

protected:
	std::string name;
	CombatType combatType;
	MechanicType mechanicType;
	uint32_t sourceId;
	uint32_t ticks;
	uint32_t flags;
	std::list<Effect*> effectList;

	//variables that should not be serialized
	CombatSource combatSource;

	friend class Effect;
};

#endif
