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
#include "otpch.h"

#include "condition.h"
#include "game.h"
#include "player.h"
#include "configmanager.h"

extern Game g_game;
extern ConfigManager g_config;

Condition* Condition::createPeriodDamageCondition(ConditionId id, uint32_t interval,
	int32_t damage, uint32_t rounds)
{
	if(rounds == 0){
		return NULL;
	}

	Condition* condition = Condition::createCondition(id, 0);
	if(condition){
		damage = std::abs(damage);
		EffectModPeriodicDamage mod(condition->getCombatType(), 0, 0, damage, rounds);
		Condition::Effect* effect = new Condition::Effect(Effect::PERIODIC_DAMAGE, interval, mod);
		condition->addEffect(effect);
		return condition;
	}

	return NULL;
}

Condition* Condition::createPeriodAverageDamageCondition(ConditionId id, uint32_t interval,
	int32_t startDamage, int32_t totalDamage)
{
	if(startDamage == 0){
		return NULL;
	}

	Condition* condition = Condition::createCondition(id, 0);
	if(condition){
		startDamage = std::abs(startDamage);
		totalDamage = std::abs(totalDamage);
		EffectModPeriodicDamage mod(condition->getCombatType(), totalDamage, (totalDamage / startDamage), startDamage, 0);
		Condition::Effect* effect = new Condition::Effect(Effect::PERIODIC_DAMAGE, interval, mod);
		condition->addEffect(effect);
		return condition;
	}

	return NULL;
}

Condition* Condition::createCondition(ConditionId id, uint32_t ticks, uint32_t sourceId /*= 0*/, uint32_t flags /*= 0*/)
{
	switch(id.value()){
		case enums::CONDITION_POISONED:
			return Condition::createCondition(id.toString(), ticks, MECHANIC_NONE, COMBAT_EARTHDAMAGE, sourceId);
		case enums::CONDITION_ELECTRIFIED:
			return Condition::createCondition(id.toString(), ticks, MECHANIC_NONE, COMBAT_ENERGYDAMAGE, sourceId);
		case enums::CONDITION_BURNING:
			return Condition::createCondition(id.toString(), ticks, MECHANIC_NONE, COMBAT_FIREDAMAGE, sourceId);
		case enums::CONDITION_DROWNING:
			return Condition::createCondition(id.toString(), ticks, MECHANIC_NONE, COMBAT_DROWNDAMAGE, sourceId);
		case enums::CONDITION_FREEZING:
			return Condition::createCondition(id.toString(), ticks, MECHANIC_NONE, COMBAT_ICEDAMAGE, sourceId);
		case enums::CONDITION_DAZZLED:
			return Condition::createCondition(id.toString(), ticks, MECHANIC_NONE, COMBAT_HOLYDAMAGE, sourceId);
		case enums::CONDITION_CURSED:
			return Condition::createCondition(id.toString(), ticks, MECHANIC_NONE, COMBAT_DEATHDAMAGE, sourceId);

		case enums::CONDITION_INVISIBLE:
		case enums::CONDITION_LIGHT:
		case enums::CONDITION_REGENERATION:
		case enums::CONDITION_REGENSOUL:
		case enums::CONDITION_EXHAUST_DAMAGE:
		case enums::CONDITION_EXHAUST_HEAL:
		case enums::CONDITION_EXHAUST_YELL:
			return Condition::createCondition(id.toString(), ticks, MECHANIC_NONE, COMBAT_NONE, sourceId);

		case enums::CONDITION_HUNTING:
		{
			Condition* condition = Condition::createCondition(id.toString(), ticks, MECHANIC_NONE, COMBAT_NONE, sourceId);
			if(condition){
				Condition::Effect* effect = Condition::Effect::createModStamina(1000, -g_config.getNumber(ConfigManager::RATE_STAMINA_LOSS));
				condition->addEffect(effect);
			}
			return condition;
		}

		case enums::CONDITION_INFIGHT:
			return Condition::createCondition(id.toString(), ticks, MECHANIC_NONE, COMBAT_NONE, sourceId, FLAG_INFIGHT);
		case enums::CONDITION_HASTE:
			return Condition::createCondition(id.toString(), ticks, MECHANIC_NONE, COMBAT_NONE, sourceId, FLAG_HASTE);
		case enums::CONDITION_PARALYZED:
			return Condition::createCondition(id.toString(), ticks, MECHANIC_PARALYZED, COMBAT_NONE, sourceId, FLAG_SLOW);
		case enums::CONDITION_DRUNK:
			return Condition::createCondition(id.toString(), ticks, MECHANIC_DRUNK, COMBAT_NONE, sourceId, FLAG_DRUNK);
		case enums::CONDITION_MANASHIELD:
			return Condition::createCondition(id.toString(), ticks, MECHANIC_NONE, COMBAT_NONE, sourceId, FLAG_MANASHIELD);

		case enums::CONDITION_SILENCED:
		case enums::CONDITION_MUTED_CHAT:
		case enums::CONDITION_MUTED_CHAT_TRADE:
			return Condition::createCondition(id.toString(), ticks, MECHANIC_SILENCED, COMBAT_NONE, sourceId);

		case enums::CONDITION_SHAPESHIFT:
			return Condition::createCondition(id.toString(), ticks, MECHANIC_SHAPESHIFT, COMBAT_NONE, sourceId);
		case enums::CONDITION_PACIFIED:
			return Condition::createCondition(id.toString(), ticks, MECHANIC_PACIFIED, COMBAT_NONE, sourceId);
		case enums::CONDITION_DISARMED:
			return Condition::createCondition(id.toString(), ticks, MECHANIC_DISARMED, COMBAT_NONE, sourceId);

		default:
			break;
	}

	return NULL;
}

