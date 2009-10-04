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

enum EffectType{
	EFFECT_PERIODIC_HEAL,
	EFFECT_PERIODIC_DAMAGE,
	EFFECT_PERIODIC_MOD_STAMINA,
	EFFECT_MOD_HEAL,
	EFFECT_MOD_DAMAGE,
	EFFECT_MOD_SPEED,
	EFFECT_REGEN_HEALTH,
	EFFECT_REGEN_MANA,
	EFFECT_REGEN_SOUL,
	EFFECT_MOD_STAT,
	EFFECT_MOD_SKILL,
	EFFECT_SHAPESHIFT,
	EFFECT_SHIELD,
	EFFECT_MANASHIELD,
	EFFECT_LIGHT,
	EFFECT_SCRIPT
};

enum ConditionFlag{
	CONDITION_FLAG_PARTYBUFF = 1
};

class Condition{
public:
	static Condition* createPeriodDamageCondition(ConditionType type,
		uint32_t interval, int32_t value, uint32_t rounds, ConditionSource source = CONDITION_SOURCE_NONE);
	static Condition* createPeriodAverageDamageCondition(ConditionType type,
		uint32_t interval, int32_t value, int32_t total, ConditionSource source = CONDITION_SOURCE_NONE);
	static Condition* createCondition(ConditionType type, uint32_t ticks,
		ConditionSource source = CONDITION_SOURCE_NONE);
	static Condition* createCondition(MechanicType mechanicType, CombatType combatType, ConditionSource source,
		uint32_t ticks, uint32_t categoryId, uint32_t flags = 0);
	static Condition* createCondition(PropStream& propStream);

	class Effect;

	Condition(MechanicType mechanicType, CombatType combatType, ConditionSource source,
		uint32_t ticks, uint32_t id, uint32_t flags) :
		mechanicType(mechanicType),
		combatType(combatType),
		source(source),
		ticks(ticks),
		id(id),
		flags(flags),
		combatSource(NULL)
		{}
	Condition(const Condition& rhs);
	~Condition();

	uint16_t getIcon() const;
	MechanicType getMechanicType() const { return mechanicType;}
	CombatType getCombatType() const { return combatType;}
	ConditionSource getSource() const { return source;}
	uint32_t getId() const {return id;}
	uint32_t getTicks() const {return ticks;}
	void setTicks(uint32_t newTicks) {ticks = newTicks;}
	void setSource(const CombatSource& _combatSource);

	bool onBegin(Creature* creature);
	void onEnd(Creature* creature, ConditionEnd reason);
	bool onUpdate(Creature* creature, const Condition* addCondition);
	bool onTick(Creature* creature, uint32_t interval);

	void addEffect(Condition::Effect* effect);

	Condition* clone()  const { return new Condition(*this); }

	//serialization
	bool isPersistent() const;
	bool unserialize(PropStream& propStream);
	bool serialize(PropWriteStream& propWriteStream);

	//class Effect
	class Effect{
	public:
		Effect(EffectType type,
			int32_t mod_type,
			int32_t mod_value,
			int32_t mod_total,
			int32_t mod_percent,
			int32_t mod_ticks,
			boost::any mod_pod = boost::any()) :
		  effectType(type),
		  mod_type(mod_type),
		  mod_value(mod_value),
		  mod_total(mod_total),
		  mod_percent(mod_percent),
		  mod_ticks(mod_ticks),
		  mod_pod(mod_pod),
		  n(0), i(0), j(0), owner_condition(NULL) {}

		~Effect(){};
		bool onBegin(Creature* creature);
		bool onEnd(Creature* creature, ConditionEnd reason);
		bool onUpdate(Creature* creature, const Condition::Effect* addEffect);
		
		bool onTick(Creature* creature, uint32_t ticks);
		//bool onCombat(const CombatSource& combatSource, Creature* creature, CombatType type, int32_t& amount);
		void setOwner(Condition* condition) {owner_condition = condition;}

		EffectType getEffectType() const {return effectType;}

	protected:
		int32_t getStatValue(Creature* creature);
		int32_t getSkillValue(Creature* creature);

		EffectType effectType;

		uint32_t mod_type;		//hp/mana/soul etc.
		int32_t mod_value;		//flat amount
		int32_t mod_total;		//total amount
		int32_t mod_percent;	//percent amount
		int32_t mod_ticks;
		boost::any mod_pod;

		//misc values, depending on effect
		int32_t n, i, j;

		Condition* owner_condition;

		friend Condition;
	};

protected:
	MechanicType mechanicType;
	CombatType combatType;
	ConditionSource source;
	uint32_t ticks;
	uint32_t id;
	uint32_t flags;
	std::list<Effect*> effectList;

	//variables that should not be serialized
	CombatSource* combatSource;

	friend Effect;
};

#endif
