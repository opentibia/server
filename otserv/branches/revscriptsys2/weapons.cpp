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

#include "weapons.h"
#include "definitions.h"
#include "combat.h"
#include "tools.h"
#include "configmanager.h"
#include "vocation.h"

#include <sstream>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

extern Game g_game;
extern Vocations g_vocations;
extern ConfigManager g_config;
extern Weapons* g_weapons;

Weapon::Weapon()
{
	scripted = true;
	id = 0;
	level = 0;
	magLevel = 0;
	mana = 0;
	manaPercent = 0;
	soul = 0;
	exhaustion = false;
	premium = false;
	enabled = true;
	wieldUnproperly = false;
	exhaustion = 0;
	range = 1;
	ammoAction = AMMOACTION_NONE;
}

Weapon::~Weapon()
{
	//
}

//monsters
int32_t Weapon::getMaxMeleeDamage(int32_t attackSkill, int32_t attackValue)
{
	return ((int32_t)std::ceil((attackSkill * (attackValue * 0.05)) + (attackValue * 0.5)));
}

//players
int32_t Weapon::getMaxWeaponDamage(int32_t attackSkill, int32_t attackValue, float attackFactor)
{
	return ((int32_t)std::ceil(((float)(attackSkill * (attackValue * 0.0425) + (attackValue * 0.2)) / attackFactor)) * 2);
}

void Weapon::setCombatParam(const CombatParams& _params)
{
	scripted = false;
	params = _params;
}
/*
bool Weapon::configureEvent(xmlNodePtr p)
{
	int intValue;
	std::string strValue;

	if(readXMLInteger(p, "id", intValue)){
	 	id = intValue;
	}
	else{
		std::cout << "Error: [Weapon::configureEvent] Weapon without id." << std::endl;
		return false;
	}

	if(readXMLInteger(p, "lvl", intValue) || readXMLInteger(p, "level", intValue)){
	 	level = intValue;
	}

	if(readXMLInteger(p, "maglv", intValue) || readXMLInteger(p, "maglevel", intValue)){
	 	magLevel = intValue;
	}

	if(readXMLInteger(p, "mana", intValue)){
	 	mana = intValue;
	}

	if(readXMLInteger(p, "manapercent", intValue)){
	 	manaPercent = intValue;
	}

	if(readXMLInteger(p, "soul", intValue)){
	 	soul = intValue;
	}

	if(readXMLInteger(p, "exhaustion", intValue)){
		exhaustion = (intValue == 1);
	}

	if(readXMLInteger(p, "prem", intValue)){
		premium = (intValue == 1);
	}

	if(readXMLInteger(p, "enabled", intValue)){
		enabled = (intValue == 1);
	}

	if(readXMLInteger(p, "unproperly", intValue)){
		wieldUnproperly = (intValue == 1);
	}

	if(readXMLInteger(p, "range", intValue)){
		std::cout << "Warning: range is not longer used in weapons.xml." << std::endl;
	}

	if(readXMLString(p, "ammo", strValue)){
		std::cout << "Warning: ammo is not longer used in weapons.xml." << std::endl;
	}

	typedef std::list<std::string> STRING_LIST;
	STRING_LIST vocStringList;
	xmlNodePtr vocationNode = p->children;
	while(vocationNode){
		if(xmlStrcmp(vocationNode->name,(const xmlChar*)"vocation") == 0){
			if(readXMLString(vocationNode, "name", strValue)){
				int32_t vocationId = g_vocations.getVocationId(strValue);

				if(vocationId != -1){
					vocWeaponMap[vocationId] = true;
					intValue = 1;
					readXMLInteger(vocationNode, "showInDescription", intValue);
					if(intValue != 0){
						toLowerCaseString(strValue);
						vocStringList.push_back(strValue);
					}
				}
			}
		}

		vocationNode = vocationNode->next;
	}

	range = Item::items[id].shootRange;

	std::string vocationString;
	if(!vocStringList.empty()){
		for(STRING_LIST::iterator it = vocStringList.begin(); it != vocStringList.end(); ++it){
			if(*it != vocStringList.front()){
				if(*it != vocStringList.back()){
					vocationString += ", ";
				}
				else{
					vocationString += " and ";
				}
			}
			vocationString += *it;
			vocationString += "s";
		}
	}

	uint32_t wieldInfo = 0;
	if(getReqLevel() > 0){
		wieldInfo |= WIELDINFO_LEVEL;
	}
	if(getReqMagLv() > 0){
		wieldInfo |= WIELDINFO_MAGLV;
	}
	if(!vocationString.empty()){
		wieldInfo |= WIELDINFO_VOCREQ;
	}
	if(isPremium()){
		wieldInfo |= WIELDINFO_PREMIUM;
	}

	if(wieldInfo != 0){
		ItemType& it = Item::items.getItemType(id);
		it.wieldInfo = wieldInfo;
		it.vocationString = vocationString;
		it.minReqLevel = getReqLevel();
		it.minReqMagicLevel = getReqMagLv();
	}

	// the weapon must be configured in order for FORMULA_SKILL to function properly
	if(configureWeapon(Item::items[getID()])){
		return true;
	}

	return false;
}

bool Weapon::loadFunction(const std::string& functionName)
{
	if(asLowerCaseString(functionName) == "internalloadweapon" || asLowerCaseString(functionName) == "default"){
		if(configureWeapon(Item::items[getID()])){
			return true;
		}
	}
	return false;
}
*/
bool Weapon::configureWeapon(const ItemType& it)
{
	id = it.id;
	return true;
}