Condition* Condition::createCondition(const std::string& name, uint32_t ticks,
	MechanicType mechanicType /*= MECHANIC_NONE*/, CombatType combatType /*= MECHANIC_NONE*/,
	uint32_t sourceId /*= 0*/, uint32_t flags /*= 0*/)
{
	return new Condition(name, combatType, mechanicType, sourceId, ticks, flags);
}

Condition* Condition::createCondition(PropStream& propStream)
{
	if(propStream.size() == 0){
		return NULL;
	}

	return createCondition("", 0);
}

Condition::~Condition()
{
	for(std::list<Condition::Effect*>::iterator it = effectList.begin(); it != effectList.end(); ++it){
		delete *it;
	}
	effectList.clear();
}

Condition::Condition(const Condition& rhs)
{
	mechanicType = rhs.mechanicType;
	combatType = rhs.combatType;
	sourceId = rhs.sourceId;
	ticks = rhs.ticks;
	name = rhs.name;
	flags = rhs.flags;
	combatSource = rhs.combatSource;

	for(std::list<Effect*>::const_iterator it = rhs.effectList.begin(); it != rhs.effectList.end(); ++it){
		addEffect(new Condition::Effect(*(*it)));
	}
}

uint16_t Condition::getIcon() const
{
	uint16_t icons = 0;

	switch(combatType.value()){
		case enums::COMBAT_ENERGYDAMAGE: icons |= ICON_ENERGY; break;
		case enums::COMBAT_EARTHDAMAGE: icons |= ICON_POISON; break;
		case enums::COMBAT_FIREDAMAGE: icons |= ICON_BURN; break;
		case enums::COMBAT_DROWNDAMAGE: icons |= ICON_DROWNING; break;
		case enums::COMBAT_ICEDAMAGE: icons |= ICON_FREEZING; break;
		case enums::COMBAT_HOLYDAMAGE: icons |= ICON_DAZZLED; break;
		case enums::COMBAT_DEATHDAMAGE: icons |= ICON_CURSED; break;

		default:
			break;
	}

	if(hasBitSet(FLAG_INFIGHT, flags)){
		icons |= ICON_SWORDS;
	}

	if(hasBitSet(FLAG_STRENGTHENED, flags)){
		icons |= ICON_PARTY_BUFF;
	}

	if(hasBitSet(FLAG_HASTE, flags)){
		icons |= ICON_HASTE;
	}

	if(hasBitSet(FLAG_MANASHIELD, flags)){
		icons |= ICON_MANASHIELD;
	}

	return icons;
}

bool Condition::onBegin(Creature* creature)
{
	for(std::list<Effect*>::iterator it = effectList.begin(); it != effectList.end(); ++it){
		if(!(*it)->onBegin(creature)){
			return false;
		}
	}

	return true;
}

