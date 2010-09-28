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

#include "creature_manager.h"
#include "creature_type.h"
#include "condition.h"
#include "tools.h"
#include "item.h"
#include "game.h"

extern Game g_game;

CreatureManager::CreatureManager()
{
	loaded = false;
}

CreatureManager::~CreatureManager()
{
}

bool CreatureManager::loadFromXml(const std::string& _datadir, bool reloading /*= false*/)
{
	loaded = false;
	datadir = _datadir;

	std::string filename = datadir + "monster/monsters.xml";

	xmlDocPtr doc = xmlParseFile(filename.c_str());
	if(doc){
		loaded = true;
		xmlNodePtr root, p;
		root = xmlDocGetRootElement(doc);

		if(xmlStrcmp(root->name,(const xmlChar*)"monsters") != 0){
			xmlFreeDoc(doc);
			loaded = false;
			return false;
		}

		p = root->children;
		while(p){
			if(p->type != XML_ELEMENT_NODE){
				p = p->next;
				continue;
			}

			if(xmlStrcmp(p->name, (const xmlChar*)"monster") == 0){
				std::string file;
				std::string name;

				if(readXMLString(p, "file", file) && readXMLString(p, "name", name)){
					file = datadir + "monster/" + file;
					loadMonsterType(file, name, reloading);
				}
			}
			else{
				std::cout << "Warning: [CreatureDatabase::loadFromXml]. Unknown node name. " << p->name << std::endl;
			}
			p = p->next;
		}

		xmlFreeDoc(doc);
	}

	return loaded;
}

bool CreatureManager::configureSpells()
{
	for(TypeMap::iterator iter = creature_types.begin(); iter != creature_types.end(); ++iter){
		CreatureType* cType = iter->second;
		for(SpellList::const_iterator it = cType->spellAttackList().begin(), spell_attack_list_end = cType->spellAttackList().end(); it != spell_attack_list_end; ++it){
			g_game.onActorLoadSpell(*it);
		}

		for(SpellList::const_iterator it = cType->spellDefenseList().begin(), spell_defense_list_end = cType->spellDefenseList().end(); it != spell_defense_list_end; ++it){
			g_game.onActorLoadSpell(*it);
		}
	}

	return true;
}
bool CreatureManager::reload()
{
	if(loadFromXml(datadir, true)){
		configureSpells();
		return true;
	}

	return false;
}

