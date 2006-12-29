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

#include "definitions.h"
#include "weapons.h"
#include "combat.h"
#include "configmanager.h"
#include <sstream>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

extern Game g_game;
extern Vocations g_vocations;
extern ConfigManager g_config;

Weapons::Weapons():
m_scriptInterface("Weapon Interface")
{
	m_scriptInterface.initState();
}

Weapons::~Weapons()
{
	clear();
}

const Weapon* Weapons::getWeapon(const Item* item) const
{
	if(!item){
		return NULL;
	}

	WeaponMap::const_iterator it = weapons.find(item->getID());
	
	if(it != weapons.end()){
		return it->second;
	}

	return NULL;
}

void Weapons::clear()
{
	WeaponMap::iterator it;
	for(it = weapons.begin(); it != weapons.end(); ++it){
		delete it->second;
	}

	weapons.clear();
}	

LuaScriptInterface& Weapons::getScriptInterface()
{
	return m_scriptInterface;	
}

std::string Weapons::getScriptBaseName()
{
	return "weapons";	
}

bool Weapons::loadDefaults()
{
	for(uint32_t i = 0; i < Item::items.size(); ++i){
		const ItemType& it = Item::items[i];

		if(it.id == 0 || weapons.find(it.id) != weapons.end()){
			continue;
		}

		if(it.weaponType != WEAPON_NONE){
			switch(it.weaponType){
				case WEAPON_AXE:
				case WEAPON_SWORD:
				case WEAPON_CLUB:
				{
					WeaponMelee* weapon = new WeaponMelee(&m_scriptInterface);
					weapon->configureWeapon(it);
					weapons[it.id] = weapon;
					break;
				}

				case WEAPON_AMMO:
				case WEAPON_DIST:
				{
					if(it.weaponType == WEAPON_DIST && it.amuType != AMMO_NONE){
						//distance weapons with ammunitions are configured seperatly
						continue;
					}

					WeaponDistance* weapon = new WeaponDistance(&m_scriptInterface);
					weapon->configureWeapon(it);
					weapons[it.id] = weapon;
					break;
				}
				default:
				{
					break;
				}
			}
		}
	}

	return true;
}

Event* Weapons::getEvent(const std::string& nodeName)
{
	if(nodeName == "melee"){
		return new WeaponMelee(&m_scriptInterface);
	}
	else if(nodeName == "distance"){
		return new WeaponDistance(&m_scriptInterface);
	}
	else if(nodeName == "wand"){
		return new WeaponWand(&m_scriptInterface);
	}
	else{
		return NULL;
	}
}

bool Weapons::registerEvent(Event* event, xmlNodePtr p)
{
	Weapon* weapon = dynamic_cast<Weapon*>(event);
	//weapon->init();
	
	if(weapon){
		weapons[weapon->getID()] = weapon;
	}
	else{
		return false;
	}
	
	return true;
}

int32_t Weapons::getMaxWeaponDamage(int32_t attackSkill, int32_t attackValue)
{
	return ((attackSkill * attackValue)/20 + attackValue);
}

Weapon::Weapon(LuaScriptInterface* _interface) :
	Event(_interface)
{
	m_scripted = true;
	id = 0;
	level = 0;
	magLevel = 0;
	mana = 0;
	manaPercent = 0;
	soul = 0;
	exhaustion = false;
	premium = false;
	enabled = true;
	exhaustion = 0;
	range = 1;
	ammoAction = AMMOACTION_NONE;
}

Weapon::~Weapon()
{
	//
}

