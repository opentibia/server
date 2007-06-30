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
#include "creature.h"
#include "tools.h"

#include <utility>
#include <sstream>

extern Game g_game;

Condition::Condition(ConditionId_t _id, ConditionType_t _type, int32_t _ticks) :
id(_id),
ticks(_ticks),
conditionType(_type)
{
	//
}

bool Condition::setParam(ConditionParam_t param, int32_t value)
{
	switch(param){
		case CONDITIONPARAM_TICKS:
		{
			ticks = value;
			return true;
		}

		default:
		{
			return false;
		}
	}

	return false;
}

bool Condition::unserialize(xmlNodePtr p)
{
	return true;
}

xmlNodePtr Condition::serialize()
{
	xmlNodePtr nodeCondition = xmlNewNode(NULL,(const xmlChar*)"condition");

	std::stringstream ss;

	ss.str("");
	ss << (int32_t)conditionType;
	xmlSetProp(nodeCondition, (const xmlChar*)"type", (const xmlChar*)ss.str().c_str());

	ss.str("");
	ss << (int32_t)id;
	xmlSetProp(nodeCondition, (const xmlChar*)"id", (const xmlChar*)ss.str().c_str());

	ss.str("");
	ss << ticks;
	xmlSetProp(nodeCondition, (const xmlChar*)"ticks", (const xmlChar*)ss.str().c_str());
	return nodeCondition;
}

bool Condition::unserialize(PropStream& propStream)
{
	unsigned char attr_type;
	while(propStream.GET_UCHAR(attr_type) && attr_type != CONDITIONATTR_END){
		if(!unserializeProp((ConditionAttr_t)attr_type, propStream)){
			return false;
			break;
		}
	}

	return true;
}

bool Condition::unserializeProp(ConditionAttr_t attr, PropStream& propStream)
{
	switch(attr){
		case CONDITIONATTR_TYPE:
		{
			int32_t value = 0;
			if(!propStream.GET_VALUE(value)){
				return false;
			}

			conditionType = (ConditionType_t)value;
			return true;
			break;
		}

		case CONDITIONATTR_ID:
		{
			int32_t value = 0;
			if(!propStream.GET_VALUE(value)){
				return false;
			}

			id = (ConditionId_t)value;
			return true;
			break;
		}

		case CONDITIONATTR_TICKS:
		{
			int32_t value = 0;
			if(!propStream.GET_VALUE(value)){
				return false;
			}

			ticks = value;
			return true;
			break;
		}

		case CONDITIONATTR_END:
		{
			return true;
			break;
		}

		default:
			return false;
	}
}

bool Condition::serialize(PropWriteStream& propWriteStream)
{
	propWriteStream.ADD_UCHAR(CONDITIONATTR_TYPE);
	propWriteStream.ADD_VALUE((int32_t)conditionType);

	propWriteStream.ADD_UCHAR(CONDITIONATTR_ID);
	propWriteStream.ADD_VALUE((int32_t)id);

	propWriteStream.ADD_UCHAR(CONDITIONATTR_TICKS);
	propWriteStream.ADD_VALUE((int32_t)ticks);
	return true;
}

bool Condition::executeCondition(Creature* creature, int32_t interval)
{
	if(ticks != -1){
		setTicks(getTicks() - interval);
		return (getTicks() > 0);
	}

	return true;
}

Condition* Condition::createCondition(ConditionId_t _id, ConditionType_t _type, int32_t _ticks, int32_t param)
{
	switch((int32_t)_type){
		case CONDITION_POISON:
		case CONDITION_FIRE:
		case CONDITION_ENERGY:
		case CONDITION_DROWN:
		{
			return new ConditionDamage(_id, _type);
			break;
		}

		case CONDITION_HASTE:
		case CONDITION_PARALYZE:
		{
			return new ConditionSpeed(_id, _type, _ticks, param);
			break;
		}

		case CONDITION_INVISIBLE:
		{
			return new ConditionInvisible(_id, _type, _ticks);
			break;
		}

		case CONDITION_OUTFIT:
		{
			return new ConditionOutfit(_id, _type, _ticks);
			break;
		}

		case CONDITION_LIGHT:
		{
			return new ConditionLight(_id, _type, _ticks, param & 0xFF, (param & 0xFF00) >> 8);
			break;
		}

		case CONDITION_REGENERATION:
		{
			return new ConditionRegeneration(_id, _type, _ticks);
			break;
		}

		case CONDITION_SOUL:
		{
			return new ConditionSoul(_id, _type, _ticks);
			break;
		}

		case CONDITION_MANASHIELD:
		{
			return new ConditionManaShield(_id, _type,_ticks);
			break;
		}

		case CONDITION_ATTRIBUTES:
		{
			return new ConditionAttributes(_id, _type,_ticks);
			break;
		}

		case CONDITION_INFIGHT:
		case CONDITION_DRUNK:
		case CONDITION_EXHAUSTED:
		case CONDITION_MUTED:
		{
			return new ConditionGeneric(_id, _type,_ticks);
			break;
		}

		default:
		{
			return NULL;
			break;
		}
	}
}


Condition* Condition::createCondition(PropStream& propStream)
{
	uint8_t attr;
	
	if(!propStream.GET_UCHAR(attr) || attr != CONDITIONATTR_TYPE){
		return NULL;
	}

	uint32_t _type = 0;
	if(!propStream.GET_ULONG(_type)){
		return NULL;
	}

	if(!propStream.GET_UCHAR(attr) || attr != CONDITIONATTR_ID){
		return NULL;
	}

	uint32_t _id = 0;
	if(!propStream.GET_ULONG(_id)){
		return NULL;
	}

	if(!propStream.GET_UCHAR(attr) || attr != CONDITIONATTR_TICKS){
		return NULL;
	}

	uint32_t _ticks = 0;
	if(!propStream.GET_ULONG(_ticks)){
		return NULL;
	}

	return createCondition((ConditionId_t)_id, (ConditionType_t)_type, _ticks, 0);
}

bool Condition::isPersistent() const
{
	if(ticks != -1 && (id == CONDITIONID_DEFAULT || id == CONDITIONID_COMBAT)){
		return true;
	}

	return false;
}

ConditionGeneric::ConditionGeneric(ConditionId_t _id, ConditionType_t _type, int32_t _ticks) :
Condition(_id, _type, _ticks)
{
	//
}

bool ConditionGeneric::startCondition(Creature* creature)
{
	return true;
}

bool ConditionGeneric::executeCondition(Creature* creature, int32_t interval)
{
	return Condition::executeCondition(creature, interval);

	/*
	bool bRemove = false;
	creature->onTickCondition(getType(), bRemove);

	if(bRemove){
		setTicks(0);
	}
	*/
}

