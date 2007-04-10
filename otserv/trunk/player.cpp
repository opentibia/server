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

#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>

#include <stdlib.h>

#include "player.h"
#include "ioplayer.h"
#include "configmanager.h"
#include "chat.h"
#include "house.h"
#include "combat.h"
#include "movement.h"
#include "weapons.h"

extern ConfigManager g_config;
extern Game g_game;
extern Chat g_chat;
extern Vocations g_vocations;
extern MoveEvents* g_moveEvents;
extern Weapons* g_weapons;

AutoList<Player> Player::listPlayer;
MuteCountMap Player::muteCountMap;
int32_t Player::maxMessageBuffer;

Player::Player(const std::string& _name, Protocol79 *p) :
Creature()
{	
	client = p;

	if(client){
		client->setPlayer(this);
	}

	name        = _name;
	setVocation(VOCATION_NONE);
	capacity   = 300.00;
	mana       = 0;
	manaMax    = 0;
	manaSpent  = 0;
	soul       = 0;
	soulMax    = 100;
	guildId    = 0;
	guildLevel = 0;

	level      = 1;
	experience = 180;
	damageImmunities = 0;
	conditionImmunities = 0;
	conditionSuppressions = 0;
	magLevel   = 20;
	accessLevel = 0;
	lastlogin  = 0;
	lastLoginSaved = 0;
	SendBuffer = false;
	npings = 0;
	internal_ping = 0;
	lastAction = 0;
	MessageBufferTicks = 0;
	MessageBufferCount = 0;

	pzLocked = false;
	bloodHitCount = 0;
	shieldBlockCount = 0;
	lastAttackBlockType = BLOCK_NONE;
	addAttackSkillPoint = false;
	attackTicks = 0;

	chaseMode = CHASEMODE_STANDSTILL;

	attackStrength = 100;
	defenseStrength = 0;
	fightMode = FIGHTMODE_ATTACK;

	tradePartner = NULL;
	tradeState = TRADE_NONE;
	tradeItem = NULL;

	lastSentStats.health = 0;
	lastSentStats.healthMax = 0;
	lastSentStats.freeCapacity = 0;
	lastSentStats.experience = 0;
	lastSentStats.level = 0;
	lastSentStats.mana = 0;
	lastSentStats.manaMax = 0;
	lastSentStats.manaSpent = 0;
	lastSentStats.magLevel = 0;
	level_percent = 0;
	maglevel_percent = 0;

	for(int32_t i = 0; i < 11; i++){
		inventory[i] = NULL;
		inventoryAbilities[i] = false;
	}

	for(int32_t i = SKILL_FIRST; i <= SKILL_LAST; ++i){
		skills[i][SKILL_LEVEL]= 10;
		skills[i][SKILL_TRIES]= 0;
		skills[i][SKILL_PERCENT] = 0;
	}

	for(int32_t i = SKILL_FIRST; i <= SKILL_LAST; ++i){
		varSkills[i] = 0;
	}

	maxDepotLimit = 1000;
	maxVipLimit = 50;
	groupFlags = 0;
 	
 	vocation_id = (Vocation_t)0;
 	
 	town = 0;

#ifdef __SKULLSYSTEM__
	redSkullTicks = 0;
	skull = SKULL_NONE;
#endif
} 

Player::~Player()
{
	for(int i = 0; i < 11; i++){
		if(inventory[i]){
			inventory[i]->setParent(NULL);
			inventory[i]->releaseThing2();
			inventory[i] = NULL;
			inventoryAbilities[i] = false;
		}
	}

	DepotMap::iterator it;
	for(it = depots.begin();it != depots.end(); it++){
		it->second->releaseThing2();
	}

	//std::cout << "Player destructor " << this->getID() << std::endl;
	if(client){
		delete client;
	}
}

void Player::setVocation(uint32_t vocId)
{
	vocation_id = (Vocation_t)vocId;
	vocation = g_vocations.getVocation(vocId);

	//Update health/mana gain condition
	Condition* condition = getCondition(CONDITION_REGENERATION, CONDITIONID_DEFAULT);
	if(condition){
		condition->setParam(CONDITIONPARAM_HEALTHGAIN, vocation->getHealthGainAmount());
		condition->setParam(CONDITIONPARAM_HEALTHTICKS, vocation->getHealthGainTicks() * 1000);
		condition->setParam(CONDITIONPARAM_MANAGAIN, vocation->getManaGainAmount());
		condition->setParam(CONDITIONPARAM_MANATICKS, vocation->getManaGainTicks() * 1000);
	}
}

uint32_t Player::getVocationId() const
{
	return vocation_id;
}

bool Player::isPushable() const
{
	bool ret = Creature::isPushable();

	if(hasFlag(PlayerFlag_CannotBePushed)){
		return false;
	}

	return ret;
}

std::string Player::getDescription(int32_t lookDistance) const
{
	std::stringstream s;
	std::string str;
	
	if(lookDistance == -1){
		s << "yourself.";

		if(getVocationId() != VOCATION_NONE)
			s << " You are " << vocation->getVocDescription() << ".";
		else
			s << " You have no vocation.";
	}
	else {	
		s << name << " (Level " << level <<").";
	
		if(sex == PLAYERSEX_FEMALE)
			s << " She";
		else
			s << " He";	
			
		if(getVocationId() != VOCATION_NONE)
			s << " is "<< vocation->getVocDescription() << ".";
		else
			s << " has no vocation.";
	}
	
	if(guildId)
	{
		if(lookDistance == -1)
			s << " You are ";
		else
		{
			if(sex == PLAYERSEX_FEMALE)
				s << " She is ";
			else
				s << " He is ";
		}
		
		if(guildRank.length())
			s << guildRank;
		else
			s << "a member";
		
		s << " of the " << guildName;
		
		if(guildNick.length())
			s << " (" << guildNick << ")";
		
		s << ".";
	}
	
	str = s.str();
	return str;
}

Item* Player::getInventoryItem(slots_t slot) const
{
	if(slot > 0 && slot < 11)
		return inventory[slot];

	return NULL;
}

void Player::setConditionSuppressions(uint32_t conditions, bool remove)
{
	if(!remove){
		conditionSuppressions |= conditions;
	}
	else{
		conditionSuppressions &= ~conditions;
	}
}

Item* Player::getWeapon() const
{
	Item* item = NULL;

	for(int slot = SLOT_RIGHT; slot <= SLOT_LEFT; slot++){
		item = getInventoryItem((slots_t)slot);
		if(!item){
			continue;
		}

		switch(item->getWeaponType()){
			case WEAPON_SWORD:
			case WEAPON_AXE:
			case WEAPON_CLUB:
			case WEAPON_WAND:
			{
				const Weapon* weapon = g_weapons->getWeapon(item);
				if(weapon){
					return item;
				}

				break;
			}

			case WEAPON_DIST:
			{
				if(item->getAmuType() != AMMO_NONE){
					Item* ammuItem = getInventoryItem(SLOT_AMMO);

					if(ammuItem && ammuItem->getAmuType() == item->getAmuType()){
						const Weapon* weapon = g_weapons->getWeapon(ammuItem);
						if(weapon){
							return ammuItem;
						}
					}
				}
				else{
					const Weapon* weapon = g_weapons->getWeapon(item);
					if(weapon){
						return item;
					}
				}
			}

			default:
				break;
		}
	}
	
	return NULL;
}

int32_t Player::getArmor() const
{
	int32_t armor = 0;
	
	if(getInventoryItem(SLOT_HEAD))
		armor += getInventoryItem(SLOT_HEAD)->getArmor();
	if(getInventoryItem(SLOT_NECKLACE))
		armor += getInventoryItem(SLOT_NECKLACE)->getArmor();
	if(getInventoryItem(SLOT_ARMOR))
		armor += getInventoryItem(SLOT_ARMOR)->getArmor();
	if(getInventoryItem(SLOT_LEGS))
		armor += getInventoryItem(SLOT_LEGS)->getArmor();
	if(getInventoryItem(SLOT_FEET))
		armor += getInventoryItem(SLOT_FEET)->getArmor();
	if(getInventoryItem(SLOT_RING))
		armor += getInventoryItem(SLOT_RING)->getArmor();
	
	return armor;
}

int32_t Player::getDefense() const
{
	int32_t baseDefense = 5;
	int32_t defense = 0;
	int32_t shieldSkill = getSkill(SKILL_SHIELD, SKILL_LEVEL);

	if(getInventoryItem(SLOT_LEFT)){
		if(getInventoryItem(SLOT_LEFT)->getDefense() > defense){
			defense = getInventoryItem(SLOT_LEFT)->getDefense();
		}
	}
	
	if(getInventoryItem(SLOT_RIGHT)){
		if(getInventoryItem(SLOT_RIGHT)->getDefense() > defense){
			defense = getInventoryItem(SLOT_RIGHT)->getDefense();
		}
	}
	
	if(defense <= 0){
		if(getInventoryItem(SLOT_HEAD)){
			defense += getInventoryItem(SLOT_HEAD)->getDefense();
		}

		if(getInventoryItem(SLOT_NECKLACE)){
			defense += getInventoryItem(SLOT_NECKLACE)->getDefense();
		}

		if(getInventoryItem(SLOT_ARMOR)){
			defense += getInventoryItem(SLOT_ARMOR)->getDefense();
		}

		if(getInventoryItem(SLOT_LEGS)){
			defense += getInventoryItem(SLOT_LEGS)->getDefense();
		}

		if(getInventoryItem(SLOT_FEET)){
			defense += getInventoryItem(SLOT_FEET)->getDefense();
		}

		if(getInventoryItem(SLOT_RING)){
			defense += getInventoryItem(SLOT_RING)->getDefense();
		}
	}

	return baseDefense + (defense * shieldSkill) / 100;
}

void Player::sendIcons() const
{
	int icons = 0;

	ConditionList::const_iterator it;
	for(it = conditions.begin(); it != conditions.end(); ++it){
		if(!isSuppress((*it)->getType())){
			icons |= (*it)->getIcons();
		}
	}

	client->sendIcons(icons);
}

void Player::updateInventoryWeigth()
{
	inventoryWeight = 0.00;
	
	if(!hasFlag(PlayerFlag_HasInfiniteCapacity)){
		for(int i = SLOT_FIRST; i < SLOT_LAST; ++i){
			Item* item = getInventoryItem((slots_t)i);
			if(item){
				inventoryWeight += item->getWeight();
			}
		}
	}
}

int Player::getPlayerInfo(playerinfo_t playerinfo) const
{
	switch(playerinfo) {
		case PLAYERINFO_LEVEL: return level; break;
		case PLAYERINFO_LEVELPERCENT: return level_percent; break;
		case PLAYERINFO_MAGICLEVEL: return magLevel; break;
		case PLAYERINFO_MAGICLEVELPERCENT: return maglevel_percent; break;
		case PLAYERINFO_HEALTH: return health; break;
		case PLAYERINFO_MAXHEALTH: return healthMax; break;
		case PLAYERINFO_MANA: return mana; break;
		case PLAYERINFO_MAXMANA: return manaMax; break;
		case PLAYERINFO_MANAPERCENT: return maglevel_percent; break;
		case PLAYERINFO_SOUL: return soul; break;
		default:
			return 0; break;
	}

	return 0;
}