void Weapon::setCombatParam(const CombatParams& _params)
{
	m_scripted = false;
	params = _params;
}

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

	if(readXMLInteger(p, "range", intValue)){
		range = intValue;
	}

	if(readXMLString(p, "ammo", strValue)){
		if(xmlStrcmp((const xmlChar*)strValue.c_str(), (const xmlChar*)"move") == 0){
			ammoAction = AMMOACTION_MOVE;
		}
		else if(xmlStrcmp((const xmlChar*)strValue.c_str(), (const xmlChar*)"removecharge") == 0){
			ammoAction = AMMOACTION_REMOVECHARGE;
		}
		else if(xmlStrcmp((const xmlChar*)strValue.c_str(), (const xmlChar*)"removecount") == 0){
			ammoAction = AMMOACTION_REMOVECOUNT;
		}
	}

	xmlNodePtr vocationNode = p->children;
	while(vocationNode){
		if(xmlStrcmp(vocationNode->name,(const xmlChar*)"vocation") == 0){
			if(readXMLString(vocationNode, "name", strValue)){
				int32_t vocationId = g_vocations.getVocationId(strValue);

				if(vocationId != -1){
					vocWeaponMap[vocationId] = true;
				}
			}
		}
		
		vocationNode = vocationNode->next;
	}

	return true;
}

bool Weapon::configureWeapon(const ItemType& it)
{
	return true;
}

std::string Weapon::getScriptEventName()
{
	return "onUseWeapon";
}

bool Weapon::playerWeaponCheck(Player* player, Creature* target) const
{
	const Position& playerPos = player->getPosition();
	const Position& targetPos = target->getPosition();

	if(playerPos.z != targetPos.z){
		return false;
	}

	if( std::max(std::abs(playerPos.x - targetPos.x), std::abs(playerPos.y - targetPos.y)) > range){
		return false;
	}

	if(player->getAccessLevel() > 0){
		return true;
	}
	
	if(!enabled){
		return false;
	}

	if(player->getLevel() < level){
		return false;
	}

	if(player->getMagicLevel() < magLevel){
		return false;
	}

	if(player->getMana() < getManaCost(player)){
		return false;
	}

	if(player->getPlayerInfo(PLAYERINFO_SOUL) < soul){
		return false;
	}

	if(!vocWeaponMap.empty()){
		if(vocWeaponMap.find(player->getVocationId()) == vocWeaponMap.end()){
			return false;
		}
	}

	return true;
}

bool Weapon::useWeapon(Player* player, Item* item, Creature* target) const
{
	if(!playerWeaponCheck(player, target)){
		return false;
	}

	return internalUseWeapon(player, item, target);
}

bool Weapon::useFist(Player* player, Creature* target)
{
	const Position& playerPos = player->getPosition();
	const Position& targetPos = target->getPosition();

	if(Position::areInRange<1,1>(playerPos, targetPos)){
		int32_t attackStrength = player->getAttackStrength();
		int32_t attackSkill = player->getSkill(SKILL_FIST, SKILL_LEVEL);
		int32_t attackValue = 7;

		int32_t maxDamage = Weapons::getMaxWeaponDamage(attackSkill, attackValue);
		int32_t damage = -(random_range(0, maxDamage) * attackStrength) / 100;

		CombatParams params;
		params.combatType = COMBAT_PHYSICALDAMAGE;
		params.blockedByArmor = true;
		params.blockedByShield = true;
		Combat::doCombatHealth(player, target, damage, damage, params);

		if(player->getAccessLevel() <= 0){
			player->addSkillAdvance(SKILL_FIST, player->getSkillPoint());
		}

		return true;
	}

	return false;
}

bool Weapon::internalUseWeapon(Player* player, Item* item, Creature* target) const
{
	if(m_scripted){
		LuaVariant var;
		var.type = VARIANT_NUMBER;
		var.number = target->getID();
		executeUseWeapon(player, var);
	}
	else{
		int32_t damage = getWeaponDamage(player, item);
		Combat::doCombatHealth(player, target, damage, damage, params);
	}

	onUsedWeapon(player, item, target->getTile());
	return true;
}

bool Weapon::internalUseWeapon(Player* player, Item* item, Tile* tile) const
{
	if(m_scripted){
		LuaVariant var;
		var.type = VARIANT_TARGETPOSITION;
		var.pos = tile->getPosition();
		executeUseWeapon(player, var);
	}
	else{
		Combat::postCombatEffects(player, tile->getPosition(), params);
		g_game.addMagicEffect(tile->getPosition(), NM_ME_PUFF);
	}

	onUsedWeapon(player, item, tile);
	return true;
}