void ConditionGeneric::endCondition(Creature* creature, ConditionEnd_t reason)
{
	//
}

void ConditionGeneric::addCondition(Creature* creature, const Condition* addCondition)
{
	if(addCondition->getType() == conditionType){
		if(addCondition->getTicks() > ticks){
			ticks = addCondition->getTicks();
		}
	}
}

uint32_t ConditionGeneric::getIcons() const
{
	switch(conditionType){
		case CONDITION_MANASHIELD:
			return ICON_MANASHIELD;
			break;
		
		case CONDITION_INFIGHT:
			return ICON_SWORDS;
			break;
		
		case CONDITION_DRUNK:
			return ICON_DRUNK;
			break;
		
		case CONDITION_EXHAUSTED:
			break;
		
		default:
			break;
	}

	return 0;
}

ConditionAttributes::ConditionAttributes(ConditionId_t _id, ConditionType_t _type, int32_t _ticks) :
	ConditionGeneric(_id, _type, _ticks)
{
	currentSkill = 0;
	currentStat = 0;
	memset(skills, 0, sizeof(skills));
	memset(stats, 0, sizeof(stats));
	memset(statsPercent, 0, sizeof(statsPercent));	
}

void ConditionAttributes::addCondition(Creature* creature, const Condition* addCondition)
{
	if(addCondition->getType() == conditionType){
		if(addCondition->getTicks() > ticks){
			ticks = addCondition->getTicks();
		}
		
		const ConditionAttributes& conditionAttrs = static_cast<const ConditionAttributes&>(*addCondition);

		if(Player* player = creature->getPlayer()){
			for(int32_t i = SKILL_FIRST; i <= SKILL_LAST; ++i){
				player->setVarSkill((skills_t)i, -skills[i]);
			}

			for(int32_t i = STAT_FIRST; i <= STAT_LAST; ++i){
				player->setVarStats((stats_t)i, -stats[i]);
			}
		}
		
		memcpy(skills, conditionAttrs.skills, sizeof(skills));
		memcpy(stats, conditionAttrs.stats, sizeof(stats));
		memcpy(statsPercent, conditionAttrs.statsPercent, sizeof(statsPercent));

		if(Player* player = creature->getPlayer()){
			updateSkills(player);

			updatePercentStats(player);
			updateStats(player);
		}
	}
}

xmlNodePtr ConditionAttributes::serialize()
{
	xmlNodePtr nodeCondition = Condition::serialize();

	std::stringstream ss;
	std::stringstream ss2;

	std::string str = "skill";
	for(int32_t i = SKILL_FIRST; i <= SKILL_LAST; ++i){
		ss.str("");
		ss << skills[i];
		ss2.str("");
		ss2 << str << i;
		xmlSetProp(nodeCondition, (const xmlChar*)ss2.str().c_str(), (const xmlChar*)ss.str().c_str());
	}

	str = "stat";
	for(int32_t i = STAT_FIRST; i <= STAT_LAST; ++i){
		ss.str("");
		ss << stats[i];
		ss2.str("");
		ss2 << str << i;
		xmlSetProp(nodeCondition, (const xmlChar*)ss2.str().c_str(), (const xmlChar*)ss.str().c_str());
	}

	return nodeCondition;
}

bool ConditionAttributes::unserialize(xmlNodePtr p)
{
	if(!Condition::unserialize(p)){
		return false;
	}

	std::string str = "skill";
	std::stringstream ss;
	int intValue;

	for(int32_t i = SKILL_FIRST; i <= SKILL_LAST; ++i){
		ss.str("");
		ss << str << i;
		if(readXMLInteger(p, ss.str().c_str(), intValue)){
			skills[i] = intValue;
		}
	}

	str = "stat";
	for(int32_t i = STAT_FIRST; i <= STAT_LAST; ++i){
		ss.str("");
		ss << str << i;
		if(readXMLInteger(p, ss.str().c_str(), intValue)){
			stats[i] = intValue;
		}
	}

	return true;
}

bool ConditionAttributes::unserializeProp(ConditionAttr_t attr, PropStream& propStream)
{
	if(attr == CONDITIONATTR_SKILLS){
		int32_t value = 0;
		if(!propStream.GET_VALUE(value)){
			return false;
		}

		skills[currentSkill] = value;
		++currentSkill;
		return true;
	}
	else if(attr == CONDITIONATTR_STATS){
		int32_t value = 0;
		if(!propStream.GET_VALUE(value)){
			return false;
		}

		stats[currentStat] = value;
		++currentStat;

		return true;
	}

	return Condition::unserializeProp(attr, propStream);
}

bool ConditionAttributes::serialize(PropWriteStream& propWriteStream)
{
	if(!Condition::serialize(propWriteStream)){
		return false;
	}

	for(int32_t i = SKILL_FIRST; i <= SKILL_LAST; ++i){
		propWriteStream.ADD_UCHAR(CONDITIONATTR_SKILLS);
		propWriteStream.ADD_VALUE(skills[i]);
	}

	for(int32_t i = STAT_FIRST; i <= STAT_LAST; ++i){
		propWriteStream.ADD_UCHAR(CONDITIONATTR_STATS);
		propWriteStream.ADD_VALUE(stats[i]);
	}

	return true;
}


bool ConditionAttributes::startCondition(Creature* creature)
{
	if(Player* player = creature->getPlayer()){
		updateSkills(player);
		updatePercentStats(player);
		updateStats(player);
	}

	return true;
}

void ConditionAttributes::updatePercentStats(Player* player)
{
	for(int32_t i = STAT_FIRST; i <= STAT_LAST; ++i){
		if(statsPercent[i] == 0){
			continue;
		}

		switch(i){
			case STAT_MAXHITPOINTS:
				stats[i] = player->getMaxHealth() * ((statsPercent[i] - 100) / 100.0);
			break;

			case STAT_MAXMANAPOINTS:
				stats[i] = player->getMaxMana() * ((statsPercent[i] - 100) / 100.0);
			break;

			case STAT_SOULPOINTS:
				stats[i] = player->getPlayerInfo(PLAYERINFO_SOUL) * ((statsPercent[i] - 100) / 100.0);
			break;

			case STAT_MAGICPOINTS:
				stats[i] = player->getMagicLevel() * ((statsPercent[i] - 100) / 100.0);
			break;
		}
	}
}

void ConditionAttributes::updateSkills(Player* player)
{
	for(int32_t i = SKILL_FIRST; i <= SKILL_LAST; ++i){
		player->setVarSkill((skills_t)i, skills[i]);
	}

	player->sendSkills();
}