int Player::getSkill(skills_t skilltype, skillsid_t skillinfo) const
{
	int32_t n = skills[skilltype][skillinfo];

	if(skillinfo == SKILL_LEVEL){
		n += varSkills[skilltype];
	}

	return n;
}

std::string Player::getSkillName(int skillid)
{
	std::string skillname;
	switch(skillid){
	case SKILL_FIST:
		skillname = "fist fighting"; 
		break;
	case SKILL_CLUB:
		skillname = "club fighting";
		break;
	case SKILL_SWORD:
		skillname = "sword fighting";
		break;
	case SKILL_AXE:
		skillname = "axe fighting";
		break;
	case SKILL_DIST:
		skillname = "distance fighting";
		break;
	case SKILL_SHIELD:
		skillname = "shielding";
		break;
	case SKILL_FISH:
		skillname = "fishing";
		break;
	default:
		skillname = "unknown";
		break;
	}
	return skillname;
}

void Player::addSkillAdvance(skills_t skill, uint32_t count)
{
	skills[skill][SKILL_TRIES] += count * g_config.getNumber(ConfigManager::RATE_SKILL);

#if __DEBUG__
	std::cout << getName() << ", has the vocation: " << (int)getVocationId() << " and is training his " << getSkillName(skill) << "(" << skill << "). Tries: " << skills[skill][SKILL_TRIES] << "(" << vocation->getReqSkillTries(skill, skills[skill][SKILL_LEVEL] + 1) << ")" << std::endl;
	std::cout << "Current skill: " << skills[skill][SKILL_LEVEL] << std::endl;
#endif

	//Need skill up?
	if(skills[skill][SKILL_TRIES] >= vocation->getReqSkillTries(skill, skills[skill][SKILL_LEVEL] + 1)){
	 	skills[skill][SKILL_LEVEL]++;
	 	skills[skill][SKILL_TRIES] = 0;
		skills[skill][SKILL_PERCENT] = 0;				 
		std::stringstream advMsg;
		advMsg << "You advanced in " << getSkillName(skill) << ".";
		client->sendTextMessage(MSG_EVENT_ADVANCE, advMsg.str());
		client->sendSkills();
	}
	else{
		//update percent
		uint32_t newPercent = std::min((uint32_t)100, (100*skills[skill][SKILL_TRIES])/vocation->getReqSkillTries(skill, skills[skill][SKILL_LEVEL]+1));
	 	if(skills[skill][SKILL_PERCENT] != newPercent){
			skills[skill][SKILL_PERCENT] = newPercent;
			client->sendSkills();
	 	}
	}
}

Container* Player::getContainer(uint32_t cid)
{
  for(ContainerVector::iterator it = containerVec.begin(); it != containerVec.end(); ++it){
		if(it->first == cid)
			return it->second;
	}

	return NULL;
}

int32_t Player::getContainerID(const Container* container) const
{
  for(ContainerVector::const_iterator cl = containerVec.begin(); cl != containerVec.end(); ++cl){
	  if(cl->second == container)
			return cl->first;
	}

	return -1;
}

void Player::addContainer(uint32_t cid, Container* container)
{
#ifdef __DEBUG__
	std::cout << getName() << ", addContainer: " << (int)cid << std::endl;
#endif
	if(cid > 0xF)
		return;

	for(ContainerVector::iterator cl = containerVec.begin(); cl != containerVec.end(); ++cl) {
		if(cl->first == cid) {
			cl->second = container;
			return;
		}
	}
	
	//id doesnt exist, create it
	containervector_pair cv;
	cv.first = cid;
	cv.second = container;

	containerVec.push_back(cv);
}

void Player::closeContainer(uint32_t cid)
{
  for(ContainerVector::iterator cl = containerVec.begin(); cl != containerVec.end(); ++cl){
	  if(cl->first == cid){
		  containerVec.erase(cl);
			break;
		}
	}

#ifdef __DEBUG__
	std::cout << getName() << ", closeContainer: " << (int)cid << std::endl;
#endif
}

uint16_t Player::getLookCorpse() const
{
	if(sex != 0)
		return ITEM_MALE_CORPSE;
	else
		return ITEM_FEMALE_CORPSE;
}