void Condition::onEnd(Creature* creature, ConditionEnd reason)
{
	for(std::list<Effect*>::iterator it = effectList.begin(); it != effectList.end(); ++it){
		(*it)->onEnd(creature, reason);
	}
}

bool Condition::onUpdate(Creature* creature, const Condition* addCondition)
{
	if(getName() != addCondition->getName()){
		//different condition
		return false;
	}

	if(getSourceId() != addCondition->getSourceId()){
		//different source (SlotType)
		return false;
	}

	if(getTicks() > addCondition->getTicks()){
		return false;
	}

	mechanicType = addCondition->mechanicType;
	combatType = addCondition->combatType;
	sourceId = addCondition->sourceId;
	ticks = addCondition->ticks;
	name = addCondition->name;
	flags = addCondition->flags;
	combatSource = addCondition->combatSource;

	bool fullUpdate = (effectList.size() != addCondition->effectList.size());
	if(!fullUpdate){
		std::list<Effect*>::iterator curIt = effectList.begin();
		for(std::list<Effect*>::const_iterator it = addCondition->effectList.begin(); it != addCondition->effectList.end(); ++it){
			if(!(*curIt)->onUpdate(creature, *it)){
				fullUpdate = true;
				break;
			}

			++curIt;
		}
	}

	if(fullUpdate){
		//Condition has been changed, maybe from a script reload, doing a full update
		for(std::list<Effect*>::iterator it = effectList.begin(); it != effectList.end(); ++it){
			(*it)->onEnd(creature, CONDITIONEND_REMOVED);
			delete (*it);
		}
		effectList.clear();

		for(std::list<Effect*>::const_iterator it = addCondition->effectList.begin(); it != addCondition->effectList.end(); ++it){
			addEffect(new Effect(*(*it)) );
		}
	}

	return true;
}

bool Condition::onTick(Creature* creature, uint32_t interval)
{
	for(std::list<Effect*>::iterator it = effectList.begin(); it != effectList.end(); ++it){
		if(!(*it)->onTick(creature, interval)){
			return false;
		}
	}

	if(ticks != 0){
		if(ticks - interval <= 0){
			return false;
		}

		ticks -= interval;
	}

	return true;
}

void Condition::addEffect(Condition::Effect* effect)
{
	effect->setOwner(this);
	effectList.push_back(effect);
}

bool Condition::isPersistent() const
{
	if(ticks == 0){
		return false;
	}

	//Other sources should not be saved
	return (sourceId == 0);
}