void ConditionAttributes::updateStats(Player* player)
{
	for(int32_t i = STAT_FIRST; i <= STAT_LAST; ++i){
		player->setVarStats((stats_t)i, stats[i]);
	}

	player->sendStats();
}

bool ConditionAttributes::executeCondition(Creature* creature, int32_t interval)
{
	return ConditionGeneric::executeCondition(creature, interval);
}

void ConditionAttributes::endCondition(Creature* creature, ConditionEnd_t reason)
{
	if(Player* player = creature->getPlayer()){
		for(int32_t i = SKILL_FIRST; i <= SKILL_LAST; ++i){
			player->setVarSkill((skills_t)i, -skills[i]);
		}

		player->sendSkills();
	}

	if(Player* player = creature->getPlayer()){
		for(int32_t i = STAT_FIRST; i <= STAT_LAST; ++i){
			player->setVarStats((stats_t)i, -stats[i]);
		}

		player->sendStats();
	}
}

bool ConditionAttributes::setParam(ConditionParam_t param, int32_t value)
{
	bool ret = ConditionGeneric::setParam(param, value);

	switch(param){
		case CONDITIONPARAM_SKILL_MELEE:
		{
			skills[SKILL_CLUB] = value;
			skills[SKILL_AXE] = value;
			skills[SKILL_SWORD] = value;
			return true;
			break;
		}

		case CONDITIONPARAM_SKILL_FIST:
		{
			skills[SKILL_FIST] = value;
			return true;
			break;
		}

		case CONDITIONPARAM_SKILL_CLUB:
		{
			skills[SKILL_CLUB] = value;
			return true;
			break;
		}

		case CONDITIONPARAM_SKILL_SWORD:
		{
			skills[SKILL_SWORD] = value;
			return true;
			break;
		}

		case CONDITIONPARAM_SKILL_AXE:
		{
			skills[SKILL_AXE] = value;
			return true;
			break;
		}

		case CONDITIONPARAM_SKILL_DISTANCE:
		{
			skills[SKILL_DIST] = value;
			return true;
			break;
		}

		case CONDITIONPARAM_SKILL_SHIELD:
		{
			skills[SKILL_SHIELD] = value;
			return true;
			break;
		}

		case CONDITIONPARAM_SKILL_FISHING:
		{
			skills[SKILL_FISH] = value;
			return true;
			break;
		}

		case CONDITIONPARAM_STAT_MAXHITPOINTS:
		{
			stats[STAT_MAXHITPOINTS] = value;
			return true;
			break;
		}

		case CONDITIONPARAM_STAT_MAXMANAPOINTS:
		{
			stats[STAT_MAXMANAPOINTS] = value;
			return true;
			break;
		}

		case CONDITIONPARAM_STAT_SOULPOINTS:
		{
			stats[STAT_SOULPOINTS] = value;
			return true;
			break;
		}

		case CONDITIONPARAM_STAT_MAGICPOINTS:
		{
			stats[STAT_MAGICPOINTS] = value;
			return true;
			break;
		}

		case CONDITIONPARAM_STAT_MAXHITPOINTSPERCENT:
		{
			if(value < 0){
				value = 0;
			}

			statsPercent[STAT_MAXHITPOINTS] = value;
			return true;
			break;
		}

		case CONDITIONPARAM_STAT_MAXMANAPOINTSPERCENT:
		{
			if(value < 0){
				value = 0;
			}

			statsPercent[STAT_MAXMANAPOINTS] = value;
			return true;
			break;
		}

		case CONDITIONPARAM_STAT_SOULPOINTSPERCENT:
		{
			if(value < 0){
				value = 0;
			}

			statsPercent[STAT_SOULPOINTS] = value;
			return true;
			break;
		}

		case CONDITIONPARAM_STAT_MAGICPOINTSPERCENT:
		{
			if(value < 0){
				value = 0;
			}

			statsPercent[STAT_MAGICPOINTS] = value;
			return true;
			break;
		}

		default:
		{
			return false;
		}
	}

	return ret;
}

ConditionRegeneration::ConditionRegeneration(ConditionId_t _id, ConditionType_t _type, int32_t _ticks) :
	ConditionGeneric(_id, _type, _ticks)
{
	internalHealthTicks = 0;
	internalManaTicks = 0;

	healthTicks = 1000;
	manaTicks = 1000;

	healthGain = 0;
	manaGain = 0;
}

void ConditionRegeneration::addCondition(Creature* creature, const Condition* addCondition)
{
	if(addCondition->getType() == conditionType){
		if(addCondition->getTicks() > ticks){
			ticks = addCondition->getTicks();
		}
		
		const ConditionRegeneration& conditionRegen = static_cast<const ConditionRegeneration&>(*addCondition);

		healthTicks = conditionRegen.healthTicks;
		manaTicks = conditionRegen.manaTicks;

		healthGain = conditionRegen.healthGain;
		manaGain = conditionRegen.manaGain;
	}
}

xmlNodePtr ConditionRegeneration::serialize()
{
	xmlNodePtr nodeCondition = Condition::serialize();

	std::stringstream ss;

	ss.str("");
	ss << healthTicks;
	xmlSetProp(nodeCondition, (const xmlChar*)"healthticks", (const xmlChar*)ss.str().c_str());

	ss.str("");
	ss << healthGain;
	xmlSetProp(nodeCondition, (const xmlChar*)"healthgain", (const xmlChar*)ss.str().c_str());

	ss.str("");
	ss << manaTicks;
	xmlSetProp(nodeCondition, (const xmlChar*)"manaticks", (const xmlChar*)ss.str().c_str());

	ss.str("");
	ss << manaGain;
	xmlSetProp(nodeCondition, (const xmlChar*)"managain", (const xmlChar*)ss.str().c_str());
	return nodeCondition;
}

bool ConditionRegeneration::unserialize(xmlNodePtr p)
{
	if(!Condition::unserialize(p)){
		return false;
	}

	int intValue;

	if(readXMLInteger(p, "healthticks", intValue)){
		healthTicks = intValue;
	}

	if(readXMLInteger(p, "healthgain", intValue)){
		healthGain = intValue;
	}

	if(readXMLInteger(p, "manaticks", intValue)){
		manaTicks = intValue;
	}

	if(readXMLInteger(p, "managain", intValue)){
		manaGain = intValue;
	}

	return true;
}