int32_t Weapon::playerWeaponCheck(Player* player, Creature* target) const
{
	const Position& playerPos = player->getPosition();
	const Position& targetPos = target->getPosition();

	if(playerPos.z != targetPos.z){
		return 0;
	}

	int32_t trueRange;
	const ItemType& it = Item::items[getID()];
	if(it.weaponType == WEAPON_AMMO){
		trueRange = player->getShootRange();
	}
	else{
		trueRange = range;
	}

	if(std::max(std::abs(playerPos.x - targetPos.x), std::abs(playerPos.y - targetPos.y)) > trueRange){
		return 0;
	}

	if(!player->hasFlag(PlayerFlag_IgnoreWeaponCheck)){

		if(!enabled){
			return 0;
		}

		if(isPremium() && !player->isPremium()){
			return 0;
		}

		if(player->getMana() < getManaCost(player)){
			return 0;
		}

		if(player->getPlayerInfo(PLAYERINFO_SOUL) < soul){
			return 0;
		}

		if(!vocWeaponMap.empty()){
			if(vocWeaponMap.find(player->getVocationId()) == vocWeaponMap.end()){
				return 0;
			}
		}

		int32_t damageModifier = 100;
		if(player->getLevel() < getReqLevel()){
			damageModifier = (isWieldedUnproperly()? damageModifier/2 : 0);
		}

		if(player->getMagicLevel() < getReqMagLv()){
			damageModifier = (isWieldedUnproperly()? damageModifier/2 : 0);
		}
		return damageModifier;

	}

	return 100;
}

bool Weapon::useWeapon(Player* player, Item* item, Creature* target) const
{
	int32_t damageModifier = playerWeaponCheck(player, target);
	if(damageModifier == 0){
		return false;
	}
	else{
		return internalUseWeapon(player, item, target, damageModifier);
	}
}

bool Weapon::useFist(Player* player, Creature* target)
{
	const Position& playerPos = player->getPosition();
	const Position& targetPos = target->getPosition();

	if(Position::areInRange<1,1>(playerPos, targetPos)){
		float attackFactor = player->getAttackFactor();
		int32_t attackSkill = player->getSkill(SKILL_FIST, SKILL_LEVEL);
		int32_t attackValue = 7;

		int32_t maxDamage = Weapons::getMaxWeaponDamage(player->getLevel(), attackSkill, attackValue, attackFactor);
		int32_t damage = -random_range(0, maxDamage, DISTRO_NORMAL);

		CombatParams params;
		params.combatType = COMBAT_PHYSICALDAMAGE;
		params.blockedByArmor = true;
		params.blockedByShield = true;
		Combat::doCombatHealth(player, target, damage, damage, params);

		if(!player->hasFlag(PlayerFlag_NotGainSkill)){
			if(player->getAddAttackSkill()){
				player->addSkillAdvance(SKILL_FIST, 1);
			}
		}

		return true;
	}

	return false;
}