bool Condition::unserialize(PropStream& propStream)
{
	uint8_t attr_type;
	while(propStream.GET_UCHAR(attr_type) && attr_type != CONDITIONATTR_END.value()){
		
		if(attr_type == enums::CONDITIONATTRIBUTE_MECHANIC)
		{
			int32_t value = 0;
			if(!propStream.GET_VALUE(value)){
				return false;
			}

			mechanicType = (MechanicType)value;
		}

		if(attr_type == enums::CONDITIONATTRIBUTE_COMBAT)
		{
			int32_t value = 0;
			if(!propStream.GET_VALUE(value)){
				return false;
			}

			combatType = (CombatType)value;
			return true;
		}

		if(attr_type == enums::CONDITIONATTRIBUTE_TICKS)
		{
			uint32_t value = 0;
			if(!propStream.GET_VALUE(value)){
				return false;
			}

			ticks = value;
		}

		if(attr_type == enums::CONDITIONATTRIBUTE_NAME)
		{
			std::string value = 0;
			if(!propStream.GET_STRING(value)){
				return false;
			}

			name = value;
		}

		if(attr_type == enums::CONDITIONATTRIBUTE_FLAGS)
		{
			uint32_t value = 0;
			if(!propStream.GET_VALUE(value)){
				return false;
			}

			flags = value;
		}

		if(attr_type == enums::CONDITIONATTRIBUTE_EFFECT)
		{
			uint32_t value = 0;
			//effect type
			if(!propStream.GET_VALUE(value)){
				return false;
			}

			//interval
			uint32_t interval;
			if(!propStream.GET_VALUE(interval)){
				return false;
			}

			//size
			uint32_t size;
			if(!propStream.GET_VALUE(size)){
				return false;
			}

			//revision
			uint32_t revision;
			if(!propStream.GET_VALUE(revision)){
				return false;
			}

			Effect::Type effectType = Effect::Type(value);
			boost::any data = boost::any();

			switch(effectType){
				case Effect::PERIODIC_HEAL:
				case Effect::PERIODIC_DAMAGE:
				{
					if(revision != 1){
						return false;
					}
					EffectModPeriodicDamage mod;
					if(!propStream.GET_VALUE(value)){
						return false;
					}
					mod.type = CombatType::fromInteger(value);

					if(!propStream.GET_VALUE(mod.total)){
						return false;
					}
					if(!propStream.GET_VALUE(mod.percent)){
						return false;
					}
					if(!propStream.GET_VALUE(mod.value)){
						return false;
					}
					if(!propStream.GET_VALUE(mod.rounds)){
						return false;
					}

					data = mod;
					break;
				}
				case Effect::PERIODIC_MOD_STAMINA:
				{
					if(revision != 1){
						return false;
					}
					EffectModPeriodicStamina mod;
					if(!propStream.GET_VALUE(mod.value)){
						return false;
					}
					data = mod;
					break;
				}
				case Effect::REGEN_HEALTH:
				case Effect::REGEN_MANA:
				case Effect::REGEN_SOUL:
				{
					if(revision != 1){
						return false;
					}
					uint32_t value;
					if(!propStream.GET_VALUE(value)){
						return false;
					}

					EffectModRegen mod;
					mod.type = PlayerStatType::fromInteger(value);

					if(!propStream.GET_VALUE(mod.percent)){
						return false;
					}
					if(!propStream.GET_VALUE(mod.value)){
						return false;
					}
					data = mod;
					break;
				}
				case Effect::MOD_SPEED:
				{
					if(revision != 1){
						return false;
					}
					EffectModSpeed mod;
					if(!propStream.GET_VALUE(mod.percent)){
						return false;
					}
					if(!propStream.GET_VALUE(mod.value)){
						return false;
					}
					if(!propStream.GET_VALUE(mod.delta)){
						return false;
					}
					data = mod;
					break;
				}
				case Effect::MOD_STAT:
				{
					if(revision != 1){
						return false;
					}
					uint32_t value;
					if(!propStream.GET_VALUE(value)){
						return false;
					}

					EffectModStat mod;
					mod.type = PlayerStatType::fromInteger(value);

					if(!propStream.GET_VALUE(mod.percent)){
						return false;
					}
					if(!propStream.GET_VALUE(mod.value)){
						return false;
					}
					if(!propStream.GET_VALUE(mod.delta)){
						return false;
					}
					data = mod;
					break;
				}
				case Effect::MOD_SKILL:
				{
					if(revision != 1){
						return false;
					}
					uint32_t value;
					if(!propStream.GET_VALUE(value)){
						return false;
					}

					EffectModSkill mod;
					mod.type = SkillType::fromInteger(value);

					if(!propStream.GET_VALUE(mod.percent)){
						return false;
					}
					if(!propStream.GET_VALUE(mod.value)){
						return false;
					}
					if(!propStream.GET_VALUE(mod.delta)){
						return false;
					}
					data = mod;
					break;
				}
				case Effect::SHAPESHIFT:
				{
					if(revision != 1){
						return false;
					}
					EffectModShapeShift mod;
					if(!propStream.GET_VALUE(mod.lookType)){
						return false;
					}
					if(!propStream.GET_VALUE(mod.lookTypeEx)){
						return false;
					}
					if(!propStream.GET_VALUE(mod.lookHead)){
						return false;
					}
					if(!propStream.GET_VALUE(mod.lookBody)){
						return false;
					}
					if(!propStream.GET_VALUE(mod.lookLegs)){
						return false;
					}
					if(!propStream.GET_VALUE(mod.lookFeet)){
						return false;
					}
					if(!propStream.GET_VALUE(mod.lookAddons)){
						return false;
					}
					data = mod;
					break;
				}
				case Effect::LIGHT:
				{
					if(revision != 1){
						return false;
					}
					EffectModLight mod;
					if(!propStream.GET_VALUE(mod.level)){
						return false;
					}
					if(!propStream.GET_VALUE(mod.color)){
						return false;
					}
					data = mod;
					break;
				}
				case Effect::DISPEL:
				{
					if(revision != 1){
						return false;
					}
					EffectModDispel mod;
					if(!propStream.GET_STRING(mod.name)){
						return false;
					}
					data = mod;
					break;
				}

				case Effect::SCRIPT:
					break;
				default: return false;
			}

			Condition::Effect* effect = new Condition::Effect(effectType, interval, data);
			effectList.push_back(effect);
		}

		if(attr_type == enums::CONDITIONATTR_END)
			return true;
	}

	return false;
}