bool ConditionRegeneration::unserializeProp(ConditionAttr_t attr, PropStream& propStream)
{
	if(attr == CONDITIONATTR_HEALTHTICKS){
		uint32_t value = 0;
		if(!propStream.GET_VALUE(value)){
			return false;
		}

		healthTicks = value;
		return true;
	}
	else if(attr == CONDITIONATTR_HEALTHGAIN){
		uint32_t value = 0;
		if(!propStream.GET_VALUE(value)){
			return false;
		}

		healthGain = value;
		return true;
	}
	else if(attr == CONDITIONATTR_MANATICKS){
		uint32_t value = 0;
		if(!propStream.GET_VALUE(value)){
			return false;
		}

		manaTicks = value;
		return true;
	}
	else if(attr == CONDITIONATTR_MANAGAIN){
		uint32_t value = 0;
		if(!propStream.GET_VALUE(value)){
			return false;
		}

		manaGain = value;
		return true;
	}

	return Condition::unserializeProp(attr, propStream);
}

bool ConditionRegeneration::serialize(PropWriteStream& propWriteStream)
{
	if(!Condition::serialize(propWriteStream)){
		return false;
	}

	propWriteStream.ADD_UCHAR(CONDITIONATTR_HEALTHTICKS);
	propWriteStream.ADD_VALUE(healthTicks);

	propWriteStream.ADD_UCHAR(CONDITIONATTR_HEALTHGAIN);
	propWriteStream.ADD_VALUE(healthGain);

	propWriteStream.ADD_UCHAR(CONDITIONATTR_MANATICKS);
	propWriteStream.ADD_VALUE(manaTicks);

	propWriteStream.ADD_UCHAR(CONDITIONATTR_MANAGAIN);
	propWriteStream.ADD_VALUE(manaGain);
	return true;
}

bool ConditionRegeneration::executeCondition(Creature* creature, int32_t interval)
{
	internalHealthTicks += interval;
	internalManaTicks += interval;

	if(!creature->isInPz()){
		if(internalHealthTicks >= healthTicks){
			internalHealthTicks = 0;
			creature->changeHealth(healthGain);
		}

		if(internalManaTicks >= manaTicks){
			internalManaTicks = 0;
			creature->changeMana(manaGain);
		}
	}

	return ConditionGeneric::executeCondition(creature, interval);
}

bool ConditionRegeneration::setParam(ConditionParam_t param, int32_t value)
{
	bool ret = ConditionGeneric::setParam(param, value);

	switch(param){
		case CONDITIONPARAM_HEALTHGAIN:
		{
			healthGain = value;
			return true;
			break;
		}

		case CONDITIONPARAM_HEALTHTICKS:
		{
			healthTicks = value;
			return true;
			break;
		}

		case CONDITIONPARAM_MANAGAIN:
		{
			manaGain = value;
			return true;
			break;
		}

		case CONDITIONPARAM_MANATICKS:
		{
			manaTicks = value;
			return true;
			break;
		}

		default:
		{
			return false;
		}
	}

	return ret;
}

ConditionSoul::ConditionSoul(ConditionId_t _id, ConditionType_t _type, int32_t _ticks) :
	ConditionGeneric(_id, _type, _ticks)
{
	internalSoulTicks = 0;
	soulTicks = 0;
	soulGain = 0;
}

void ConditionSoul::addCondition(Creature* creature, const Condition* addCondition)
{
	if(addCondition->getType() == conditionType){
		if(addCondition->getTicks() > ticks){
			ticks = addCondition->getTicks();
		}
		
		const ConditionSoul& conditionSoul = static_cast<const ConditionSoul&>(*addCondition);

		soulTicks = conditionSoul.soulTicks;
		soulGain = conditionSoul.soulGain;
	}
}

xmlNodePtr ConditionSoul::serialize()
{
	xmlNodePtr nodeCondition = Condition::serialize();

	std::stringstream ss;

	ss.str("");
	ss << soulGain;
	xmlSetProp(nodeCondition, (const xmlChar*)"soulgain", (const xmlChar*)ss.str().c_str());

	ss.str("");
	ss << soulTicks;
	xmlSetProp(nodeCondition, (const xmlChar*)"soulticks", (const xmlChar*)ss.str().c_str());

	return nodeCondition;
}

bool ConditionSoul::unserialize(xmlNodePtr p)
{
	if(!Condition::unserialize(p)){
		return false;
	}

	int intValue;

	if(readXMLInteger(p, "soulgain", intValue)){
		soulGain = intValue;
	}

	if(readXMLInteger(p, "soulticks", intValue)){
		soulTicks = intValue;
	}

	return true;
}

bool ConditionSoul::unserializeProp(ConditionAttr_t attr, PropStream& propStream)
{
	if(attr == CONDITIONATTR_SOULGAIN){
		uint32_t value = 0;
		if(!propStream.GET_VALUE(value)){
			return false;
		}

		soulGain = value;
		return true;
	}
	else if(attr == CONDITIONATTR_SOULTICKS){
		uint32_t value = 0;
		if(!propStream.GET_VALUE(value)){
			return false;
		}

		soulTicks = value;
		return true;
	}

	return Condition::unserializeProp(attr, propStream);
}

bool ConditionSoul::serialize(PropWriteStream& propWriteStream)
{
	if(!Condition::serialize(propWriteStream)){
		return false;
	}

	propWriteStream.ADD_UCHAR(CONDITIONATTR_SOULGAIN);
	propWriteStream.ADD_VALUE(soulGain);

	propWriteStream.ADD_UCHAR(CONDITIONATTR_SOULTICKS);
	propWriteStream.ADD_VALUE(soulTicks);

	return true;
}

bool ConditionSoul::executeCondition(Creature* creature, int32_t interval)
{
	internalSoulTicks += interval;

	if(Player* player = creature->getPlayer()){
		if(!player->isInPz()){
			if(internalSoulTicks >= soulTicks){
				internalSoulTicks = 0;
				player->changeSoul(soulGain);
			}
		}
	}

	return ConditionGeneric::executeCondition(creature, interval);
}

bool ConditionSoul::setParam(ConditionParam_t param, int32_t value)
{
	bool ret = ConditionGeneric::setParam(param, value);

	switch(param){
		case CONDITIONPARAM_SOULGAIN:
		{
			soulGain = value;
			return true;
			break;
		}

		case CONDITIONPARAM_SOULTICKS:
		{
			soulTicks = value;
			return true;
			break;
		}

		default:
		{
			return false;
		}
	}

	return ret;
}

ConditionDamage::ConditionDamage(ConditionId_t _id, ConditionType_t _type) :
Condition(_id, _type, 0)
{
 	delayed = false;
	forceUpdate = false;
	owner = 0;
	minDamage = 0;
	maxDamage = 0;
	startDamage = 0;
	tickInterval = 2000;
}

