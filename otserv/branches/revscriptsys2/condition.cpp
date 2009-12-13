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
				ConditionEffect effect = ConditionEffect::createModStamina(1000, -g_config.getNumber(ConfigManager::RATE_STAMINA_LOSS));
				condition->addEffect(effect);
			}
			return condition;
		}

		case enums::CONDITION_INFIGHT:
			return Condition::createCondition(id.toString(), ticks, MECHANIC_NONE, COMBAT_NONE, sourceId, enums::ICON_SWORDS);
		case enums::CONDITION_HASTE:
			return Condition::createCondition(id.toString(), ticks, MECHANIC_NONE, COMBAT_NONE, sourceId, enums::ICON_HASTE);
		case enums::CONDITION_PARALYZED:
			return Condition::createCondition(id.toString(), ticks, MECHANIC_PARALYZED, COMBAT_NONE, sourceId, enums::ICON_PARALYZE);
		case enums::CONDITION_DRUNK:
			return Condition::createCondition(id.toString(), ticks, MECHANIC_DRUNK, COMBAT_NONE, sourceId, enums::ICON_DRUNK);
		case enums::CONDITION_MANASHIELD:
			return Condition::createCondition(id.toString(), ticks, MECHANIC_NONE, COMBAT_NONE, sourceId, enums::ICON_MANASHIELD);

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
			return Condition::createCondition(id.toString(), ticks, MECHANIC_NONE, COMBAT_NONE, sourceId);
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

	for(std::list<ConditionEffect>::const_iterator it = rhs.effectList.begin(); it != rhs.effectList.end(); ++it){
		addEffect(*it);
	}
}

IconType Condition::getIcon() const
{
	IconType icons = ICON_NONE;

	for(IconType::iterator i = ICON_POISON; i != IconType::end(); ++i){
		if(hasBitSet((*i).value(), flags)){
			icons |= *i;
		}
	}

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

	return icons;
}

bool Condition::onBegin(Creature* creature)
{
	for(std::list<ConditionEffect>::iterator it = effectList.begin(); it != effectList.end(); ++it){
		if(!(*it).onBegin(creature)){
			return false;
		}
	}

	return true;
}

void Condition::onEnd(Creature* creature, ConditionEnd reason)
{
	for(std::list<ConditionEffect>::iterator it = effectList.begin(); it != effectList.end(); ++it){
		(*it).onEnd(creature, reason);
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
		std::list<ConditionEffect>::iterator curIt = effectList.begin();
		for(std::list<ConditionEffect>::const_iterator it = addCondition->effectList.begin(); it != addCondition->effectList.end(); ++it){
			if(!(*curIt).onUpdate(creature, *it)){
				fullUpdate = true;
				break;
			}

			++curIt;
		}
	}

	if(fullUpdate){
		//Condition has been changed, maybe from a script reload, doing a full update
		for(std::list<ConditionEffect>::iterator it = effectList.begin(); it != effectList.end(); ++it){
			(*it).onEnd(creature, CONDITIONEND_REMOVED);
		}
		effectList.clear();

		for(std::list<ConditionEffect>::const_iterator it = addCondition->effectList.begin(); it != addCondition->effectList.end(); ++it){
			addEffect(*it);
		}
	}

	return true;
}

bool Condition::onTick(Creature* creature, uint32_t interval)
{
	for(std::list<ConditionEffect>::iterator it = effectList.begin(); it != effectList.end(); ++it){
		if(!(*it).onTick(creature, interval)){
			return false;
		}
	}

	if(ticks != 0){
		if((((int64_t)ticks) - ((int64_t)interval)) <= 0){
			return false;
		}

		ticks -= interval;
	}

	return true;
}

void Condition::addEffect(ConditionEffect effect)
{
	effect.setOwner(this);
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
			std::string value;
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
			ConditionEffect effect;
			if(!effect.unserialize(propStream)){
				return false;
			}

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

	for(std::list<ConditionEffect>::iterator it = effectList.begin(); it != effectList.end(); ++it){
		ConditionEffect& effect = (*it);
		effect.serialize(propWriteStream);
	}

	return true;
}