bool Condition::serialize(PropWriteStream& propWriteStream)
{
	propWriteStream.ADD_UCHAR(*CONDITIONATTRIBUTE_MECHANIC);
	propWriteStream.ADD_VALUE(mechanicType.value());

	propWriteStream.ADD_UCHAR(*CONDITIONATTRIBUTE_COMBAT);
	propWriteStream.ADD_VALUE(combatType.value());

	propWriteStream.ADD_UCHAR(*CONDITIONATTRIBUTE_SOURCE);
	propWriteStream.ADD_VALUE(sourceId);

	propWriteStream.ADD_UCHAR(*CONDITIONATTRIBUTE_TICKS);
	propWriteStream.ADD_VALUE(ticks);

	propWriteStream.ADD_UCHAR(*CONDITIONATTRIBUTE_NAME);
	propWriteStream.ADD_STRING(name);

	propWriteStream.ADD_UCHAR(*CONDITIONATTRIBUTE_FLAGS);
	propWriteStream.ADD_VALUE(flags);

	for(std::list<Effect*>::iterator it = effectList.begin(); it != effectList.end(); ++it){
		propWriteStream.ADD_UCHAR(*CONDITIONATTRIBUTE_EFFECT);
		propWriteStream.ADD_ULONG((*it)->type);
		propWriteStream.ADD_ULONG((*it)->interval);

		switch((*it)->type){
			case Effect::PERIODIC_HEAL:
			case Effect::PERIODIC_DAMAGE:
			{
				EffectModPeriodicDamage& mod = (*it)->getModEffect<EffectModPeriodicDamage>();
				propWriteStream.ADD_ULONG(sizeof(mod));
				propWriteStream.ADD_ULONG(1); /*revision*/
				propWriteStream.ADD_VALUE((uint32_t)mod.type.value());
				propWriteStream.ADD_VALUE(mod.total);
				propWriteStream.ADD_VALUE(mod.percent);
				propWriteStream.ADD_VALUE(mod.value);
				propWriteStream.ADD_VALUE(mod.rounds);
				break;
			}
			case Effect::PERIODIC_MOD_STAMINA:
			{
				EffectModPeriodicStamina& mod = (*it)->getModEffect<EffectModPeriodicStamina>();
				propWriteStream.ADD_ULONG(sizeof(mod));
				propWriteStream.ADD_ULONG(1);
				propWriteStream.ADD_VALUE(mod.value);
				break;
			}
			case Effect::REGEN_HEALTH:
			case Effect::REGEN_MANA:
			case Effect::REGEN_SOUL:
			{
				EffectModRegen& mod = (*it)->getModEffect<EffectModRegen>();
				propWriteStream.ADD_ULONG(sizeof(mod));
				propWriteStream.ADD_ULONG(1);
				propWriteStream.ADD_VALUE((uint32_t)mod.type.value());
				propWriteStream.ADD_VALUE(mod.percent);
				propWriteStream.ADD_VALUE(mod.value);
				break;
			}
			case Effect::MOD_SPEED:
			{
				EffectModSpeed& mod = (*it)->getModEffect<EffectModSpeed>();
				propWriteStream.ADD_ULONG(sizeof(mod));
				propWriteStream.ADD_ULONG(1);
				propWriteStream.ADD_VALUE(mod.percent);
				propWriteStream.ADD_VALUE(mod.value);
				propWriteStream.ADD_VALUE(mod.delta);
				break;
			}
			case Effect::MOD_STAT:
			{
				EffectModStat& mod = (*it)->getModEffect<EffectModStat>();
				propWriteStream.ADD_ULONG(sizeof(mod));
				propWriteStream.ADD_ULONG(1);
				propWriteStream.ADD_VALUE((uint32_t)mod.type.value());
				propWriteStream.ADD_VALUE(mod.percent);
				propWriteStream.ADD_VALUE(mod.value);
				propWriteStream.ADD_VALUE(mod.delta);
				break;
			}
			case Effect::MOD_SKILL:
			{
				EffectModSkill& mod = (*it)->getModEffect<EffectModSkill>();
				propWriteStream.ADD_ULONG(sizeof(mod));
				propWriteStream.ADD_ULONG(1);
				propWriteStream.ADD_VALUE((uint32_t)mod.type.value());
				propWriteStream.ADD_VALUE(mod.percent);
				propWriteStream.ADD_VALUE(mod.value);
				propWriteStream.ADD_VALUE(mod.delta);
				break;
			}
			case Effect::SHAPESHIFT:
			{
				EffectModShapeShift& mod = (*it)->getModEffect<EffectModShapeShift>();
				propWriteStream.ADD_ULONG(sizeof(mod));
				propWriteStream.ADD_ULONG(1);				
				propWriteStream.ADD_VALUE(mod.lookType);
				propWriteStream.ADD_VALUE(mod.lookTypeEx);
				propWriteStream.ADD_VALUE(mod.lookHead);
				propWriteStream.ADD_VALUE(mod.lookBody);
				propWriteStream.ADD_VALUE(mod.lookLegs);
				propWriteStream.ADD_VALUE(mod.lookFeet);
				propWriteStream.ADD_VALUE(mod.lookAddons);
				break;
			}
			case Effect::LIGHT:
			{
				EffectModLight& mod = (*it)->getModEffect<EffectModLight>();
				propWriteStream.ADD_ULONG(sizeof(mod));
				propWriteStream.ADD_ULONG(1);
				propWriteStream.ADD_VALUE(mod.level);
				propWriteStream.ADD_VALUE(mod.color);
				break;
			}
			case Effect::DISPEL:
			{
				EffectModDispel& mod = (*it)->getModEffect<EffectModDispel>();
				propWriteStream.ADD_ULONG(sizeof(mod));
				propWriteStream.ADD_ULONG(1);
				propWriteStream.ADD_STRING(mod.name);
				break;
			}
			default: return false;
		}
	}

	return true;
}

