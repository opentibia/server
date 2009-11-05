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

//class ConditionEffect
class ConditionEffect{
public:
	enum Type{
		//periodic
		PERIODIC_HEAL,
		PERIODIC_DAMAGE,
		PERIODIC_MOD_STAMINA,
		REGEN_HEALTH,
		REGEN_MANA,
		REGEN_SOUL,
		PERIODIC_TRIGGER,

		//start/end
		MOD_SPEED,
		MOD_STAT,
		MOD_SKILL,
		SHAPESHIFT,
		LIGHT,
		DISPEL,

		SCRIPT,

		//AURA_MOD_SPEED,
		//AURA_MOD_STAT,
		//AURA_MOD_SKILL,
		//AURA_PERIODIC_DAMAGE
	};

	struct ModPeriodicDamage{
		ModPeriodicDamage() {}
		ModPeriodicDamage(CombatType type, int32_t total, int32_t percent, int32_t value, int32_t rounds) : 
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

	struct ModPeriodicStamina{
		ModPeriodicStamina() {}
		ModPeriodicStamina(int32_t value) :
			value(value) {}

		int32_t value;
	};

	struct ModStat{
		ModStat() {}
		ModStat(PlayerStatType type, int32_t value, bool percent = false) :
			type(type),
			percent((percent ? value : 0)),
			value((percent ? 0 : value)),
			delta(0) {}
		PlayerStatType type;
		int32_t percent;
		int32_t value;

		//
		int32_t delta;
	};

	struct ModSkill{
		ModSkill() {}
		ModSkill(SkillType type, int32_t value, bool percent = false) :
			type(type),
			percent((percent ? value : 0)),
			value((percent ? 0 : value)),
			delta(0) {}
		SkillType type;
		int32_t percent;
		int32_t value;

		//
		int32_t delta;
	};

	struct ModSpeed{
		ModSpeed() {}
		ModSpeed(int32_t percent, int32_t value) :
			percent(percent),
			value(value),
			delta(0) {}

		int32_t percent;
		int32_t value;

		//
		int32_t delta;
	};

	struct ModRegen{
		ModRegen() {}
		ModRegen(int32_t value) :
			type(0),
			percent(0),
			value(value) {}
		ModRegen(PlayerStatType type, int32_t percent) :
			type(type),
			percent(percent),
			value(0) {}

		PlayerStatType type;
		int32_t percent;
		int32_t value;
	};

	struct ModLight{
		ModLight() {}
		ModLight(int32_t level, int32_t color) :
			level(level),
			color(color) {}

		int32_t level;
		int32_t color;
	};