bool Weapon::internalUseWeapon(Player* player, Item* item, Creature* target, int32_t damageModifier) const
{
	if(scripted){
		// REVSCRIPT TODO Event callback
		/*
		LuaVariant var;
		var.type = VARIANT_NUMBER;
		var.number = target->getID();
		executeUseWeapon(player, var);
		*/
	}
	else{
		int32_t damage = (getWeaponDamage(player, target, item) * damageModifier) / 100;
		Combat::doCombatHealth(player, target, damage, damage, params);
	}

	if(g_config.getNumber(ConfigManager::REMOVE_AMMUNITION)){
		onUsedAmmo(player, item, target->getTile());
	}
	onUsedWeapon(player, item, target->getTile());
	return true;
}

bool Weapon::internalUseWeapon(Player* player, Item* item, Tile* tile) const
{
	if(scripted){
		// REVSCRIPT TODO Event callback
		/*
		LuaVariant var;
		var.type = VARIANT_TARGETPOSITION;
		var.pos = tile->getPosition();
		executeUseWeapon(player, var);
		*/
	}
	else{
		Combat::postCombatEffects(player, tile->getPosition(), params);
		g_game.addMagicEffect(tile->getPosition(), NM_ME_PUFF);
	}

	if(g_config.getNumber(ConfigManager::REMOVE_AMMUNITION)){
		onUsedAmmo(player, item, tile);
	}
	onUsedWeapon(player, item, tile);
	return true;
}

void Weapon::onUsedWeapon(Player* player, Item* item, Tile* destTile) const
{
	if(!player->hasFlag(PlayerFlag_NotGainSkill)){
		skills_t skillType;
		uint32_t skillPoint = 0;
		if(getSkillType(player, item, skillType, skillPoint)){
			player->addSkillAdvance(skillType, skillPoint);
		}
	}

	if(!player->hasFlag(PlayerFlag_HasNoExhaustion)){
		if(exhaustion){
			player->addCombatExhaust(g_game.getFightExhaustionTicks());
		}
	}


	int32_t manaCost = getManaCost(player);

	if(manaCost > 0){
		player->addManaSpent(manaCost);
		if(!player->hasFlag(PlayerFlag_HasInfiniteMana)){
			player->changeMana(-manaCost);
		}
	}

	if(!player->hasFlag(PlayerFlag_HasInfiniteSoul)){
		int32_t soulCost = soul;

		if(soulCost > 0){
			player->changeSoul(-soulCost);
		}
	}
}

void Weapon::onUsedAmmo(Player* player, Item* item, Tile* destTile) const
{
	if(ammoAction == AMMOACTION_REMOVECOUNT){
		int32_t newCount = std::max(0, item->getItemCount() - 1);
		g_game.transformItem(player, item, item->getID(), newCount);
	}
	else if(ammoAction == AMMOACTION_REMOVECHARGE){
		int32_t newCharge = std::max((int32_t)0, ((int32_t)item->getCharges()) - 1);
		g_game.transformItem(player, item, item->getID(), newCharge);
	}
	else if(ammoAction == AMMOACTION_MOVE){
		g_game.internalMoveItem(player, item->getParent(), destTile, INDEX_WHEREEVER, item, 1, NULL, FLAG_NOLIMIT);
	}
	else if(ammoAction == AMMOACTION_MOVEBACK){
		//do nothing
	}
	else if(item->hasCharges()){
		int32_t newCharge = std::max((int32_t)0, ((int32_t)item->getCharges()) - 1);
		g_game.transformItem(player, item, item->getID(), newCharge);
	}
}

int32_t Weapon::getManaCost(const Player* player) const
{
	if(mana != 0){
		return mana;
	}

	if(manaPercent != 0){
		int32_t maxMana = player->getMaxMana();
		int32_t manaCost = (maxMana * manaPercent) / 100;
		return manaCost;
	}

	return 0;
}