bool ConditionDamage::setParam(ConditionParam_t param, int32_t value)
{
	bool ret = Condition::setParam(param, value);

	switch(param){
		case CONDITIONPARAM_OWNER:
		{
			owner = value;
			return true;
			break;
		}

		case CONDITIONPARAM_FORCEUPDATE:
		{
			forceUpdate = (value != 0);
			return true;
			break;
		}		

		case CONDITIONPARAM_DELAYED:
		{
			delayed = (value != 0);
			return true;
			break;
		}

		case CONDITIONPARAM_MAXVALUE:
		{
			maxDamage = std::abs(value);
			break;
		}

		case CONDITIONPARAM_MINVALUE:
		{
			minDamage = std::abs(value);
			break;
		}

		case CONDITIONPARAM_STARTVALUE:
		{
			startDamage = std::abs(value);
			break;
		}

		case CONDITIONPARAM_TICKINTERVAL:
		{
			tickInterval = std::abs(value);
			break;
		}

		default:
		{
			return false;
			break;
		}
	}

	return ret;
}

xmlNodePtr ConditionDamage::serialize()
{
	xmlNodePtr nodeCondition = Condition::serialize();

	std::stringstream ss;

	ss.str("");
	ss << delayed;
	xmlSetProp(nodeCondition, (const xmlChar*)"delayed", (const xmlChar*)ss.str().c_str());

	ss.str("");
	ss << owner;
	xmlSetProp(nodeCondition, (const xmlChar*)"owner", (const xmlChar*)ss.str().c_str());

	for(DamageList::const_iterator it = damageList.begin(); it != damageList.end(); ++it){
		xmlNodePtr nodeValueListNode = xmlNewNode(NULL, (const xmlChar*)"damage");
		
		ss.str("");
		ss << (*it).timeLeft;
		xmlSetProp(nodeValueListNode, (const xmlChar*)"duration", (const xmlChar*)ss.str().c_str());

		ss.str("");
		ss << (*it).value;
		xmlSetProp(nodeValueListNode, (const xmlChar*)"value", (const xmlChar*)ss.str().c_str());

		ss.str("");
		ss << (*it).interval;
		xmlSetProp(nodeValueListNode, (const xmlChar*)"interval", (const xmlChar*)ss.str().c_str());

		xmlAddChild(nodeCondition, nodeValueListNode);
	}

	return nodeCondition;
}

bool ConditionDamage::unserialize(xmlNodePtr p)
{
	if(!Condition::unserialize(p)){
		return false;
	}

	setTicks(0);

	int intValue;

	if(readXMLInteger(p, "delayed", intValue)){
		delayed = (intValue == 1);
	}

	if(readXMLInteger(p, "owner", intValue)){
		owner = intValue;
	}

	xmlNodePtr nodeList = p->children;
	while(nodeList){
		if(xmlStrcmp(nodeList->name, (const xmlChar*)"damage") == 0){

			IntervalInfo damageInfo;
			damageInfo.interval = 0;
			damageInfo.timeLeft = 0;
			damageInfo.value = 0;

			if(readXMLInteger(nodeList, "duration", intValue)){
				damageInfo.timeLeft = intValue;
			}

			if(readXMLInteger(nodeList, "value", intValue)){
				damageInfo.value = intValue;
			}

			if(readXMLInteger(nodeList, "interval", intValue)){
				damageInfo.interval = intValue;
			}

			setTicks(getTicks() + damageInfo.interval);
			damageList.push_back(damageInfo);
		}

		nodeList = nodeList->next;
	}

	return true;
}

bool ConditionDamage::unserializeProp(ConditionAttr_t attr, PropStream& propStream)
{
	if(attr == CONDITIONATTR_DELAYED){
		bool value = false;
		if(!propStream.GET_VALUE(value)){
			return false;
		}

		delayed = value;
		return true;
	}
	else if(attr == CONDITIONATTR_OWNER){
		uint32_t value = 0;
		if(!propStream.GET_VALUE(value)){
			return false;
		}

		owner = value;
		return true;
	}
	else if(attr == CONDITIONATTR_INTERVALDATA){
		IntervalInfo damageInfo;
		if(!propStream.GET_VALUE(damageInfo)){
			return false;
		}

		setTicks(getTicks() + damageInfo.interval);
		damageList.push_back(damageInfo);
		return true;
	}

	return Condition::unserializeProp(attr, propStream);
}

bool ConditionDamage::serialize(PropWriteStream& propWriteStream)
{
	if(!Condition::serialize(propWriteStream)){
		return false;
	}

	propWriteStream.ADD_UCHAR(CONDITIONATTR_DELAYED);
	propWriteStream.ADD_VALUE(delayed);

	propWriteStream.ADD_UCHAR(CONDITIONATTR_OWNER);
	propWriteStream.ADD_VALUE(owner);

	for(DamageList::const_iterator it = damageList.begin(); it != damageList.end(); ++it){
		propWriteStream.ADD_UCHAR(CONDITIONATTR_INTERVALDATA);
		propWriteStream.ADD_VALUE((*it));
	}

	return true;
}

void ConditionDamage::addDamage(uint32_t rounds, uint32_t time, int32_t value)
{
	//rounds, time, damage
	for(unsigned int i = 0; i < rounds; ++i){
		IntervalInfo damageInfo;
		damageInfo.interval = time;
		damageInfo.timeLeft = time;
		damageInfo.value = value;
		
		damageList.push_back(damageInfo);
		ticks += time;
	}
}

bool ConditionDamage::init()
{
	if(damageList.empty()){
		setTicks(0);

		int32_t amount = random_range(minDamage, maxDamage);

		if(amount != 0){
			if(startDamage > maxDamage){
				startDamage = maxDamage;
			}
			else if(startDamage == 0){
				startDamage = std::max((int32_t)1, (int32_t)std::ceil(((float)amount / 20.0)));
			}

			std::list<int32_t> list;
			ConditionDamage::generateDamageList(amount, startDamage, list);

			for(std::list<int32_t>::iterator it = list.begin(); it != list.end(); ++it){
				addDamage(1, tickInterval, -*it);
			}
		}
	}

	return (!damageList.empty());
}
bool ConditionDamage::startCondition(Creature* creature)
{
	if(!init()){
		return false;
	}

	if(!delayed){
		int32_t damage = 0;
		if(getNextDamage(damage)){
			return doDamage(creature, damage);
		}
	}

	return true;

	/*
	if(delayed){
		return true;
	}

	int32_t damage = 0;
	if(getNextDamage(damage)){
		return doDamage(creature, damage);
	}

	return false;
	*/
}

bool ConditionDamage::executeCondition(Creature* creature, int32_t interval)
{
	if(!damageList.empty()){
		IntervalInfo& damageInfo = damageList.front();

		bool bRemove = true;
		creature->onTickCondition(getType(), bRemove);
		damageInfo.timeLeft -= interval;

		if(damageInfo.timeLeft <= 0){
			int32_t damage = damageInfo.value;

			if(bRemove){
				damageList.pop_front();
			}
			else{
				//restore timeLeft
				damageInfo.timeLeft = damageInfo.interval;
			}

			doDamage(creature, damage);
		}
		
		if(!bRemove){
			interval = 0;
		}
	}

	return Condition::executeCondition(creature, interval);
}