void Player::dropLoot(Container* corpse)
{
	if(corpse && lootDrop){
		for(int i = SLOT_FIRST; i < SLOT_LAST; ++i){
			Item* item = inventory[i];
	#ifdef __SKULLSYSTEM__
			if(item && ((item->getContainer()) || random_range(1, 100) <= 10 || getSkull() == SKULL_RED)){
	#else
			if(item && ((item->getContainer()) || random_range(1, 100) <= 10)){
	#endif
				g_game.internalMoveItem(this, corpse, INDEX_WHEREEVER, item, item->getItemCount());
			}
		}
	}
}

void Player::addStorageValue(const uint32_t key, const int32_t value)
{
	if(IS_IN_KEYRANGE(key, RESERVED_RANGE)){
		if(IS_IN_KEYRANGE(key, OUTFITS_RANGE)){
			Outfit outfit;
			outfit.looktype = value >> 16;
			outfit.addons = value & 0xFF;
			if(outfit.addons > 3){
				std::cout << "Warning: No valid addons value key:" << key << " value: " << (int)(value & 0xFF) << " player: " << getName() << std::endl;
			}
			else{
				m_playerOutfits.addOutfit(outfit);
			}
		}
		else{
			//warning
			std::cout << "Warning: unknown reserved key: " << key << " player: " << getName() << std::endl;
		}
	}
	else{
		storageMap[key] = value;
	}
}

bool Player::getStorageValue(const uint32_t key, int32_t& value) const
{
	StorageMap::const_iterator it;
	it = storageMap.find(key);
	if(it != storageMap.end()){
		value = it->second;
		return true;
	}
	else{
		value = 0;
		return false;
	}
}

bool Player::canSee(const Position& pos) const
{
	return client->canSee(pos);
}

bool Player::canSeeCreature(const Creature* creature) const
{
	if(creature->isInvisible() && !creature->getPlayer() && !hasFlag(PlayerFlag_CanSenseInvisibility) && !canSeeInvisibility()){
		return false;
	}

	return true;
}


Depot* Player::getDepot(uint32_t depotId, bool autoCreateDepot)
{	
	DepotMap::iterator it = depots.find(depotId);
	if(it != depots.end()){	
		return it->second;
	}

	//depot does not yet exist

	//create a new depot?
	if(autoCreateDepot){
		Depot* depot = NULL;
		Item* tmpDepot = Item::CreateItem(ITEM_LOCKER1);
		if(tmpDepot->getContainer() && (depot = tmpDepot->getContainer()->getDepot())){
			Item* depotChest = Item::CreateItem(ITEM_DEPOT);
			depot->__internalAddThing(depotChest);

			addDepot(depot, depotId);
			return depot;
		}
		else{
			g_game.FreeThing(tmpDepot);
			std::cout << "Failure: Creating a new depot with id: "<< depotId <<
				", for player: " << getName() << std::endl;
		}
	}

	return NULL;
}

bool Player::addDepot(Depot* depot, uint32_t depotId)
{
	Depot* depot2 = getDepot(depotId, false);
	if(depot2){
		return false;
	}
	
	depots[depotId] = depot;
	depot->setMaxDepotLimit(maxDepotLimit);
	return true;
}

void Player::sendCancelMessage(ReturnValue message) const
{
	switch(message){
	case RET_DESTINATIONOUTOFREACH:
		sendCancel("Destination is out of reach.");
		break;

	case RET_NOTMOVEABLE:
		sendCancel("You cannot move this object.");
		break;

	case RET_DROPTWOHANDEDITEM:
		sendCancel("Drop the double-handed object first.");
		break;

	case RET_BOTHHANDSNEEDTOBEFREE:
		sendCancel("Both hands needs to be free.");
		break;

	case RET_CANNOTBEDRESSED:
		sendCancel("You cannot dress this object there.");
		break;

	case RET_PUTTHISOBJECTINYOURHAND:
		sendCancel("Put this object in your hand.");
		break;

	case RET_PUTTHISOBJECTINBOTHHANDS:
		sendCancel("Put this object in both hands.");
		break;

	case RET_CANONLYUSEONEWEAPON:
		sendCancel("You may only use one weapon.");
		break;

	case RET_TOOFARAWAY:
		sendCancel("Too far away.");
		break;

	case RET_FIRSTGODOWNSTAIRS:
		sendCancel("First go downstairs.");
		break;

	case RET_FIRSTGOUPSTAIRS:
		sendCancel("First go upstairs.");
		break;

	case RET_NOTENOUGHCAPACITY:
		sendCancel("This object is too heavy.");
		break;
		
	case RET_CONTAINERNOTENOUGHROOM:
		sendCancel("You cannot put more objects in this container.");
		break;

	case RET_NEEDEXCHANGE:
	case RET_NOTENOUGHROOM:
		sendCancel("There is not enough room.");
		break;

	case RET_CANNOTPICKUP:
		sendCancel("You cannot pickup this object.");
		break;

	case RET_CANNOTTHROW:
		sendCancel("You cannot throw there.");
		break;

	case RET_THEREISNOWAY:
		sendCancel("There is no way.");
		break;
		
	case RET_THISISIMPOSSIBLE:
		sendCancel("This is impossible.");
		break;

	case RET_PLAYERISPZLOCKED:
		sendCancel("You can not enter a protection zone after attacking another player.");
		break;

	case RET_PLAYERISNOTINVITED:
		sendCancel("You are not invited.");
		break;

	case RET_CREATUREDOESNOTEXIST:
		sendCancel("Creature does not exist.");
		break;

	case RET_DEPOTISFULL:
		sendCancel("You cannot put more items in this depot.");
		break;

	case RET_CANNOTUSETHISOBJECT:
		sendCancel("You can not use this object.");
		break;

	case RET_PLAYERWITHTHISNAMEISNOTONLINE:
		sendCancel("A player with this name is not online.");
		break;

	case RET_NOTREQUIREDLEVELTOUSERUNE:
		sendCancel("You do not have the required magic level to use this rune.");
		break;
		
	case RET_YOUAREALREADYTRADING:
		sendCancel("You are already trading.");
		break;

	case RET_THISPLAYERISALREADYTRADING:
		sendCancel("This player is already trading.");
		break;

	case RET_YOUMAYNOTLOGOUTDURINGAFIGHT:
		sendCancel("You may not logout during or immediately after a fight!");
		break;

	case RET_DIRECTPLAYERSHOOT:
		sendCancel("You are not allowed to shoot directly on players.");
		break;

	case RET_NOTENOUGHLEVEL:
		sendCancel("You do not have enough level.");
		break;

	case RET_NOTENOUGHMAGICLEVEL:
		sendCancel("You do not have enough magic level.");
		break;

	case RET_NOTENOUGHMANA:
		sendCancel("You do not have enough mana.");
		break;

	case RET_NOTENOUGHSOUL:
		sendCancel("You do not have enough soul");
		break;

	case RET_YOUAREEXHAUSTED:
		sendCancel("You are exhausted.");
		break;

	case RET_CANONLYUSETHISRUNEONCREATURES:
		sendCancel("You can only use this rune on creatures.");
		break;
	
	case RET_PLAYERISNOTREACHABLE:
		sendCancel("Player is not reachable.");
		break;

	case RET_ACTIONNOTPERMITTEDINPROTECTIONZONE:
		sendCancel("This action is not permitted in a protection zone.");
		break;
	
	case RET_YOUMAYNOTATTACKTHISPLAYER:
		sendCancel("You may not attack this player.");
		break;

	case RET_YOUMAYNOTATTACKTHISCREATURE:
		sendCancel("You may not attack this creature.");
		break;

	case RET_YOUMAYNOTATTACKAPERSONINPROTECTIONZONE:
		sendCancel("You may not attack a person in a protection zone.");
		break;
	
	case RET_YOUMAYNOTATTACKAPERSONWHILEINPROTECTIONZONE:
		sendCancel("You may not attack a person while you are in a protection zone.");
		break;
	

	case RET_NOTPOSSIBLE:
	default:
		sendCancel("Sorry, not possible.");
		break;
	}
}

void Player::sendStats()
{
	//update level and magLevel percents
	if(lastSentStats.experience != getExperience() || lastSentStats.level != level){
		uint32_t currentExpLevel = getExpForLv(level);
		int32_t percent = (100*(experience - currentExpLevel)) / std::max((int32_t)1, (int32_t)(getExpForLv(level + 1) - currentExpLevel));

		if(percent < 0){
			percent = 0;
		}
		else if(percent > 100){
			percent = 100;
		}

		level_percent = percent;
	}
			
	if(lastSentStats.manaSpent != manaSpent || lastSentStats.magLevel != magLevel){
		int32_t percent = (100 * manaSpent) / std::max((int32_t)1, (int32_t)(vocation->getReqMana(magLevel + 1)));
		if(percent < 0){
			percent = 0;
		}
		else if(percent > 100){
			percent = 100;
		}

		maglevel_percent = percent;
	}
			
	//save current stats 
	lastSentStats.health = getHealth();
	lastSentStats.healthMax = getMaxHealth();
	lastSentStats.freeCapacity = getFreeCapacity();
	lastSentStats.experience = getExperience();
	lastSentStats.level = getLevel();
	lastSentStats.mana = getMana();
	lastSentStats.manaMax = getPlayerInfo(PLAYERINFO_MAXMANA);
	lastSentStats.magLevel = getMagicLevel();
	lastSentStats.manaSpent = manaSpent;
	
	client->sendStats();
}

void Player::sendPing(uint32_t interval)
{
	internal_ping = internal_ping + interval;
	if(internal_ping >= 5000){ //1 ping each 5 seconds
		internal_ping = 0;
		npings++;
		client->sendPing();
	}
	if(npings >= 6){
		//std::cout << "logout" << std::endl;
		if(hasCondition(CONDITION_INFIGHT) && health > 0){
			//logout?
			//client->logout();
		}
		else{
			//client->logout();			
		}
	}
}

void Player::sendHouseWindow(House* _house, uint32_t _listid) const
{
	std::string text;
	if(_house->getAccessList(_listid, text)){
		client->sendHouseWindow(_house, _listid, text);
	}
}


//container
void Player::sendAddContainerItem(const Container* container, const Item* item)
{
  for(ContainerVector::const_iterator cl = containerVec.begin(); cl != containerVec.end(); ++cl){
		if(cl->second == container){
			client->sendAddContainerItem(cl->first, item);
		}
	}
}

void Player::sendUpdateContainerItem(const Container* container, uint8_t slot, const Item* oldItem, const Item* newItem)
{
  for(ContainerVector::const_iterator cl = containerVec.begin(); cl != containerVec.end(); ++cl){
		if(cl->second == container){
			client->sendUpdateContainerItem(cl->first, slot, newItem);
		}
	}
}

void Player::sendRemoveContainerItem(const Container* container, uint8_t slot, const Item* item)
{
  for(ContainerVector::const_iterator cl = containerVec.begin(); cl != containerVec.end(); ++cl){
		if(cl->second == container){
			client->sendRemoveContainerItem(cl->first, slot);
		}
	}
}

void Player::onAddTileItem(const Position& pos, const Item* item)
{
	Creature::onAddTileItem(pos, item);
}

void Player::onUpdateTileItem(const Position& pos, uint32_t stackpos, const Item* oldItem, const Item* newItem)
{
	Creature::onUpdateTileItem(pos, stackpos, oldItem, newItem);

	if(oldItem != newItem){
		onRemoveTileItem(pos, stackpos, oldItem);
	}

	if(tradeState != TRADE_TRANSFER){
		if(tradeItem && oldItem == tradeItem){
			g_game.playerCloseTrade(this);
		}
	}
}

void Player::onRemoveTileItem(const Position& pos, uint32_t stackpos, const Item* item)
{
	if(tradeState != TRADE_TRANSFER){
		checkTradeState(item);

		if(tradeItem){
			const Container* container = item->getContainer();
			if(container && container->isHoldingItem(tradeItem)){
				g_game.playerCloseTrade(this);
			}
		}
	}
}

void Player::onUpdateTile(const Position& pos)
{
	//
}

void Player::onCreatureAppear(const Creature* creature, bool isLogin)
{
	if(isLogin && creature == this){
		Item* item;
		for(int slot = SLOT_FIRST; slot < SLOT_LAST; ++slot){
			if(item = getInventoryItem((slots_t)slot)){
				item->__startDecaying();
				g_moveEvents->onPlayerEquip(this, item, (slots_t)slot, true);
			}
		}

		if(!storedConditionList.empty()){
			for(ConditionList::const_iterator it = storedConditionList.begin(); it != storedConditionList.end(); ++it){
				addCondition(*it);
			}

			storedConditionList.clear();
		}
	}
}

void Player::onAttackedCreatureDissapear(bool isLogout)
{
	sendCancelTarget();

	if(!isLogout){
		sendTextMessage(MSG_STATUS_SMALL, "Target lost.");
	}
}

void Player::onFollowCreatureDissapear(bool isLogout)
{
	sendCancelTarget();

	if(!isLogout){
		sendTextMessage(MSG_STATUS_SMALL, "Target lost.");
	}
}

void Player::onAttackedCreatureEnterProtectionZone(const Creature* creature)
{
	if(!hasFlag(PlayerFlag_IgnoreProtectionZone)){
		Creature::onAttackedCreatureEnterProtectionZone(creature);
	}
}

void Player::onCreatureDisappear(const Creature* creature, uint32_t stackpos, bool isLogout)
{
	Creature::onCreatureDisappear(creature, stackpos, isLogout);

	if(creature == this){
		
		if(isLogout){
			loginPosition = getPosition();
		}

		if(eventWalk != 0){
			setFollowCreature(NULL);
		}

		if(tradePartner){
			g_game.playerCloseTrade(this);
		}

		g_chat.removeUserFromAllChannels(this);
		bool saved = false;
		for(uint32_t tries = 0; tries < 3; ++tries){
			if(IOPlayer::instance()->savePlayer(this)){
				saved = true;
				break;
			}
		}
		if(!saved){
			std::cout << "Error while saving player: " << getName() << std::endl;
		}

#ifdef __DEBUG_PLAYERS__
		std::cout << (uint32_t)g_game.getPlayersOnline() << " players online." << std::endl;
#endif
	}
}

void Player::onCreatureMove(const Creature* creature, const Position& newPos, const Position& oldPos,
	uint32_t oldStackPos, bool teleport)
{
	Creature::onCreatureMove(creature, newPos, oldPos, oldStackPos, teleport);

	if(creature == this){
		if(tradeState != TRADE_TRANSFER){
			//check if we should close trade
			if(tradeItem){
				if(!Position::areInRange<1,1,0>(tradeItem->getPosition(), getPosition())){
					g_game.playerCloseTrade(this);
				}
			}

			if(tradePartner){
				if(!Position::areInRange<2,2,0>(tradePartner->getPosition(), getPosition())){
					g_game.playerCloseTrade(this);
				}
			}
		}
	}
}

//container
void Player::onAddContainerItem(const Container* container, const Item* item)
{
	checkTradeState(item);
}

void Player::onUpdateContainerItem(const Container* container, uint8_t slot, const Item* oldItem, const Item* newItem)
{
	if(oldItem != newItem){
		onRemoveContainerItem(container, slot, oldItem);
	}

	if(tradeState != TRADE_TRANSFER){
		checkTradeState(oldItem);
	}
}

void Player::onRemoveContainerItem(const Container* container, uint8_t slot, const Item* item)
{
	if(tradeState != TRADE_TRANSFER){
		checkTradeState(item);
		
		if(tradeItem){
			if(tradeItem->getParent() != container && container->isHoldingItem(tradeItem)){
				g_game.playerCloseTrade(this);
			}
		}
	}
}

void Player::onCloseContainer(const Container* container)
{
  for(ContainerVector::const_iterator cl = containerVec.begin(); cl != containerVec.end(); ++cl){
		if(cl->second == container){
			client->sendCloseContainer(cl->first);
		}
	}
}

void Player::onSendContainer(const Container* container)
{
	bool hasParent = (dynamic_cast<const Container*>(container->getParent()) != NULL);

	for(ContainerVector::const_iterator cl = containerVec.begin(); cl != containerVec.end(); ++cl){
		if(cl->second == container){
			client->sendContainer(cl->first, container, hasParent);
		}
	}
}

//inventory
void Player::onAddInventoryItem(slots_t slot, Item* item)
{
	//
}

void Player::onUpdateInventoryItem(slots_t slot, Item* oldItem, Item* newItem)
{
	if(oldItem != newItem){
		onRemoveInventoryItem(slot, oldItem);
	}

	if(tradeState != TRADE_TRANSFER){
		checkTradeState(oldItem);
	}
}

void Player::onRemoveInventoryItem(slots_t slot, Item* item)
{
	//setItemAbility(slot, false);

	if(tradeState != TRADE_TRANSFER){
		checkTradeState(item);

		if(tradeItem){
			const Container* container = item->getContainer();
			if(container && container->isHoldingItem(tradeItem)){
				g_game.playerCloseTrade(this);
			}
		}
	}
}

void Player::checkTradeState(const Item* item)
{
	if(tradeItem && tradeState != TRADE_TRANSFER){
		if(tradeItem == item){
			g_game.playerCloseTrade(this);
		}
		else{
			const Container* container = dynamic_cast<const Container*>(item->getParent());

			while(container != NULL){
				if(container == tradeItem){
					g_game.playerCloseTrade(this);
					break;
				}

				container = dynamic_cast<const Container*>(container->getParent());
			}
		}
	}
}

bool Player::NeedUpdateStats()
{
	if(lastSentStats.health != getHealth() ||
		 lastSentStats.healthMax != healthMax ||
		 (int)lastSentStats.freeCapacity != (int)getFreeCapacity() ||
		 lastSentStats.experience != getExperience() ||
		 lastSentStats.level != getLevel() ||
		 lastSentStats.mana != getMana() ||
		 lastSentStats.manaMax != manaMax ||
		 lastSentStats.manaSpent != manaSpent ||
		 lastSentStats.magLevel != magLevel){
		return true;
	}
	else{
		return false;
	}
}

void Player::onThink(uint32_t interval)
{
	Creature::onThink(interval);
	sendPing(interval);

	MessageBufferTicks += interval;

	if(MessageBufferTicks >= 1500){
		MessageBufferTicks = 0;
		addMessageBuffer();
	}

#ifdef __SKULLSYSTEM__
	checkRedSkullTicks(interval);
#endif
}

bool Player::isMuted(uint32_t& muteTime)
{
	if(hasFlag(PlayerFlag_CannotBeMuted)){
		return false;
	}

	Condition* condition = getCondition(CONDITION_MUTED, CONDITIONID_DEFAULT);
	if(condition){
		muteTime = std::max((uint32_t)1, (uint32_t)condition->getTicks() / 1000);
		return true;
	}

	muteTime = 0;
	return false;
}

void Player::addMessageBuffer()
{
	if(MessageBufferCount > 0 && Player::maxMessageBuffer != 0 && !hasFlag(PlayerFlag_CannotBeMuted)){
		MessageBufferCount -= 1;
	}
}

void Player::removeMessageBuffer()
{
	if(!hasFlag(PlayerFlag_CannotBeMuted) && MessageBufferCount <= Player::maxMessageBuffer + 1 && Player::maxMessageBuffer != 0){
		MessageBufferCount += 1;

		if(MessageBufferCount > Player::maxMessageBuffer){
			uint32_t muteCount = 1;
			MuteCountMap::iterator it = muteCountMap.find(getGUID());
			if(it != muteCountMap.end()){
				muteCount = it->second;
			}

			uint32_t muteTime = 5 * muteCount * muteCount;
			muteCountMap[getGUID()] = muteCount + 1;
			Condition* condition = Condition::createCondition(CONDITIONID_DEFAULT, CONDITION_MUTED, muteTime * 1000, 0);
			addCondition(condition);

			std::stringstream ss;
			ss << "You are muted for " << muteTime << " seconds.";
			sendTextMessage(MSG_STATUS_SMALL, ss.str());
		}
	}
}

void Player::drainHealth(Creature* attacker, CombatType_t combatType, int32_t damage)
{
	Creature::drainHealth(attacker, combatType, damage);

	sendStats();

	std::stringstream ss;
	if(damage == 1) {
		ss << "You lose 1 hitpoint";
	}
	else
		ss << "You lose " << damage << " hitpoints";

	if(attacker){
		ss << " due to an attack by " << attacker->getNameDescription();
	}

	ss << ".";

	sendTextMessage(MSG_EVENT_DEFAULT, ss.str());
}

void Player::drainMana(Creature* attacker, int32_t manaLoss)
{
	Creature::drainMana(attacker, manaLoss);
	
	sendStats();

	std::stringstream ss;
	if(attacker){
		ss << "You lose " << manaLoss << " mana blocking an attack by " << attacker->getNameDescription() << ".";
	}
	else{
		ss << "You lose " << manaLoss << " mana.";
	}

	sendTextMessage(MSG_EVENT_DEFAULT, ss.str());
}

void Player::addManaSpent(uint32_t amount)
{
	if(amount != 0 && !hasFlag(PlayerFlag_NotGainMana)){
		manaSpent += amount * g_config.getNumber(ConfigManager::RATE_MAGIC);
		int reqMana = vocation->getReqMana(magLevel + 1);

		if(manaSpent >= reqMana){
			manaSpent -= reqMana;
			magLevel++;
			
			std::stringstream MaglvMsg;
			MaglvMsg << "You advanced to magic level " << magLevel << ".";
			sendTextMessage(MSG_EVENT_ADVANCE, MaglvMsg.str());
			sendStats();
		}
	}
}

void Player::addExperience(uint32_t exp)
{
	experience += exp;
	int prevLevel = getLevel();
	int newLevel = getLevel();

	while(experience >= getExpForLv(newLevel + 1)){
		++newLevel;
		healthMax += vocation->getHPGain();
		health += vocation->getHPGain();
		manaMax += vocation->getManaGain();
		mana += vocation->getManaGain();
		capacity += vocation->getCapGain();
	}

	if(prevLevel != newLevel){
		//int32_t oldSpeed = getBaseSpeed();		
		level = newLevel;
		updateBaseSpeed();

		int32_t newSpeed = getBaseSpeed();
		setBaseSpeed(newSpeed);

		g_game.changeSpeed(this, 0);
		g_game.addCreatureHealth(this);

		std::stringstream levelMsg;
		levelMsg << "You advanced from Level " << prevLevel << " to Level " << newLevel << ".";
		sendTextMessage(MSG_EVENT_ADVANCE, levelMsg.str());
	}

	sendStats();
}

void Player::onBlockHit(BlockType_t blockType)
{
	if(shieldBlockCount > 0){
		--shieldBlockCount;
		
		if(hasShield()){
			addSkillAdvance(SKILL_SHIELD, 1);
		}
	}
}

void Player::onAttackedCreatureBlockHit(Creature* target, BlockType_t blockType)
{
	Creature::onAttackedCreatureBlockHit(target, blockType);

	lastAttackBlockType = blockType;

	switch(blockType){
		case BLOCK_NONE:
		{
			addAttackSkillPoint = true;
			bloodHitCount = 30;
			shieldBlockCount = 30;
			
			break;
		}

		case BLOCK_DEFENSE:
		case BLOCK_ARMOR:
		{
			//need to draw blood every 30 hits
			if(bloodHitCount > 0){
				addAttackSkillPoint = true;
				--bloodHitCount;
			}
			else{
				addAttackSkillPoint = false;
			}

			break;
		}

		default:
		{
			addAttackSkillPoint = false;
			break;
		}
	}
}

bool Player::hasShield() const
{
	bool result = false;
	Item* item;

	item = getInventoryItem(SLOT_LEFT);
	if(item && item->getWeaponType() == WEAPON_SHIELD){
		result = true;
	}

	item = getInventoryItem(SLOT_RIGHT);
	if(item && item->getWeaponType() == WEAPON_SHIELD){
		result = true;
	}

	return result;
}

BlockType_t Player::blockHit(Creature* attacker, CombatType_t combatType, int32_t& damage,
	bool checkDefense /* = false*/, bool checkArmor /* = false*/)
{
	BlockType_t blockType = Creature::blockHit(attacker, combatType, damage, checkDefense, checkArmor);

	if(attacker){
		sendCreatureSquare(attacker, SQ_COLOR_BLACK);
	}

	if(blockType != BLOCK_NONE){
		return blockType;
	}

	/*
	if(blockType != BLOCK_NONE){
		if(blockType != BLOCK_IMMUNITY){
			if(shieldBlockCount > 0){
				--shieldBlockCount;
				
				if(hasShield()){
					addSkillAdvance(SKILL_SHIELD, 1);
				}
			}
		}

		return blockType;
	}
	*/

	bool absorbedDamage;

	//reduce damage against inventory items
	Item* item = NULL;
	for(int slot = SLOT_FIRST; slot < SLOT_LAST; ++slot){
		if(!isItemAbilityEnabled((slots_t)slot)){
			continue;
		}

		if(!(item = getInventoryItem((slots_t)slot)))
			continue;

		const ItemType& it = Item::items[item->getID()];
		absorbedDamage = false;

		if(it.abilities.absorbPercentAll != 0){
			if(it.abilities.absorbPercentAll > 0){
				damage = (int32_t)std::ceil(damage * ((float)(100 - it.abilities.absorbPercentAll) / 100));
				absorbedDamage = true;
			}
		}

		switch(combatType){
			case COMBAT_PHYSICALDAMAGE:
			{
				if(it.abilities.absorbPercentPhysical > 0){
					damage = (int32_t)std::ceil(damage * ((float)(100 - it.abilities.absorbPercentPhysical) / 100));
					absorbedDamage = true;
				}
				break;
			}

			case COMBAT_FIREDAMAGE:
			{
				if(it.abilities.absorbPercentFire > 0){
					damage = (int32_t)std::ceil(damage * ((float)(100 - it.abilities.absorbPercentFire) / 100));
					absorbedDamage = true;
				}
				break;
			}

			case COMBAT_ENERGYDAMAGE:
			{
				if(it.abilities.absorbPercentEnergy > 0){
					damage = (int32_t)std::ceil(damage * ((float)(100 - it.abilities.absorbPercentEnergy) / 100));
					absorbedDamage = true;
				}
				break;
			}

			case COMBAT_POISONDAMAGE:
			{
				if(it.abilities.absorbPercentPoison > 0){
					damage = (int32_t)std::ceil(damage * ((float)(100 - it.abilities.absorbPercentPoison) / 100));
					absorbedDamage = true;
				}
				break;
			}

			case COMBAT_LIFEDRAIN:
			{
				if(it.abilities.absorbPercentLifeDrain > 0){
					damage = (int32_t)std::ceil(damage * ((float)(100 - it.abilities.absorbPercentLifeDrain) / 100));
					absorbedDamage = true;
				}
				break;
			}

			case COMBAT_MANADRAIN:
			{
				if(it.abilities.absorbPercentManaDrain > 0){
					damage = (int32_t)std::ceil(damage * ((float)(100 - it.abilities.absorbPercentManaDrain) / 100));
					absorbedDamage = true;
				}
				break;
			}

			case COMBAT_DROWNDAMAGE:
			{
				if(it.abilities.absorbPercentDrown > 0){
					damage = (int32_t)std::ceil(damage * ((float)(100 - it.abilities.absorbPercentDrown) / 100));
					absorbedDamage = true;
				}
				break;
			}

			default:
				break;
		}
	
		if(absorbedDamage){
			int32_t charges = item->getItemCharge();

			if(charges != 0){
				g_game.transformItem(item, item->getID(), charges - 1);
			}
		}
	}

	if(damage <= 0){
		damage = 0;
		blockType = BLOCK_DEFENSE;
	}

	return blockType;
}

uint32_t Player::getIP() const
{
	return client->getIP();
}

void Player::die()
{
	Creature::die();

	sendTextMessage(MSG_EVENT_ADVANCE, "You are dead.");
	loginPosition = masterPos;

	//Magic level loss
	uint32_t sumMana = 0;
	int32_t lostMana = 0;
	
	//sum up all the mana
	for(int32_t i = 1; i <= magLevel; ++i){
		sumMana += vocation->getReqMana(i);
	}

	sumMana += manaSpent;

	lostMana = (int32_t)(sumMana * 0.1);   //player loses 10% of all spent mana when he dies
    
	while(lostMana > manaSpent){
		lostMana -= manaSpent;
		manaSpent = vocation->getReqMana(magLevel);
		magLevel--;
	}

	manaSpent -= lostMana;
	//

	//Skill loss
	uint32_t lostSkillTries;
	uint32_t sumSkillTries;
	for(uint32_t i = 0; i <= 6; ++i){  //for each skill
		lostSkillTries = 0;         //reset to 0
		sumSkillTries = 0;

		for(uint32_t c = 11; c <= skills[i][SKILL_LEVEL]; ++c) { //sum up all required tries for all skill levels
			sumSkillTries += vocation->getReqSkillTries(i, c);
		}

		sumSkillTries += skills[i][SKILL_TRIES];
		lostSkillTries = (uint32_t)(sumSkillTries * 0.1);           //player loses 10% of his skill tries

		while(lostSkillTries > skills[i][SKILL_TRIES]){
			lostSkillTries -= skills[i][SKILL_TRIES];
			skills[i][SKILL_TRIES] = vocation->getReqSkillTries(i, skills[i][SKILL_LEVEL]);
			if(skills[i][SKILL_LEVEL] > 10){
				skills[i][SKILL_LEVEL]--;
			}
			else{
				skills[i][SKILL_LEVEL] = 10;
				skills[i][SKILL_TRIES] = 0;
				lostSkillTries = 0;
				break;
			}
		}

		skills[i][SKILL_TRIES] -= lostSkillTries;
	}               
	//

	//Level loss
	int32_t newLevel = level;
	while((uint32_t)(experience - getLostExperience()) < getExpForLv(newLevel)){
		if(newLevel > 1)
			newLevel--;
		else
			break;
	}

	if(newLevel != level){
		std::stringstream lvMsg;
		lvMsg << "You were downgraded from level " << level << " to level " << newLevel << ".";
		client->sendTextMessage(MSG_EVENT_ADVANCE, lvMsg.str());
	}

	for(ConditionList::iterator it = conditions.begin(); it != conditions.end();){
		if((*it)->isPersistent()){
			Condition* condition = *it;
			it = conditions.erase(it);

			condition->endCondition(this, CONDITIONEND_DIE);
			onEndCondition(condition->getType());
			delete condition;
		}
		else{
			++it;
		}
	}

	//client->sendReLoginWindow();
}

Item* Player::getCorpse()
{
	Item* corpse = Creature::getCorpse();
	if(corpse && corpse->getContainer()){
		std::stringstream ss;

		ss << "You recognize " << getNameDescription() << ".";

		Creature* lastHitCreature = NULL;
		Creature* mostDamageCreature = NULL;

		if(getKillers(&lastHitCreature, &mostDamageCreature) && lastHitCreature){
			ss << " " << (getSex() == PLAYERSEX_FEMALE ? "She" : "He") << " was killed by "
				<< lastHitCreature->getNameDescription() << ".";
		}
		
		corpse->setSpecialDescription(ss.str());
	}

	return corpse;
}

void Player::preSave()
{
	if(health <= 0){
		experience -= getLostExperience();
				
		while(level > 1 && experience < getExpForLv(level)){
			--level;
			healthMax = std::max((int32_t)0, (healthMax - (int32_t)vocation->getHPGain()));
			manaMax = std::max((int32_t)0, (manaMax - (int32_t)vocation->getManaGain()));
			capacity = std::max((double)0, (capacity - (double)vocation->getCapGain()));
		}

		health = healthMax;
		mana = manaMax;
	}
}

void Player::addExhaustionTicks(uint32_t ticks)
{
	Condition* condition = Condition::createCondition(CONDITIONID_DEFAULT, CONDITION_EXHAUSTED, ticks, 0);
	addCondition(condition);
}

void Player::addInFightTicks()
{
	Condition* condition = Condition::createCondition(CONDITIONID_DEFAULT, CONDITION_INFIGHT, g_game.getInFightTicks(), 0);
	addCondition(condition);
}

void Player::addDefaultRegeneration(uint32_t addTicks)
{
	Condition* condition = getCondition(CONDITION_REGENERATION, CONDITIONID_DEFAULT);

	if(condition){
		condition->setTicks(condition->getTicks() + addTicks);
	}
	else{
		condition = Condition::createCondition(CONDITIONID_DEFAULT, CONDITION_REGENERATION, addTicks, 0);
		condition->setParam(CONDITIONPARAM_HEALTHGAIN, vocation->getHealthGainAmount());
		condition->setParam(CONDITIONPARAM_HEALTHTICKS, vocation->getHealthGainTicks() * 1000);
		condition->setParam(CONDITIONPARAM_MANAGAIN, vocation->getManaGainAmount());
		condition->setParam(CONDITIONPARAM_MANATICKS, vocation->getManaGainTicks() * 1000);

		addCondition(condition);
	}
}

void Player::removeList()
{
	listPlayer.removeList(getID());
	
	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
	{
		(*it).second->notifyLogOut(this);
	}	
}

void Player::addList()
{
	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
	{
		(*it).second->notifyLogIn(this);
	}	
	
	listPlayer.addList(this);
}

void Player::notifyLogIn(Player* login_player)
{
	VIPListSet::iterator it = VIPList.find(login_player->getGUID());
	if(it != VIPList.end()){
		client->sendVIPLogIn(login_player->getGUID());
		sendTextMessage(MSG_STATUS_SMALL, (login_player->getName() + " has logged in."));
	}
}

void Player::notifyLogOut(Player* logout_player)
{
	VIPListSet::iterator it = VIPList.find(logout_player->getGUID());
	if(it != VIPList.end()){
		client->sendVIPLogOut(logout_player->getGUID());
		sendTextMessage(MSG_STATUS_SMALL, (logout_player->getName() + " has logged out."));
	}
}

bool Player::removeVIP(uint32_t _guid)
{
	VIPListSet::iterator it = VIPList.find(_guid);
	if(it != VIPList.end()){
		VIPList.erase(it);
		return true;
	}
	return false;
}

bool Player::addVIP(uint32_t _guid, std::string& name, bool isOnline, bool internal /*=false*/)
{
	if(guid == _guid){
		if(!internal)
			sendTextMessage(MSG_STATUS_SMALL, "You cannot add yourself.");
		return false;
	}
	
	if(VIPList.size() > maxVipLimit){
		if(!internal)
			sendTextMessage(MSG_STATUS_SMALL, "You cannot add more buddies.");
		return false;
	}
	
	VIPListSet::iterator it = VIPList.find(_guid);
	if(it != VIPList.end()){
		if(!internal)
			sendTextMessage(MSG_STATUS_SMALL, "This player is already in your list.");
		return false;
	}
	
	VIPList.insert(_guid);
	
	if(!internal)
		client->sendVIP(_guid, name, isOnline);
	
	return true;
}

//close container and its child containers
void Player::autoCloseContainers(const Container* container)
{
	typedef std::vector<uint32_t> CloseList;
	CloseList closeList;
	
	for(ContainerVector::iterator it = containerVec.begin(); it != containerVec.end(); ++it){
		Container* tmpcontainer = it->second;
		while(tmpcontainer != NULL){
			if(tmpcontainer->isRemoved() || tmpcontainer == container){
				closeList.push_back(it->first);
				break;
			}
			
			tmpcontainer = dynamic_cast<Container*>(tmpcontainer->getParent());
		}
	}
	
	for(CloseList::iterator it = closeList.begin(); it != closeList.end(); ++it){
		closeContainer(*it);
		client->sendCloseContainer(*it);
	}						
}

bool Player::hasCapacity(const Item* item, uint32_t count) const
{
	if(hasFlag(PlayerFlag_CannotPickupItem)){
		return false;
	}

	if(!hasFlag(PlayerFlag_HasInfiniteCapacity) && item->getTopParent() != this){
		double itemWeight = 0;

		if(item->isStackable()){
			itemWeight = Item::items[item->getID()].weight * count;
		}
		else
			itemWeight = item->getWeight();

		return (itemWeight < getFreeCapacity());
	}

	return true;
}

ReturnValue Player::__queryAdd(int32_t index, const Thing* thing, uint32_t count,
	uint32_t flags) const
{
	const Item* item = thing->getItem();
	if(item == NULL){
		return RET_NOTPOSSIBLE;
	}

	bool childIsOwner = ((flags & FLAG_CHILDISOWNER) == FLAG_CHILDISOWNER);
	bool skipLimit = ((flags & FLAG_NOLIMIT) == FLAG_NOLIMIT);

	if(childIsOwner){
		//a child container is querying the player, just check if enough capacity
		if(skipLimit || hasCapacity(item, count))
			return RET_NOERROR;
		else
			return RET_NOTENOUGHCAPACITY;
	}

	if(!item->isPickupable()){
		return RET_CANNOTPICKUP;
	}
	
	ReturnValue ret = RET_NOERROR;
	
	if((item->getSlotPosition() & SLOTP_HEAD) ||
		(item->getSlotPosition() & SLOTP_NECKLACE) ||
		(item->getSlotPosition() & SLOTP_BACKPACK) ||
		(item->getSlotPosition() & SLOTP_ARMOR) ||
		(item->getSlotPosition() & SLOTP_LEGS) ||
		(item->getSlotPosition() & SLOTP_FEET) ||
		(item->getSlotPosition() & SLOTP_RING)){
		ret = RET_CANNOTBEDRESSED;
	}
	else if(item->getSlotPosition() & SLOTP_TWO_HAND){
		ret = RET_PUTTHISOBJECTINBOTHHANDS;
	}
	else if((item->getSlotPosition() & SLOTP_RIGHT) || (item->getSlotPosition() & SLOTP_LEFT)){
		ret = RET_PUTTHISOBJECTINYOURHAND;
	}

	//check if we can dress this object
	switch(index){
		case SLOT_HEAD:
			if(item->getSlotPosition() & SLOTP_HEAD)
				ret = RET_NOERROR;
			break;
		case SLOT_NECKLACE:
			if(item->getSlotPosition() & SLOTP_NECKLACE)
				ret = RET_NOERROR;
			break;
		case SLOT_BACKPACK:
			if(item->getSlotPosition() & SLOTP_BACKPACK)
				ret = RET_NOERROR;
			break;
		case SLOT_ARMOR:
			if(item->getSlotPosition() & SLOTP_ARMOR)
				ret = RET_NOERROR;
			break;
		case SLOT_RIGHT:
			if(item->getSlotPosition() & SLOTP_RIGHT){
				//check if we already carry an item in the other hand
				if(item->getSlotPosition() & SLOTP_TWO_HAND){
					if(inventory[SLOT_LEFT] && inventory[SLOT_LEFT] != item){
						ret = RET_BOTHHANDSNEEDTOBEFREE;
					}
					else
					ret = RET_NOERROR;
				}
				else{
					//check if we already carry a double-handed item
					if(inventory[SLOT_LEFT]){
						if(inventory[SLOT_LEFT]->getSlotPosition() & SLOTP_TWO_HAND){
							ret = RET_DROPTWOHANDEDITEM;
						}
						//check if weapon, can only carry one weapon
						else if(item != inventory[SLOT_LEFT] && inventory[SLOT_LEFT]->isWeapon() &&
							(inventory[SLOT_LEFT]->getWeaponType() != WEAPON_SHIELD) &&
							(inventory[SLOT_LEFT]->getWeaponType() != WEAPON_AMMO) &&
							item->isWeapon() && (item->getWeaponType() != WEAPON_SHIELD) && (item->getWeaponType() != WEAPON_AMMO)){
								ret = RET_CANONLYUSEONEWEAPON;
						}
						else
							ret = RET_NOERROR;
					}
					else
						ret = RET_NOERROR;
				}
			}
			break;
		case SLOT_LEFT:
			if(item->getSlotPosition() & SLOTP_LEFT){
				//check if we already carry an item in the other hand
				if(item->getSlotPosition() & SLOTP_TWO_HAND){
					if(inventory[SLOT_RIGHT] && inventory[SLOT_RIGHT] != item){
						ret = RET_BOTHHANDSNEEDTOBEFREE;
					}
					else
						ret = RET_NOERROR;
				}
				else{
					//check if we already carry a double-handed item
					if(inventory[SLOT_RIGHT]){
						if(inventory[SLOT_RIGHT]->getSlotPosition() & SLOTP_TWO_HAND){
							ret = RET_DROPTWOHANDEDITEM;
						}
						//check if weapon, can only carry one weapon
						else if(item != inventory[SLOT_RIGHT] && inventory[SLOT_RIGHT]->isWeapon() &&
							(inventory[SLOT_RIGHT]->getWeaponType() != WEAPON_SHIELD) &&
							(inventory[SLOT_RIGHT]->getWeaponType() != WEAPON_AMMO) &&
							item->isWeapon() && (item->getWeaponType() != WEAPON_SHIELD) && (item->getWeaponType() != WEAPON_AMMO)){
								ret = RET_CANONLYUSEONEWEAPON;
						}
						else
							ret = RET_NOERROR;
					}
					else
						ret = RET_NOERROR;
				}
			}
			break;
		case SLOT_LEGS:
			if(item->getSlotPosition() & SLOTP_LEGS)
				ret = RET_NOERROR;
			break;
		case SLOT_FEET:
			if(item->getSlotPosition() & SLOTP_FEET)
				ret = RET_NOERROR;
			break;
		case SLOT_RING:
			if(item->getSlotPosition() & SLOTP_RING)
				ret = RET_NOERROR;
			break;
		case SLOT_AMMO:
			if(item->getSlotPosition() & SLOTP_AMMO)
				ret = RET_NOERROR;
			break;
		case SLOT_WHEREEVER:
			ret = RET_NOTENOUGHROOM;
			break;
		case -1:
			ret = RET_NOTENOUGHROOM;
			break;

		default:
			ret = RET_NOTPOSSIBLE;
			break;
	}

	if(ret == RET_NOERROR || ret == RET_NOTENOUGHROOM){
		//need an exchange with source?
		if(getInventoryItem((slots_t)index) != NULL){
			if(!getInventoryItem((slots_t)index)->isStackable() || getInventoryItem((slots_t)index)->getID() != item->getID()){
				return RET_NEEDEXCHANGE;
			}
		}

		//check if enough capacity
		if(hasCapacity(item, count))
			return ret;
		else
			return RET_NOTENOUGHCAPACITY;
	}

	return ret;
}

ReturnValue Player::__queryMaxCount(int32_t index, const Thing* thing, uint32_t count, uint32_t& maxQueryCount,
	uint32_t flags) const
{
	const Item* item = thing->getItem();
	if(item == NULL){
		maxQueryCount = 0;
		return RET_NOTPOSSIBLE;
	}

	const Thing* destThing = __getThing(index);
	const Item* destItem = NULL;
	if(destThing)
		destItem = destThing->getItem();

	if(destItem){
		if(destItem->isStackable() && item->getID() == destItem->getID()){
			maxQueryCount = 100 - destItem->getItemCount();
		}
		else
			maxQueryCount = 0;
	}
	else{
		if(item->isStackable())
			maxQueryCount = 100;
		else
			maxQueryCount = 1;

		return RET_NOERROR;
	}

	if(maxQueryCount < count)
		return RET_NOTENOUGHROOM;
	else
		return RET_NOERROR;
}

ReturnValue Player::__queryRemove(const Thing* thing, uint32_t count) const
{
	int32_t index = __getIndexOfThing(thing);

	if(index == -1){
		return RET_NOTPOSSIBLE;
	}
	
	const Item* item = thing->getItem();
	if(item == NULL){
		return RET_NOTPOSSIBLE;
	}

	if(count == 0 || (item->isStackable() && count > item->getItemCount())){
		return RET_NOTPOSSIBLE;
	}

	if(item->isNotMoveable()){
		return RET_NOTMOVEABLE;
	}

	return RET_NOERROR;
}

Cylinder* Player::__queryDestination(int32_t& index, const Thing* thing, Item** destItem,
	uint32_t& flags)
{
	if(index == 0 /*drop to capacity window*/ || index == INDEX_WHEREEVER){
		*destItem = NULL;

		const Item* item = thing->getItem();
		if(item == NULL){
			return this;
		}

		//find a appropiate slot
		for(int i = SLOT_FIRST; i < SLOT_LAST; ++i){
			if(inventory[i] == NULL){
				if(__queryAdd(i, item, item->getItemCount(), 0) == RET_NOERROR){
					index = i;
					return this;
				}
			}
		}

		//try containers
		for(int i = SLOT_FIRST; i < SLOT_LAST; ++i){
			if(Container* subContainer = dynamic_cast<Container*>(inventory[i])){
				if(subContainer != tradeItem && subContainer->__queryAdd(-1, item, item->getItemCount(), 0) == RET_NOERROR){
					index = INDEX_WHEREEVER;
					*destItem = NULL;
					return subContainer;
				}
			}
		}

		return this;
	}

	Thing* destThing = __getThing(index);

	if(destThing)
		*destItem = destThing->getItem();

	Cylinder* subCylinder = dynamic_cast<Cylinder*>(destThing);

	if(subCylinder){
		index = INDEX_WHEREEVER;
		*destItem = NULL;
		return subCylinder;
	}
	else
		return this;
}

void Player::__addThing(Thing* thing)
{
	__addThing(0, thing);
}

void Player::__addThing(int32_t index, Thing* thing)
{
	if(index < 0 || index > 11){
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Player::__addThing], " << "player: " << getName() << ", index: " << index << ", index < 0 || index > 11" << std::endl;
		DEBUG_REPORT
#endif
		return /*RET_NOTPOSSIBLE*/;
	}

	if(index == 0){
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Player::__addThing], " << "player: " << getName() << ", index == 0" << std::endl;
		DEBUG_REPORT
#endif
		return /*RET_NOTENOUGHROOM*/;
	}

	Item* item = thing->getItem();
	if(item == NULL){
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Player::__addThing], " << "player: " << getName() << ", item == NULL" << std::endl;
		DEBUG_REPORT