bool Weapon::executeUseWeapon(Player* player) const
{
	// REVSCRIPT TODO EVENT CALL
	//onUseWeapon(cid, var)
	/*
	if(m_scriptInterface->reserveScriptEnv()){
		ScriptEnviroment* env = m_scriptInterface->getScriptEnv();

		#ifdef __DEBUG_LUASCRIPTS__
		std::stringstream desc;
		desc << "onUseWeapon - " << player->getName();
		env->setEventDesc(desc.str());
		#endif

		env->setScriptId(m_scriptId, m_scriptInterface);
		env->setRealPos(player->getPosition());

		lua_State* L = m_scriptInterface->getLuaState();

		uint32_t cid = env->addThing(player);

		m_scriptInterface->pushFunction(m_scriptId);
		lua_pushnumber(L, cid);
		m_scriptInterface->pushVariant(L, var);

		int32_t result = m_scriptInterface->callFunction(2);
		m_scriptInterface->releaseScriptEnv();

		return (result == LUA_NO_ERROR);
	}
	else{
		std::cout << "[Error] Call stack overflow. Weapon::executeUseWeapon" << std::endl;
		return false;
	}
	*/
	return true;
}

WeaponMelee::WeaponMelee()
{
	params.blockedByArmor = true;
	params.blockedByShield = true;
	params.combatType = COMBAT_PHYSICALDAMAGE;
	elementType = COMBAT_NONE;
	elementDamage = 0;
}

bool WeaponMelee::configureWeapon(const ItemType& it)
{
	scripted = false;
	elementType = it.abilities.elementType;
	elementDamage = it.abilities.elementDamage;
	return Weapon::configureWeapon(it);
}

bool WeaponMelee::useWeapon(Player* player, Item* item, Creature* target) const
{
	if(!Weapon::useWeapon(player, item, target)){
		return false;
	}

	if(elementDamage != 0){
		int32_t damage = getElementDamage(player, item);
		CombatParams eParams;
		eParams.combatType = elementType;
		eParams.isAggressive = true;
		eParams.useCharges = true;
		Combat::doCombatHealth(player, target, damage, damage, eParams);
	}

	return true;
}

void WeaponMelee::onUsedWeapon(Player* player, Item* item, Tile* destTile) const
{
	Weapon::onUsedWeapon(player, item, destTile);
}

void WeaponMelee::onUsedAmmo(Player* player, Item* item, Tile* destTile) const
{
	Weapon::onUsedAmmo(player, item, destTile);
}

bool WeaponMelee::getSkillType(const Player* player, const Item* item,
	skills_t& skill, uint32_t& skillpoint) const
{
	skillpoint = 0;

	if(player->getAddAttackSkill()){
		switch(player->getLastAttackBlockType()){
			case BLOCK_DEFENSE:
			case BLOCK_ARMOR:
			case BLOCK_NONE:
			{
				skillpoint = 1;
				break;
			}

			default:
			{
				skillpoint = 0;
				break;
			}
		}
	}

	WeaponType_t weaponType = item->getWeaponType();

	switch(weaponType){
		case WEAPON_SWORD:
		{
			skill = SKILL_SWORD;
			return true;
			break;
		}

		case WEAPON_CLUB:
		{
			skill = SKILL_CLUB;
			return true;
			break;
		}

		case WEAPON_AXE:
		{
			skill = SKILL_AXE;
			return true;
			break;
		}
		default:
		{
			return false;
			break;
		}
	}
}

int32_t WeaponMelee::getElementDamage(const Player* player, const Item* item) const
{
	int32_t attackSkill = player->getWeaponSkill(item);
	float attackFactor = player->getAttackFactor();
	int32_t maxValue = Weapons::getMaxWeaponDamage(player->getLevel(), attackSkill, elementDamage, attackFactor);
	return -random_range(0, maxValue, DISTRO_NORMAL);
}

int32_t WeaponMelee::getWeaponDamage(const Player* player, const Creature* target, const Item* item, bool maxDamage /*= false*/) const
{
	int32_t attackSkill = player->getWeaponSkill(item);
	int32_t attackValue = std::max((int32_t)0, ((int32_t)item->getAttack() - elementDamage));
	float attackFactor = player->getAttackFactor();
	int32_t maxValue = Weapons::getMaxWeaponDamage(player->getLevel(), attackSkill, attackValue, attackFactor);

	if(maxDamage){
		return -maxValue;
	}

	return -random_range(0, maxValue, DISTRO_NORMAL);
}