	struct ModShapeShift{
		ModShapeShift() {}
		ModShapeShift(uint32_t lookType, uint32_t lookTypeEx, uint32_t lookHead,
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

	struct ModDispel{
		ModDispel() {}
		ModDispel(const std::string& name) :
			name(name) {}

		std::string name;
	};

	//ModScript just hold a name that we can listen to with registerOnConditionEffect
	struct ModScript{
		ModScript() {}
		ModScript(const std::string& name) :
			name(name) {}

		std::string name;
	};

	static ConditionEffect createPeriodicHeal(uint32_t interval, int32_t value, int32_t rounds)
	{
		ModPeriodicDamage mod(COMBAT_HEALING, 0, 0, value, rounds);
		return ConditionEffect(ConditionEffect::PERIODIC_HEAL, interval, mod);
	}

	static ConditionEffect createPeriodicDamage(uint32_t interval, CombatType type, int32_t total,
		int32_t percent, int32_t value, int32_t rounds)
	{
		ModPeriodicDamage mod(type, total, percent, value, rounds);
		return ConditionEffect(ConditionEffect::PERIODIC_DAMAGE, interval, mod);
	}

	static ConditionEffect createModStamina(uint32_t interval, int32_t value)
	{
		ModPeriodicStamina mod(value);
		return ConditionEffect(ConditionEffect::PERIODIC_MOD_STAMINA, interval, mod);
	}

	static ConditionEffect createRegenHealth(uint32_t interval, int32_t value)
	{
		ModRegen mod(value);
		return ConditionEffect(ConditionEffect::REGEN_HEALTH, interval, mod);
	}

	static ConditionEffect createRegenPercentHealth(uint32_t interval, PlayerStatType type, int32_t percent)
	{
		ModRegen mod(type, percent);
		return ConditionEffect(ConditionEffect::REGEN_HEALTH, interval, mod);
	}

	static ConditionEffect createRegenMana(uint32_t interval, int32_t value)
	{
		ModRegen mod(value);
		return ConditionEffect(ConditionEffect::REGEN_MANA, interval, mod);
	}

	static ConditionEffect createRegenPercentMana(uint32_t interval, PlayerStatType type, int32_t percent)
	{
		ModRegen mod(type, percent);
		return ConditionEffect(ConditionEffect::REGEN_MANA, interval, mod);
	}

	static ConditionEffect createRegenSoul(uint32_t interval, int32_t value)
	{
		ModRegen mod(value);
		return ConditionEffect(ConditionEffect::REGEN_SOUL, interval, mod);
	}

	static ConditionEffect createRegenPercentSoul(uint32_t interval, PlayerStatType type, int32_t percent)
	{
		ModRegen mod(type, percent);
		return ConditionEffect(ConditionEffect::REGEN_SOUL, interval, mod);
	}

	static ConditionEffect createModSpeed(int32_t percent, int32_t value)
	{
		ModSpeed mod(percent, value);
		return ConditionEffect(ConditionEffect::MOD_SPEED, 0, mod);
	}

	static ConditionEffect createModStat(PlayerStatType type, int32_t value)
	{
		ModStat mod(type, value);
		return ConditionEffect(ConditionEffect::MOD_STAT, mod);
	}

	static ConditionEffect createModPercentStat(PlayerStatType type, int32_t percent)
	{
		ModStat mod(type, percent, true);
		return ConditionEffect(ConditionEffect::MOD_STAT, mod);
	}

	static ConditionEffect createModSkill(SkillType type, int32_t value)
	{
		ModSkill mod(type, value);
		return ConditionEffect(ConditionEffect::MOD_SKILL, 0, mod);
	}

	static ConditionEffect createModPercentSkill(SkillType type, int32_t percent)
	{
		ModSkill mod(type, percent, true);
		return ConditionEffect(ConditionEffect::MOD_SKILL, 0, mod);
	}

	static ConditionEffect createModShapeShift(const OutfitType& outfit)
	{
		ModShapeShift mod(outfit.lookType, outfit.lookTypeEx, outfit.lookHead,
			outfit.lookBody, outfit.lookLegs, outfit.lookFeet, outfit.lookAddons);
		return ConditionEffect(ConditionEffect::SHAPESHIFT, mod);
	}

	static ConditionEffect createModLight(int32_t level, int32_t color)
	{
		ModLight mod(level, color);
		return ConditionEffect(ConditionEffect::LIGHT, mod);
	}

	static ConditionEffect createModDispel(const std::string& name)
	{
		ModDispel mod(name);
		return ConditionEffect(ConditionEffect::DISPEL, mod);
	}

	static ConditionEffect createModScript(const std::string& name, int32_t interval = 0)
	{
		ModScript mod(name);
		return ConditionEffect(ConditionEffect::SCRIPT, interval, mod);
	}

	ConditionEffect() :
		type(type),
		interval(0),
		data(boost::any()),
		tickCount(0),
		owner_condition(NULL)
		{}

	ConditionEffect(ConditionEffect::Type type, uint32_t interval, boost::any data = boost::any()) :
		type(type),
		interval(interval),
		data(data),
		tickCount(0),
		owner_condition(NULL)
		{}

	ConditionEffect(ConditionEffect::Type type, boost::any data = boost::any()) :
		type(type),
		interval(0),
		data(data),
		tickCount(0),
		owner_condition(NULL)
		{}

	~ConditionEffect(){};
	bool onBegin(Creature* creature);
	bool onEnd(Creature* creature, ConditionEnd reason);
	bool onUpdate(Creature* creature, const ConditionEffect& addEffect);

	bool onTick(Creature* creature, uint32_t ticks);
	void setOwner(Condition* condition) {owner_condition = condition;}
	Condition* getOwner() const {return owner_condition;}

	std::string getName()
	{
		switch(type){
			case PERIODIC_HEAL: return "healing";
			case PERIODIC_DAMAGE: return "damage";
			case PERIODIC_MOD_STAMINA: return "stamina";
			case REGEN_HEALTH: return "regeneration";
			case REGEN_MANA: return "regeneration";
			case REGEN_SOUL: return "soul";
			case MOD_SPEED: return "speed";
			case MOD_STAT: return "attributes";
			case MOD_SKILL: return "skills";
			case SHAPESHIFT: return "shapeshift";
			case LIGHT: return "light";
			case DISPEL: return "dispel";
			case SCRIPT:
			{
				ConditionEffect::ModScript& mod = getModEffect<ConditionEffect::ModScript>();
				return mod.name;
			}

			default:
				return "unknown";
		}
	}

	ConditionEffect::Type getEffectType() const {return type;}
	template<typename T> T& getModEffect() {return boost::any_cast<T&>(data);}

	//serialization
	bool unserialize(PropStream& propStream);
	bool serialize(PropWriteStream& propWriteStream);

protected:
	int32_t getStatValue(Creature* creature, PlayerStatType statType, int32_t percent, int32_t value);
	int32_t getSkillValue(Creature* creature, SkillType skillType, int32_t percent, int32_t value);

	ConditionEffect::Type type;
	uint32_t interval;
	boost::any data;

	//variables that should not be serialized
	uint32_t tickCount;
	Condition* owner_condition;

	friend class Condition;
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
		FLAG_DRUNK			= 32,
		//FLAG_PERSISTENT	= 64
	};

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