#endif
		return /*RET_NOTPOSSIBLE*/;
	}

	item->setParent(this);
	inventory[index] = item;

	//send to client
	sendAddInventoryItem((slots_t)index, item);

	//event methods
	onAddInventoryItem((slots_t)index, item);
}

void Player::__updateThing(Thing* thing, uint32_t count)
{
	int32_t index = __getIndexOfThing(thing);
	if(index == -1){
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Player::__updateThing], " << "player: " << getName() << ", index == -1" << std::endl;
		DEBUG_REPORT
#endif
		return /*RET_NOTPOSSIBLE*/;
	}

	Item* item = thing->getItem();
	if(item == NULL){
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Player::__updateThing], " << "player: " << getName() << ", item == NULL" << std::endl;
		DEBUG_REPORT
#endif
		return /*RET_NOTPOSSIBLE*/;
	}

	item->setItemCountOrSubtype(count);

	//send to client
	sendUpdateInventoryItem((slots_t)index, item, item);

	//event methods
	onUpdateInventoryItem((slots_t)index, item, item);
}

void Player::__replaceThing(uint32_t index, Thing* thing)
{
	if(index < 0 || index > 11){
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Player::__replaceThing], " << "player: " << getName() << ", index: " << index << ",  index < 0 || index > 11" << std::endl;
		DEBUG_REPORT
#endif
		return /*RET_NOTPOSSIBLE*/;
	}
	
	Item* oldItem = getInventoryItem((slots_t)index);
	if(!oldItem){
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Player::__updateThing], " << "player: " << getName() << ", oldItem == NULL" << std::endl;
		DEBUG_REPORT