int32_t Condition::Effect::getStatValue(Creature* creature, PlayerStatType statType, int32_t percent, int32_t value)
{
	if(percent != 0){
		switch(statType.value()){
			case enums::STAT_MAXHITPOINTS:
				return (int32_t)(creature->getMaxMana() * (percent / 100.f));
			
				case enums::STAT_MAXMANAPOINTS:
					return (int32_t)(creature->getMaxMana() * (percent / 100.f));

			default:
				break;
		}

		if(Player* player = creature->getPlayer()){
			switch(statType.value()){
				case enums::STAT_SOULPOINTS:
					return (int32_t)(player->getPlayerInfo(PLAYERINFO_SOUL) * (percent / 100.f));
				case enums::STAT_MAGICPOINTS:
					return (int32_t)(player->getMagicLevel() * (percent / 100.f));
				default:
					break;
			}
		}
	}
	else{
		return value;
	}

	return 0;
}

int32_t Condition::Effect::getSkillValue(Creature* creature, SkillType skillType, int32_t percent, int32_t value)
{
	if(percent != 0){
		if(Player* player = creature->getPlayer()){
			int32_t currSkill = player->getSkill(skillType, SKILL_LEVEL);
			return (int32_t)(currSkill * (percent / 100.f));
		}
	}
	else{
		return value;
	}

	return 0;
}