int32_t ConditionEffect::getStatValue(Creature* creature, PlayerStatType statType, int32_t percent, int32_t value)
{
	if(percent != 0){
		switch(statType.value()){
			case enums::STAT_MAXHITPOINTS:
				return (int32_t)((float)creature->getMaxHealth() * (percent / 100.f));
			
			case enums::STAT_MAXMANAPOINTS:
				return (int32_t)((float)creature->getMaxMana() * (percent / 100.f));

			default:
				break;
		}

		if(Player* player = creature->getPlayer()){
			switch(statType.value()){
				case enums::STAT_SOULPOINTS:
					return (int32_t)((float)player->getPlayerInfo(PLAYERINFO_SOUL) * (percent / 100.f));
				case enums::STAT_MAGICPOINTS:
					return (int32_t)((float)player->getMagicLevel() * (percent / 100.f));
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

int32_t ConditionEffect::getSkillValue(Creature* creature, SkillType skillType, int32_t percent, int32_t value)
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

bool ConditionEffect::onBegin(Creature* creature)
{
	if(g_game.onConditionEffectBegin(creature, *this)){
		//handled by script
		return false;
	}

	switch(type){
		case ConditionEffect::PERIODIC_HEAL:
		case ConditionEffect::PERIODIC_DAMAGE:
		{
			ConditionEffect::ModPeriodicDamage& modPeriodicDamage = getModEffect<ConditionEffect::ModPeriodicDamage>();

			if(modPeriodicDamage.rounds == 0){
				//average damage
				modPeriodicDamage.total = random_range(modPeriodicDamage.min, modPeriodicDamage.max);
				modPeriodicDamage.percent = modPeriodicDamage.total / modPeriodicDamage.first;
				modPeriodicDamage.value = modPeriodicDamage.first;
			}
			else{
				//periodic damage
				modPeriodicDamage.value = random_range(modPeriodicDamage.min, modPeriodicDamage.max);
			}

			break;
		}

		case ConditionEffect::MOD_SPEED:
		{
			ConditionEffect::ModSpeed& modSpeed = getModEffect<ConditionEffect::ModSpeed>();
			int32_t delta = (int32_t)std::ceil(((float)creature->getBaseSpeed()) * ((float)modSpeed.percent) / 100 + modSpeed.value);
			g_game.changeSpeed(creature, delta);
			modSpeed.delta += delta;
			break;
		}

		case ConditionEffect::LIGHT:
		{
			const ConditionEffect::ModLight& modLight = getModEffect<const ConditionEffect::ModLight>();
			LightInfo lightInfo;
			lightInfo.level = modLight.level;
			lightInfo.color = modLight.color;
			creature->setCreatureLight(lightInfo);
			g_game.changeLight(creature);

			//set the interval for the periodic light changes
			interval = owner_condition->getTicks()/lightInfo.level;
			break;
		}

		case ConditionEffect::SHAPESHIFT:
		{
			const ConditionEffect::ModShapeShift& modShapeShift = getModEffect<const ConditionEffect::ModShapeShift>();
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

		case ConditionEffect::DISPEL:
		{
			const ConditionEffect::ModDispel& modDispel = getModEffect<const ConditionEffect::ModDispel>();
			CombatSource combatSource = owner_condition->combatSource;
			combatSource.setSourceIsCondition(true);
			creature->removeCondition(modDispel.name, combatSource);
			break;
		}

		default:
			break;
	}

	if(Player* player = creature->getPlayer()){
		switch(type){
			case ConditionEffect::MOD_STAT:
			{
				ConditionEffect::ModStat& modStat = getModEffect<ConditionEffect::ModStat>();
				int32_t delta = getStatValue(creature, modStat.type, modStat.percent, modStat.value);
				player->setVarStats(modStat.type, delta);
				player->sendStats();
				modStat.delta += delta;
				break;
			}
			
			case ConditionEffect::MOD_SKILL:
			{
				ConditionEffect::ModSkill& modSkill = getModEffect<ConditionEffect::ModSkill>();
				int32_t delta = getSkillValue(creature, modSkill.type, modSkill.percent, modSkill.value);
				player->setVarSkill(modSkill.type, delta);
				player->sendSkills();
				modSkill.delta += delta;
				break;
			}

			default:
				break;
		}
	}

	return true;
}

bool ConditionEffect::onEnd(Creature* creature, ConditionEnd reason)
{
	if(g_game.onConditionEffectEnd(creature, *this, reason)){
		//handled by script
		return false;
	}

	switch(type){
		case ConditionEffect::MOD_SPEED:
		{
			//revert our changes
			const ConditionEffect::ModSpeed& modSpeed = getModEffect<const ConditionEffect::ModSpeed>();
			g_game.changeSpeed(creature, -modSpeed.delta);
			break;
		}

		case ConditionEffect::LIGHT:
		{
			creature->setNormalCreatureLight();
			g_game.changeLight(creature);
			break;
		}

		case ConditionEffect::SHAPESHIFT:
		{
			g_game.internalCreatureChangeOutfit(creature, creature->getDefaultOutfit());
			break;
		}

		default:
			break;
	}

	if(Player* player = creature->getPlayer()){
		switch(type){
			case ConditionEffect::MOD_STAT:
			{
				//revert our changes
				const ConditionEffect::ModStat& modStat = getModEffect<const ConditionEffect::ModStat>();
				player->setVarStats(modStat.type, -modStat.delta);
				player->sendStats();
				break;
			}
			
			case ConditionEffect::MOD_SKILL:
			{
				//revert our changes
				const ConditionEffect::ModSkill& modSkill = getModEffect<const ConditionEffect::ModSkill>();
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

bool ConditionEffect::onUpdate(Creature* creature, const ConditionEffect& addEffect)
{
	if(type != addEffect.type){
		return false;
	}

	onEnd(creature, CONDITIONEND_UPDATE);

	data = addEffect.data;
	interval = addEffect.interval;

	return onBegin(creature);
}

bool ConditionEffect::onTick(Creature* creature, uint32_t ticks)
{
	if(interval == 0){
		return true;
	}

	tickCount += ticks;
	if(tickCount >= interval){
		tickCount = 0;

		switch(type){
			case ConditionEffect::PERIODIC_HEAL:
			case ConditionEffect::PERIODIC_DAMAGE:
			{
				ConditionEffect::ModPeriodicDamage& modPeriodicDamage = getModEffect<ConditionEffect::ModPeriodicDamage>();
				CombatSource combatSource = owner_condition->combatSource;
				combatSource.setSourceItem(NULL);
				combatSource.setSourceIsCondition(true);

				if(type == ConditionEffect::PERIODIC_HEAL){
					int32_t heal = modPeriodicDamage.value;
					//std::cout << "Healing " << heal << " to " << creature->getName() << std::endl;
					if(!g_game.combatBlockHit(COMBAT_HEALING, combatSource, creature, heal, false, false)){
						g_game.combatDamage(COMBAT_HEALING, combatSource, creature, heal);
					}
				}
				else{
					int32_t damage = -modPeriodicDamage.value;
					//std::cout << "Dealing " << damage << " "  << modPeriodicDamage.type.toString() << " to " << creature->getName() << std::endl;
					if(!g_game.combatBlockHit(modPeriodicDamage.type, combatSource, creature, damage, false, false)){
						g_game.combatDamage(modPeriodicDamage.type, combatSource, creature, damage);
					}
				}

				if(const MagicField* field = creature->getParentTile()->getFieldItem()){
					if(field->getCombatType() == modPeriodicDamage.type){
						//The creature is still standing in the field so the damage should
						//not be counted towards the total damage.
						return true;
					}
				}

				//average damage
				if(modPeriodicDamage.total != 0){
					//total damage done
					modPeriodicDamage.sum += modPeriodicDamage.first;

					//number of rounds done
					modPeriodicDamage.roundCompleted++;

					int32_t curRounds = (int32_t)std::ceil(((float)modPeriodicDamage.percent) / modPeriodicDamage.first);
					if(modPeriodicDamage.roundCompleted >= curRounds){
						modPeriodicDamage.roundCompleted = 0;
						--modPeriodicDamage.first;
						modPeriodicDamage.value = modPeriodicDamage.first;
					}

					if(modPeriodicDamage.sum >= modPeriodicDamage.total || modPeriodicDamage.first == 0){
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
				break;
			}

			case ConditionEffect::REGEN_HEALTH:
			case ConditionEffect::REGEN_MANA:
			case ConditionEffect::REGEN_SOUL:
			{
				if(creature->getZone() != ZONE_PROTECTION){
					ConditionEffect::ModRegen modRegen = getModEffect<ConditionEffect::ModRegen>();
					int32_t value = getStatValue(creature, modRegen.type, modRegen.percent, modRegen.value);
					//std::cout << "Regen " << value << " to " << creature->getName() << std::endl;
					if(type == ConditionEffect::REGEN_HEALTH){
						creature->changeHealth(value);
					}
					else if(type == ConditionEffect::REGEN_MANA){
						creature->changeMana(value);
					}
					else if(Player* player = creature->getPlayer()){
						player->changeSoul(value);
					}
				}
				break;
			}

			case ConditionEffect::LIGHT:
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

			case ConditionEffect::PERIODIC_MOD_STAMINA:
			{
				if(Player* player = creature->getPlayer()){
					ConditionEffect::ModPeriodicStamina modStamina = getModEffect<ConditionEffect::ModPeriodicStamina>();
					if(modStamina.value < 0){
						player->removeStamina(ticks * std::abs(modStamina.value));
					}
					else{
						player->addStamina(ticks * modStamina.value);
					}
				}
			}


			default:
				break;
		}

		if(g_game.onConditionEffectTick(creature, *this, ticks)){
			//handled by script
			return false;
		}

	}
	return true;
}

bool ConditionEffect::unserialize(PropStream& propStream)
{
	uint32_t value;
	if(!propStream.GET_VALUE(value)){
		return false;
	}
	
	type = ConditionEffect::Type(value);

	if(!propStream.GET_VALUE(interval)){
		return false;
	}

	//revision
	uint16_t revision;
	if(!propStream.GET_VALUE(revision)){
		return false;
	}

	switch(type){
		case ConditionEffect::PERIODIC_HEAL:
		case ConditionEffect::PERIODIC_DAMAGE:
		{
			if(revision != 1){
				return false;
			}
			
			ConditionEffect::ModPeriodicDamage mod;
			if(!propStream.GET_VALUE(value)){
				return false;
			}
			mod.type = CombatType::fromInteger(value);

			if(!propStream.GET_VALUE(mod.min)){
				return false;
			}
			if(!propStream.GET_VALUE(mod.max)){
				return false;
			}
			if(!propStream.GET_VALUE(mod.value)){
				return false;
			}
			if(!propStream.GET_VALUE(mod.total)){
				return false;
			}
			if(!propStream.GET_VALUE(mod.percent)){
				return false;
			}
			if(!propStream.GET_VALUE(mod.first)){
				return false;
			}
			if(!propStream.GET_VALUE(mod.rounds)){
				return false;
			}

			data = mod;
			break;
		}
		case ConditionEffect::PERIODIC_MOD_STAMINA:
		{
			if(revision != 1){
				return false;
			}
			ConditionEffect::ModPeriodicStamina mod;
			if(!propStream.GET_VALUE(mod.value)){
				return false;
			}
			data = mod;
			break;
		}
		case ConditionEffect::REGEN_HEALTH:
		case ConditionEffect::REGEN_MANA:
		case ConditionEffect::REGEN_SOUL:
		{
			if(revision != 1){
				return false;
			}
			uint32_t value;
			if(!propStream.GET_VALUE(value)){
				return false;
			}

			ConditionEffect::ModRegen mod;
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
		case ConditionEffect::MOD_SPEED:
		{
			if(revision != 1){
				return false;
			}
			ConditionEffect::ModSpeed mod;
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
		case ConditionEffect::MOD_STAT:
		{
			if(revision != 1){
				return false;
			}
			uint32_t value;
			if(!propStream.GET_VALUE(value)){
				return false;
			}

			ConditionEffect::ModStat mod;
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
		case ConditionEffect::MOD_SKILL:
		{
			if(revision != 1){
				return false;
			}
			uint32_t value;
			if(!propStream.GET_VALUE(value)){
				return false;
			}

			ConditionEffect::ModSkill mod;
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
		case ConditionEffect::SHAPESHIFT:
		{
			if(revision != 1){
				return false;
			}
			ConditionEffect::ModShapeShift mod;
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
		case ConditionEffect::LIGHT:
		{
			if(revision != 1){
				return false;
			}
			ConditionEffect::ModLight mod;
			if(!propStream.GET_VALUE(mod.level)){
				return false;
			}
			if(!propStream.GET_VALUE(mod.color)){
				return false;
			}
			data = mod;
			break;
		}
		case ConditionEffect::DISPEL:
		{
			if(revision != 1){
				return false;
			}
			ConditionEffect::ModDispel mod;
			if(!propStream.GET_STRING(mod.name)){
				return false;
			}
			data = mod;
			break;
		}

		case ConditionEffect::SCRIPT:
		{
			if(revision != 1){
				return false;
			}
			ConditionEffect::ModScript mod;
			if(!propStream.GET_STRING(mod.name)){
				return false;
			}
			data = mod;
			break;
		}
		default: return false;
	}

	return true;
}

bool ConditionEffect::serialize(PropWriteStream& propWriteStream)
{
	propWriteStream.ADD_UCHAR(*CONDITIONATTRIBUTE_EFFECT);
	propWriteStream.ADD_VALUE((uint32_t)type);
	propWriteStream.ADD_VALUE(interval);

	switch(type){
		case ConditionEffect::PERIODIC_HEAL:
		case ConditionEffect::PERIODIC_DAMAGE:
		{
			ConditionEffect::ModPeriodicDamage& mod = getModEffect<ConditionEffect::ModPeriodicDamage>();
			propWriteStream.ADD_USHORT(1); //revision
			propWriteStream.ADD_VALUE((uint32_t)mod.type.value());
			propWriteStream.ADD_VALUE(mod.min);
			propWriteStream.ADD_VALUE(mod.max);
			propWriteStream.ADD_VALUE(mod.value);
			propWriteStream.ADD_VALUE(mod.total);
			propWriteStream.ADD_VALUE(mod.percent);
			propWriteStream.ADD_VALUE(mod.first);
			propWriteStream.ADD_VALUE(mod.rounds);
			break;
		}
		case ConditionEffect::PERIODIC_MOD_STAMINA:
		{
			ConditionEffect::ModPeriodicStamina& mod = getModEffect<ConditionEffect::ModPeriodicStamina>();
			propWriteStream.ADD_USHORT(1); //revision
			propWriteStream.ADD_VALUE(mod.value);
			break;
		}
		case ConditionEffect::REGEN_HEALTH:
		case ConditionEffect::REGEN_MANA:
		case ConditionEffect::REGEN_SOUL:
		{
			ConditionEffect::ModRegen& mod = getModEffect<ConditionEffect::ModRegen>();
			propWriteStream.ADD_USHORT(1); //revision
			propWriteStream.ADD_VALUE((uint32_t)mod.type.value());
			propWriteStream.ADD_VALUE(mod.percent);
			propWriteStream.ADD_VALUE(mod.value);
			break;
		}
		case ConditionEffect::MOD_SPEED:
		{
			ConditionEffect::ModSpeed& mod = getModEffect<ConditionEffect::ModSpeed>();
			propWriteStream.ADD_USHORT(1); //revision
			propWriteStream.ADD_VALUE(mod.percent);
			propWriteStream.ADD_VALUE(mod.value);
			propWriteStream.ADD_VALUE(mod.delta);
			break;
		}
		case ConditionEffect::MOD_STAT:
		{
			ConditionEffect::ModStat& mod = getModEffect<ConditionEffect::ModStat>();
			propWriteStream.ADD_USHORT(1); //revision
			propWriteStream.ADD_VALUE((uint32_t)mod.type.value());
			propWriteStream.ADD_VALUE(mod.percent);
			propWriteStream.ADD_VALUE(mod.value);
			propWriteStream.ADD_VALUE(mod.delta);
			break;
		}
		case ConditionEffect::MOD_SKILL:
		{
			ConditionEffect::ModSkill& mod = getModEffect<ConditionEffect::ModSkill>();
			propWriteStream.ADD_USHORT(1); //revision
			propWriteStream.ADD_VALUE((uint32_t)mod.type.value());
			propWriteStream.ADD_VALUE(mod.percent);
			propWriteStream.ADD_VALUE(mod.value);
			propWriteStream.ADD_VALUE(mod.delta);
			break;
		}
		case ConditionEffect::SHAPESHIFT:
		{
			ConditionEffect::ModShapeShift& mod = getModEffect<ConditionEffect::ModShapeShift>();
			propWriteStream.ADD_USHORT(1); //revision
			propWriteStream.ADD_VALUE(mod.lookType);
			propWriteStream.ADD_VALUE(mod.lookTypeEx);
			propWriteStream.ADD_VALUE(mod.lookHead);
			propWriteStream.ADD_VALUE(mod.lookBody);
			propWriteStream.ADD_VALUE(mod.lookLegs);
			propWriteStream.ADD_VALUE(mod.lookFeet);
			propWriteStream.ADD_VALUE(mod.lookAddons);
			break;
		}
		case ConditionEffect::LIGHT:
		{
			ConditionEffect::ModLight& mod = getModEffect<ConditionEffect::ModLight>();
			propWriteStream.ADD_USHORT(1); //revision
			propWriteStream.ADD_VALUE(mod.level);
			propWriteStream.ADD_VALUE(mod.color);
			break;
		}
		case ConditionEffect::DISPEL:
		{
			ConditionEffect::ModDispel& mod = getModEffect<ConditionEffect::ModDispel>();
			propWriteStream.ADD_USHORT(1); //revision
			propWriteStream.ADD_STRING(mod.name);
			break;
		}
		case ConditionEffect::SCRIPT:
		{
			ConditionEffect::ModScript& mod = getModEffect<ConditionEffect::ModScript>();
			propWriteStream.ADD_USHORT(1); //revision
			propWriteStream.ADD_STRING(mod.name);
			break;
		}
		default: return false;
	}

	return true;
}