#endif
		return /*RET_NOTPOSSIBLE*/;
	}

	Item* item = thing->getItem();
	if(item == NULL){
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Player::__updateThing], " << "player: " << getName() << ", item == NULL" << std::endl;
		DEBUG_REPORT
#endif
		return /*RET_NOTPOSSIBLE*/;
	}

	//send to client
	sendUpdateInventoryItem((slots_t)index, oldItem, item);
	
	//event methods
	onUpdateInventoryItem((slots_t)index, oldItem, item);

	item->setParent(this);
	inventory[index] = item;
}

void Player::__removeThing(Thing* thing, uint32_t count)
{
	Item* item = thing->getItem();
	if(item == NULL){
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Player::__removeThing], " << "player: " << getName() << ", item == NULL" << std::endl;
		DEBUG_REPORT
#endif
		return /*RET_NOTPOSSIBLE*/;
	}

	int32_t index = __getIndexOfThing(thing);
	if(index == -1){
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Player::__removeThing], " << "player: " << getName() << ", index == -1" << std::endl;
		DEBUG_REPORT
#endif
		return /*RET_NOTPOSSIBLE*/;
	}

	if(item->isStackable()){
		if(count == item->getItemCount()){
			//send change to client
			sendRemoveInventoryItem((slots_t)index, item);

			//event methods
			onRemoveInventoryItem((slots_t)index, item);

			item->setParent(NULL);
			inventory[index] = NULL;
		}
		else{
			int newCount = std::max(0, (int)(item->getItemCount() - count));
			item->setItemCount(newCount);

			//send change to client
			sendUpdateInventoryItem((slots_t)index, item, item);

			//event methods
			onUpdateInventoryItem((slots_t)index, item, item);
		}
	}
	else{
		//send change to client
		sendRemoveInventoryItem((slots_t)index, item);
	
		//event methods
		onRemoveInventoryItem((slots_t)index, item);

		item->setParent(NULL);
		inventory[index] = NULL;
	}
}