bool Condition::Effect::onBegin(Creature* creature)
{
	switch(type){
		case Effect::MOD_SPEED:
		{
			EffectModSpeed& modSpeed = getModEffect<EffectModSpeed>();
			modSpeed.delta = (int32_t)std::ceil(((float)creature->getBaseSpeed()) * ((float)modSpeed.percent) / 100 + modSpeed.value);
			g_game.changeSpeed(creature, modSpeed.delta);
			break;
		}

		case Effect::LIGHT:
		{
			//interval = getTicks()/lightInfo.level;
			EffectModLight& modLight = getModEffect<EffectModLight>();
			LightInfo lightInfo;
			lightInfo.level = modLight.level;
			lightInfo.color = modLight.color;
			creature->setCreatureLight(lightInfo);
			g_game.changeLight(creature);
			break;
		}

		case Effect::SHAPESHIFT:
		{
			EffectModShapeShift& modShapeShift = getModEffect<EffectModShapeShift>();
			OutfitType outfit;
			outfit.lookType = modShapeShift.lookType;
			outfit.lookTypeEx = modShapeShift.lookTypeEx;
			outfit.lookHead = modShapeShift.lookHead;
			outfit.lookBody = modShapeShift.lookBody;
			outfit.lookLegs = modShapeShift.lookLegs;
			outfit.lookFeet = modShapeShift.lookFeet;
			g_game.internalCreatureChangeOutfit(creature, outfit);
			break;
		}

		case Effect::DISPEL:
		{
			EffectModDispel& modDispel = getModEffect<EffectModDispel>();			
			CombatSource combatSource = owner_condition->combatSource;
			combatSource.setSourceIsCondition(true);
			creature->removeCondition(modDispel.name, combatSource);
			break;
		}

		case Effect::SCRIPT:
		{
			if(g_game.onConditionBegin(creature, owner_condition)){
				//handled by script
				return false;
			}
			break;
		}
		default:
			break;
	}

	if(Player* player = creature->getPlayer()){
		switch(type){
			case Effect::MOD_STAT:
			{
				EffectModStat& modStat = getModEffect<EffectModStat>();
				modStat.delta = getStatValue(creature, modStat.type, modStat.percent, modStat.value);
				player->setVarStats(modStat.type, modStat.delta);
				player->sendStats();
				break;
			}
			
			case Effect::MOD_SKILL:
			{
				EffectModSkill& modSkill = getModEffect<EffectModSkill>();
				modSkill.delta = getSkillValue(creature, modSkill.type, modSkill.percent, modSkill.value);
				player->setVarSkill(modSkill.type, modSkill.delta);
				player->sendSkills();
				break;
			}

			default:
				break;
		}
	}

	return true;
}

bool Condition::Effect::onEnd(Creature* creature, ConditionEnd reason)
{
	switch(type){
		case Effect::MOD_SPEED:
		{
			//revert our changes
			EffectModSpeed& modSpeed = getModEffect<EffectModSpeed>();
			g_game.changeSpeed(creature, -modSpeed.delta);
			break;
		}

		case Effect::LIGHT:
		{
			creature->setNormalCreatureLight();
			g_game.changeLight(creature);
			break;
		}

		case Effect::SHAPESHIFT:
		{
			g_game.internalCreatureChangeOutfit(creature, creature->getDefaultOutfit());
			break;
		}

		case Effect::SCRIPT:
		{
			if(g_game.onConditionEnd(creature, owner_condition, reason)){
				//handled by script
				return false;
			}
			break;
		}

		default:
			break;
	}

	if(Player* player = creature->getPlayer()){
		switch(type){
			case Effect::MOD_STAT:
			{
				//revert our changes
				EffectModStat& modStat = getModEffect<EffectModStat>();
				player->setVarStats(modStat.type, -modStat.delta);
				player->sendStats();
				break;
			}
			
			case Effect::MOD_SKILL:
			{
				//revert our changes
				EffectModSkill& modSkill = getModEffect<EffectModSkill>();
				player->setVarSkill(modSkill.type, -modSkill.delta);
				player->sendSkills();
				break;
			}

			default:
				break;
		}
	}

	return true;
}

bool Condition::Effect::onUpdate(Creature* creature, const Condition::Effect* addEffect)
{
	if(type != addEffect->type){
		return false;
	}

	onEnd(creature, CONDITIONEND_UPDATE);

	data = addEffect->data;
	interval = addEffect->interval;
	return true;
}