WeaponDistance::WeaponDistance()
{
	hitChance = -1;
	maxHitChance = 0;
	breakChance = 0;
	ammuAttackValue = 0;
	ammoAction = AMMOACTION_REMOVECOUNT;
	params.blockedByArmor = true;
	params.combatType = COMBAT_PHYSICALDAMAGE;
}
/*
bool WeaponDistance::configureEvent(xmlNodePtr p)
{
	if(!Weapon::configureEvent(p)){
		return false;
	}

	const ItemType& it = Item::items[id];

	//default values
	if(it.amuType != AMMO_NONE){
		//hit chance on two-handed weapons is limited to 90%
		maxHitChance = 90;
	}
	else{
		//one-handed is set to 75%
		maxHitChance = 75;
	}

	if(it.hitChance != -1){
		hitChance = it.hitChance;
	}

	if(it.maxHitChance != -1){
		maxHitChance = it.maxHitChance;
	}

	if(it.breakChance != -1){
		breakChance = it.breakChance;
	}

	if(it.ammoAction != AMMOACTION_NONE){
		ammoAction = it.ammoAction;
	}

	int intValue;
	if(readXMLInteger(p, "hitChance", intValue) || readXMLInteger(p, "hitchance", intValue)){
		std::cout << "Warning: hitChance is not longer used in weapons.xml." << std::endl;
	}
	if(readXMLInteger(p, "breakChance", intValue) || readXMLInteger(p, "breakchance", intValue)){
		std::cout << "Warning: breakChance is not longer used in weapons.xml." << std::endl;
	}

	return true;
}*/