int32_t Player::__getIndexOfThing(const Thing* thing) const
{
	for(int i = SLOT_FIRST; i < SLOT_LAST; ++i){
		if(inventory[i] == thing)
			return i;
	}

	return -1;
}

int32_t Player::__getFirstIndex() const
{
	return SLOT_FIRST;
}

int32_t Player::__getLastIndex() const
{
	return SLOT_LAST;
}

uint32_t Player::__getItemTypeCount(uint16_t itemId, int32_t subType /*= -1*/, bool itemCount /*= true*/) const
{
	uint32_t count = 0;

	std::list<const Container*> listContainer;
	ItemList::const_iterator cit;
	Container* tmpContainer = NULL;
	Item* item = NULL;

	for(int i = SLOT_FIRST; i < SLOT_LAST; i++){
		if(item = inventory[i]){
			if(item->getID() == itemId && (subType == -1 || subType == item->getSubType())){

				if(itemCount){
					count+= item->getItemCount();
				}
				else{
					if(item->isRune()){
						count+= item->getItemCharge();
					}
					else{
						count+= item->getItemCount();
					}
				}
			}

			if(tmpContainer = item->getContainer()){
				listContainer.push_back(tmpContainer);
			}
		}
	}
	
	while(listContainer.size() > 0){
		const Container* container = listContainer.front();
		listContainer.pop_front();
		
		count+= container->__getItemTypeCount(itemId, subType, itemCount);

		for(cit = container->getItems(); cit != container->getEnd(); ++cit){
			if(tmpContainer = (*cit)->getContainer()){
				listContainer.push_back(tmpContainer);
			}
		}
	}

	return count;
}