bool ConditionDamage::getNextDamage(int32_t& damage)
{
	if(!damageList.empty()){
		IntervalInfo& damageInfo = damageList.front();

		damage = damageInfo.value;
		damageList.pop_front();
		return true;
	}

	return false;
}

bool ConditionDamage::doDamage(Creature* creature, int32_t damage)
{
	if(creature->isSuppress(getType())){
		return true;
	}

	CombatType_t combatType = COMBAT_NONE;

	switch(conditionType){
		case CONDITION_FIRE:
			combatType = COMBAT_FIREDAMAGE;
			break;

		case CONDITION_ENERGY:
			combatType = COMBAT_ENERGYDAMAGE;
			break;

		case CONDITION_DROWN:
			combatType = COMBAT_DROWNDAMAGE;
			break;

		case CONDITION_POISON:
			combatType = COMBAT_POISONDAMAGE;
			break;
		
		default:
			break;
	}

	Creature* attacker = g_game.getCreatureByID(owner);

	if(g_game.combatBlockHit(combatType, attacker, creature, damage, false, false)){
		return false;
	}

	return g_game.combatChangeHealth(combatType, attacker, creature, damage);
}

void ConditionDamage::endCondition(Creature* creature, ConditionEnd_t reason)
{
	//
}

void ConditionDamage::addCondition(Creature* creature, const Condition* addCondition)
{
	if(addCondition->getType() == conditionType){
		const ConditionDamage& conditionDamage = static_cast<const ConditionDamage&>(*addCondition);

		if(conditionDamage.doForceUpdate() || conditionDamage.getTotalDamage() > getTotalDamage()){
			ticks = addCondition->getTicks();
			owner = conditionDamage.owner;
			maxDamage = conditionDamage.maxDamage;
			minDamage = conditionDamage.minDamage;
			startDamage = conditionDamage.startDamage;
			tickInterval = conditionDamage.tickInterval;
			int32_t nextTimeLeft = tickInterval;

			if(!damageList.empty()){
				//save previous timeLeft
				IntervalInfo& damageInfo = damageList.front();
				nextTimeLeft = damageInfo.timeLeft;
				damageList.clear();
			}

			damageList = conditionDamage.damageList;
			
			if(init()){
				if(!damageList.empty()){
					//restore last timeLeft
					IntervalInfo& damageInfo = damageList.front();
					damageInfo.timeLeft = nextTimeLeft;
				}

				if(!delayed){
					int32_t damage = 0;
					if(getNextDamage(damage)){
						doDamage(creature, damage);
					}
				}
			}
		}
	}
}

int32_t ConditionDamage::getTotalDamage() const
{
	int32_t result = 0;

	if(!damageList.empty()){
		for(DamageList::const_iterator it = damageList.begin(); it != damageList.end(); ++it){
			result += it->value;
		}
	}
	else{
		result = maxDamage + (maxDamage - minDamage) / 2;
	}

	return std::abs(result);
}

uint32_t ConditionDamage::getIcons() const
{
	switch(conditionType){
		case CONDITION_FIRE:
			return ICON_BURN;
			break;

		case CONDITION_ENERGY:
			return ICON_ENERGY;
			break;

		case CONDITION_DROWN:
			return ICON_DROWNING;
			break;

		case CONDITION_POISON:
			return ICON_POISON;
			break;
			
		default:
			break;
	}

	return 0;
}

void ConditionDamage::generateDamageList(int32_t amount, int32_t start, std::list<int32_t>& list)
{
	amount = std::abs(amount);
	int32_t sum = 0;
	int32_t med = 0;
	float x1, x2;

	for(int32_t i = start; i > 0; --i){
		int32_t n = start + 1 - i;
		med = (n * amount) / start;

		do{
			sum += i;
			list.push_back(i);

			x1 = std::fabs(1.0 - (((float)sum) + i) / med);
			x2 = std::fabs(1.0 - (((float)sum) / med));

		}while(x1 < x2);
	}
}

ConditionSpeed::ConditionSpeed(ConditionId_t _id, ConditionType_t _type, int32_t _ticks, int32_t changeSpeed) :
Condition(_id, _type, _ticks)
{
	speedDelta = changeSpeed;
	mina = 0.0f;
	minb = 0.0f;
	maxa = 0.0f;
	maxb = 0.0f;
}

void ConditionSpeed::setFormulaVars(float _mina, float _minb, float _maxa, float _maxb)
{
	mina = _mina;
	minb = _minb;
	maxa = _maxa;
	maxb = _maxb;
}

void ConditionSpeed::getFormulaValues(int32_t var, int32_t& min, int32_t& max) const
{
	min = (int32_t)std::ceil(var * 1.f * mina + minb);
	max = (int32_t)std::ceil(var * 1.f * maxa + maxb);
}

bool ConditionSpeed::setParam(ConditionParam_t param, int32_t value)
{
	bool ret = Condition::setParam(param, value);

	switch(param){
		case CONDITIONPARAM_SPEED:
		{
			speedDelta = value;
			if(value > 0){
				conditionType = CONDITION_HASTE;
			}
			else{
				conditionType = CONDITION_PARALYZE;
			}

			return true;
		}
		default:
		{
			return false;
		}
	}

	return ret;
}

xmlNodePtr ConditionSpeed::serialize()
{
	xmlNodePtr nodeCondition = Condition::serialize();

	std::stringstream ss;

	ss.str("");
	ss << speedDelta;
	xmlSetProp(nodeCondition, (const xmlChar*)"delta", (const xmlChar*)ss.str().c_str());

	ss.str("");
	ss << mina;
	xmlSetProp(nodeCondition, (const xmlChar*)"mina", (const xmlChar*)ss.str().c_str());

	ss.str("");
	ss << minb;
	xmlSetProp(nodeCondition, (const xmlChar*)"minb", (const xmlChar*)ss.str().c_str());

	ss.str("");
	ss << maxa;
	xmlSetProp(nodeCondition, (const xmlChar*)"maxa", (const xmlChar*)ss.str().c_str());

	ss.str("");
	ss << maxb;
	xmlSetProp(nodeCondition, (const xmlChar*)"maxb", (const xmlChar*)ss.str().c_str());

	return nodeCondition;
}