bool CreatureManager::deserializeSpell(xmlNodePtr node, SpellBlock& sb)
{
	std::string name = "";
	if(!readXMLString(node, "name", name)){
		return false;
	}

	int intValue;
	std::string strValue;
	if(readXMLInteger(node, "speed", intValue) || readXMLInteger(node, "interval", intValue)){
		sb.speed = std::max(1, intValue);
	}

	if(readXMLInteger(node, "chance", intValue)){
		if(intValue < 0 || intValue > 100){
			intValue = 100;
		}

		sb.chance = intValue;
	}

	if(readXMLInteger(node, "range", intValue)){
		if(intValue < 0 ){
			intValue = 0;
		}

		if(intValue > Map::maxViewportX * 2){
			intValue = Map::maxViewportX * 2;
		}

		sb.range = intValue;
	}

	if(readXMLInteger(node, "min", intValue)){
		sb.min = std::abs(intValue);
	}

	if(readXMLInteger(node, "max", intValue)){
		sb.max = std::abs(intValue);
	}

	//normalize values
	if(std::abs(sb.min) > std::abs(sb.max)){
		std::swap(sb.max, sb.min);
	}

	if(readXMLInteger(node, "length", intValue)){
		sb.length = intValue;

		if(sb.length > 0){
			sb.spread = 3;

			//need direction spell
			if(readXMLInteger(node, "spread", intValue)){
				sb.spread = std::max(0, intValue);
			}
		}
	}
	else if(readXMLInteger(node, "radius", intValue)){
		sb.radius = intValue;

		//target spell
		if(readXMLInteger(node, "target", intValue)){
			sb.needTarget = (intValue != 0);
		}
	}

	sb.needTarget = sb.needTarget || (sb.radius == 0 && sb.length == 0);

	if(asLowerCaseString(name) == "melee"){
		int32_t attackValue = 0;
		int32_t attackSkill = 0;
		if(readXMLInteger(node, "attack", attackValue)){
			if(readXMLInteger(node, "skill", attackSkill)){
				sb.min = 0;
				sb.max = (int32_t)std::ceil((attackSkill * (attackValue * 0.05)) + (attackValue * 0.5));
			}
		}

		sb.range = 1;
		sb.blockedByShield = true;
		sb.blockedByArmor = true;
		sb.needTarget = true;
		sb.damageType = COMBAT_PHYSICALDAMAGE;

		uint32_t interval = 10000;
		int32_t startDamage = 10;
		int32_t totalDamage = 0;

		CombatType conditionDamageType = COMBAT_NONE;
		if(readXMLInteger(node, "fire", intValue)){
			conditionDamageType = COMBAT_FIREDAMAGE;
			sb.condition.type = CONDITION_BURNING;
		}
		else if(readXMLInteger(node, "poison", intValue)){
			conditionDamageType = COMBAT_EARTHDAMAGE;
			sb.condition.type = CONDITION_POISONED;
			totalDamage = intValue;
			startDamage = 5;
			interval = 5000;
		}
		else if(readXMLInteger(node, "energy", intValue)){
			conditionDamageType = COMBAT_ENERGYDAMAGE;
			sb.condition.type = CONDITION_ELECTRIFIED;
			startDamage = 20;
			totalDamage = intValue;
		}
		else if(readXMLInteger(node, "drown", intValue)){
			conditionDamageType = COMBAT_DROWNDAMAGE;
			sb.condition.type = CONDITION_DROWNING;
			totalDamage = intValue;
		}
		else if(readXMLInteger(node, "freeze", intValue)){
			conditionDamageType = COMBAT_ICEDAMAGE;
			sb.condition.type = CONDITION_FREEZING;
			totalDamage = intValue;
		}
		else if(readXMLInteger(node, "dazzle", intValue)){
			conditionDamageType = COMBAT_HOLYDAMAGE;
			sb.condition.type = CONDITION_DAZZLED;
			totalDamage = intValue;
		}
		else if(readXMLInteger(node, "curse", intValue)){
			conditionDamageType = COMBAT_DEATHDAMAGE;
			sb.condition.type = CONDITION_CURSED;
			totalDamage = intValue;
		}

		if(readXMLInteger(node, "tick", intValue) && intValue > 0){
			interval = intValue;
		}

		if(sb.condition.type != CONDITION_NONE){
			sb.condition.interval = interval;
			sb.condition.effect = ConditionEffect::createPeriodicDamage(interval, conditionDamageType, totalDamage, totalDamage, startDamage);
		}
	}
	else if(asLowerCaseString(name) == "physical"){
		sb.damageType = COMBAT_PHYSICALDAMAGE;
		sb.blockedByArmor = true;
	}
	else if(asLowerCaseString(name) == "poison" || asLowerCaseString(name) == "earth"){
		sb.damageType = COMBAT_EARTHDAMAGE;
	}
	else if(asLowerCaseString(name) == "fire"){
		sb.damageType = COMBAT_FIREDAMAGE;
	}
	else if(asLowerCaseString(name) == "energy"){
		sb.damageType = COMBAT_ENERGYDAMAGE;
	}
	else if(asLowerCaseString(name) == "drown"){
		sb.damageType = COMBAT_DROWNDAMAGE;
	}
	else if(asLowerCaseString(name) == "ice"){
		sb.damageType = COMBAT_ICEDAMAGE;
	}
	else if(asLowerCaseString(name) == "holy"){
		sb.damageType = COMBAT_HOLYDAMAGE;
	}
	else if(asLowerCaseString(name) == "death"){
		sb.damageType = COMBAT_DEATHDAMAGE;
	}
	else if(asLowerCaseString(name) == "lifedrain"){
		sb.damageType = COMBAT_LIFEDRAIN;
	}
	else if(asLowerCaseString(name) == "manadrain"){
		sb.damageType = COMBAT_MANADRAIN;
	}
	else if(asLowerCaseString(name) == "healing"){
		sb.damageType = COMBAT_HEALING;
	}
	else if(asLowerCaseString(name) == "speed" || asLowerCaseString(name) == "paralyze"){
		int32_t speedChange = 0;
		sb.condition.duration = 10000;

		if(readXMLInteger(node, "duration", intValue)){
			sb.condition.duration = intValue;
		}

		if(readXMLInteger(node, "speedchange", intValue)){
			speedChange = intValue;

			if(speedChange < -1000){
				//cant be slower than 100%
				speedChange = -1000;
			}
		}

		if(speedChange > 0){
			sb.condition.type = CONDITION_HASTE;
			sb.aggressive = false;
		}
		else{
			sb.condition.type = CONDITION_PARALYZED;
		}

		sb.condition.effect = ConditionEffect::createModSpeed(std::floor(speedChange / 10.0), 0);
	}
	else if(asLowerCaseString(name) == "outfit"){
		int32_t duration = 10000;

		if(readXMLInteger(node, "duration", intValue)){
			duration = intValue;
		}

		if(readXMLString(node, "monster", strValue)){
			CreatureType* cType = getMonsterType(strValue);
			if(cType){
				sb.condition.type = CONDITION_SHAPESHIFT;
				sb.condition.duration = duration;
				sb.condition.effect = ConditionEffect::createModShapeShift(cType->outfit());
				sb.aggressive = false;
			}
		}
		else if(readXMLInteger(node, "item", intValue)){
			OutfitType outfit;
			outfit.lookTypeEx = intValue;
		
			sb.condition.type = CONDITION_SHAPESHIFT;
			sb.condition.duration = duration;
			sb.condition.effect = ConditionEffect::createModShapeShift(outfit);
			sb.aggressive = false;
		}
	}
	else if(asLowerCaseString(name) == "invisible"){
		sb.condition.type = CONDITION_INVISIBLE;
		sb.condition.duration = 10000;
		sb.aggressive = false;

		if(readXMLInteger(node, "duration", intValue)){
			sb.condition.duration = intValue;
		}

		//sb.condition.effect = ConditionEffect::createModScript("invisible");
	}
	else if(asLowerCaseString(name) == "drunk"){
		sb.condition.type = CONDITION_DRUNK;
		//sb.condition.effect = ConditionEffect::createModDrunk();
		sb.condition.duration = 10000;

		if(readXMLInteger(node, "duration", intValue)){
			sb.condition.duration = intValue;
		}

		//sb.condition.effect = ConditionEffect::createModScript("drunk");
	}
	/*
	else if(asLowerCaseString(name) == "firefield"){
		sb.field = ITEM_FIREFIELD_PVP;
	}
	else if(asLowerCaseString(name) == "poisonfield"){
		sb.field = ITEM_POISONFIELD_PVP;
	}
	else if(asLowerCaseString(name) == "energyfield"){
		sb.field = ITEM_ENERGYFIELD_PVP;
	}
	*/
	else if(asLowerCaseString(name) == "firecondition" ||
			asLowerCaseString(name) == "poisoncondition" ||
			asLowerCaseString(name) == "energycondition" ||
			asLowerCaseString(name) == "drowncondition"){
		sb.condition.type = CONDITION_NONE;

		CombatType conditionDamageType = COMBAT_NONE;
		uint32_t interval = 2000;
		if(name == "firecondition"){
			conditionDamageType = COMBAT_FIREDAMAGE;
			sb.condition.type = CONDITION_BURNING;
			interval = 10000;
		}
		else if(name == "poisoncondition"){
			conditionDamageType = COMBAT_EARTHDAMAGE;
			sb.condition.type = CONDITION_POISONED;
			interval = 5000;
		}
		else if(name == "energycondition"){
			conditionDamageType = COMBAT_ENERGYDAMAGE;
			sb.condition.type = CONDITION_ELECTRIFIED;
			interval = 10000;
		}
		else if(name == "drowncondition"){
			conditionDamageType = COMBAT_DROWNDAMAGE;
			sb.condition.type = CONDITION_DROWNING;
			interval = 5000;
		}

		if(readXMLInteger(node, "tick", intValue) && intValue > 0){
			interval = intValue;
		}

		int32_t minDamage = std::abs(sb.min);
		int32_t maxDamage = std::abs(sb.max);
		int32_t startDamage = 0;

		if(readXMLInteger(node, "start", intValue)){
			intValue = std::abs(intValue);

			if(intValue <= minDamage){
				startDamage = intValue;
			}
		}

		sb.condition.interval = interval;
		sb.condition.effect = ConditionEffect::createPeriodicDamage(interval, conditionDamageType, minDamage, maxDamage, startDamage);
	}
	else if(asLowerCaseString(name) == "strength" || asLowerCaseString(name) == "effect"){
		//
	}
	else{
		sb.configureSpell = false;
		return true;
	}

	xmlNodePtr attributeNode = node->children;
	while(attributeNode){
		if(xmlStrcmp(attributeNode->name, (const xmlChar*)"attribute") == 0){
			if(readXMLString(attributeNode, "key", strValue)){
				if(asLowerCaseString(strValue) == "shooteffect"){
					if(readXMLString(attributeNode, "value", strValue)){
						try{
							sb.shootEffect = ShootEffect::fromString(asLowerCaseString(strValue));
							//
						} catch(enum_conversion_error& e){
							std::cout << "Warning: [CreatureManager::deserializeSpell]. "  << sb.name << " - Unknown shootEffect " << e.what() << std::endl;
						}
					}
				}
				else if(asLowerCaseString(strValue) == "areaeffect"){
					if(readXMLString(attributeNode, "value", strValue)){
						try{
							sb.areaEffect = MagicEffect::fromString(asLowerCaseString(strValue));
							//
						} catch(enum_conversion_error& e){
							std::cout << "Warning: [CreatureManager::deserializeSpell]. "  << sb.name << " - Unknown areaEffect " << e.what() << std::endl;
						}
					}
				}
			}
		}

		attributeNode = attributeNode->next;
	}

	return true;
}