	const std::string& getName() const {return name;}
	CombatType getCombatType() const { return combatType;}
	MechanicType getMechanicType() const { return mechanicType;}
	uint32_t getSourceId() const { return sourceId;}
	uint32_t getTicks() const {return ticks;}
	uint32_t getFlags() const {return flags;}
	IconType getIcon() const;

	void setName(const std::string& _name) {name = _name;}
	void setCombatType(CombatType _combatType) {combatType = _combatType;}
	void setMechanicType(MechanicType _mechanicType) {mechanicType = _mechanicType;}
	void setSourceId(uint32_t _sourceId) {sourceId = _sourceId;}
	void setTicks(uint32_t _ticks) {ticks = _ticks;}
	void setSource(const CombatSource& _combatSource) {combatSource = _combatSource;}
	void setFlags(uint32_t _flags) {flags = _flags;}

	bool onBegin(Creature* creature);
	void onEnd(Creature* creature, ConditionEnd reason);
	bool onUpdate(Creature* creature, const Condition* addCondition);
	bool onTick(Creature* creature, uint32_t ticks);

	void addEffect(ConditionEffect effect);

	Condition* clone()  const { return new Condition(*this); }

	//serialization
	bool isPersistent() const;
	bool unserialize(PropStream& propStream);
	bool serialize(PropWriteStream& propWriteStream);

protected:
	std::string name;
	CombatType combatType;
	MechanicType mechanicType;
	uint32_t sourceId;
	uint32_t ticks;
	uint32_t flags;
	std::list<ConditionEffect> effectList;

	//variables that should not be serialized
	CombatSource combatSource;

	friend class ConditionEffect;
};

#endif