bool WeaponDistance::configureWeapon(const ItemType& it)
{
	scripted = false;

	//default values
	if(it.amuType != AMMO_NONE){
		//hit chance on two-handed weapons is limited to 90%
		maxHitChance = 90;
	}
	else{
		//one-handed is set to 75%
		maxHitChance = 75;
	}

	params.distanceEffect = it.shootType;
	range = it.shootRange;
	ammuAttackValue = it.attack;

	if(it.hitChance > 0){
		hitChance = it.hitChance;
	}

	if(it.maxHitChance > 0){
		maxHitChance = it.maxHitChance;
	}

	if(it.breakChance > 0){
		breakChance = it.breakChance;
	}

	if(it.ammoAction != AMMOACTION_NONE){
		ammoAction = it.ammoAction;
	}

	return Weapon::configureWeapon(it);
}
/*
// REVSCRIPT TODO ?
int32_t WeaponDistance::playerWeaponCheck(Player* player, Creature* target) const
{
	const ItemType& it = Item::items[id];
	Item* bow = player->getWeapon(true);
	if(bow && bow->getWeaponType() == WEAPON_DIST && bow->getID() != id){ //Be sure we are using a bow
		const Weapon* weap = g_weapons->getWeapon(bow);
		if(weap){
			return weap->playerWeaponCheck(player, target);
		}
	}

	return Weapon::playerWeaponCheck(player, target);
}
*/
bool WeaponDistance::useWeapon(Player* player, Item* item, Creature* target) const
{
	int32_t damageModifier = playerWeaponCheck(player, target);
	if(damageModifier == 0){
		return false;
	}

	int32_t chance;
	if(hitChance == -1){
		//hit chance is based on distance to target and distance skill
		uint32_t skill = player->getSkill(SKILL_DIST, SKILL_LEVEL);
		const Position& playerPos = player->getPosition();
		const Position& targetPos = target->getPosition();
		uint32_t distance = std::max(std::abs(playerPos.x - targetPos.x), std::abs(playerPos.y - targetPos.y));

		if(maxHitChance == 75){
			//chance for one-handed weapons
			switch(distance){
				case 1: chance = (uint32_t)((float)std::min(skill, (uint32_t)74)) + 1; break;
				case 2: chance = (uint32_t)((float)2.4 * std::min(skill, (uint32_t)28)) + 8; break;
				case 3: chance = (uint32_t)((float)1.55 * std::min(skill, (uint32_t)45)) + 6; break;
				case 4: chance = (uint32_t)((float)1.25 * std::min(skill, (uint32_t)58)) + 3; break;
				case 5: chance = (uint32_t)((float)std::min(skill, (uint32_t)74)) + 1; break;
				case 6: chance = (uint32_t)((float)0.8 * std::min(skill, (uint32_t)90)) + 3; break;
				case 7: chance = (uint32_t)((float)0.7 * std::min(skill, (uint32_t)104)) + 2; break;
				default: chance = hitChance; break;
			}
		}
		else if(maxHitChance == 90){
			//formula for two-handed weapons
			switch(distance){
				case 1: chance = (uint32_t)((float)1.2 * std::min(skill, (uint32_t)74)) + 1; break;
				case 2: chance = (uint32_t)((float)3.2 * std::min(skill, (uint32_t)28)); break;
				case 3: chance = (uint32_t)((float)2.0 * std::min(skill, (uint32_t)45)); break;
				case 4: chance = (uint32_t)((float)1.55 * std::min(skill, (uint32_t)58)); break;
				case 5: chance = (uint32_t)((float)1.2 * std::min(skill, (uint32_t)74)) + 1; break;
				case 6: chance = (uint32_t)((float)1.0 * std::min(skill, (uint32_t)90)); break;
				case 7: chance = (uint32_t)((float)1.0 * std::min(skill, (uint32_t)90)); break;
				default: chance = hitChance; break;
			}
		}
		else if(maxHitChance == 100){
			switch(distance){
				case 1: chance = (uint32_t)((float)1.35 * std::min(skill, (uint32_t)73)) + 1; break;
				case 2: chance = (uint32_t)((float)3.2 * std::min(skill, (uint32_t)30)) + 4; break;
				case 3: chance = (uint32_t)((float)2.05 * std::min(skill, (uint32_t)48)) + 2; break;
				case 4: chance = (uint32_t)((float)1.5 * std::min(skill, (uint32_t)65)) + 2; break;
				case 5: chance = (uint32_t)((float)1.35 * std::min(skill, (uint32_t)73)) + 1; break;
				case 6: chance = (uint32_t)((float)1.2 * std::min(skill, (uint32_t)87)) - 4; break;
				case 7: chance = (uint32_t)((float)1.1 * std::min(skill, (uint32_t)90)) + 1; break;
				default: chance = hitChance; break;
			}
		}
		else{
			chance = maxHitChance;
		}
	}
	else{
		chance = hitChance;
	}

	Item* bow = player->getWeapon(true);
	if(bow && bow != item && bow->getHitChance() > 0){ //Be sure we are using a bow, not a spear
		chance += bow->getHitChance();
	}

	if(chance >= random_range(1, 100)){
		Weapon::internalUseWeapon(player, item, target, damageModifier);
	}
	else{
		//miss target
		typedef std::pair<int32_t, int32_t> dPair;
		std::vector<dPair> destList;
		destList.push_back(dPair(-1, -1));
		destList.push_back(dPair(-1, 0));
		destList.push_back(dPair(-1, 1));
		destList.push_back(dPair(0, -1));
		destList.push_back(dPair(0, 1));
		destList.push_back(dPair(1, -1));
		destList.push_back(dPair(1, 0));
		destList.push_back(dPair(1, 1));

		std::random_shuffle(destList.begin(), destList.end());

		Position destPos = target->getPosition();
		Tile* destTile = target->getTile();
		Tile* tmpTile = NULL;

		for(std::vector<dPair>::iterator it = destList.begin(); it != destList.end(); ++it){
			tmpTile = g_game.getTile(destPos.x + it->first, destPos.y + it->second, destPos.z);
			if(tmpTile && !tmpTile->hasProperty(IMMOVABLEBLOCKSOLID)){
				destTile = tmpTile;
				break;
			}
		}

		Weapon::internalUseWeapon(player, item, destTile);
	}

	return true;
}