#define SHOW_XML_WARNING(desc) std::cout << "Warning: [CreatureManager::loadMonster]. " << desc << ". " << file << std::endl;
#define SHOW_XML_ERROR(desc) std::cout << "Error: [CreatureManager::loadMonster]. " << desc << ". " << file << std::endl;

bool CreatureManager::loadMonsterType(const std::string& file, const std::string& monster_name, bool reloading /*= false*/)
{
	if(file.empty())
		return false;

	bool monsterLoad;
	CreatureType* mType = NULL;

	// If another creature has been loaded from the same file before (reload?)
	TypeMap::iterator iter = creature_types.begin();
	while(iter != creature_types.end()){
		if(iter->second->fileLoaded() == file){
			mType = iter->second;
			break;
		}
		++iter;
	}

	// If not, create a new type
	if(!mType)
		mType = new CreatureType();

	mType->fileLoaded (file);

	monsterLoad = true;
	xmlDocPtr doc = xmlParseFile(file.c_str());

	if(doc){
		xmlNodePtr root, p;
		root = xmlDocGetRootElement(doc);

		if(xmlStrcmp(root->name,(const xmlChar*)"monster") != 0){
			std::cerr << "Malformed XML: " << file << std::endl;
		}

		int intValue;
		std::string strValue;

		p = root->children;

		if(readXMLString(root, "name", strValue)){
			mType->name(strValue);
		}
		else
			monsterLoad = false;

		if(readXMLString(root, "nameDescription", strValue))
			mType->nameDescription(strValue);
		else
			mType->nameDescription(asLowerCaseString("a " + mType->name()));

		if(readXMLString(root, "race", strValue)){
			toLowerCaseString(strValue);
			if((strValue == "venom") || (atoi(strValue.c_str()) == enums::RACE_VENOM)){
				mType->race(RACE_VENOM);
			}
			else if((strValue == "blood") || (atoi(strValue.c_str()) == enums::RACE_BLOOD)){
				mType->race(RACE_BLOOD);
			}
			else if((strValue == "undead") || (atoi(strValue.c_str()) == enums::RACE_UNDEAD)){
				mType->race(RACE_UNDEAD);
			}
			else if((strValue == "fire") || (atoi(strValue.c_str()) == enums::RACE_FIRE)){
				mType->race(RACE_FIRE);
			}
			else if((strValue == "energy") || (atoi(strValue.c_str()) == enums::RACE_ENERGY)){
				mType->race(RACE_ENERGY);
			}
			else{
				SHOW_XML_WARNING("Unknown race type " << strValue);
			}
		}

		if(readXMLInteger(root, "experience", intValue)){
			mType->experience(intValue);
		}

		if(readXMLInteger(root, "speed", intValue)){
			mType->base_speed(intValue);
		}

		if(readXMLInteger(root, "manacost", intValue)){
			mType->manaCost(intValue);
		}

		while(p){
			if(p->type != XML_ELEMENT_NODE){
				p = p->next;
				continue;
			}

			if(xmlStrcmp(p->name, (const xmlChar*)"health") == 0){

				if(readXMLInteger(p, "now", intValue)){
					mType->health(intValue);
				}
				else{
					SHOW_XML_ERROR("Missing health.now");
					monsterLoad = false;
				}

				if(readXMLInteger(p, "max", intValue)){
					mType->health_max(intValue);
				}
				else{
					SHOW_XML_ERROR("Missing health.max");
					monsterLoad = false;
				}
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"flags") == 0){
				xmlNodePtr tmpNode = p->children;
				while(tmpNode){
					if(xmlStrcmp(tmpNode->name, (const xmlChar*)"flag") == 0){

						if(readXMLInteger(tmpNode, "summonable", intValue)){
							mType->isSummonable(intValue != 0);
						}

						if(readXMLInteger(tmpNode, "attackable", intValue)){
							mType->isAttackable(intValue != 0);
						}

						if(readXMLInteger(tmpNode, "hostile", intValue)){
							mType->isHostile(intValue != 0);
						}

						if(readXMLInteger(tmpNode, "illusionable", intValue)){
							mType->isIllusionable(intValue != 0);
						}

						if(readXMLInteger(tmpNode, "convinceable", intValue)){
							mType->isConvinceable(intValue != 0);
						}

						if(readXMLInteger(tmpNode, "pushable", intValue)){
							mType->pushable(intValue != 0);
						}

						if(readXMLInteger(tmpNode, "canpushitems", intValue)){
							mType->canPushItems(intValue != 0);
						}

						if(readXMLInteger(tmpNode, "canpushcreatures", intValue)){
							mType->canPushCreatures(intValue != 0);
						}

						if(readXMLInteger(tmpNode, "staticattack", intValue)){
							if(intValue < 0){
								SHOW_XML_WARNING("staticattack lower than 0");
								intValue = 0;
							}

							if(intValue > 100){
								SHOW_XML_WARNING("staticattack greater than 100");
								intValue = 100;
							}

							mType->staticAttackChance(intValue);
						}

						if(readXMLInteger(tmpNode, "lightlevel", intValue)){
							mType->lightLevel(intValue);
						}

						if(readXMLInteger(tmpNode, "lightcolor", intValue)){
							mType->lightColor(intValue);
						}

						if(readXMLInteger(tmpNode, "targetdistance", intValue)){
							/*if(intValue > 6){
								SHOW_XML_WARNING("targetdistance greater than 6");
							}*/
							mType->targetDistance(std::max(1, intValue));
						}

						if(readXMLInteger(tmpNode, "runonhealth", intValue)){
							mType->fleeHealth(intValue);
						}
						
						if(readXMLInteger(tmpNode, "lureable", intValue)){
							mType->isLureable(intValue != 0);
						}
					}

					tmpNode = tmpNode->next;
				}
				//if a monster can push creatures,
				// it should not be pushable
				if(mType->canPushCreatures() && mType->pushable()){
					mType->pushable(false);
				}
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"targetchange") == 0){

				if(readXMLInteger(p, "speed", intValue) || readXMLInteger(p, "interval", intValue)){
					mType->changeTargetSpeed(std::max(1, intValue));
				}
				else{
					SHOW_XML_WARNING("Missing targetchange.speed");
				}

				if(readXMLInteger(p, "chance", intValue)){
					mType->changeTargetChance(intValue);
				}
				else{
					SHOW_XML_WARNING("Missing targetchange.chance");
				}
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"strategy") == 0){

				if(readXMLInteger(p, "attack", intValue)){
					//mType->attackStrength(intValue);
				}

				if(readXMLInteger(p, "defense", intValue)){
					//mType->defenseStrength(intValue);
				}
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"look") == 0){
				OutfitType& outfit = mType->outfit();

				if(readXMLInteger(p, "type", intValue)){
					outfit.lookType = intValue;

					if(readXMLInteger(p, "head", intValue)){
						outfit.lookHead = intValue;
					}

					if(readXMLInteger(p, "body", intValue)){
						outfit.lookBody = intValue;
					}

					if(readXMLInteger(p, "legs", intValue)){
						outfit.lookLegs = intValue;
					}

					if(readXMLInteger(p, "feet", intValue)){
						outfit.lookFeet = intValue;
					}

					if(readXMLInteger(p, "addons", intValue)){
						outfit.lookAddons = intValue;
					}
				}
				else if(readXMLInteger(p, "typeex", intValue)){
					outfit.lookTypeEx = intValue;
				}
				else{
					SHOW_XML_WARNING("Missing look type/typeex");
				}

				if(readXMLInteger(p, "corpse", intValue)){
					mType->corpseId(intValue);
				}
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"attacks") == 0){
				xmlNodePtr tmpNode = p->children;
				uint32_t index = 1;
				while(tmpNode){
					if(xmlStrcmp(tmpNode->name, (const xmlChar*)"attack") == 0){

						std::stringstream ss;
						ss << monster_name << "_attack_" << index;
						SpellBlock sb;
						sb.name = ss.str();
						if(deserializeSpell(tmpNode, sb)){
							mType->spellAttackList().push_back(sb);
							++index;
						}
						else{
							SHOW_XML_WARNING("Cant load spell");
						}
					}
					tmpNode = tmpNode->next;
				}
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"defenses") == 0){
				if(readXMLInteger(p, "defense", intValue)){
					mType->defense(intValue);
				}

				if(readXMLInteger(p, "armor", intValue)){
					mType->armor(intValue);
				}

				xmlNodePtr tmpNode = p->children;
				uint32_t index = 1;
				while(tmpNode){
					if(xmlStrcmp(tmpNode->name, (const xmlChar*)"defense") == 0){

						std::stringstream ss;
						ss << monster_name << "_defense_" << index;
						SpellBlock sb;
						sb.name = ss.str();
						if(deserializeSpell(tmpNode, sb)){
							mType->spellDefenseList().push_back(sb);
							++index;
						}
						else{
							SHOW_XML_WARNING("Cant load spell");
						}
					}
					tmpNode = tmpNode->next;
				}
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"immunities") == 0){
				xmlNodePtr tmpNode = p->children;
				while(tmpNode){
					if(xmlStrcmp(tmpNode->name, (const xmlChar*)"immunity") == 0){

						if(readXMLString(tmpNode, "name", strValue)){
							toLowerCaseString(strValue);

							if(strValue == "physical"){
								mType->damageImmunities() |= COMBAT_PHYSICALDAMAGE;
							}
							else if(strValue == "energy"){
								mType->damageImmunities() |= COMBAT_ENERGYDAMAGE;
							}
							else if(strValue == "fire"){
								mType->damageImmunities() |= COMBAT_FIREDAMAGE;
							}
							else if(strValue == "poison" || strValue == "earth"){
								mType->damageImmunities() |= COMBAT_EARTHDAMAGE;
							}
							else if(strValue == "drown"){
								mType->damageImmunities() |= COMBAT_DROWNDAMAGE;
							}
							else if(strValue == "ice"){
								mType->damageImmunities() |= COMBAT_ICEDAMAGE;
							}
							else if(strValue == "holy"){
								mType->damageImmunities() |= COMBAT_HOLYDAMAGE;
							}
							else if(strValue == "death"){
								mType->damageImmunities() |= COMBAT_DEATHDAMAGE;
							}
							else if(strValue == "lifedrain"){
								mType->damageImmunities() |= COMBAT_LIFEDRAIN;
							}
							else if(strValue == "outfit"){
								mType->mechanicImmunities() |= MECHANIC_SHAPESHIFT;
							}
							else if(strValue == "pacify"){
								mType->mechanicImmunities() |= MECHANIC_PACIFIED;
							}
							else if(strValue == "disarm"){
								mType->mechanicImmunities() |= MECHANIC_DISARMED;
							}
							else if(strValue == "shield"){
								mType->mechanicImmunities() |= MECHANIC_SHIELDED;
							}
							else if(strValue == "silence"){
								mType->mechanicImmunities() |= MECHANIC_SILENCED;
							}
							else if(strValue == "paralyze"){
								mType->mechanicImmunities() |= MECHANIC_PARALYZED;
							}
							else if(strValue == "invisible"){
								mType->mechanicImmunities() |= MECHANIC_INVISIBLE;
							}
							else if(strValue == "drunk"){
								mType->mechanicImmunities() |= MECHANIC_DRUNK;
							}
							else{
								SHOW_XML_WARNING("Unknown immunity name " << strValue);
							}
						}
						//old immunities code
						else if(readXMLInteger(tmpNode, "physical", intValue)){
							if(intValue != 0){
								mType->damageImmunities() |= COMBAT_PHYSICALDAMAGE;
							}
						}
						else if(readXMLInteger(tmpNode, "energy", intValue)){
							if(intValue != 0){
								mType->damageImmunities() |= COMBAT_ENERGYDAMAGE;
							}
						}
						else if(readXMLInteger(tmpNode, "fire", intValue)){
							if(intValue != 0){
								mType->damageImmunities() |= COMBAT_FIREDAMAGE;
							}
						}
						else if(readXMLInteger(tmpNode, "poison", intValue) ||
								readXMLInteger(tmpNode, "earth", intValue)){
							if(intValue != 0){
								mType->damageImmunities() |= COMBAT_EARTHDAMAGE;
							}
						}
						else if(readXMLInteger(tmpNode, "drown", intValue)){
							if(intValue != 0){
								mType->damageImmunities() |= COMBAT_DROWNDAMAGE;
							}
						}
						else if(readXMLInteger(tmpNode, "ice", intValue)){
							if(intValue != 0){
								mType->damageImmunities() |= COMBAT_ICEDAMAGE;
							}
						}
						else if(readXMLInteger(tmpNode, "holy", intValue)){
							if(intValue != 0){
								mType->damageImmunities() |= COMBAT_HOLYDAMAGE;
							}
						}
						else if(readXMLInteger(tmpNode, "death", intValue)){
							if(intValue != 0){
								mType->damageImmunities() |= COMBAT_DEATHDAMAGE;
							}
						}
						else if(readXMLInteger(tmpNode, "lifedrain", intValue)){
							if(intValue != 0){
								mType->damageImmunities() |= COMBAT_LIFEDRAIN;
							}
						}
						else if(readXMLInteger(tmpNode, "paralyze", intValue)){
							if(intValue != 0){
								mType->mechanicImmunities() |= MECHANIC_PARALYZED;
							}
						}
						else if(readXMLInteger(tmpNode, "outfit", intValue)){
							if(intValue != 0){
								mType->mechanicImmunities() |= MECHANIC_SHAPESHIFT;
							}
						}
						else if(readXMLInteger(tmpNode, "drunk", intValue)){
							if(intValue != 0){
								mType->mechanicImmunities() |= MECHANIC_DRUNK;
							}
						}
						else if(readXMLInteger(tmpNode, "invisible", intValue)){
							if(intValue != 0){
								mType->mechanicImmunities() |= MECHANIC_INVISIBLE;
							}
						}
						else{
							SHOW_XML_WARNING("Unknown immunity " << strValue);
						}
					}

					tmpNode = tmpNode->next;
				}
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"voices") == 0){
				xmlNodePtr tmpNode = p->children;

				if(readXMLInteger(p, "speed", intValue) || readXMLInteger(p, "interval", intValue)){
					mType->yellSpeedTicks(intValue);
				}
				else{
					SHOW_XML_WARNING("Missing voices.speed");
				}

				if(readXMLInteger(p, "chance", intValue)){
					mType->yellChance(intValue);
				}
				else{
					SHOW_XML_WARNING("Missing voices.chance");
				}

				while(tmpNode){
					if(xmlStrcmp(tmpNode->name, (const xmlChar*)"voice") == 0){

						VoiceBlock vb;
						vb.text = "";
						vb.yellText = false;

						if(readXMLString(tmpNode, "sentence", strValue)){
							vb.text = strValue;
						}
						else{
							SHOW_XML_WARNING("Missing voice.sentence");
						}

						if(readXMLInteger(tmpNode, "yell", intValue)){
							vb.yellText = (intValue != 0);
						}

						mType->voiceVector().push_back(vb);
					}

					tmpNode = tmpNode->next;
				}
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"loot") == 0){
				xmlNodePtr tmpNode = p->children;
				while(tmpNode){
					if(tmpNode->type != XML_ELEMENT_NODE){
						tmpNode = tmpNode->next;
						continue;
					}

					LootBlock lootBlock;
					if(loadLootItem(tmpNode, lootBlock)){
						mType->lootItems().push_back(lootBlock);
					}
					else{
						SHOW_XML_WARNING("Cant load loot");
					}

					tmpNode = tmpNode->next;
				}
			}
		else if(xmlStrcmp(p->name, (const xmlChar*)"elements") == 0){
				xmlNodePtr tmpNode = p->children;
				while(tmpNode){
					if(xmlStrcmp(tmpNode->name, (const xmlChar*)"element") == 0){
						if(readXMLInteger(tmpNode, "physicalPercent", intValue)){
							mType->elementMap()[COMBAT_PHYSICALDAMAGE] = intValue;
						}
						if(readXMLInteger(tmpNode, "icePercent", intValue)){
							mType->elementMap()[COMBAT_ICEDAMAGE] = intValue;
						}
						if(readXMLInteger(tmpNode, "earthPercent", intValue)){
							mType->elementMap()[COMBAT_EARTHDAMAGE] = intValue;
						}
						if(readXMLInteger(tmpNode, "firePercent", intValue)){
							mType->elementMap()[COMBAT_FIREDAMAGE] = intValue;
						}
						if(readXMLInteger(tmpNode, "energyPercent", intValue)){
							mType->elementMap()[COMBAT_ENERGYDAMAGE] = intValue;
						}
						if(readXMLInteger(tmpNode, "holyPercent", intValue)){
							mType->elementMap()[COMBAT_HOLYDAMAGE] = intValue;
						}
						if(readXMLInteger(tmpNode, "deathPercent", intValue)){
							mType->elementMap()[COMBAT_DEATHDAMAGE] = intValue;
						}
					}
					tmpNode = tmpNode->next;
				}
			}
			else if(xmlStrcmp(p->name, (const xmlChar*)"summons") == 0){

				if(readXMLInteger(p, "maxSummons", intValue)){
					mType->maxSummons(std::min(intValue, 100));
				}
				else{
					SHOW_XML_WARNING("Missing summons.maxSummons");
				}

				xmlNodePtr tmpNode = p->children;
				while(tmpNode){

					if(xmlStrcmp(tmpNode->name, (const xmlChar*)"summon") == 0){
						int32_t chance = 100;
						int32_t speed = 1000;

						if(readXMLInteger(tmpNode, "speed", intValue) || readXMLInteger(tmpNode, "interval", intValue)){
							speed = intValue;
						}

						if(readXMLInteger(tmpNode, "chance", intValue)){
							chance = intValue;
						}

						if(readXMLString(tmpNode, "name", strValue)){
							SummonBlock sb;
							sb.name = strValue;
							sb.speed = speed;
							sb.chance = chance;

							mType->summonList().push_back(sb);
						}
						else{
							SHOW_XML_WARNING("Missing summon.name");
						}
					}

					tmpNode = tmpNode->next;
				}
			}
			else{
				SHOW_XML_WARNING("Unknown attribute type - " << p->name);
			}

			p = p->next;
		}

		xmlFreeDoc(doc);
	}
	else{
		monsterLoad = false;
	}

	if(monsterLoad){
		std::string lowername = monster_name;
		creature_types[asLowerCaseString(monster_name)] = mType;

		return true;
	}
	delete mType;
	return false;
}