bool ConditionSpeed::unserialize(xmlNodePtr p)
{
	if(!Condition::unserialize(p)){
		return false;
	}

	int intValue;

	if(readXMLInteger(p, "delta", intValue)){
		speedDelta = intValue;
	}

	float floatValue;

	if(readXMLFloat(p, "mina", floatValue)){
		mina = floatValue;
	}

	if(readXMLFloat(p, "minb", floatValue)){
		minb = floatValue;
	}

	if(readXMLFloat(p, "maxa", floatValue)){
		maxa = floatValue;
	}

	if(readXMLFloat(p, "maxb", floatValue)){
		maxb = floatValue;
	}

	return true;
}

bool ConditionSpeed::unserializeProp(ConditionAttr_t attr, PropStream& propStream)
{
	if(attr == CONDITIONATTR_SPEEDDELTA){
		int32_t value = 0;
		if(!propStream.GET_VALUE(value)){
			return false;
		}

		speedDelta = value;
		return true;
	}
	else if(attr == CONDITIONATTR_FORMULA_MINA){
		float value = 0;
		if(!propStream.GET_VALUE(value)){
			return false;
		}

		mina = value;
		return true;
	}
	else if(attr == CONDITIONATTR_FORMULA_MINB){
		float value = 0;
		if(!propStream.GET_VALUE(value)){
			return false;
		}

		minb = value;
		return true;
	}
	else if(attr == CONDITIONATTR_FORMULA_MAXA){
		float value = 0;
		if(!propStream.GET_VALUE(value)){
			return false;
		}

		maxa = value;
		return true;
	}
	else if(attr == CONDITIONATTR_FORMULA_MAXB){
		float value = 0;
		if(!propStream.GET_VALUE(value)){
			return false;
		}

		maxb = value;
		return true;
	}

	return Condition::unserializeProp(attr, propStream);
}

bool ConditionSpeed::serialize(PropWriteStream& propWriteStream)
{
	if(!Condition::serialize(propWriteStream)){
		return false;
	}

	propWriteStream.ADD_UCHAR(CONDITIONATTR_SPEEDDELTA);
	propWriteStream.ADD_VALUE(speedDelta);

	propWriteStream.ADD_UCHAR(CONDITIONATTR_FORMULA_MINA);
	propWriteStream.ADD_VALUE(mina);

	propWriteStream.ADD_UCHAR(CONDITIONATTR_FORMULA_MINB);
	propWriteStream.ADD_VALUE(minb);

	propWriteStream.ADD_UCHAR(CONDITIONATTR_FORMULA_MAXA);
	propWriteStream.ADD_VALUE(maxa);

	propWriteStream.ADD_UCHAR(CONDITIONATTR_FORMULA_MAXB);
	propWriteStream.ADD_VALUE(maxb);

	return true;
}

bool ConditionSpeed::startCondition(Creature* creature)
{
	if(speedDelta == 0){
		int32_t min;
		int32_t max;
		getFormulaValues(creature->getBaseSpeed(), min, max);
		speedDelta = random_range(min, max);
	}

	g_game.changeSpeed(creature, speedDelta);
	return true;
}

bool ConditionSpeed::executeCondition(Creature* creature, int32_t interval)
{
	return Condition::executeCondition(creature, interval);
}

void ConditionSpeed::endCondition(Creature* creature, ConditionEnd_t reason)
{
	g_game.changeSpeed(creature, -speedDelta);
}

void ConditionSpeed::addCondition(Creature* creature, const Condition* addCondition)
{
	if(addCondition->getType() == conditionType){
		if(addCondition->getTicks() > ticks){
			ticks = addCondition->getTicks();
		}

		const ConditionSpeed& conditionSpeed = static_cast<const ConditionSpeed&>(*addCondition);
		int32_t oldSpeedDelta = speedDelta;
		speedDelta = conditionSpeed.speedDelta;
		mina = conditionSpeed.mina;
		maxa = conditionSpeed.maxa;
		minb = conditionSpeed.minb;
		maxb = conditionSpeed.maxb;

		if(speedDelta == 0){
			int32_t min;
			int32_t max;
			getFormulaValues(creature->getBaseSpeed(), min, max);
			speedDelta = random_range(min, max);
		}
		
		int32_t newSpeedChange = (speedDelta - oldSpeedDelta);
		if(newSpeedChange != 0){
			g_game.changeSpeed(creature, newSpeedChange);
		}
	}
}

uint32_t ConditionSpeed::getIcons() const
{
	switch(conditionType){
		case CONDITION_HASTE:
			return ICON_HASTE;
			break;

		case CONDITION_PARALYZE:
			return ICON_PARALYZE;
			break;
		
		default:
			break;
	}

	return 0;
}

ConditionInvisible::ConditionInvisible(ConditionId_t _id, ConditionType_t _type, int32_t _ticks) :
ConditionGeneric(_id, _type, _ticks)
{
	//
}

bool ConditionInvisible::startCondition(Creature* creature)
{
	g_game.internalCreatureChangeVisible(creature, false);
	return true;
}

void ConditionInvisible::endCondition(Creature* creature, ConditionEnd_t reason)
{
	if(!creature->isInvisible()){
		g_game.internalCreatureChangeVisible(creature, true);
	}
}

ConditionOutfit::ConditionOutfit(ConditionId_t _id, ConditionType_t _type, int32_t _ticks) :
Condition(_id, _type, _ticks)
{
	//
}

void ConditionOutfit::addOutfit(Outfit_t outfit)
{
	outfits.push_back(outfit);
}

xmlNodePtr ConditionOutfit::serialize()
{
	xmlNodePtr nodeCondition = Condition::serialize();

	//serialize outfit

	return nodeCondition;
}

bool ConditionOutfit::unserialize(xmlNodePtr p)
{
	if(!Condition::unserialize(p)){
		return false;
	}

	//unserialize outfit

	return true;
}

bool ConditionOutfit::unserializeProp(ConditionAttr_t attr, PropStream& propStream)
{
	/*
	if(attr == CONDITIONATTR_OUTFIT){
		int32_t value = 0;
		if(!propStream.GET_VALUE(value)){
			return false;
		}

		outfit = value;
		return true;
	}
	*/

	return Condition::unserializeProp(attr, propStream);
}

bool ConditionOutfit::serialize(PropWriteStream& propWriteStream)
{
	if(!Condition::serialize(propWriteStream)){
		return false;
	}

	//TODO: serialize outfits

	return true;
}

bool ConditionOutfit::startCondition(Creature* creature)
{
	changeOutfit(creature);
	return true;
}

bool ConditionOutfit::executeCondition(Creature* creature, int32_t interval)
{
	return Condition::executeCondition(creature, interval);
}