void Weapon::onUsedWeapon(Player* player, Item* item, Tile* destTile) const
{
	if(ammoAction == AMMOACTION_REMOVECOUNT){
		int32_t newCount = std::max(0, item->getItemCount() - 1);
		g_game.transformItem(item, item->getID(), newCount);
	}
	else if(ammoAction == AMMOACTION_REMOVECHARGE){
		int32_t newCharge = std::max(0, item->getItemCharge() - 1);
		g_game.transformItem(item, item->getID(), newCharge);
	}
	else if(ammoAction == AMMOACTION_MOVE){
		g_game.internalMoveItem(item->getParent(), destTile, INDEX_WHEREEVER, item, 1, FLAG_NOLIMIT);
	}
	else if(item->hasCharges()){
		int32_t newCharge = std::max(0, item->getItemCharge() - 1);
		g_game.transformItem(item, item->getID(), newCharge);
	}

	if(player->getAccessLevel() > 0){
		return;
	}

	skills_t skillType;
	uint32_t skillPoint = 0;
	if(getSkillType(player, item, skillType, skillPoint)){
		player->addSkillAdvance(skillType, skillPoint);
	}

	if(exhaustion){
		player->addExhaustionTicks();
	}

	int32_t manaCost = getManaCost(player);

	if(manaCost > 0){
		player->changeMana(-manaCost);
		player->addManaSpent(manaCost);
	}

	int32_t soulCost = soul;

	if(soulCost > 0){
		player->changeSoul(-soulCost);
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


bool Weapon::executeUseWeapon(Player* player, const LuaVariant& var) const
{
	//onUseWeapon(cid, var)
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

		LuaVariant* pVar = new LuaVariant(var);
		uint32_t variant = env->addVariant(pVar);

		m_scriptInterface->pushFunction(m_scriptId);
		lua_pushnumber(L, cid);
		lua_pushnumber(L, variant);

		int32_t result = m_scriptInterface->callFunction(2);
		m_scriptInterface->releaseScriptEnv();
		
		return (result == LUA_NO_ERROR);
	}
	else{
		std::cout << "[Error] Call stack overflow. Weapon::executeUseWeapon" << std::endl;
		return false;
	}
}


WeaponMelee::WeaponMelee(LuaScriptInterface* _interface) :
	Weapon(_interface)
{
	//
}

bool WeaponMelee::configureEvent(xmlNodePtr p)
{
	if(!Weapon::configureEvent(p)){
		return false;
	}

	return true;
}

bool WeaponMelee::configureWeapon(const ItemType& it)
{
	m_scripted = false;
	
	params.blockedByArmor = true;
	params.blockedByShield = true;
	params.combatType = COMBAT_PHYSICALDAMAGE;
	return true;
}

void WeaponMelee::onUsedWeapon(Player* player, Item* item, Tile* destTile) const
{
	Weapon::onUsedWeapon(player, item, destTile);
}

bool WeaponMelee::getSkillType(const Player* player, const Item* item,
	skills_t& skill, uint32_t& skillpoint) const
{
	skillpoint = player->getSkillPoint();
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

int32_t WeaponMelee::getWeaponDamage(const Player* player, const Item* item) const
{
	WeaponType_t weaponType = item->getWeaponType();

	int32_t attackSkill = 0;

	switch(weaponType){
		case WEAPON_SWORD:
			attackSkill = player->getSkill(SKILL_SWORD, SKILL_LEVEL);
			break;

		case WEAPON_CLUB:
		{
			attackSkill = player->getSkill(SKILL_CLUB, SKILL_LEVEL);
			break;
		}

		case WEAPON_AXE:
		{
			attackSkill = player->getSkill(SKILL_AXE, SKILL_LEVEL);
			break;
		}
		default:
		{
			break;
		}
	}

	int32_t attackStrength = player->getAttackStrength();
	int32_t attackValue = item->getAttack();
	int32_t maxDamage = Weapons::getMaxWeaponDamage(attackSkill, attackValue);
	return -(random_range(0, maxDamage) * attackStrength) / 100;
}

WeaponDistance::WeaponDistance(LuaScriptInterface* _interface) :
	Weapon(_interface)
{
	hitChance = 50;
	ammuAttackValue = 20;
}

bool WeaponDistance::configureEvent(xmlNodePtr p)
{
	if(!Weapon::configureEvent(p)){
		return false;
	}

	int intValue;

	if(readXMLInteger(p, "hitchance", intValue)){
		hitChance = intValue;
	}
	
	return true;
}

bool WeaponDistance::configureWeapon(const ItemType& it)
{
	m_scripted = false;

	ammoAction = AMMOACTION_REMOVECOUNT;
	params.blockedByArmor = true;
	params.combatType = COMBAT_PHYSICALDAMAGE;

	id = it.id;
	ammuAttackValue = it.attack;
	params.distanceEffect = it.shootType;

	switch(it.amuType){
		case AMMO_ARROW:
		{
			hitChance = 80;
			range = 5;
			break;
		}

		case AMMO_BOLT:
		{
			hitChance = 90;
			range = 6;
			break;
		}

		default:
			hitChance = 50;
			range = 4;
			break;
	}

	return true;
}

bool WeaponDistance::playerWeaponCheck(Player* player, Creature* target) const
{
	if(!Weapon::playerWeaponCheck(player, target)){
		return false;
	}

	return true;
}

bool WeaponDistance::useWeapon(Player* player, Item* item, Creature* target) const
{
	if(!playerWeaponCheck(player, target)){
		return false;
	}

	Position destPos = target->getPosition();
	Tile* destTile = target->getTile();

	if(rand() % 100 < hitChance){
		Weapon::internalUseWeapon(player, item, target);
	}
	else{
		//miss target
		int dx = random_range(-1, 1);
		int dy = random_range(-1, 1);

		destTile = g_game.getTile(destPos.x + dx, destPos.y + dy, destPos.z);
		
		if(destTile && !destTile->hasProperty(BLOCKINGANDNOTMOVEABLE)){
			destPos.x += dx;
			destPos.y += dy;
		}
		else{
			destTile = target->getTile();
		}

		Weapon::internalUseWeapon(player, item, destTile);
	}

	return true;
}

void WeaponDistance::onUsedWeapon(Player* player, Item* item, Tile* destTile) const
{
	Weapon::onUsedWeapon(player, item, destTile);
}

int32_t WeaponDistance::getWeaponDamage(const Player* player, const Item* item) const
{
	int32_t attackSkill = player->getSkill(SKILL_DIST, SKILL_LEVEL);

	int32_t attackStrength = player->getAttackStrength();
	int32_t maxDamage = Weapons::getMaxWeaponDamage(attackSkill, ammuAttackValue);
	return -(random_range(0, maxDamage) * attackStrength) / 100;
}

bool WeaponDistance::getSkillType(const Player* player, const Item* item,
	skills_t& skill, uint32_t& skillpoint) const
{
	skill = SKILL_DIST;
	skillpoint = player->getSkillPoint();
	return true;
}

WeaponWand::WeaponWand(LuaScriptInterface* _interface) :
	Weapon(_interface)
{
	minChange = 0;
	maxChange = 0;
}

bool WeaponWand::configureEvent(xmlNodePtr p)
{
	if(!Weapon::configureEvent(p)){
		return false;
	}

	int intValue;

	if(readXMLInteger(p, "min", intValue)){
		minChange = intValue;
	}

	if(readXMLInteger(p, "max", intValue)){
		maxChange = intValue;
	}

	return true;
}

int32_t WeaponWand::getWeaponDamage(const Player* player, const Item* item) const
{
	return random_range(-minChange, -maxChange);
}