Thing* Player::__getThing(uint32_t index) const
{
	if(index >= SLOT_FIRST && index < SLOT_LAST)
		return inventory[index];

	return NULL;
}

void Player::postAddNotification(Thing* thing, int32_t index, cylinderlink_t link /*= LINK_OWNER*/)
{
	if(link == LINK_OWNER){
		//calling movement scripts
		g_moveEvents->onPlayerEquip(this, thing->getItem(), (slots_t)index, true);
	}

	if(link == LINK_OWNER || link == LINK_TOPPARENT){
		updateItemsLight();
		updateInventoryWeigth();
		client->sendStats();
	}

	if(const Item* item = thing->getItem()){
		if(const Container* container = item->getContainer()){
			onSendContainer(container);
		}
	}
	else if(const Creature* creature = thing->getCreature()){
		if(creature == this){
			//check containers
			std::vector<Container*> containers;
			for(ContainerVector::iterator it = containerVec.begin(); it != containerVec.end(); ++it){
				if(!Position::areInRange<1,1,0>(it->second->getPosition(), getPosition())){
					containers.push_back(it->second);
				}
			}

			for(std::vector<Container*>::const_iterator it = containers.begin(); it != containers.end(); ++it){
				autoCloseContainers(*it);
			}
		}
	}
}

void Player::postRemoveNotification(Thing* thing, int32_t index, bool isCompleteRemoval, cylinderlink_t link /*= LINK_OWNER*/)
{
	if(link == LINK_OWNER){
		//calling movement scripts
		g_moveEvents->onPlayerEquip(this, thing->getItem(), (slots_t)index, false);
	}

	if(link == LINK_OWNER || link == LINK_TOPPARENT){
		updateItemsLight();
		updateInventoryWeigth();
		client->sendStats();
	}

	if(const Item* item = thing->getItem()){
		if(const Container* container = item->getContainer()){
			if(!container->isRemoved() &&
					(container->getTopParent() == this || (dynamic_cast<const Container*>(container->getTopParent()))) &&
					Position::areInRange<1,1,0>(getPosition(), container->getPosition())){
				onSendContainer(container);
			}
			else{
				autoCloseContainers(container);
			}
		}
	}
}

void Player::__internalAddThing(Thing* thing)
{
	__internalAddThing(0, thing);
}

void Player::__internalAddThing(uint32_t index, Thing* thing)
{
#ifdef __DEBUG__MOVESYS__NOTICE
	std::cout << "[Player::__internalAddThing] index: " << index << std::endl;
#endif

	Item* item = thing->getItem();
	if(item == NULL){
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Player::__internalAddThing] item == NULL" << std::endl;
#endif
		return;
	}
		
	//index == 0 means we should equip this item at the most appropiate slot
	if(index == 0){
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Player::__internalAddThing] index == 0" << std::endl;
		DEBUG_REPORT
#endif
		return;
	}

	if(index > 0 && index < 11){
		if(inventory[index]){
#ifdef __DEBUG__MOVESYS__
			std::cout << "Warning: [Player::__internalAddThing], player: " << getName() << ", items[index] is not empty." << std::endl;
			//DEBUG_REPORT
#endif
			return;
		}

		inventory[index] = item;
		item->setParent(this);
  }
}

bool Player::setFollowCreature(Creature* creature)
{
	if(!Creature::setFollowCreature(creature)){
		setFollowCreature(NULL);
		setAttackedCreature(NULL);

		sendCancelMessage(RET_THEREISNOWAY);
		sendCancelTarget();
		stopEventWalk();
		return false;
	}

	return true;
}

bool Player::setAttackedCreature(Creature* creature)
{
	if(!Creature::setAttackedCreature(creature)){
		sendCancelTarget();
		return false;
	}

	if(chaseMode == CHASEMODE_FOLLOW && creature){
		if(followCreature != creature){
			//chase opponent
			setFollowCreature(creature);
		}
	}
	else{
		setFollowCreature(NULL);
	}

	return true;
}

uint32_t Player::getAttackSpeed()
{
	return 2000;
}

void Player::onAttacking(uint32_t interval)
{
	if(attackTicks < getAttackSpeed()){
		attackTicks += interval;
	}

	Creature::onAttacking(interval);
}

void Player::doAttacking(uint32_t interval)
{
	if(getAttackSpeed() <= attackTicks){
		bool result = false;

		Item* tool = getWeapon();
		const Weapon* weapon = g_weapons->getWeapon(tool);

		if(weapon && weapon->checkLastAction(this, 100)){
			attackTicks = 0;
			result = weapon->useWeapon(this, tool, attackedCreature);
		}
		else{
			attackTicks = 0;
			result = Weapon::useFist(this, attackedCreature);
		}

		if(!result){
			//make next instant
			attackTicks = getAttackSpeed();
		}
	}
}

int32_t Player::getGainedExperience(Creature* attacker) const
{
	if(g_game.getWorldType() == WORLD_TYPE_PVP_ENFORCED){
		Player* attackerPlayer = attacker->getPlayer();
		if(attackerPlayer && attackerPlayer != this){
				/*Formula
				a = attackers level * 0.9
				b = victims level
				c = victims experience

				y = (1 - (a / b)) * 0.05 * c
				*/

				int32_t a = (int32_t)std::floor(attackerPlayer->getLevel() * 0.9);
				int32_t b = getLevel();
				int32_t c = getExperience();
				
				int32_t result = std::max((int32_t)0, (int32_t)std::floor( getDamageRatio(attacker) * ((double)(1 - (((double)a / b)))) * 0.05 * c ) );
				return result * g_config.getNumber(ConfigManager::RATE_EXPERIENCE);
		}
	}

	return 0;
}

void Player::onFollowCreature(const Creature* creature)
{
	if(!creature){
		stopEventWalk();
	}
}

void Player::setChaseMode(chaseMode_t mode)
{
	chaseMode_t prevChaseMode = chaseMode;
	chaseMode = mode;

	/*
	if(mode == 1){
		chaseMode = CHASEMODE_FOLLOW;
	}
	else{
		chaseMode = CHASEMODE_STANDSTILL;
	}
	*/
	
	if(prevChaseMode != chaseMode){
		if(chaseMode == CHASEMODE_FOLLOW){
			if(!followCreature && attackedCreature){
				//chase opponent
				setFollowCreature(attackedCreature);
			}
		}
		else if(attackedCreature){
			setFollowCreature(NULL);
			stopEventWalk();
		}
	}
}

void Player::setFightMode(fightMode_t mode)
{
	fightMode = mode;

	switch(fightMode){
		case FIGHTMODE_ATTACK:
		{
			attackStrength = 100;
			defenseStrength = 0;
			break;
		}

		case FIGHTMODE_BALANCED:
		{
			attackStrength = 50;
			defenseStrength = 50;
			break;
		}

		case FIGHTMODE_DEFENSE:
		{
			attackStrength = 30;
			defenseStrength = 70;
			break;
		}
	}
}

void Player::onWalkAborted()
{
	sendCancelWalk();
}

void Player::getCreatureLight(LightInfo& light) const
{
	if(internalLight.level > itemsLight.level){
		light = internalLight;
	}
	else{
		light = itemsLight;
	}
}

void Player::updateItemsLight(bool internal /*=false*/)
{
	LightInfo maxLight;
	LightInfo curLight;
	for(int i = SLOT_FIRST; i < SLOT_LAST; ++i){
		Item* item = getInventoryItem((slots_t)i);
		if(item){
			item->getLight(curLight);
			if(curLight.level > maxLight.level){
				maxLight = curLight;
			}
		}
	}
	if(itemsLight.level != maxLight.level || itemsLight.color != maxLight.color){
		itemsLight = maxLight;
		if(!internal){
			g_game.changeLight(this);
		}
	}
}

void Player::onAddCondition(ConditionType_t type)
{
	Creature::onAddCondition(type);
	sendIcons();
}

void Player::onAddCombatCondition(ConditionType_t type)
{
	if(type == CONDITION_POISON){
		sendTextMessage(MSG_STATUS_DEFAULT, "You are poisoned.");
	}
	else if(type == CONDITION_DROWN){
		sendTextMessage(MSG_STATUS_DEFAULT, "You are drowning.");
	}
	else if(type == CONDITION_PARALYZE){
		sendTextMessage(MSG_STATUS_DEFAULT, "You are paralyzed.");
	}
	else if(type == CONDITION_DRUNK){
		sendTextMessage(MSG_STATUS_DEFAULT, "You are drunk.");
	}
}

void Player::onEndCondition(ConditionType_t type)
{
	Creature::onEndCondition(type);
	sendIcons();

	if(type == CONDITION_INFIGHT){
		damageMap.clear();
		pzLocked = false;

#ifdef __SKULLSYSTEM__
		if(getSkull() != SKULL_RED){
			clearAttacked();
			g_game.changeSkull(this, SKULL_NONE);
		}
#endif
	}
}