bool CreatureManager::loadLootItem(xmlNodePtr node, LootBlock& lootBlock)
{
	int intValue;
	std::string strValue;

	if(readXMLInteger(node, "id", intValue)){
		lootBlock.id = intValue;
	}

	if(lootBlock.id == 0){
		return false;
	}

	if(readXMLInteger(node, "countmax", intValue)){
		lootBlock.countmax = intValue;

		if(lootBlock.countmax > 100){
			lootBlock.countmax = 100;
		}
	}
	else{
		//std::cout << "missing countmax for loot id = "<< lootBlock.id << std::endl;
		lootBlock.countmax = 1;
	}

	if(readXMLInteger(node, "chance", intValue) || readXMLInteger(node, "chance1", intValue)){
		lootBlock.chance = intValue;

		if(lootBlock.chance > MAX_LOOTCHANCE){
			lootBlock.chance = MAX_LOOTCHANCE;
		}
	}
	else{
		//std::cout << "missing chance for loot id = "<< lootBlock.id << std::endl;
		lootBlock.chance = MAX_LOOTCHANCE;
	}

	if(Item::items[lootBlock.id].isContainer()){
		loadLootContainer(node, lootBlock);
	}

	//optional
	if(readXMLInteger(node, "subtype", intValue)){
		lootBlock.subType = intValue;
	}

	if(readXMLInteger(node, "actionId", intValue)){
		lootBlock.actionId = intValue;
	}

	if(readXMLString(node, "text", strValue)){
		lootBlock.text = strValue;
	}

	if(readXMLString(node, "dropEmpty", strValue)){
		lootBlock.dropEmpty = (asLowerCaseString(strValue) == "true");
	}

	return true;
}

bool CreatureManager::loadLootContainer(xmlNodePtr node, LootBlock& lBlock)
{
	if(node == NULL){
		return false;
	}

	xmlNodePtr tmpNode = node->children;
	xmlNodePtr p;

	if(tmpNode == NULL){
		return false;
	}

	while(tmpNode){
		if(xmlStrcmp(tmpNode->name, (const xmlChar*)"inside") == 0){
			p = tmpNode->children;
			while(p){
				LootBlock lootBlock;
				if(loadLootItem(p, lootBlock)){
					lBlock.childLoot.push_back(lootBlock);
				}
				p = p->next;
			}
			return true;
		}//inside

		tmpNode = tmpNode->next;
	}

	return false;
}

CreatureType* CreatureManager::getMonsterType(const std::string& name)
{
	TypeMap::iterator finder = creature_types.find(asLowerCaseString(name));
	if(finder == creature_types.end())
		return NULL;

	return finder->second;
}