void ConditionOutfit::changeOutfit(Creature* creature, int32_t index /*= -1*/)
{
	if(!outfits.empty()){
		if(index == -1){
			index = random_range(0, outfits.size() - 1);
		}

		Outfit_t outfit = outfits[index];
		g_game.internalCreatureChangeOutfit(creature, outfit);
	}
}

void ConditionOutfit::endCondition(Creature* creature, ConditionEnd_t reason)
{
	g_game.internalCreatureChangeOutfit(creature, creature->getDefaultOutfit());
}

void ConditionOutfit::addCondition(Creature* creature, const Condition* addCondition)
{
	if(addCondition->getType() == conditionType){
		if(addCondition->getTicks() > ticks){
			ticks = addCondition->getTicks();
		}

		const ConditionOutfit& conditionOutfit = static_cast<const ConditionOutfit&>(*addCondition);
		outfits = conditionOutfit.outfits;

		changeOutfit(creature);
	}
}

uint32_t ConditionOutfit::getIcons() const
{
	return 0;
}

ConditionLight::ConditionLight(ConditionId_t _id, ConditionType_t _type, int32_t _ticks, int32_t _lightlevel, int32_t _lightcolor) :
Condition(_id, _type, _ticks)
{
	lightInfo.level = _lightlevel;
	lightInfo.color = _lightcolor;
	internalLightTicks = 0;
	lightChangeInterval = 0;
}
	
bool ConditionLight::startCondition(Creature* creature)
{
	internalLightTicks = 0;
	lightChangeInterval = ticks/lightInfo.level;
	creature->setCreatureLight(lightInfo);
	g_game.changeLight(creature);
	return true;
}

bool ConditionLight::executeCondition(Creature* creature, int32_t interval)
{
	internalLightTicks += interval;
	if(internalLightTicks >= lightChangeInterval){
		internalLightTicks = 0;
		LightInfo creatureLight;
		creature->getCreatureLight(creatureLight);
		if(creatureLight.level > 0){
			--creatureLight.level;
			creature->setCreatureLight(creatureLight);
			g_game.changeLight(creature);
		}
	}
	return Condition::executeCondition(creature, interval);
}

void ConditionLight::endCondition(Creature* creature, ConditionEnd_t reason)
{
	creature->setNormalCreatureLight();
	g_game.changeLight(creature);
}

void ConditionLight::addCondition(Creature* creature, const Condition* addCondition)
{
	if(addCondition->getType() == conditionType){
		const ConditionLight& conditionLight = static_cast<const ConditionLight&>(*addCondition);
		
		//replace old light values with the new ones
		
		lightInfo.level = conditionLight.lightInfo.level;
		lightInfo.color = conditionLight.lightInfo.color;
		uint32_t addTicks = conditionLight.getTicks();
		lightChangeInterval = addTicks/lightInfo.level;
		internalLightTicks = 0;
		setTicks(addTicks);
		creature->setCreatureLight(lightInfo);
		g_game.changeLight(creature);
	}
}

bool ConditionLight::setParam(ConditionParam_t param, int32_t value)
{
	bool ret = Condition::setParam(param, value);
	if(!ret){
		switch(param){
		case CONDITIONPARAM_LIGHT_LEVEL:
			lightInfo.level = value;
			return true;
			break;
		case CONDITIONPARAM_LIGHT_COLOR:
			lightInfo.color = value;
			return true;
			break;
		default:
			return false;
		}
	}
	return false;
}

xmlNodePtr ConditionLight::serialize()
{
	xmlNodePtr nodeCondition = Condition::serialize();

	std::stringstream ss;

	ss.str("");
	ss << lightInfo.color;
	xmlSetProp(nodeCondition, (const xmlChar*)"lightcolor", (const xmlChar*)ss.str().c_str());

	ss.str("");
	ss << lightInfo.level;
	xmlSetProp(nodeCondition, (const xmlChar*)"lightlevel", (const xmlChar*)ss.str().c_str());

	ss.str("");
	ss << internalLightTicks;
	xmlSetProp(nodeCondition, (const xmlChar*)"lightticks", (const xmlChar*)ss.str().c_str());

	ss.str("");
	ss << lightChangeInterval;
	xmlSetProp(nodeCondition, (const xmlChar*)"lightinterval", (const xmlChar*)ss.str().c_str());

	return nodeCondition;
}

bool ConditionLight::unserialize(xmlNodePtr p)
{
	if(!Condition::unserialize(p)){
		return false;
	}

	int intValue;

	if(readXMLInteger(p, "lightcolor", intValue)){
		lightInfo.color = intValue;
	}

	if(readXMLInteger(p, "lightlevel", intValue)){
		lightInfo.level = intValue;
	}

	if(readXMLInteger(p, "lightticks", intValue)){
		internalLightTicks = intValue;
	}

	if(readXMLInteger(p, "lightinterval", intValue)){
		lightChangeInterval = intValue;
	}

	return true;
}

bool ConditionLight::unserializeProp(ConditionAttr_t attr, PropStream& propStream)
{
	if(attr == CONDITIONATTR_LIGHTCOLOR){
		uint32_t value = 0;
		if(!propStream.GET_VALUE(value)){
			return false;
		}

		lightInfo.color = value;
		return true;
	}
	else if(attr == CONDITIONATTR_LIGHTLEVEL){
		uint32_t value = 0;
		if(!propStream.GET_VALUE(value)){
			return false;
		}

		lightInfo.level = value;
		return true;
	}
	else if(attr == CONDITIONATTR_LIGHTTICKS){
		uint32_t value = 0;
		if(!propStream.GET_VALUE(value)){
			return false;
		}

		internalLightTicks = value;
		return true;
	}
	else if(attr == CONDITIONATTR_LIGHTINTERVAL){
		uint32_t value = 0;
		if(!propStream.GET_VALUE(value)){
			return false;
		}

		lightChangeInterval = value;
		return true;
	}

	return Condition::unserializeProp(attr, propStream);
}

bool ConditionLight::serialize(PropWriteStream& propWriteStream)
{
	if(!Condition::serialize(propWriteStream)){
		return false;
	}

	propWriteStream.ADD_UCHAR(CONDITIONATTR_LIGHTCOLOR);
	propWriteStream.ADD_VALUE(lightInfo.color);

	propWriteStream.ADD_UCHAR(CONDITIONATTR_LIGHTLEVEL);
	propWriteStream.ADD_VALUE(lightInfo.level);

	propWriteStream.ADD_UCHAR(CONDITIONATTR_LIGHTTICKS);
	propWriteStream.ADD_VALUE(internalLightTicks);

	propWriteStream.ADD_UCHAR(CONDITIONATTR_LIGHTINTERVAL);
	propWriteStream.ADD_VALUE(lightChangeInterval);

	return true;
}

uint32_t ConditionLight::getIcons() const
{
	return 0;
}