void WeaponDistance::onUsedWeapon(Player* player, Item* item, Tile* destTile) const
{
	Weapon::onUsedWeapon(player, item, destTile);
}

void WeaponDistance::onUsedAmmo(Player* player, Item* item, Tile* destTile) const
{
	if((ammoAction == AMMOACTION_MOVE || ammoAction == AMMOACTION_MOVEBACK) &&
			breakChance > 0 && random_range(1, 100) < breakChance){
		int32_t newCount = std::max(0, item->getItemCount() - 1);
		g_game.transformItem(player, item, item->getID(), newCount);
	}
	else{
		Weapon::onUsedAmmo(player, item, destTile);
	}
}

int32_t WeaponDistance::getWeaponDamage(const Player* player, const Creature* target, const Item* item, bool maxDamage /*= false*/) const
{
	int32_t attackValue = ammuAttackValue;
	if(item->getWeaponType() == WEAPON_AMMO){
		Item* bow = const_cast<Player*>(player)->getWeapon(true);
		if(bow){
			attackValue += bow->getAttack();
		}
	}

	int32_t attackSkill = player->getSkill(SKILL_DIST, SKILL_LEVEL);
	float attackFactor = player->getAttackFactor();
	int32_t maxValue = Weapons::getMaxWeaponDamage(player->getLevel(), attackSkill, attackValue, attackFactor);


	if(maxDamage){
		return -maxValue;
	}

	int32_t minValue = 0;
	if(target){
		if(target->getPlayer()){
			minValue = (int32_t)std::ceil(player->getLevel() * 0.1);
		}
		else{
			minValue = (int32_t)std::ceil(player->getLevel() * 0.2);
		}
	}

	return -random_range(minValue, maxValue, DISTRO_NORMAL);
}

bool WeaponDistance::getSkillType(const Player* player, const Item* item,
	skills_t& skill, uint32_t& skillpoint) const
{
	skill = SKILL_DIST;
	skillpoint = 0;

	if(player->getAddAttackSkill()){
		switch(player->getLastAttackBlockType()){
			case BLOCK_NONE:
			{
				skillpoint = 2;
				break;
			}

			case BLOCK_DEFENSE:
			case BLOCK_ARMOR:
			{
				skillpoint = 1;
				break;
			}

			default:
				skillpoint = 0;
				break;
		}
	}

	return true;
}

WeaponWand::WeaponWand()
{
	minChange = 0;
	maxChange = 0;
}
/*
bool WeaponWand::configureEvent(xmlNodePtr p)
{
	if(!Weapon::configureEvent(p)){
		return false;
	}

	int intValue;
	std::string strValue;

	if(readXMLInteger(p, "min", intValue)){
		minChange = intValue;
	}

	if(readXMLInteger(p, "max", intValue)){
		maxChange = intValue;
	}

	if(readXMLString(p, "type", strValue)){
		//TODO: use a tool-function
		if(asLowerCaseString(strValue) == "earth"){
			params.combatType = COMBAT_EARTHDAMAGE;
		}
		else if(asLowerCaseString(strValue) == "ice"){
			params.combatType = COMBAT_ICEDAMAGE;
		}
		else if(asLowerCaseString(strValue) == "energy"){
			params.combatType = COMBAT_ENERGYDAMAGE;
		}
		else if(asLowerCaseString(strValue) == "fire"){
			params.combatType = COMBAT_FIREDAMAGE;
		}
		else if(asLowerCaseString(strValue) == "death"){
			params.combatType = COMBAT_DEATHDAMAGE;
		}
		else if(asLowerCaseString(strValue) == "holy"){
			params.combatType = COMBAT_HOLYDAMAGE;
		}
	}

	return true;
}
*/
bool WeaponWand::configureWeapon(const ItemType& it)
{
	scripted = false;
	range = it.shootRange;
	params.distanceEffect = it.shootType;

	return Weapon::configureWeapon(it);
}

int32_t WeaponWand::getWeaponDamage(const Player* player, const Creature* target, const Item* item, bool maxDamage /*= false*/) const
{
	if(maxDamage){
		return -maxChange;
	}

	return random_range(-minChange, -maxChange, DISTRO_NORMAL);
}