bool Condition::Effect::onTick(Creature* creature, uint32_t ticks)
{
	if(interval == 0){
		return true;
	}

	tickCount += ticks;
	if(tickCount >= interval){
		tickCount = 0;

		switch(type){
			case Effect::PERIODIC_HEAL:
			case Effect::PERIODIC_DAMAGE:
			{
				EffectModPeriodicDamage& modPeriodicDamage = getModEffect<EffectModPeriodicDamage>();
				CombatSource combatSource = owner_condition->combatSource;
				combatSource.setSourceItem(NULL);
				combatSource.setSourceIsCondition(true);

				if(type == Effect::PERIODIC_HEAL){
					int32_t heal = modPeriodicDamage.value;
					//std::cout << "Healing " << mod_value << " "  << combatType.toString() << " to " << creature->getName() << std::endl;
					if(!g_game.combatBlockHit(COMBAT_HEALING, combatSource, creature, heal, false, false)){
						g_game.combatChangeHealth(COMBAT_HEALING, combatSource, creature, heal);
					}
				}
				else{
					int32_t damage = -modPeriodicDamage.value;
					//std::cout << "Dealing " << mod_value << " "  << combatType.toString() << " to " << creature->getName() << std::endl;
					if(!g_game.combatBlockHit(modPeriodicDamage.type, combatSource, creature, damage, false, false)){
						g_game.combatChangeHealth(modPeriodicDamage.type, combatSource, creature, damage);
					}
				}

				bool skip = false;
				if(const MagicField* field = creature->getTile()->getFieldItem()){
					if(field->getCombatType() == modPeriodicDamage.type){
						//The creature is still standing in the field so the damage should
						//not be counted towards the total damage.
						skip = true;
					}
				}

				if(!skip){
					//average damage
					if(modPeriodicDamage.total != 0){
						//total damage done
						modPeriodicDamage.sum += modPeriodicDamage.value;

						//number of rounds done
						modPeriodicDamage.roundCompleted++;

						int32_t curRounds = std::ceil(((float)modPeriodicDamage.percent) / modPeriodicDamage.value);
						if(modPeriodicDamage.roundCompleted >= curRounds){
							modPeriodicDamage.roundCompleted = 0;
							--modPeriodicDamage.value;
						}

						if(modPeriodicDamage.sum >= modPeriodicDamage.total || modPeriodicDamage.value == 0){
							return false;
						}
					}
					else{
						//number of rounds
						modPeriodicDamage.roundCompleted++;

						if(modPeriodicDamage.roundCompleted >= modPeriodicDamage.rounds ){
							return false;
						}
					}
				}
				break;
			}

			case Effect::REGEN_HEALTH:
			case Effect::REGEN_MANA:
			case Effect::REGEN_SOUL:
			{
				if(creature->getZone() != ZONE_PROTECTION){
					EffectModRegen modRegen = getModEffect<EffectModRegen>();
					if(type == Effect::REGEN_HEALTH){
						creature->changeHealth(getStatValue(creature, modRegen.type, modRegen.percent, modRegen.value));
					}
					else if(type == Effect::REGEN_MANA){
						creature->changeMana(getStatValue(creature, modRegen.type, modRegen.percent, modRegen.value));
					}
					else if(Player* player = creature->getPlayer()){
						player->changeSoul(getStatValue(creature, modRegen.type, modRegen.percent, modRegen.value));
					}
				}
				break;
			}

			case Effect::LIGHT:
			{
				LightInfo creatureLight;
				creature->getCreatureLight(creatureLight);
				if(creatureLight.level > 0){
					--creatureLight.level;
					creature->setCreatureLight(creatureLight);
					g_game.changeLight(creature);
				}
				break;
			}

			case Effect::PERIODIC_MOD_STAMINA:
			{
				if(Player* player = creature->getPlayer()){
					EffectModPeriodicStamina modStamina = getModEffect<EffectModPeriodicStamina>();
					if(modStamina.value < 0){
						player->removeStamina(ticks * std::abs(modStamina.value));
					}
					else{
						player->addStamina(ticks * modStamina.value);
					}
				}
			}

			case Effect::SCRIPT:
			{
				if(g_game.onConditionTick(creature, owner_condition, ticks)){
					//handled by script
					return false;
				}
				break;
			}

			default:
				break;
		}

	}
	return true;
}

/*
TODO: Use Effect::SCRIPT instead?
bool Condition::Effect::onCombat(const CombatSource& combatSource, Creature* creature, CombatType type, int32_t& amount)
{
	switch(effectType){
		case Effect::MANASHIELD:
		{
			//TODO:
			break;
		}

		case Effect::SHIELD:
		{
			//TODO:
			break;
		}

		case Effect::MOD_HEAL:
		case Effect::MOD_DAMAGE:
		{
			if(hasBitSet(type.value(), mod_type)){
				if(percent != 0){
					amount = amount + (int32_t)(amount * percent / 100.f);
				}
				else{
					amount = amount + value;
				}
			}

			break;
		}

		case Effect::SCRIPT:
		{
			//TODO:
			break;
		}

		default:
			break;
	}

	return true;
}
*/