void Player::onCombatRemoveCondition(const Creature* attacker, Condition* condition)
{
	//Creature::onCombatRemoveCondition(attacker, condition);
	bool remove = true;
	
	if(condition->getId() != 0){
		remove = false;

		//Means the condition is from an item, id == slot
		if(g_game.getWorldType() == WORLD_TYPE_PVP_ENFORCED){
			Item* item = getInventoryItem((slots_t)condition->getId());
			if(item){
				//25% chance to destroy the item
				if(25 >= random_range(0, 100)){
					g_game.internalRemoveItem(item);
				}
			}
		}
	}

	if(remove){
		removeCondition(condition);
	}
}

void Player::onAttackedCreature(Creature* target)
{
	Creature::onAttackedCreature(target);

	if(!hasFlag(PlayerFlag_NotGainInFight)){
		if(target != this){
			if(Player* targetPlayer = target->getPlayer()){
				pzLocked = true;

#ifdef __SKULLSYSTEM__
				if(!targetPlayer->hasAttacked(this)){
					if(targetPlayer->getSkull() == SKULL_NONE && getSkull() == SKULL_NONE){
						//add a white skull
						g_game.changeSkull(this, SKULL_WHITE);
					}

					if(!hasAttacked(targetPlayer) && getSkull() == SKULL_NONE){
						//show yellow skull
						targetPlayer->sendCreatureSkull(this, SKULL_YELLOW);
					}

					addAttacked(targetPlayer);
				}
#endif
			}
		}

		addInFightTicks();
	}
}

void Player::onAttacked()
{
	Creature::onAttacked();

	if(!hasFlag(PlayerFlag_NotGainInFight)){
		addInFightTicks();
	}
}

void Player::onAttackedCreatureDrainHealth(Creature* target, int32_t points)
{
	//TODO: Share damage points with team (share exp)
	Creature::onAttackedCreatureDrainHealth(target, points);
}

void Player::onKilledCreature(Creature* target)
{
	if(hasFlag(PlayerFlag_NotGenerateLoot)){
		target->setDropLoot(false);
	}

	Creature::onKilledCreature(target);

	if(!hasFlag(PlayerFlag_NotGainInFight)){
		if(Player* targetPlayer = target->getPlayer()){
#ifdef __SKULLSYSTEM__
			if(!targetPlayer->hasAttacked(this) && targetPlayer->getSkull() == SKULL_NONE){
				addUnjustifiedDead(targetPlayer);
			}
#endif
			
			if(hasCondition(CONDITION_INFIGHT)){
				pzLocked = true;
				Condition* condition = Condition::createCondition(CONDITIONID_DEFAULT, CONDITION_INFIGHT, 60 * 1000 * 15, 0);
				addCondition(condition);
			}
		}
	}
}

void Player::onGainExperience(int32_t gainExperience)
{
	if(hasFlag(PlayerFlag_NotGainExperience)){
		gainExperience = 0;
	}

	Creature::onGainExperience(gainExperience);

	if(gainExperience > 0){
		//soul regeneration
		if(gainExperience >= getLevel()){
			Condition* condition = Condition::createCondition(CONDITIONID_DEFAULT, CONDITION_SOUL, 4 * 60 * 1000, 0);
			//1 soulpoint / 2 minutes for normal players
			//1 soulpoint / 15 seconds for premium players

			condition->setParam(CONDITIONPARAM_SOULGAIN, 1);
			condition->setParam(CONDITIONPARAM_SOULTICKS, 15 * 1000);
			addCondition(condition);
		}

		addExperience(gainExperience);
	}
}

bool Player::isImmune(CombatType_t type) const
{
	if(hasFlag(PlayerFlag_CannotBeAttacked)){
		return true;
	}

	return Creature::isImmune(type);
}

bool Player::isImmune(ConditionType_t type) const
{
	if(hasFlag(PlayerFlag_CannotBeAttacked)){
		return true;
	}

	return Creature::isImmune(type);
}

bool Player::isAttackable() const
{
	if(hasFlag(PlayerFlag_CannotBeAttacked)){
		return false;
	}

	return true;
}

void Player::changeHealth(int32_t healthChange)
{
	Creature::changeHealth(healthChange);
	sendStats();
}

void Player::changeMana(int32_t manaChange)
{
	Creature::changeMana(manaChange);
	sendStats();
}

void Player::changeSoul(int32_t soulChange)
{
	if(soulChange > 0){
		soul += std::min(soulChange, soulMax - soul);
	}
	else{
		soul = std::max((int32_t)0, soul + soulChange);
	}

	sendStats();
}

#ifdef __SKULLSYSTEM__
Skulls_t Player::getSkull() const
{
	if(hasFlag(PlayerFlag_NotGainInFight)){
		return SKULL_NONE;
	}
		
	return skull;
}

Skulls_t Player::getSkullClient(const Player* player) const
{
	if(!player){
		return SKULL_NONE;
	}

	Skulls_t skull;
	skull = player->getSkull();
	if(skull == SKULL_NONE){
		if(player->hasAttacked(this)){
			skull = SKULL_YELLOW;
		}
	}

	return skull;
}

bool Player::hasAttacked(const Player* attacked) const
{
	if(hasFlag(PlayerFlag_NotGainInFight) || !attacked){
		return false;
	}
	
	AttackedSet::const_iterator it;
	uint32_t attackedId = attacked->getID();
	it = attackedSet.find(attackedId);
	if(it != attackedSet.end()){
		return true;
	}

	return false;
}

void Player::addAttacked(const Player* attacked)
{
	if(hasFlag(PlayerFlag_NotGainInFight) || !attacked || attacked == this){
		return;
	}

	AttackedSet::iterator it;
	uint32_t attackedId = attacked->getID();
	it = attackedSet.find(attackedId);
	if(it == attackedSet.end()){
		attackedSet.insert(attackedId);
	}
}

void Player::clearAttacked()
{
	attackedSet.clear();
}

void Player::addUnjustifiedDead(const Player* attacked)
{
	if(hasFlag(PlayerFlag_NotGainInFight) || attacked == this){
		return;
	}
		
	std::stringstream Msg;
	Msg << "Warning! The murder of " << attacked->getName() << " was not justified.";
	client->sendTextMessage(MSG_STATUS_WARNING, Msg.str());
	redSkullTicks = redSkullTicks + 12 * 3600 * 1000;
	if(redSkullTicks >= 3*24*3600*1000){
		g_game.changeSkull(this, SKULL_RED);
	}
}

void Player::checkRedSkullTicks(int32_t ticks)
{
	if(redSkullTicks - ticks > 0)
		redSkullTicks = redSkullTicks - ticks;
	
	if(redSkullTicks < 1000 && !hasCondition(CONDITION_INFIGHT) && skull != SKULL_NONE){
		g_game.changeSkull(this, SKULL_NONE);
	}
}
#endif

const OutfitListType& Player::getPlayerOutfits()
{	
	return m_playerOutfits.getOutfits();
}

bool Player::canWear(uint32_t _looktype, uint32_t _addons)
{
	if(m_playerOutfits.isInList(_looktype, _addons)){
		return true;
	}	
	return false;
}

void Player::genReservedStorageRange()
{
	uint32_t base_key;
	//generate outfits range
	base_key = PSTRG_OUTFITS_RANGE_START + 1;
	
	const OutfitList& global_outfits = Outfits::getInstance()->getOutfitList(sex);
	
	const OutfitListType& outfits = m_playerOutfits.getOutfits();
	OutfitListType::const_iterator it;
	for(it = outfits.begin(); it != outfits.end(); ++it){
		uint32_t looktype = (*it)->looktype;
		uint32_t addons = (*it)->addons;
		if(!global_outfits.isInList(looktype, addons)){
			int32_t value = (looktype << 16) | (addons & 0xFF);
			storageMap[base_key] = value;
			base_key++;
			if(base_key > PSTRG_OUTFITS_RANGE_START + PSTRG_OUTFITS_RANGE_SIZE){
				std::cout << "Warning: [Player::genReservedStorageRange()] Player " << getName() << " with more than 500 outfits!." << std::endl;
				break;
			}
		}
	}
}

void Player::addOutfit(uint32_t _looktype, uint32_t _addons)
{
	Outfit outfit;
	outfit.looktype = _looktype;
	outfit.addons = _addons;
	m_playerOutfits.addOutfit(outfit);
}

bool Player::remOutfit(uint32_t _looktype, uint32_t _addons)
{
	Outfit outfit;
	outfit.looktype = _looktype;
	outfit.addons = _addons;
	return m_playerOutfits.remOutfit(outfit);
}

void Player::setSex(playersex_t player_sex)
{
	sex = player_sex;
	//add default outfits to player outfits
	Outfits* outfits = Outfits::getInstance();
	const OutfitListType& global_outfits = outfits->getOutfits(sex);
	OutfitListType::const_iterator it;
	Outfit outfit;
	for(it = global_outfits.begin(); it != global_outfits.end(); ++it){
		outfit.looktype = (*it)->looktype;
		outfit.addons = (*it)->addons;
		m_playerOutfits.addOutfit(outfit);
	}
}

void Player::setSkillsPercents()
{
	uint32_t percent = 0;

	percent = (100 * manaSpent) / std::max((int32_t)1, (int32_t)(vocation->getReqMana(magLevel + 1)));
	if(percent < 0){
		percent = 0;
	}
	else if(percent > 100){
		percent = 100;
	}

	maglevel_percent = percent;

	percent = (100*(getExperience() - getExpForLv(getLevel()))) /
		std::max((int32_t)1, (int32_t)((getExpForLv(getLevel() + 1) - getExpForLv(getLevel()))));

	if(percent < 0){
		percent = 0;
	}
	else if(percent > 100){
		percent = 100;
	}

	level_percent = percent;

	for(unsigned int i = SKILL_FIRST; i < SKILL_LAST; ++i){
		percent = (100*skills[i][SKILL_TRIES]) / std::max((int32_t)1, (int32_t)(vocation->getReqSkillTries(i, skills[i][SKILL_LEVEL]+1)));

		if(percent < 0){
			percent = 0;
		}
		else if(percent > 100){
			percent = 100;
		}

		skills[i][SKILL_PERCENT] = percent;
	}
}

void Player::learnInstantSpell(const std::string& name)
{
	if(!hasLearnedInstantSpell(name)){
		learnedInstantSpellList.push_back(name);
	}
}

bool Player::hasLearnedInstantSpell(const std::string& name) const
{
	if(hasFlag(PlayerFlag_CannotUseSpells)){
		return false;
	}

	if(hasFlag(PlayerFlag_IgnoreSpellCheck)){
		return true;
	}

	for(LearnedInstantSpellList::const_iterator it = learnedInstantSpellList.begin();
			it != learnedInstantSpellList.end(); ++it){
		if(strcasecmp((*it).c_str(), name.c_str()) == 0){
			return true;
		}
	}

	return false;
}
