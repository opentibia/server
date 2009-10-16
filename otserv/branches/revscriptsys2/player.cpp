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

#include "player.h"
#include "tasks.h"
#include "scheduler.h"
#include "enums.h"
#include "ioplayer.h"
#include "game.h"
#include "configmanager.h"
#include "chat.h"
#include "house.h"
#include "actor.h"
#include "combat.h"
#include "status.h"
#include "beds.h"
#include "depot.h"
#include "party.h"
#include "weapons.h"

extern ConfigManager g_config;
extern Game g_game;
extern Chat g_chat;
extern Vocations g_vocations;

AutoList<Player> Player::listPlayer;
MuteCountMap Player::muteCountMap;
int32_t Player::maxMessageBuffer = 0;
ChannelStatementMap Player::channelStatementMap;
uint32_t Player::channelStatementGuid = 0;

#ifdef __ENABLE_SERVER_DIAGNOSTIC__
uint32_t Player::playerCount = 0;
#endif

Player::Player(const std::string& _name, ProtocolGame* p) :
Creature()
{
	client = p;
	isConnecting = false;
	if(client){
		client->setPlayer(this);
	}
	accountId   = 0;
	name        = _name;
	setVocation((Vocation*)NULL);
	capacity   = 300.00;
	mana       = 0;
	manaMax    = 0;
	manaSpent  = 0;
	soul       = 0;
	soulMax    = 100;
	guildId    = 0;
	guildLevel = 0;

	level      = 1;
	levelPercent = 0;
	magLevelPercent = 0;
	magLevel   = 0;
	experience = 0;
	mechanicImmunities = MECHANIC_NONE;
	mechanicSuppressions = MECHANIC_NONE;
	damageImmunities = COMBAT_NONE;
	accessLevel = 0;
	violationLevel = 0;
	lastip = 0;
	lastLoginSaved = 0;
	lastLogout = 0;
	lastLoginMs = 0;
	last_ping = OTSYS_TIME();
	last_pong = OTSYS_TIME();
	MessageBufferTicks = 0;
	MessageBufferCount = 0;
	nextAction = 0;
	stamina = MAX_STAMINA;

	pzLocked = false;
	bloodHitCount = 0;
	shieldBlockCount = 0;
	lastAttackBlockType = BLOCK_NONE;
	addAttackSkillPoint = false;
	lastAttack = 0;
	shootRange = 1;

	chaseMode = CHASEMODE_STANDSTILL;
	fightMode = FIGHTMODE_ATTACK;
	safeMode = true;

	tradePartner = NULL;
	tradeState = TRADE_NONE;
	tradeItem = NULL;

	walkTask = NULL;
	walkTaskEvent = 0;
	actionTaskEvent = 0;
	nextStepEvent = 0;

	idleTime = 0;
	idleWarned = false;

	for(SlotType::iterator i = SlotType::begin(); i != SlotType::end(); ++i){
		inventory[i->value()] = NULL;
		inventoryAbilities[i->value()] = false;
	}

	for(SkillType::iterator i = SkillType::begin(); i != SkillType::end(); ++i){
		skills[i->value()][SKILL_LEVEL]= 10;
		skills[i->value()][SKILL_TRIES]= 0;
		skills[i->value()][SKILL_PERCENT] = 0;
	}

	for(SkillType::iterator i = SkillType::begin(); i != SkillType::end(); ++i){
		varSkills[i->value()] = 0;
	}

	for(PlayerStatType::iterator i = PlayerStatType::begin(); i != PlayerStatType::end(); ++i){
		varStats[i->value()] = 0;
	}

	for(LossType::iterator i = LossType::begin(); i != LossType::end(); ++i){
		lossPercent[i->value()] = 10;
	}

	for(LevelType::iterator i = LevelType::begin(); i != LevelType::end(); ++i){
		rateValue[i->value()] = 1.0f;
	}

	maxDepotLimit = 1000;
	maxVipLimit = 50;
	groupFlags = 0;
	premiumDays = 0;

	sex = SEX_FEMALE;

 	town = 0;
 	lastip = 0;

	windowTextId = 0;
	writeItem = NULL;
	maxWriteLen = 0;

	editHouse = NULL;
	editListId = 0;

	setParty(NULL);

#ifdef __SKULLSYSTEM__
	lastSkullTime = 0;
	skullType = SKULL_NONE;
#endif

#ifdef __ENABLE_SERVER_DIAGNOSTIC__
	playerCount++;
#endif
}

Player::~Player()
{
	for(SlotType::iterator i = SlotType::begin(); i != SlotType::end(); ++i){
		if(getInventoryItem(*i)){
			getInventoryItem(*i)->setParent(NULL);
			getInventoryItem(*i)->unRef();
			inventory[i->value()] = NULL;
			inventoryAbilities[i->value()] = false;
		}
	}

	DepotMap::iterator it;
	for(it = depots.begin();it != depots.end(); it++){
		it->second->unRef();
	}

	//std::cout << "Player destructor " << this << std::endl;

	setWriteItem(NULL);
	setEditHouse(NULL);
	setNextWalkActionTask(NULL);

#ifdef __ENABLE_SERVER_DIAGNOSTIC__
	playerCount--;
#endif
}

void Player::setVocation(Vocation* voc)
{
	if(voc){
		vocation = voc;
	}
	else{
		// No vocation is always 0
		vocation = g_vocations.getVocation(0);
	}

	removeCondition(CONDITION_REGENERATION);

	//Set the player's max soul according to their vocation
	soulMax = vocation->getSoulMax();
}

void Player::setVocation(uint32_t id = 0)
{
	setVocation(g_vocations.getVocation(id));
}

uint32_t Player::getVocationId() const
{
	return vocation->getID();
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

		if(hasFlag(PlayerFlag_ShowGroupInsteadOfVocation))
			s << " You are " << getGroupName() << ".";
		else if(vocation->getID() != 0)
			s << " You are " << vocation->getVocDescription() << ".";
		else
			s << " You have no vocation.";
	}
	else {
		s << name << " (Level " << level <<").";

		s << " " << playerSexSubjectString(getSex());

		if(hasFlag(PlayerFlag_ShowGroupInsteadOfVocation))
			s << " is " << getGroupName() << ".";
		else if(vocation->getID() != 0)
			s << " is " << vocation->getVocDescription() << ".";
		else
			s << " has no vocation.";
	}

	if(guildId)
	{
		if(lookDistance == -1){
			s << " You are ";
		}
		else{
			s << " " << playerSexSubjectString(getSex()) << " is ";
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

Item* Player::getInventoryItem(SlotType slot) const
{
	if(slot >= SLOT_FIRST && slot < SLOT_LAST)
		return inventory[slot.value()];
	if(slot == SLOT_HAND)
		return inventory[*SLOT_LEFT]? inventory[*SLOT_LEFT] : inventory[*SLOT_RIGHT];

	return NULL;
}

Item* Player::getEquippedItem(SlotType slot) const
{
	Item* item = getInventoryItem(slot);
	if(item){
		if(slot == SLOT_RIGHT || slot == SLOT_LEFT)
			return (item->getWieldPosition() == SLOT_HAND) ? item : NULL;
		else
			return (slot == item->getWieldPosition()) ? item : NULL;
	}

	return NULL;
}

Item* Player::getWeapon(bool ignoreAmmu /*= false*/)
{
	Item* item;

	for(SlotType::iterator slot = SLOT_RIGHT; slot <= SLOT_LEFT; ++slot){
		item = getInventoryItem(*slot);
		if(!item)
			continue;

		switch(item->getWeaponType().value()){
			case enums::WEAPON_SWORD:
			case enums::WEAPON_AXE:
			case enums::WEAPON_CLUB:
			case enums::WEAPON_WAND:
			{
				const Weapon* weapon = item->getWeapon();
				if(weapon){
					return item;
				}

				break;
			}

			case enums::WEAPON_DIST:
			{
				if(!ignoreAmmu && item->getAmmoType() != AMMO_NONE){
					Item* ammuItem = getInventoryItem(SLOT_AMMO);

					if(ammuItem && ammuItem->getAmmoType() == item->getAmmoType()){
						const Weapon* weapon = ammuItem->getWeapon();
						if(weapon){
							shootRange = item->getShootRange();
							return ammuItem;
						}
					}
				}
				else{
					const Weapon* weapon = item->getWeapon();
					if(weapon){
						shootRange = item->getShootRange();
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

WeaponType Player::getWeaponType()
{
	Item* item = getWeapon();
	if(!item){
		return WEAPON_NONE;
	}

	return item->getWeaponType();
}

int32_t Player::getWeaponSkill(const Item* item) const
{
	if(!item){
		return getSkill(SKILL_FIST, SKILL_LEVEL);
	}

	WeaponType weaponType = item->getWeaponType();
	int32_t attackSkill;

	switch(weaponType.value()){
		case enums::WEAPON_SWORD:
			attackSkill = getSkill(SKILL_SWORD, SKILL_LEVEL);
			break;

		case enums::WEAPON_CLUB:
		{
			attackSkill = getSkill(SKILL_CLUB, SKILL_LEVEL);
			break;
		}

		case enums::WEAPON_AXE:
		{
			attackSkill = getSkill(SKILL_AXE, SKILL_LEVEL);
			break;
		}

		case enums::WEAPON_DIST:
		{
			attackSkill = getSkill(SKILL_DIST, SKILL_LEVEL);
			break;
		}
		default:
		{
			attackSkill = 0;
			break;
		}
	}
	return attackSkill;
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

	return (vocation->getArmorDefense() != 1.0 ? int32_t(armor * vocation->getArmorDefense()) : armor);
}

void Player::getShieldAndWeapon(const Item* &shield, const Item* &weapon) const
{
	Item* item;
	shield = NULL;
	weapon = NULL;
	for(SlotType::iterator slot = SLOT_RIGHT; slot <= SLOT_LEFT; ++slot){
		item = getInventoryItem(*slot);
		if(item){
			switch(item->getWeaponType().value()){
			case enums::WEAPON_NONE:
				break;
			case enums::WEAPON_SHIELD:
				shield = item;
				break;
			default: // weapons that are not shields
				weapon = item;
				break;
			}
		}
	}
	return;
}

int32_t Player::getDefense() const
{
	int32_t baseDefense = 5;
	int32_t defenseSkill = 0;
	int32_t defenseValue = 0;
	int32_t extraDef = 0;
	float defenseFactor = getDefenseFactor();
	const Item* weapon = NULL;
	const Item* shield = NULL;
	getShieldAndWeapon(shield, weapon);

	if(weapon){
		defenseValue = weapon->getDefense();
		defenseSkill = getWeaponSkill(weapon);
		extraDef = weapon->getExtraDef();
	}

	if(shield && shield->getDefense() >= defenseValue){
		defenseValue = shield->getDefense() + extraDef;
		defenseSkill = getSkill(SKILL_SHIELD, SKILL_LEVEL);
	}

	defenseValue += baseDefense;

	if(defenseSkill == 0)
		return 0;

	if(vocation && vocation->getBaseDefense() != 1.0){
		defenseValue = int32_t(defenseValue * vocation->getBaseDefense());
	}

	return ((int32_t)std::ceil(((float)(defenseSkill * (defenseValue * 0.015)) + (defenseValue * 0.1)) * defenseFactor));
}

double Player::getAttackFactor() const
{
	// This factor is divided by
	double modes[FightMode::size] = {
		1.0,
		1.2,
		2.0
	};
	return modes[fightMode.value()];
}

double Player::getDefenseFactor() const
{
	// This factor is multiplied by
	double modes[FightMode::size] = {
		1.0,
		1.2,
		2.0
	};
	return modes[fightMode.value()];
}

IconType Player::getIcons() const
{
	IconType icons = ICON_NONE;

	ConditionList::const_iterator it;
	for(it = conditions.begin(); it != conditions.end(); ++it){
		if(!isCured(*it)){
			icons |= (*it)->getIcon();
		}
	}

	if(isPzLocked()){
		icons |= ICON_PZBLOCK;
	}

	if(getParentTile()->getZone() == ZONE_PROTECTION){
		icons |= ICON_PZ;
	}

	return icons;
}

void Player::sendIcons() const
{
	if(client){
		client->sendIcons(getIcons().value());
	}
}

void Player::sendAddTileItem(const Tile* tile, const Position& pos, const Item* item)
{
	if(client)
		client->sendAddTileItem(tile, pos, tile->getClientIndexOfThing(this, item), item);
}

void Player::sendUpdateTileItem(const Tile* tile, const Position& pos, const Item* olditem, const Item* newitem)
{
	if(client)
		client->sendUpdateTileItem(tile, pos, tile->getClientIndexOfThing(this, newitem), newitem);
}

void Player::sendRemoveTileItem(const Tile* tile, const Position& pos, uint32_t stackpos, const Item* item)
{
	if(client)
		client->sendRemoveTileItem(tile, pos, stackpos);
}

void Player::sendUpdateTile(const Tile* tile, const Position& pos)
{
	if(client)
		client->sendUpdateTile(tile, pos);
}


void Player::sendCreatureAppear(const Creature* creature, const Position& pos)
{
	if(client)
		client->sendAddCreature(creature, pos, creature->getParentTile()->getClientIndexOfThing(this, creature));
}

void Player::sendCreatureDisappear(const Creature* creature, uint32_t stackpos, bool isLogout)
{
	if(client)
		client->sendRemoveCreature(creature, creature->getPosition(), stackpos, isLogout);
}

void Player::sendCreatureMove(const Creature* creature, const Tile* newTile, const Position& newPos,
	const Tile* oldTile, const Position& oldPos, uint32_t oldStackPos, bool teleport)
{
	if(client)
		client->sendMoveCreature(creature, newTile, newPos, newTile->getClientIndexOfThing(this, creature),
			oldTile, oldPos, oldStackPos, teleport);
}

void Player::sendCreatureTurn(const Creature* creature)
{
	if(client)
		client->sendCreatureTurn(creature, creature->getParentTile()->getClientIndexOfThing(this, creature));
}

void Player::sendCreatureSay(const Creature* creature, SpeakClass type, const std::string& text)
{
	if(client)
		client->sendCreatureSay(creature, type, text);
}

void Player::sendCreatureSquare(const Creature* creature, SquareColor color)
{
	if(client)
		client->sendCreatureSquare(creature, color);
}

void Player::sendCreatureChangeOutfit(const Creature* creature, const OutfitType& outfit)
{
	if(client)
		client->sendCreatureOutfit(creature, outfit);
}

void Player::sendCreatureChangeVisible(const Creature* creature, bool visible)
{
	if(client){
		if(creature->getPlayer()){
			if(creature == this || !creature->getPlayer()->hasFlag(PlayerFlag_CannotBeSeen)){
				if(visible){
					client->sendCreatureOutfit(creature, creature->getCurrentOutfit());
				}
				else{
					static OutfitType outfit;
					client->sendCreatureOutfit(creature, outfit);
				}
			}
		}
		else{
			if(canSeeInvisibility()){
				client->sendCreatureOutfit(creature, creature->getCurrentOutfit());
			}
			else{
				if(visible){
					client->sendAddCreature(creature, creature->getPosition(), creature->getParentTile()->getClientIndexOfThing(this, creature));
				}
				else{
					client->sendRemoveCreature(creature, creature->getPosition(), creature->getParentTile()->getClientIndexOfThing(this, creature), false);
				}
			}
		}
	}
}

void Player::sendCreatureLight(const Creature* creature)
{
	if(client)
		client->sendCreatureLight(creature);
}

void Player::sendCreatureShield(const Creature* creature)
{
	if(client)
		client->sendCreatureShield(creature);
}

void Player::updateInventoryWeight()
{
	inventoryWeight = 0.00;

	if(!hasFlag(PlayerFlag_HasInfiniteCapacity)){
		for(SlotType::iterator i = SLOT_FIRST; i < SLOT_LAST; ++i){
			Item* item = getInventoryItem(*i);
			if(item){
				inventoryWeight += item->getWeight();
			}
		}
	}
}

int32_t Player::getPlayerInfo(playerinfo_t playerinfo) const
{
	switch(playerinfo) {
		case PLAYERINFO_LEVEL: return level; break;
		case PLAYERINFO_LEVELPERCENT: return levelPercent; break;
		case PLAYERINFO_MAGICLEVEL: return std::max((int32_t)0, ((int32_t)magLevel + varStats[*STAT_MAGICPOINTS])); break;
		case PLAYERINFO_MAGICLEVELPERCENT: return magLevelPercent; break;
		case PLAYERINFO_HEALTH: return health; break;
		case PLAYERINFO_MAXHEALTH: return std::max((int32_t)1, ((int32_t)healthMax + varStats[*STAT_MAXHITPOINTS])); break;
		case PLAYERINFO_MANA: return mana; break;
		case PLAYERINFO_MAXMANA: return std::max((int32_t)0, ((int32_t)manaMax + varStats[*STAT_MAXMANAPOINTS])); break;
		case PLAYERINFO_SOUL: return std::max((int32_t)0, ((int32_t)soul + varStats[*STAT_SOULPOINTS])); break;
		case PLAYERINFO_MAXSOUL: return soulMax; break;
		default:
			return 0; break;
	}

	return 0;
}

uint64_t Player::getLostExperience() const
{
	if(!skillLoss)
		return 0;

	if(level < 25)
		return experience * lossPercent[*LOSS_EXPERIENCE] / 1000;

	double levels_to_lose = (getLevel() + 50) / 100.;
	uint64_t xp_to_lose = 0;
	uint32_t clevel = getLevel();

	while(levels_to_lose >= 1.0){
		xp_to_lose += (getExpForLevel(clevel) - getExpForLevel(clevel - 1));
		clevel--;
		levels_to_lose -= 1.0;
	}
	if(levels_to_lose > 0.0)
		xp_to_lose += uint64_t((getExpForLevel(clevel) - getExpForLevel(clevel - 1)) * levels_to_lose);

	return xp_to_lose * lossPercent[*LOSS_EXPERIENCE] / 100;
}

int32_t Player::getSkill(SkillType skilltype, skillsid_t skillinfo) const
{
	int32_t n = skills[skilltype.value()][skillinfo];

	if(skillinfo == SKILL_LEVEL){
		n += varSkills[skilltype.value()];
	}

	return std::max((int32_t)0, (int32_t)n);
}

void Player::addSkillAdvance(SkillType skill, uint32_t count, bool useMultiplier /*= true*/)
{
	if(useMultiplier){
		count = uint32_t(count * getRateValue(LevelType(skill.value())));
	}
	skills[skill.value()][SKILL_TRIES] += count * g_config.getNumber(ConfigManager::RATE_SKILL);

#if __DEBUG__
	std::cout << getName() << ", has the vocation: " << (int)getVocationId() << " and is training his " << skill << ". Tries: " << skills[skill.value()][SKILL_TRIES] << "(" << vocation->getReqSkillTries(skill, skills[skill.value()][SKILL_LEVEL] + 1) << ")" << std::endl;
	std::cout << "Current skill: " << skills[skill.value()][SKILL_LEVEL] << std::endl;
#endif

	//Need skill up?
	if(skills[skill.value()][SKILL_TRIES] >= vocation->getReqSkillTries(skill, skills[skill.value()][SKILL_LEVEL] + 1)){
		int oldlevel = skills[skill.value()][SKILL_LEVEL]++;
		skills[skill.value()][SKILL_TRIES] = 0;
		skills[skill.value()][SKILL_PERCENT] = 0;

		g_game.onPlayerAdvance(this, LevelType(skill.value()), oldlevel, skills[skill.value()][SKILL_LEVEL]);

		sendSkills();
	}
	else{
		//update percent
		uint32_t newPercent = Player::getPercentLevel(skills[skill.value()][SKILL_TRIES], vocation->getReqSkillTries(skill, skills[skill.value()][SKILL_LEVEL] + 1));
		if(skills[skill.value()][SKILL_PERCENT] != newPercent){
			skills[skill.value()][SKILL_PERCENT] = newPercent;
			sendSkills();
		}
	}
}

void Player::setVarStats(PlayerStatType stat, int32_t modifier)
{
	varStats[stat.value()] += modifier;

	if(stat == STAT_MAXHITPOINTS)
	{
		if(getHealth() > getMaxHealth()){
			//Creature::changeHealth is called  to avoid sendStats()
			Creature::changeHealth(getMaxHealth() - getHealth());
		}
		else{
			g_game.addCreatureHealth(this);
		}
	}

	else if(STAT_MAXMANAPOINTS)
	{
		if(getMana() > getMaxMana()){
			//Creature::changeMana is called  to avoid sendStats()
			Creature::changeMana(getMaxMana() - getMana());
		}
	}
}

int32_t Player::getDefaultStats(PlayerStatType stat)
{
	if(stat == STAT_MAXHITPOINTS)
		return getMaxHealth() - getVarStats(STAT_MAXHITPOINTS);

	if(stat == STAT_MAXMANAPOINTS)
		return getMaxMana() - getVarStats(STAT_MAXMANAPOINTS);

	if(stat == STAT_SOULPOINTS)
		return getPlayerInfo(PLAYERINFO_SOUL) - getVarStats(STAT_SOULPOINTS);

	if(stat == STAT_MAGICPOINTS)
		return getMagicLevel() - getVarStats(STAT_MAGICPOINTS);

	return 0;
}

int32_t Player::getStepSpeed() const
{
	if(getSpeed() > PLAYER_MAX_SPEED){
		return PLAYER_MAX_SPEED;
	}
	else if(getSpeed() < PLAYER_MIN_SPEED){
		return PLAYER_MIN_SPEED;
	}

	return getSpeed();
}

void Player::updateBaseSpeed()
{
	if(!hasFlag(PlayerFlag_SetMaxSpeed)){
		baseSpeed = 220 + (2* (level - 1));
	}
	else{
		baseSpeed = 900;
	};
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

bool Player::canOpenCorpse(uint32_t ownerId)
{
	if(getID() == ownerId){
		return true;
	}

	if(party && party->canOpenCorpse(ownerId)){
		return true;
	}

	return false;
}

uint16_t Player::getCorpseId() const
{
	if(sex != 0)
		return ITEM_MALE_CORPSE;
	else
		return ITEM_FEMALE_CORPSE;
}

void Player::dropLoot(Container* corpse)
{
	if(!corpse){
		return;
	}

	uint32_t itemLoss = lossPercent[*LOSS_ITEMS];
	uint32_t backpackLoss = lossPercent[*LOSS_CONTAINERS];
#ifdef __SKULLSYSTEM__
	if(getSkull() == SKULL_RED || getSkull() == SKULL_BLACK){
		itemLoss = 100;
		backpackLoss = 100;
	}
#endif
	if(!lootDrop){
		itemLoss = 0;
		backpackLoss = 0;
	}

	if(itemLoss > 0 || backpackLoss > 0){
		for(SlotType::iterator i = SLOT_FIRST; i < SLOT_LAST; ++i){
			Item* item = getInventoryItem(*i);
			if(item){
				if((item->getContainer() && (uint32_t)random_range(1, 100) <= backpackLoss) || (!item->getContainer() && (uint32_t)random_range(1, 100) <= itemLoss)){
					g_game.internalMoveItem(NULL, this, corpse, INDEX_WHEREEVER, item, item->getItemCount(), 0);
				}
			}
		}
	}
}

bool Player::canSee(const Position& pos) const
{
	if(client){
		return client->canSee(pos);
	}

	return false;
}

bool Player::canSeeInvisibility() const
{
	return hasFlag(PlayerFlag_CanSenseInvisibility);
}

bool Player::canSeeCreature(const Creature* creature) const
{
	if(canSeeInvisibility() || creature == this){
		return true;
	}

	if(creature->getPlayer()){
		if(creature->getPlayer()->hasFlag(PlayerFlag_CannotBeSeen)){
			return false;
		}

		return true;
	}
	else if(creature->isInvisible()){
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
	switch(message.value()){
	case enums::RET_DESTINATIONOUTOFREACH:
		sendCancel("Destination is out of reach.");
		break;

	case enums::RET_NOTMOVEABLE:
		sendCancel("You cannot move this object.");
		break;

	case enums::RET_DROPTWOHANDEDITEM:
		sendCancel("Drop the double-handed object first.");
		break;

	case enums::RET_BOTHHANDSNEEDTOBEFREE:
		sendCancel("Both hands needs to be free.");
		break;

	case enums::RET_CANNOTBEDRESSED:
		sendCancel("You cannot dress this object there.");
		break;

	case enums::RET_PUTTHISOBJECTINYOURHAND:
		sendCancel("Put this object in your hand.");
		break;

	case enums::RET_PUTTHISOBJECTINBOTHHANDS:
		sendCancel("Put this object in both hands.");
		break;

	case enums::RET_CANONLYUSEONEWEAPON:
		sendCancel("You may only use one weapon.");
		break;

	case enums::RET_CANONLYUSEONESHIELD:
		sendCancel("You may only use one shield.");
		break;

	case enums::RET_FIRSTGODOWNSTAIRS:
		sendCancel("First go downstairs.");
		break;

	case enums::RET_FIRSTGOUPSTAIRS:
		sendCancel("First go upstairs.");
		break;

	case enums::RET_NOTENOUGHCAPACITY:
		sendCancel("This object is too heavy.");
		break;

	case enums::RET_CONTAINERNOTENOUGHROOM:
		sendCancel("You cannot put more objects in this container.");
		break;

	case enums::RET_NEEDEXCHANGE:
	case enums::RET_NOTENOUGHROOM:
		sendCancel("There is not enough room.");
		break;

	case enums::RET_CANNOTPICKUP:
		sendCancel("You cannot pickup this object.");
		break;

	case enums::RET_CANNOTTHROW:
		sendCancel("You cannot throw there.");
		break;

	case enums::RET_THEREISNOWAY:
		sendCancel("There is no way.");
		break;

	case enums::RET_THISISIMPOSSIBLE:
		sendCancel("This is impossible.");
		break;

	case enums::RET_PLAYERISPZLOCKED:
		sendCancel("You can not enter a protection zone after attacking another player.");
		break;

	case enums::RET_PLAYERISNOTINVITED:
		sendCancel("You are not invited.");
		break;

	case enums::RET_CREATUREDOESNOTEXIST:
		sendCancel("Creature does not exist.");
		break;

	case enums::RET_DEPOTISFULL:
		sendCancel("You cannot put more items in this depot.");
		break;

	case enums::RET_CANNOTUSETHISOBJECT:
		sendCancel("You can not use this object.");
		break;

	case enums::RET_PLAYERWITHTHISNAMEISNOTONLINE:
		sendCancel("A player with this name is not online.");
		break;

	case enums::RET_NOTREQUIREDLEVELTOUSERUNE:
		sendCancel("You do not have the required magic level to use this rune.");
		break;

	case enums::RET_YOUAREALREADYTRADING:
		sendCancel("You are already trading.");
		break;

	case enums::RET_THISPLAYERISALREADYTRADING:
		sendCancel("This player is already trading.");
		break;

	case enums::RET_YOUMAYNOTLOGOUTDURINGAFIGHT:
		sendCancel("You may not logout during or immediately after a fight!");
		break;

	case enums::RET_DIRECTPLAYERSHOOT:
		sendCancel("You are not allowed to shoot directly on players.");
		break;

	case enums::RET_NOTENOUGHLEVEL:
		sendCancel("You do not have enough level.");
		break;

	case enums::RET_NOTENOUGHMAGICLEVEL:
		sendCancel("You do not have enough magic level.");
		break;

	case enums::RET_NOTENOUGHMANA:
		sendCancel("You do not have enough mana.");
		break;

	case enums::RET_NOTENOUGHSOUL:
		sendCancel("You do not have enough soul");
		break;

	case enums::RET_YOUAREEXHAUSTED:
		sendCancel("You are exhausted.");
		break;

	case enums::RET_CANONLYUSETHISRUNEONCREATURES:
		sendCancel("You can only use this rune on creatures.");
		break;

	case enums::RET_PLAYERISNOTREACHABLE:
		sendCancel("Player is not reachable.");
		break;

	case enums::RET_CREATUREISNOTREACHABLE:
		sendCancel("Creature is not reachable.");
		break;

	case enums::RET_ACTIONNOTPERMITTEDINPROTECTIONZONE:
		sendCancel("This action is not permitted in a protection zone.");
		break;

	case enums::RET_YOUMAYNOTATTACKTHISPERSON:
		sendCancel("You may not attack this person.");
		break;

	case enums::RET_YOUMAYNOTATTACKTHISCREATURE:
		sendCancel("You may not attack this creature.");
		break;

	case enums::RET_YOUMAYNOTATTACKAPERSONINPROTECTIONZONE:
		sendCancel("You may not attack a person in a protection zone.");
		break;

	case enums::RET_YOUMAYNOTATTACKAPERSONWHILEINPROTECTIONZONE:
		sendCancel("You may not attack a person while you are in a protection zone.");
		break;

	case enums::RET_YOUCANONLYUSEITONCREATURES:
		sendCancel("You can only use it on creatures.");
		break;

	case enums::RET_TURNSECUREMODETOATTACKUNMARKEDPLAYERS:
		sendCancel("Turn secure mode off if you really want to attack unmarked players.");
		break;

	case enums::RET_YOUNEEDPREMIUMACCOUNT:
		sendCancel("You need a premium account to use this spell.");
		break;

	case enums::RET_YOUNEEDTOLEARNTHISSPELL:
		sendCancel("You need to learn this spell first.");
		break;

	case enums::RET_YOURVOCATIONCANNOTUSETHISSPELL:
		sendCancel("Your vocation cannot use this spell.");
		break;

	case enums::RET_YOUNEEDAWEAPONTOUSETHISSPELL:
		sendCancel("You need to equip a weapon to use this spell.");
		break;

	case enums::RET_PLAYERISPZLOCKEDLEAVEPVPZONE:
		sendCancel("You can not leave a pvp zone after attacking another player.");
		break;

	case enums::RET_PLAYERISPZLOCKEDENTERPVPZONE:
		sendCancel("You can not enter a pvp zone after attacking another player.");
		break;

	case enums::RET_ACTIONNOTPERMITTEDINANONPVPZONE:
		sendCancel("This action is not permitted in a non-pvp zone.");
		break;

	case enums::RET_YOUCANNOTLOGOUTHERE:
		sendCancel("You can not logout here.");
		break;

	case enums::RET_YOUNEEDAMAGICITEMTOCASTSPELL:
		sendCancel("You need a magic item to cast this spell.");
		break;

	case enums::RET_CANNOTCONJUREITEMHERE:
		sendCancel("You cannot conjure items here.");
		break;

	case enums::RET_YOUNEEDTOSPLITYOURSPEARS:
		sendCancel("You need to split your spears first.");
		break;

	case enums::RET_NAMEISTOOAMBIGIOUS:
		sendCancel("Name is too ambigious.");
		break;

	case enums::RET_YOUARENOTTHEOWNER:
		sendCancel("You are not the owner.");
		break;

	case enums::RET_NOTREQUIREDPROFESSION:
		sendCancel("You don't have the required profession.");
		break;

	case enums::RET_NOTREQUIREDLEVEL:
		sendCancel("You don't have the required level.");
		break;

	case enums::RET_NEEDPREMIUMTOEQUIPITEM:
		sendCancel("You need a premium account to equip this item.");
		break;

	case enums::RET_NOTPOSSIBLE:
	default:
		sendCancel("Sorry, not possible.");
		break;
	}
}

void Player::sendStats()
{
	if(client){
		client->sendStats();
	}
}

/*
bool Player::hasShopItemForSale(uint32_t itemId, uint8_t subType)
{
	for(std::list<ShopInfo>::const_iterator it = shopItemList.begin(); it != shopItemList.end(); ++it){
		if(it->itemId == itemId && (*it).buyPrice > 0){
			const ItemType& iit = Item::items[itemId];
			if(iit.stackable || iit.isRune()){
				return it->subType == subType;
			}

			return true;
		}
	}

	return false;
}

void Player::updateSaleShopList(uint32_t itemId)
{
	const ItemType& itemtype = Item::items[itemId];
	for(std::list<ShopInfo>::const_iterator it = shopItemList.begin(); it != shopItemList.end(); ++it){
		if(it->itemId == itemId || itemtype.currency != 0){
			if(client){
				client->sendSaleItemList(shopItemList);
			}

			break;
		}
	}
}
*/


void Player::sendOutfitWindow(const OutfitList& outfitList)
{
	validOutfitList = outfitList;
	if(client)
		client->sendOutfitWindow(outfitList);
}

bool Player::canWearOutfit(const OutfitType& ot) const
{
	for(OutfitList::const_iterator oiter = validOutfitList.begin(); oiter != validOutfitList.end(); ++oiter){
		const Outfit& o = *oiter;
		if(o.lookType == ot.lookType && (o.addons & ot.lookAddons) == o.addons)
			return true;
	}
	return false;
}

Item* Player::getWriteItem(uint32_t& _windowTextId, uint16_t& _maxWriteLen)
{
	_windowTextId = windowTextId;
	_maxWriteLen = maxWriteLen;
	return writeItem;
}

void Player::setWriteItem(Item* item, uint16_t _maxWriteLen /*= 0*/)
{
	windowTextId++;
	if(writeItem){
		writeItem->unRef();
	}

	if(item){
		writeItem = item;
		maxWriteLen = _maxWriteLen;
		writeItem->addRef();
	}
	else{
		writeItem = NULL;
		maxWriteLen = 0;
	}
}

House* Player::getEditHouse(uint32_t& _windowTextId, uint32_t& _listId)
{
	_windowTextId = windowTextId;
	_listId = editListId;
	return editHouse;
}

void Player::setEditHouse(House* house, uint32_t listId /*= 0*/)
{
	windowTextId++;
	editHouse = house;
	editListId = listId;
}

void Player::sendHouseWindow(House* house, uint32_t listId) const
{
	if(client){
		std::string text;
		if(house->getAccessList(listId, text)){
			client->sendHouseWindow(windowTextId, house, listId, text);
		}
	}
}

//container
void Player::sendAddContainerItem(const Container* container, const Item* item)
{
	if(client){
		for(ContainerVector::const_iterator cl = containerVec.begin(); cl != containerVec.end(); ++cl){
			if(cl->second == container){
				client->sendAddContainerItem(cl->first, item);
			}
		}
	}
}

void Player::sendUpdateContainerItem(const Container* container, uint8_t slot, const Item* oldItem, const Item* newItem)
{
	if(client){
		for(ContainerVector::const_iterator cl = containerVec.begin(); cl != containerVec.end(); ++cl){
			if(cl->second == container){
				client->sendUpdateContainerItem(cl->first, slot, newItem);
			}
		}
	}
}

void Player::sendRemoveContainerItem(const Container* container, uint8_t slot, const Item* item)
{
	if(client){
		for(ContainerVector::const_iterator cl = containerVec.begin(); cl != containerVec.end(); ++cl){
			if(cl->second == container){
				client->sendRemoveContainerItem(cl->first, slot);
			}
		}
	}
}

void Player::onAddTileItem(const Tile* tile, const Position& pos, const Item* item)
{
	Creature::onAddTileItem(tile, pos, item);
}

void Player::onUpdateTileItem(const Tile* tile, const Position& pos,
	const Item* oldItem, const ItemType& oldType, const Item* newItem, const ItemType& newType)
{
	Creature::onUpdateTileItem(tile, pos, oldItem, oldType, newItem, newType);

	if(oldItem != newItem){
		onRemoveTileItem(tile, pos, oldType, oldItem);
	}

	if(tradeState != TRADE_TRANSFER){
		if(tradeItem && oldItem == tradeItem){
			g_game.internalCloseTrade(this);
		}
	}
}

void Player::onRemoveTileItem(const Tile* tile, const Position& pos,
	const ItemType& iType, const Item* item)
{
	Creature::onRemoveTileItem(tile, pos, iType, item);

	if(tradeState != TRADE_TRANSFER){
		checkTradeState(item);

		if(tradeItem){
			const Container* container = item->getContainer();
			if(container && container->isHoldingItem(tradeItem)){
				g_game.internalCloseTrade(this);
			}
		}
	}
}

void Player::onUpdateTile(const Tile* tile, const Position& pos)
{
	Creature::onUpdateTile(tile, pos);
}

void Player::onCreatureAppear(const Creature* creature, bool isLogin)
{
	Creature::onCreatureAppear(creature, isLogin);

	if(isLogin && creature == this){
		Item* item;
		for(SlotType::iterator slot = SLOT_FIRST; slot < SLOT_LAST; ++slot){
			if((item = getInventoryItem(*slot))){
				g_game.startDecay(item);
				g_game.onPlayerEquipItem(this, item, *slot, true);
			}
		}

		if(!storedConditionList.empty()){
			for(ConditionList::const_iterator it = storedConditionList.begin(); it != storedConditionList.end(); ++it){
				addCondition(*it);
			}

			storedConditionList.clear();
		}

		//[ added for beds system
		BedItem* bed = Beds::instance().getBedBySleeper(getGUID());
		if(bed){
			bed->wakeUp();
			#ifdef __DEBUG__
			std::cout << "Player " << getName() << " waking up." << std::endl;
			#endif
		}
		//]

		if(lastLogout > 0)
		{
			int64_t timeOff = time(NULL) - lastLogout - 600;
			if(timeOff > 0){
				int32_t stamina_rate = g_config.getNumber(ConfigManager::RATE_STAMINA_GAIN);
				int32_t slow_stamina_rate = g_config.getNumber(ConfigManager::SLOW_RATE_STAMINA_GAIN);
				int32_t quick_stamina_max = MAX_STAMINA - g_config.getNumber(ConfigManager::STAMINA_EXTRA_EXPERIENCE_DURATION);
				int64_t gain;
				bool checkSlowStamina = true;

				if(getStamina() < quick_stamina_max){
					gain = timeOff * stamina_rate;
					if(getStamina() + gain > quick_stamina_max){
						timeOff -= (quick_stamina_max - getStamina()) / stamina_rate;
						addStamina(quick_stamina_max - getStamina());
					}
					else{
						addStamina(gain);
						checkSlowStamina = false;
					}
				}

				if(getStamina() < MAX_STAMINA && checkSlowStamina){
					gain = timeOff * slow_stamina_rate;
					addStamina(gain);
				}
			}

			sendStats();
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

void Player::onChangeZone(ZoneType zone)
{
	if(attackedCreature){
		if(zone == ZONE_PROTECTION){
			if(!hasFlag(PlayerFlag_IgnoreProtectionZone)){
				setAttackedCreature(NULL);
				onAttackedCreatureDissapear(false);
			}
		}
		else if(zone == ZONE_NOPVP){
			if( (attackedCreature->getPlayer() ||
					(attackedCreature->isPlayerSummon()) ) &&
					!hasFlag(PlayerFlag_IgnoreProtectionZone)){
				setAttackedCreature(NULL);
				onAttackedCreatureDissapear(false);
			}
		}
	}

	sendIcons();
}

void Player::onAttackedCreatureChangeZone(ZoneType zone)
{
	if(zone == ZONE_PROTECTION){
		if(!hasFlag(PlayerFlag_IgnoreProtectionZone)){
			setAttackedCreature(NULL);
			onAttackedCreatureDissapear(false);
		}
	}
	else if(zone == ZONE_NOPVP){
		if(attackedCreature->getPlayer() && !hasFlag(PlayerFlag_IgnoreProtectionZone)){
			setAttackedCreature(NULL);
			onAttackedCreatureDissapear(false);
		}
	}
	else if(zone == ZONE_NORMAL){
		//attackedCreature can leave a pvp zone if not pzlocked
		if(g_game.getWorldType() == WORLD_TYPE_NOPVP){
			if(attackedCreature->getPlayer()){
				setAttackedCreature(NULL);
				onAttackedCreatureDissapear(false);
			}
		}
	}
}

void Player::onCreatureDisappear(const Creature* creature, bool isLogout)
{
	Creature::onCreatureDisappear(creature, isLogout);

	if(creature == this){
		if(isLogout){
			loginPosition = getPosition();
		}

		if(eventWalk != 0){
			setFollowCreature(NULL);
		}

		clearPartyInvitations();
		if(getParty()){
			getParty()->leaveParty(this);
		}

		if(tradePartner){
			g_game.internalCloseTrade(this);
		}

		// REVSCRIPTSYS
		// Close shop window when player logs out
		//closeShopWindow();

		g_game.cancelRuleViolation(this);

		if(hasFlag(PlayerFlag_CanAnswerRuleViolations)){
			std::list<Player*> closeReportList;
			for(RuleViolationsMap::const_iterator it = g_game.getRuleViolations().begin(); it != g_game.getRuleViolations().end(); ++it){
				if(it->second->gamemaster == this){
					closeReportList.push_back(it->second->reporter);
				}
			}

			for(std::list<Player*>::iterator it = closeReportList.begin(); it != closeReportList.end(); ++it){
				g_game.closeRuleViolation(*it);
			}
		}

		g_chat.removeUserFromAllChannels(this);

		lastLogout = time(NULL);
		IOPlayer::instance()->updateLogoutInfo(this);

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

/*
void Player::closeShopWindow()
{
	//unreference callbacks
	int32_t onBuy;
	int32_t onSell;

	Npc* npc = getShopOwner(onBuy, onSell);
	if(npc){
		setShopOwner(NULL, -1, -1);
		// REVSCRIPT TODO
		//npc->onPlayerEndTrade(this, onBuy, onSell);
		sendCloseShop();
	}
}
*/

void Player::onWalk(Direction& dir)
{
	Creature::onWalk(dir);
	setNextActionTask(NULL);
	setNextAction(OTSYS_TIME() + getStepDuration(dir));
}

void Player::onCreatureMove(const Creature* creature, const Tile* newTile, const Position& newPos,
	const Tile* oldTile, const Position& oldPos, bool teleport)
{
	Creature::onCreatureMove(creature, newTile, newPos, oldTile, oldPos, teleport);

	if(creature == this){
		if(tradeState != TRADE_TRANSFER){
			//check if we should close trade
			if(tradeItem){
				if(!Position::areInRange<1,1,0>(tradeItem->getPosition(), getPosition())){
					g_game.internalCloseTrade(this);
				}
			}

			if(tradePartner){
				if(!Position::areInRange<2,2,0>(tradePartner->getPosition(), getPosition())){
					g_game.internalCloseTrade(this);
				}
			}
		}

		if(getParty()){
			getParty()->updateSharedExperience();
		}

		if(teleport || (oldPos.z != newPos.z)){
			addCondition(Condition::createCondition(CONDITION_DISARMED, g_game.getStairhopExhaustion()));
			addCondition(Condition::createCondition(CONDITION_EXHAUST_DAMAGE, g_game.getStairhopExhaustion()));
		}
	}
}

//container
void Player::onAddContainerItem(const Container* container, const Item* item)
{
	checkTradeState(item);
}

void Player::onUpdateContainerItem(const Container* container, uint8_t slot,
	const Item* oldItem, const ItemType& oldType, const Item* newItem, const ItemType& newType)
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
				g_game.internalCloseTrade(this);
			}
		}
	}
}

void Player::onCloseContainer(const Container* container)
{
	if(client){
		for(ContainerVector::const_iterator cl = containerVec.begin(); cl != containerVec.end(); ++cl){
			if(cl->second == container){
				client->sendCloseContainer(cl->first);
			}
		}
	}
}

void Player::onSendContainer(const Container* container)
{
	if(client){
		bool hasParent = (dynamic_cast<const Container*>(container->getParent()) != NULL);

		for(ContainerVector::const_iterator cl = containerVec.begin(); cl != containerVec.end(); ++cl){
			if(cl->second == container){
				client->sendContainer(cl->first, container, hasParent);
			}
		}
	}
}

//inventory
void Player::onAddInventoryItem(SlotType slot, Item* item)
{
	//
}

void Player::onUpdateInventoryItem(SlotType slot, Item* oldItem, const ItemType& oldType,
	Item* newItem, const ItemType& newType)
{
	if(oldItem != newItem){
		onRemoveInventoryItem(slot, oldItem);
	}

	if(tradeState != TRADE_TRANSFER){
		checkTradeState(oldItem);
	}
}

void Player::onRemoveInventoryItem(SlotType slot, Item* item)
{
	//setItemAbility(slot, false);

	if(tradeState != TRADE_TRANSFER){
		checkTradeState(item);

		if(tradeItem){
			const Container* container = item->getContainer();
			if(container && container->isHoldingItem(tradeItem)){
				g_game.internalCloseTrade(this);
			}
		}
	}
}

void Player::checkTradeState(const Item* item)
{
	if(tradeItem && tradeState != TRADE_TRANSFER){
		if(tradeItem == item){
			g_game.internalCloseTrade(this);
		}
		else{
			const Container* container = dynamic_cast<const Container*>(item->getParent());

			while(container != NULL){
				if(container == tradeItem){
					g_game.internalCloseTrade(this);
					break;
				}

				container = dynamic_cast<const Container*>(container->getParent());
			}
		}
	}
}

void Player::setNextWalkActionTask(SchedulerTask* task)
{
	if(walkTaskEvent != 0){
		g_scheduler.stopEvent(walkTaskEvent);
		walkTaskEvent = 0;
	}
	delete walkTask;
	walkTask = task;
	resetIdle();
}

void Player::setNextWalkTask(SchedulerTask* task)
{
	if(nextStepEvent != 0){
		g_scheduler.stopEvent(nextStepEvent);
		nextStepEvent = 0;
	}

	if(task){
		nextStepEvent = g_scheduler.addEvent(task);
		resetIdle();
	}
}

void Player::setNextActionTask(SchedulerTask* task)
{
	if(actionTaskEvent != 0){
		g_scheduler.stopEvent(actionTaskEvent);
		actionTaskEvent = 0;
	}

	if(task){
		actionTaskEvent = g_scheduler.addEvent(task);
		resetIdle();
	}
}

uint32_t Player::getNextActionTime() const
{
	int64_t time = nextAction - OTSYS_TIME();
	if(time < SCHEDULER_MINTICKS){
		return SCHEDULER_MINTICKS;
	}

	return time;
}


void Player::onThink(uint32_t interval)
{
	Creature::onThink(interval);

	int64_t timeNow = OTSYS_TIME();
	if(timeNow - last_ping >= 5000){
		last_ping = timeNow;

		if(client){
			client->sendPing();
		}
	}

	if(canLogout()){
		if(OTSYS_TIME() - last_pong >= 60000){
			if(client){
				client->logout(true);
			}
			else{
				//Occurs when the player closes the game without logging out (x-logging).
				if(g_game.onPlayerLogout(this, false, true))
					g_game.removeCreature(this, true);
			}
		}
	}

	MessageBufferTicks += interval;
	if(MessageBufferTicks >= 1500){
		MessageBufferTicks = 0;
		addMessageBuffer();
	}

	checkIdleTime(interval);
#ifdef __SKULLSYSTEM__
	checkSkullTicks(interval);
#endif
}

uint32_t Player::getMuteTime()
{
	if(hasFlag(PlayerFlag_CannotBeMuted)){
		return 0;
	}

	Condition* condition = getCondition(CONDITION_MUTED_CHAT);
	if(condition){
		return (uint32_t)condition->getTicks() / 1000;
	}

	return 0;
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
			Condition* condition = Condition::createCondition(CONDITION_MUTED_CHAT, muteTime * 1000);
			addCondition(condition);

			std::stringstream ss;
			ss << "You are muted for " << muteTime << " seconds.";
			sendTextMessage(MSG_STATUS_SMALL, ss.str());
		}
	}
}

void Player::drainHealth(CombatType combatType, const CombatSource& combatSource, int32_t damage, bool showtext /*= true*/)
{
	Creature::drainHealth(combatType, combatSource, damage, showtext);

	sendStats();

	if(showtext){
		std::stringstream ss;
		if(damage == 1) {
			ss << "You lose 1 hitpoint";
		}
		else
			ss << "You lose " << damage << " hitpoints";

		if(combatSource.isSourceCreature()){
			ss << " due to an attack by " << combatSource.getSourceCreature()->getNameDescription();
		}

		ss << ".";

		sendTextMessage(MSG_EVENT_DEFAULT, ss.str());
	}
}

void Player::drainMana(const CombatSource& combatSource, int32_t points, bool showtext /*= true*/)
{
	Creature::drainMana(combatSource, points, showtext);

	sendStats();

	if(showtext){
		std::stringstream ss;
		if(combatSource.isSourceCreature()){
			ss << "You lose " << points << " mana blocking an attack by " << combatSource.getSourceCreature()->getNameDescription() << ".";
		}
		else{
			ss << "You lose " << points << " mana.";
		}

		sendTextMessage(MSG_EVENT_DEFAULT, ss.str());
	}
}

void Player::addManaSpent(uint32_t amount, bool useMultiplier /*= true*/)
{
	if(amount != 0 && !hasFlag(PlayerFlag_NotGainMana)){
		if(useMultiplier){
			amount = uint32_t(amount * getRateValue(LEVEL_MAGIC));
		}
		manaSpent += amount * g_config.getNumber(ConfigManager::RATE_MAGIC);
		uint32_t reqMana = vocation->getReqMana(magLevel + 1);

		if(manaSpent >= reqMana){
			manaSpent -= reqMana;
			int oldlevel = magLevel++;

			g_game.onPlayerAdvance(this, LEVEL_MAGIC, oldlevel, magLevel);

			sendStats();
		}

		magLevelPercent = Player::getPercentLevel(manaSpent, vocation->getReqMana(magLevel + 1));
	}
}

void Player::addExperience(uint64_t exp)
{
	experience += exp;
	uint32_t prevLevel = getLevel();
	uint32_t newLevel = getLevel();

	uint64_t currLevelExp = Player::getExpForLevel(newLevel);
	uint64_t nextLevelExp = Player::getExpForLevel(newLevel + 1);
	if(nextLevelExp < currLevelExp) {
		// Cannot gain more experience
		// Perhaps some sort of notice should be printed here?
		return;
	}
	while(experience >= nextLevelExp) {
		++newLevel;
		healthMax += vocation->getHPGain();
		health += vocation->getHPGain();
		manaMax += vocation->getManaGain();
		mana += vocation->getManaGain();
		capacity += vocation->getCapGain();
		nextLevelExp = Player::getExpForLevel(newLevel + 1);
	}

	if(prevLevel != newLevel){
		level = newLevel;
		updateBaseSpeed();

		int32_t newSpeed = getBaseSpeed();
		setBaseSpeed(newSpeed);

		g_game.changeSpeed(this, 0);
		g_game.addCreatureHealth(this);

		if(getParty()){
			getParty()->updateSharedExperience();
		}

		g_game.onPlayerAdvance(this, LEVEL_EXPERIENCE, prevLevel, newLevel);
	}

	currLevelExp = Player::getExpForLevel(level);
	nextLevelExp = Player::getExpForLevel(level + 1);
	if(nextLevelExp > currLevelExp) {
		uint32_t newPercent = Player::getPercentLevel(getExperience() - currLevelExp, Player::getExpForLevel(level + 1) - currLevelExp);
		levelPercent = newPercent;
	}
	else {
		levelPercent = 0;
	}

	sendStats();
}

void Player::removeExperience(uint64_t exp, bool updateStats /*= true*/)
{
	experience -= std::min(exp, experience);
	uint32_t prevLevel = getLevel();
	uint32_t newLevel = getLevel();

	while(newLevel > 1 && experience < Player::getExpForLevel(newLevel)){
		newLevel--;
		healthMax = std::max((int32_t)0, (healthMax - (int32_t)vocation->getHPGain()));
		manaMax = std::max((int32_t)0, (manaMax - (int32_t)vocation->getManaGain()));
		capacity = std::max((double)0, (capacity - (double)vocation->getCapGain()));
	}

	if(prevLevel != newLevel){
		level = newLevel;
		std::stringstream levelMsg;
		levelMsg << "You were downgraded from Level " << prevLevel << " to Level " << newLevel << ".";
		sendTextMessage(MSG_EVENT_ADVANCE, levelMsg.str());
	}

	//Only if player is not going to be removed (usually when dying)
	if(updateStats){
		bool sentStats = false;

		uint64_t currLevelExp = Player::getExpForLevel(level);
		uint64_t nextLevelExp = Player::getExpForLevel(level + 1);
		if(nextLevelExp > currLevelExp){
			uint32_t newPercent = Player::getPercentLevel(getExperience() - currLevelExp, Player::getExpForLevel(level + 1) - currLevelExp);
			levelPercent = newPercent;
		}
		else{
			levelPercent = 0;
		}

		if(prevLevel != newLevel){
			int32_t healthChange = health > healthMax ? (health - healthMax) : 0;
			int32_t manaChange = mana > manaMax ? (mana - manaMax) : 0;
			changeMana(-manaChange);
			changeHealth(-healthChange);
			sentStats = true;
		}

		if(!sentStats){
			sendStats();
		}
	}
}

uint32_t Player::getPercentLevel(uint64_t count, uint32_t nextLevelCount)
{
	if(nextLevelCount > 0){
		uint32_t result = ((uint32_t)((double)count / nextLevelCount * 100));
		if(result < 0 || result > 100){
			return 0;
		}

		return result;
	}

	return 0;
}

void Player::onBlockHit(BlockType blockType)
{
	if(shieldBlockCount > 0){
		--shieldBlockCount;

		if(hasShield()){
			addSkillAdvance(SKILL_SHIELD, 1);
		}
	}
}

void Player::onAttackedCreatureBlockHit(Creature* target, BlockType blockType)
{
	Creature::onAttackedCreatureBlockHit(target, blockType);

	lastAttackBlockType = blockType;

	if(blockType == BLOCK_NONE)
	{
		addAttackSkillPoint = true;
		bloodHitCount = 30;
		shieldBlockCount = 30;
	}

	else if(blockType == BLOCK_DEFENSE || blockType == BLOCK_ARMOR)
	{
		//need to draw blood every 30 hits
		if(bloodHitCount > 0){
			addAttackSkillPoint = true;
			--bloodHitCount;
		}
		else{
			addAttackSkillPoint = false;
		}
	}
	else{
		addAttackSkillPoint = false;
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
BlockType Player::blockHit(CombatType combatType, const CombatSource& combatSource, int32_t& damage,
	bool checkDefense /* = false*/, bool checkArmor /* = false*/)
{
	BlockType blockType = Creature::blockHit(combatType, combatSource, damage, checkDefense, checkArmor);

	Creature* attacker = combatSource.getSourceCreature();
	if(attacker){
		sendCreatureSquare(attacker, SQ_COLOR_BLACK);
	}

	if(blockType != BLOCK_NONE)
		return blockType;

	if(damage != 0){
		//reduce damage against inventory items
		Item* item = NULL;
		for(SlotType::iterator slot = SLOT_FIRST; slot < SLOT_LAST; ++slot){
			if(!(item = getEquippedItem(*slot)))
				continue;

			const ItemType& it = Item::items[item->getID()];

			if(it.abilities.absorb.reduce(combatType, damage)){
				int32_t charges = item->getCharges();
				if(charges != 0)
					// Are the person hit responsible for the action...?
					g_game.transformItem(NULL, item, item->getID(), charges - 1);
			}
		}

		if(damage <= 0){
			damage = 0;
			blockType = BLOCK_DEFENSE;
		}
	}
	return blockType;
}

uint32_t Player::getIP() const
{
	if(client){
		return client->getIP();
	}
	return 0;
}

void Player::onDie()
{
	if(getZone() != ZONE_PVP){
		bool isLootPrevented = false;
		bool isSkillPrevented = false;

		Item* item = NULL;
		for(SlotType::iterator slot = SLOT_FIRST; slot < SLOT_LAST; ++slot){
			if(!(item = getEquippedItem(*slot))){
				continue;
			}
			const ItemType& it = Item::items[item->getID()];
			if((it.abilities.preventItemLoss && !isLootPrevented) || (it.abilities.preventSkillLoss && !isSkillPrevented)){
				if(it.abilities.preventItemLoss){
					isLootPrevented = true;
				}
				if(it.abilities.preventSkillLoss){
					isSkillPrevented = true;
				}
				int32_t newCharge = std::max((int32_t)0, ((int32_t)item->getCharges()) - 1);
				g_game.transformItem(NULL, item, item->getID(), newCharge);
			}
		}

		#ifdef __SKULLSYSTEM__
		if(isLootPrevented && getSkull() != SKULL_RED && getSkull() != SKULL_BLACK){
		#else
		if(isLootPrevented){
		#endif
			setDropLoot(false);
		}
		if(isSkillPrevented){
			setLossSkill(false);
		}

		DeathList killers = getKillers(g_config.getNumber(ConfigManager::DEATH_ASSIST_COUNT));
		IOPlayer::instance()->addPlayerDeath(this, killers);

		#ifdef __SKULLSYSTEM__
		for(DeathList::const_iterator it = killers.begin(); it != killers.end(); ++it){
			if(it->isCreatureKill() && it->isUnjustKill()){
				Creature* attacker = it->getKillerCreature();
				Player* attackerPlayer = attacker->getPlayer();

				if(attacker->isPlayerSummon()){
					attackerPlayer = attacker->getPlayerMaster();
				}

				if(attackerPlayer){
					attackerPlayer->addUnjustifiedDead(this);
				}
			}
		}
		#endif
	}

	Creature::onDie();
}

void Player::die()
{
	ConditionEnd conditionEnd = CONDITIONEND_DEATH;
	if(getZone() == ZONE_PVP){
		conditionEnd = CONDITIONEND_REMOVED;
	}

	for(ConditionList::iterator it = conditions.begin(); it != conditions.end();){
		if((*it)->isPersistent()){
			Condition* condition = *it;

			onEndCondition(condition);
			it = conditions.erase(it);
			condition->onEnd(this, conditionEnd);
			onEndCondition(condition, false);
			delete condition;
		}
		else{
			++it;
		}
	}

	if(getZone() != ZONE_PVP){
		loginPosition = masterPos;

		if(skillLoss){
			uint64_t expLost = getLostExperience();

			//Level loss
			removeExperience(expLost, false);
			double lostPercent = 1. - (double(experience - expLost) / double(experience)); // 0.1 if 10% was lost

			//Magic level loss
			uint32_t sumMana = 0;
			int32_t lostMana = 0;

			for(uint32_t i = 1; i <= magLevel; ++i){
				sumMana += vocation->getReqMana(i);
			}

			sumMana += manaSpent;

			double lostPercentMana = lostPercent * lossPercent[*LOSS_MANASPENT] / 100;
			lostMana = (int32_t)std::ceil(sumMana * lostPercentMana);

			while((uint32_t)lostMana > manaSpent && magLevel > 0){
				lostMana -= manaSpent;
				manaSpent = vocation->getReqMana(magLevel);
				magLevel--;
			}

			manaSpent = std::max((int32_t)0, (int32_t)manaSpent - lostMana);
			magLevelPercent = Player::getPercentLevel(manaSpent, vocation->getReqMana(magLevel + 1));

			//Skill loss
			uint32_t lostSkillTries;
			uint32_t sumSkillTries;
			for(SkillType::iterator i = SkillType::begin(); i != SkillType::end(); ++i){
				lostSkillTries = 0;
				sumSkillTries = 0;

				for(uint32_t c = 11; c <= skills[i->value()][SKILL_LEVEL]; ++c) {
					sumSkillTries += vocation->getReqSkillTries(*i, c);
				}

				sumSkillTries += skills[i->value()][SKILL_TRIES];
				double lossPercentSkill = lostPercent * lossPercent[*LOSS_SKILLTRIES] / 100;
				lostSkillTries = (uint32_t)std::ceil(sumSkillTries * lossPercentSkill);

				while(lostSkillTries > skills[i->value()][SKILL_TRIES]){
					lostSkillTries -= skills[i->value()][SKILL_TRIES];
					skills[i->value()][SKILL_TRIES] = vocation->getReqSkillTries(*i, skills[i->value()][SKILL_LEVEL]);
					if(skills[i->value()][SKILL_LEVEL] > 10){
						skills[i->value()][SKILL_LEVEL]--;
					}
					else{
						skills[i->value()][SKILL_LEVEL] = 10;
						skills[i->value()][SKILL_TRIES] = 0;
						lostSkillTries = 0;
						break;
					}
				}

				skills[i->value()][SKILL_TRIES] = std::max((int32_t)0, (int32_t)(skills[i->value()][SKILL_TRIES] - lostSkillTries));
			}
		}

		Creature::die();
		sendReLoginWindow();
	}
	else{
		preSave();
		setDropLoot(true);
		setLossSkill(true);
		sendStats();
		g_game.internalTeleport(NULL, this, getTemplePosition());
		g_game.addCreatureHealth(this);
		onThink(EVENT_CREATURE_THINK_INTERVAL);
	}
}

Item* Player::dropCorpse()
{
	if(getZone() != ZONE_PVP){
		return Creature::dropCorpse();
	}

	return NULL;
}

Item* Player::createCorpse()
{
	Item* corpse = Creature::createCorpse();
	if(corpse && corpse->getContainer()){
		std::stringstream ss;

		ss << "You recognize " << getNameDescription() << ".";

		DeathList killers = getKillers(0);
		if(!killers.empty() && (*killers.begin()).isCreatureKill()){
			ss << " " << playerSexSubjectString(getSex()) << " was killed by "
				<< ((*killers.begin()).getKillerCreature())->getNameDescription() << ".";
		}

		corpse->setSpecialDescription(ss.str());
	}

	return corpse;
}

void Player::preSave()
{
	if(health <= 0){
		#ifdef __SKULLSYSTEM__
		if(getSkull() != SKULL_BLACK){
		#endif
			health = healthMax;
			mana = manaMax;
		#ifdef __SKULLSYSTEM__
		}
		else{
			if(healthMax >= 40){
				health = 40;
			}
			else{
				health = healthMax;
				mana = 0;
			}
		}
		#endif
	}
}

void Player::addCombatExhaust(uint32_t ticks)
{
	if(!hasFlag(PlayerFlag_HasNoExhaustion)){
		Condition* condition = Condition::createCondition(CONDITION_EXHAUST_DAMAGE, ticks);
		addCondition(condition);
	}
}

void Player::addHealExhaust(uint32_t ticks)
{
	if(!hasFlag(PlayerFlag_HasNoExhaustion)){
		Condition* condition = Condition::createCondition(CONDITION_EXHAUST_HEAL, ticks);
		addCondition(condition);
	}
}

void Player::addInFightTicks(uint32_t ticks, bool pzlock /*= false*/)
{
	if(!hasFlag(PlayerFlag_NotGainInFight)){
		if(pzlock){
			pzLocked = true;
			sendIcons();
		}
		Condition* condition = Condition::createCondition(CONDITION_INFIGHT, ticks);
		addCondition(condition);
	}
}

void Player::addDefaultRegeneration(uint32_t addTicks)
{
	Condition* condition = getCondition(CONDITION_REGENERATION);
	if(condition){
		condition->setTicks(condition->getTicks() + addTicks);
	}
	else{
		condition = Condition::createCondition(CONDITION_REGENERATION, addTicks);
		if(condition){
			Condition::Effect effectRegenHealth = Condition::Effect::createRegenHealth(vocation->getHealthGainTicks() * 1000, vocation->getHealthGainAmount());
			condition->addEffect(effectRegenHealth);
			Condition::Effect effectRegenMana = Condition::Effect::createRegenMana(vocation->getManaGainTicks() * 1000, vocation->getManaGainAmount());
			condition->addEffect(effectRegenMana);

			addCondition(condition);
		}
	}
}

void Player::removeList()
{
	listPlayer.removeList(getID());

	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
	{
		(*it).second->notifyLogOut(this);
	}

	Status::instance()->removePlayer();
}

void Player::addList()
{
	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it)
	{
		(*it).second->notifyLogIn(this);
	}

	listPlayer.addList(this);

	Status::instance()->addPlayer();
}

void Player::kickPlayer()
{
	if(client){
		client->logout(true);
	}
	else{
		g_game.removeCreature(this);
	}
}

void Player::notifyLogIn(Player* login_player)
{
	if(client){
		VIPListSet::iterator it = VIPList.find(login_player->getGUID());
		if(it != VIPList.end()){
			client->sendVIPLogIn(login_player->getGUID());
			client->sendTextMessage(MSG_STATUS_SMALL, (login_player->getName() + " has logged in."));
		}
	}
}

void Player::notifyLogOut(Player* logout_player)
{
	if(client){
		VIPListSet::iterator it = VIPList.find(logout_player->getGUID());
		if(it != VIPList.end()){
			client->sendVIPLogOut(logout_player->getGUID());
			client->sendTextMessage(MSG_STATUS_SMALL, (logout_player->getName() + " has logged out."));
		}
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

	if(client && !internal){
		client->sendVIP(_guid, name, isOnline);
	}

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
		if(client){
			client->sendCloseContainer(*it);
		}
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

	if((item->getSlotPosition() & SLOTPOSITION_HEAD) ||
		(item->getSlotPosition() & SLOTPOSITION_NECKLACE) ||
		(item->getSlotPosition() & SLOTPOSITION_BACKPACK) ||
		(item->getSlotPosition() & SLOTPOSITION_ARMOR) ||
		(item->getSlotPosition() & SLOTPOSITION_LEGS) ||
		(item->getSlotPosition() & SLOTPOSITION_FEET) ||
		(item->getSlotPosition() & SLOTPOSITION_RING)){
		ret = RET_CANNOTBEDRESSED;
	}
	else if(item->getSlotPosition() & SLOTPOSITION_TWO_HAND){
		ret = RET_PUTTHISOBJECTINBOTHHANDS;
	}
	else if((item->getSlotPosition() & SLOTPOSITION_RIGHT) || (item->getSlotPosition() & SLOTPOSITION_LEFT)){
		ret = RET_PUTTHISOBJECTINYOURHAND;
	}

	//check if we can dress this object
	SlotType slot(index);
	if(slot == SLOT_HEAD){
		if(item->getSlotPosition() & SLOTPOSITION_HEAD)
			ret = RET_NOERROR;
	}
	else if(slot == SLOT_NECKLACE){
		if(item->getSlotPosition() & SLOTPOSITION_NECKLACE)
			ret = RET_NOERROR;
	}
	else if(slot == SLOT_BACKPACK){
		if(item->getSlotPosition() & SLOTPOSITION_BACKPACK)
			ret = RET_NOERROR;
	}
	else if(slot == SLOT_ARMOR){
		if(item->getSlotPosition() & SLOTPOSITION_ARMOR)
			ret = RET_NOERROR;
	}
	else if(slot == SLOT_RIGHT){
		if(item->getSlotPosition() & SLOTPOSITION_RIGHT){
			//check if we already carry an item in the other hand
			if(item->getSlotPosition() & SLOTPOSITION_TWO_HAND){
				if(getInventoryItem(SLOT_LEFT) && getInventoryItem(SLOT_LEFT) != item){
					ret = RET_BOTHHANDSNEEDTOBEFREE;
				}
				else
					ret = RET_NOERROR;
			}
			else{
				ret = RET_NOERROR;
				if(getInventoryItem(SLOT_LEFT)){
					const Item* leftItem = getInventoryItem(SLOT_LEFT);
					//check if we already carry a double-handed item
					if(leftItem->getSlotPosition() & SLOTPOSITION_TWO_HAND){
						ret = RET_DROPTWOHANDEDITEM;
					}
					else if(!(item == leftItem && count == item->getItemCount())){
						if(item->getWeaponType() != WEAPON_NONE && item->getWeaponType() != WEAPON_AMMO){
							//check so we only equip one shield
							if(item->getWeaponType() == WEAPON_SHIELD){
								if(leftItem->getWeaponType() == WEAPON_SHIELD){
									ret = RET_CANONLYUSEONESHIELD;
								}
							}
							else{
								//check so we can only equip one weapon
								if(	leftItem->getWeaponType() != WEAPON_NONE &&
									leftItem->getWeaponType() != WEAPON_SHIELD &&
									leftItem->getWeaponType() != WEAPON_AMMO){
									ret = RET_CANONLYUSEONEWEAPON;
								}
							}
						}
					}
				}
			}
		}
	}
	else if(slot == SLOT_LEFT){
		if(item->getSlotPosition() & SLOTPOSITION_LEFT){
			//check if we already carry an item in the other hand
			if(item->getSlotPosition() & SLOTPOSITION_TWO_HAND){
				if(getInventoryItem(SLOT_RIGHT) && getInventoryItem(SLOT_RIGHT) != item){
					ret = RET_BOTHHANDSNEEDTOBEFREE;
				}
				else
					ret = RET_NOERROR;
			}
			else{
				ret = RET_NOERROR;
				if(getInventoryItem(SLOT_RIGHT)){
					const Item* rightItem = getInventoryItem(SLOT_RIGHT);
					//check if we already carry a double-handed item
					if(rightItem->getSlotPosition() & SLOTPOSITION_TWO_HAND){
						ret = RET_DROPTWOHANDEDITEM;
					}
					else if(!(item == rightItem && count == item->getItemCount())){
						if(item->getWeaponType() != WEAPON_NONE && item->getWeaponType() != WEAPON_AMMO){
							if(item->getWeaponType() == WEAPON_SHIELD){
								//check so we only equip one shield
								if(rightItem->getWeaponType() == WEAPON_SHIELD){
									ret = RET_CANONLYUSEONESHIELD;
								}
							}
							else{
								//check so we can only equip one weapon
								if(	rightItem->getWeaponType() != WEAPON_NONE &&
									rightItem->getWeaponType() != WEAPON_SHIELD &&
									rightItem->getWeaponType() != WEAPON_AMMO){
									ret = RET_CANONLYUSEONEWEAPON;
								}
							}
						}
					}
				}
			}
		}
	}
	else if(slot == SLOT_LEGS){
		if(item->getSlotPosition() & SLOTPOSITION_LEGS)
			ret = RET_NOERROR;
	}
	else if(slot == SLOT_FEET){
		if(item->getSlotPosition() & SLOTPOSITION_FEET)
			ret = RET_NOERROR;
	}
	else if(slot == SLOT_RING){
		if(item->getSlotPosition() & SLOTPOSITION_RING)
			ret = RET_NOERROR;
	}
	else if(slot == SLOT_AMMO){
		if(item->getSlotPosition() & SLOTPOSITION_AMMO)
			ret = RET_NOERROR;
	}
	else if(slot == SLOT_WHEREEVER){
		ret = RET_NOTENOUGHROOM;
	}
	else if(index == -1){
		ret = RET_NOTENOUGHROOM;
	}
	else{
		ret = RET_NOTPOSSIBLE;
	}

	if(ret == RET_NOERROR || ret == RET_NOTENOUGHROOM){
		//need an exchange with source?
		if(getInventoryItem((SlotType)index) != NULL){
			if(!getInventoryItem((SlotType)index)->isStackable() || getInventoryItem((SlotType)index)->getID() != item->getID()){
				return RET_NEEDEXCHANGE;
			}
		}

		// REVSCRIPT TODO event callback (onEquip)

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

ReturnValue Player::__queryRemove(const Thing* thing, uint32_t count, uint32_t flags) const
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

	if(!item->isMoveable() && !hasBitSet(FLAG_IGNORENOTMOVEABLE, flags)){
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
		for(SlotType::iterator i = SLOT_FIRST; i < SLOT_LAST; ++i){
			if(getInventoryItem(*i) == NULL){
				if(__queryAdd(i->value(), item, item->getItemCount(), 0) == RET_NOERROR){
					index = i->value();
					return this;
				}
			}
		}

		//try containers
		std::list<Container*> containerList;
		for(SlotType::iterator i = SLOT_FIRST; i < SLOT_LAST; ++i){
			if(getInventoryItem(*i) == tradeItem){
				continue;
			}

			if(Container* subContainer = getInventoryItem(*i)->getContainer()){
				if(subContainer->__queryAdd(-1, item, item->getItemCount(), 0) == RET_NOERROR){
					index = INDEX_WHEREEVER;
					*destItem = NULL;
					return subContainer;
				}
				containerList.push_back(subContainer);
			}
		}

		//check deeper in the containers
		for(std::list<Container*>::iterator it = containerList.begin(); it != containerList.end(); ++it){
			for(ContainerIterator iit = (*it)->begin(); iit != (*it)->end(); ++iit){
				if((*iit) == tradeItem){
					continue;
				}

				Container* subContainer = dynamic_cast<Container*>(*iit);
				if(subContainer && subContainer->__queryAdd(-1, item, item->getItemCount(), 0) == RET_NOERROR){
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

void Player::__addThing(Creature* actor, Thing* thing)
{
	__addThing(actor, 0, thing);
}

void Player::__addThing(Creature* actor, int32_t index, Thing* thing)
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
	sendAddInventoryItem((SlotType)index, item);

	//event methods
	onAddInventoryItem((SlotType)index, item);
}

void Player::__updateThing(Creature* actor, Thing* thing, uint16_t itemId, uint32_t count)
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

	const ItemType& oldType = Item::items[item->getID()];
	const ItemType& newType = Item::items[itemId];

	item->setID(itemId);
	item->setSubType(count);

	//send to client
	sendUpdateInventoryItem((SlotType)index, item, item);

	//event methods
	onUpdateInventoryItem((SlotType)index, item, oldType, item, newType);
}

void Player::__replaceThing(Creature* actor, uint32_t index, Thing* thing)
{
	if(index < 0 || index > 11){
#ifdef __DEBUG__MOVESYS__
		std::cout << "Failure: [Player::__replaceThing], " << "player: " << getName() << ", index: " << index << ",  index < 0 || index > 11" << std::endl;
		DEBUG_REPORT
#endif
		return /*RET_NOTPOSSIBLE*/;
	}

	Item* oldItem = getInventoryItem(SlotType(index));
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

	const ItemType& oldType = Item::items[oldItem->getID()];
	const ItemType& newType = Item::items[item->getID()];

	//send to client
	sendUpdateInventoryItem((SlotType)index, oldItem, item);

	//event methods
	onUpdateInventoryItem((SlotType)index, oldItem, oldType, item, newType);

	item->setParent(this);
	inventory[index] = item;
}

void Player::__removeThing(Creature* actor, Thing* thing, uint32_t count)
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
			sendRemoveInventoryItem((SlotType)index, item);

			//event methods
			onRemoveInventoryItem((SlotType)index, item);

			item->setParent(NULL);
			inventory[index] = NULL;
		}
		else{
			uint8_t newCount = (uint8_t)std::max((int32_t)0, (int32_t)(item->getItemCount() - count));
			item->setItemCount(newCount);

			const ItemType& it = Item::items[item->getID()];

			//send change to client
			sendUpdateInventoryItem((SlotType)index, item, item);

			//event methods
			onUpdateInventoryItem((SlotType)index, item, it, item, it);
		}
	}
	else{
		//send change to client
		sendRemoveInventoryItem((SlotType)index, item);

		//event methods
		onRemoveInventoryItem((SlotType)index, item);

		item->setParent(NULL);
		inventory[index] = NULL;
	}
}

int32_t Player::__getIndexOfThing(const Thing* thing) const
{
	for(SlotType::iterator i = SLOT_FIRST; i < SLOT_LAST; ++i){
		if(getInventoryItem(*i) == thing)
			return i->value();
	}

	return -1;
}

int32_t Player::__getFirstIndex() const
{
	return *SLOT_FIRST;
}

int32_t Player::__getLastIndex() const
{
	return *SLOT_LAST;
}

uint32_t Player::__getItemTypeCount(uint16_t itemId, int32_t subType /*= -1*/, bool itemCount /*= true*/) const
{
	uint32_t count = 0;

	for(SlotType::iterator i = SLOT_FIRST; i < SLOT_LAST; i++){
		Item* item = getInventoryItem(*i);

		if(item){
			if(item->getID() == itemId)
				count += Item::countByType(item, subType, itemCount);

			Container* container = item->getContainer();
			if(container)
				for(ContainerIterator iter = container->begin(), end = container->end(); iter != end; ++iter)
					if((*iter)->getID() == itemId)
						count += Item::countByType(*iter, subType, itemCount);
		}
	}

	return count;
}

std::map<uint32_t, uint32_t>& Player::__getAllItemTypeCount(
	std::map<uint32_t, uint32_t>& countMap, bool itemCount /*= true*/) const
{
	for(SlotType::iterator i = SLOT_FIRST; i < SLOT_LAST; i++){
		Item* item = getInventoryItem(*i);

		if(item){
			countMap[item->getID()] += Item::countByType(item, -1, itemCount);

			Container* container = item->getContainer();

			if(container)
				for(ContainerIterator iter = container->begin(), end = container->end(); iter != end; ++iter)
					countMap[(*iter)->getID()] += Item::countByType(*iter, -1, itemCount);
		}
	}

	return countMap;
}

Thing* Player::__getThing(uint32_t index) const
{
	SlotType slot(index);
	if(slot >= SLOT_FIRST && slot < SLOT_LAST)
		return getInventoryItem(slot);

	return NULL;
}

void Player::postAddNotification(Creature* actor, Thing* thing, const Cylinder* oldParent, int32_t index, cylinderlink_t link /*= LINK_OWNER*/)
{
	if(link == LINK_OWNER){
		g_game.onPlayerEquipItem(this, thing->getItem(), (SlotType)index, true);
	}

	bool requireListUpdate = true;
	if(link == LINK_OWNER || link == LINK_TOPPARENT){
		const Item* i = (oldParent? oldParent->getItem() : NULL);

		// Check if we owned the old container too, so we don't need to do anything,
		// as the list was updated in postRemoveNotification
		assert(i? i->getContainer() != NULL : true);

		if(i)
			requireListUpdate = i->getContainer()->getHoldingPlayer() != this;
		else
			requireListUpdate = oldParent != this;

		updateInventoryWeight();
		updateItemsLight();
		sendStats();
	}

	if(const Item* item = thing->getItem()){
		if(const Container* container = item->getContainer()){
			onSendContainer(container);
		}

		// REVSCRIPTSYS TODO
		// Shop..
		/*
		if(shopOwner){
			updateSaleShopList(item->getID());
		}
		*/
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

void Player::postRemoveNotification(Creature* actor, Thing* thing, const Cylinder* newParent, int32_t index, bool isCompleteRemoval, cylinderlink_t link /*= LINK_OWNER*/)
{
	if(link == LINK_OWNER){
		g_game.onPlayerEquipItem(this, thing->getItem(), (SlotType)index, false /*,isCompleteRemoval*/);
	}

	bool requireListUpdate = true;
	if(link == LINK_OWNER || link == LINK_TOPPARENT){
		const Item* i = (newParent? newParent->getItem() : NULL);

		// Check if we owned the old container too, so we don't need to do anything,
		// as the list was updated in postRemoveNotification
		assert(i? i->getContainer() != NULL : true);
		if(i)
			requireListUpdate = i->getContainer()->getHoldingPlayer() != this;
		else
			requireListUpdate = newParent != this;

		updateInventoryWeight();
		updateItemsLight();
		sendStats();
	}


	if(const Item* item = thing->getItem()){
		if(const Container* container = item->getContainer()){

			if(container->isRemoved() || !Position::areInRange<1,1,0>(getPosition(), container->getPosition())){
				//removed or out of range
				autoCloseContainers(container);
			}
			else if(container->getTopParent() == this){
				//the player equipped it
				onSendContainer(container);
			}
			else if(const Container* topContainer = dynamic_cast<const Container*>(container->getTopParent())){
				//moved the backpack
				if(const Depot* depot = dynamic_cast<const Depot*>(topContainer)){
					//moved into a depot
					bool isOwner = false;
					for(DepotMap::iterator it = depots.begin(); it != depots.end(); ++it){
						if(it->second == depot){
							//the player is the owner of the depot
							isOwner = true;
							onSendContainer(container);
						}
					}

					if(!isOwner){
						autoCloseContainers(container);
					}
				}
				else{
					onSendContainer(container);
				}
			}
			else{
				autoCloseContainers(container);
			}
		}

		// REVSCRIPTSYS TODO
		//
		/*
		if(shopOwner && requireListUpdate){
			updateSaleShopList(item->getID());
		}
		*/
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

	if(index >= uint32_t(*SLOT_FIRST) && index < uint32_t(*SLOT_LAST)){
		if(getInventoryItem(SlotType(index))){
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

bool Player::setFollowCreature(Creature* creature, bool fullPathSearch /*= false*/)
{
	if(!Creature::setFollowCreature(creature, fullPathSearch)){
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

	if(creature){
		g_dispatcher.addTask(createTask(
			boost::bind(&Game::checkCreatureAttack, &g_game, getID())));
	}

	return true;
}

void Player::getPathSearchParams(const Creature* creature, FindPathParams& fpp) const
{
	Creature::getPathSearchParams(creature, fpp);
	fpp.fullPathSearch = true;
}

uint32_t Player::getAttackSpeed() const
{
	return g_config.getNumber(ConfigManager::ATTACK_SPEED);
}

void Player::onAttacking(uint32_t interval)
{
	Creature::onAttacking(interval);
}

void Player::doAttacking(uint32_t interval)
{
	if(lastAttack == 0){
		// - 1 to compensate for timer resolution etc.
		lastAttack = OTSYS_TIME() - getAttackSpeed() - 1;
	}

	// Can't attack while pacified
	if(hasCondition(CONDITION_DISARMED))
	{
		return;
	}

	if((OTSYS_TIME() - lastAttack) >= getAttackSpeed() ){
		Item* tool = getWeapon();
		const Weapon* weapon = NULL;
		bool result = false;
		if(tool && (weapon = tool->getWeapon())){
			if(!weapon->interruptSwing()){
				result = weapon->useWeapon(this, tool, attackedCreature);
			}
			else if(!canDoAction()){
				uint32_t delay = getNextActionTime();
				SchedulerTask* task = createSchedulerTask(delay, boost::bind(&Game::checkCreatureAttack,
					&g_game, getID()));
				setNextActionTask(task);
			}
			else {
				// If the player is not exhausted OR if the player's weapon
				// does not have hasExhaust, use the weapon.
				if(!hasCondition(CONDITION_EXHAUST_DAMAGE))
				{
					result = weapon->useWeapon(this, tool, attackedCreature);
				}
			}
		}
		else{
			result = Weapon::useFist(this, attackedCreature);
		}

		if(result){
			lastAttack = OTSYS_TIME();
		}
	}
}

uint64_t Player::getGainedExperience(Creature* attacker) const
{
	if(g_game.getWorldType() == WORLD_TYPE_PVPE){
		Player* attackerPlayer = attacker->getPlayer();
		if(attackerPlayer && attackerPlayer != this && skillLoss){
				/*Formula
				a = attackers level * 0.9
				b = victims level
				c = victims experience

				y = (1 - (a / b)) * 0.05 * c
				*/

				uint32_t a = (int32_t)std::floor(attackerPlayer->getLevel() * 0.9);
				uint32_t b = getLevel();
				uint64_t c = getExperience();

				uint64_t result = std::max((uint64_t)0, (uint64_t)std::floor(getDamageRatio(attacker) * std::max((double)0, ((double)(1 - (((double)a / b))))) * 0.05 * c ));
				return result * g_config.getNumber(ConfigManager::RATE_EXPERIENCE_PVP);
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

void Player::setChaseMode(ChaseMode mode)
{
	ChaseMode prevChaseMode = chaseMode;
	chaseMode = mode;

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

void Player::setFightMode(FightMode mode)
{
	fightMode = mode;
}

void Player::onWalkAborted()
{
	setNextWalkActionTask(NULL);
	sendCancelWalk();
}

void Player::onWalkComplete()
{
	if(walkTask){
		walkTaskEvent = g_scheduler.addEvent(walkTask);
		walkTask = NULL;
	}
}

void Player::stopWalk()
{
	if(!listWalkDir.empty()){
		stopEventWalk();
	}
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
	for(SlotType::iterator i = SLOT_FIRST; i < SLOT_LAST; ++i){
		Item* item = getInventoryItem(*i);
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

void Player::onAddCondition(const Condition* condition, bool preAdd /*= true*/)
{
	Creature::onAddCondition(condition, preAdd);
	sendIcons();
}

void Player::onAddCombatCondition(const Condition* condition, bool preAdd /*= true*/)
{
	if(condition->getCombatType() == COMBAT_EARTHDAMAGE){
		sendTextMessage(MSG_STATUS_DEFAULT, "You are poisoned.");
	}
	else if(condition->getCombatType() == COMBAT_DROWNDAMAGE){
		sendTextMessage(MSG_STATUS_DEFAULT, "You are drowning.");
	}
	else if(condition->getMechanicType() == MECHANIC_PARALYZED){
		sendTextMessage(MSG_STATUS_DEFAULT, "You are paralyzed.");
	}
	else if(condition->getMechanicType() == MECHANIC_DRUNK){
		sendTextMessage(MSG_STATUS_DEFAULT, "You are drunk.");
	}
}

void Player::onEndCondition(const Condition* condition, bool preEnd /*= true*/)
{
	Creature::onEndCondition(condition, preEnd);

	if(condition->getName() == CONDITION_INFIGHT.toString()){
		onIdleStatus();
		pzLocked = false;

#ifdef __SKULLSYSTEM__
		if(getSkull() != SKULL_RED && getSkull() != SKULL_BLACK){
			clearAttacked();
			setSkull(SKULL_NONE);
			g_game.updateCreatureSkull(this);
		}
#endif
	}

	sendIcons();
}

void Player::onCombatRemoveCondition(const CombatSource& combatSource, Condition* condition)
{
	//Creature::onCombatRemoveCondition(combatSource, condition);

	if(g_game.getWorldType() == WORLD_TYPE_PVPE){
		if(	condition->getSourceId() != 0 &&
			combatSource.isSourceCreature() &&
			combatSource.getSourceCreature()->getPlayer() )
		{
			SlotType slot = SlotType::fromInteger(condition->getSourceId());
			Item* item = getInventoryItem(slot);
			if(item){
				//25% chance to destroy the item
				if(25 >= random_range(1, 100)){
					g_game.internalRemoveItem(NULL, item);
				}
			}
		}
		else{
			removeCondition(condition);
		}
	}
	else{
		removeCondition(condition);
	}
}

void Player::onAttackedCreature(Creature* target)
{
	Creature::onAttackedCreature(target);

	if(!hasFlag(PlayerFlag_NotGainInFight)){
		if(target != this){
			if(Player* targetPlayer = target->getPlayer()){
				if(checkPzBlockOnCombat(targetPlayer)){
					pzLocked = true;
				}

#ifdef __SKULLSYSTEM__
				if( !isPartner(targetPlayer) &&
					!Combat::isInPvpZone(this, targetPlayer) &&
					!targetPlayer->hasAttacked(this)){

					addAttacked(targetPlayer);

					if(targetPlayer->getSkull() == SKULL_NONE && getSkull() == SKULL_NONE){
						//add a white skull
						setSkull(SKULL_WHITE);
						g_game.updateCreatureSkull(this);
					}


					if(getSkull() == SKULL_NONE){
						//yellow skull
						targetPlayer->sendCreatureSkull(this);
					}
				}
#endif
			}
		}

		addInFightTicks(g_game.getInFightTicks());
	}
}

void Player::onSummonAttackedCreature(Creature* summon, Creature* target)
{
	Creature::onSummonAttackedCreature(summon, target);

	onAttackedCreature(target);
}

void Player::onAttacked()
{
	Creature::onAttacked();

	if(!hasFlag(PlayerFlag_NotGainInFight)){
		addInFightTicks(g_game.getInFightTicks());
	}
}

void Player::onIdleStatus()
{
	Creature::onIdleStatus();
	if(getParty()){
		getParty()->clearPlayerPoints(this);
	}
}

void Player::onPlacedCreature()
{
	Creature::onPlacedCreature();

	if(g_game.onPlayerLogin(this)){
		kickPlayer(); //The script won't let the player be online for now.
	}
}

void Player::sendReLoginWindow()
{
	if(client){
		client->sendReLoginWindow();
	}
}

void Player::onAttackedCreatureDrainHealth(Creature* target, int32_t points)
{
	Creature::onAttackedCreatureDrainHealth(target, points);

	if(target && getParty() && !Combat::isPlayerCombat(target)){
		Actor* tmpMonster = target->getActor();
		if(tmpMonster && tmpMonster->isHostile()){
			//We have fulfilled a requirement for shared experience
			getParty()->addPlayerDamageMonster(this, points);
		}
	}

	std::stringstream ss;
	ss << "You deal " << points << " damage to " << target->getNameDescription() << ".";
	sendTextMessage(MSG_EVENT_DEFAULT, ss.str());
}

void Player::onSummonAttackedCreatureDrainHealth(Creature* summon, Creature* target, int32_t points)
{
	std::stringstream ss;
	ss << "Your " << summon->getName() << " deals " << points << " damage to " << target->getNameDescription() << ".";
	sendTextMessage(MSG_EVENT_DEFAULT, ss.str());
}

void Player::onAttackedCreatureDrainMana(Creature* target, int32_t points)
{
	Creature::onAttackedCreatureDrainMana(target, points);

	std::stringstream ss;
	ss << "You drain " << points << " mana from " << target->getNameDescription() << ".";
	sendTextMessage(MSG_EVENT_DEFAULT, ss.str());
}

void Player::onSummonAttackedCreatureDrainMana(Creature* summon, Creature* target, int32_t points)
{
	std::stringstream ss;
	ss << "Your " << summon->getName() << " drains " << points << " mana from " << target->getNameDescription() << ".";
	sendTextMessage(MSG_EVENT_DEFAULT, ss.str());
}

void Player::onTargetCreatureGainHealth(Creature* target, int32_t points)
{
	Creature::onTargetCreatureGainHealth(target, points);
	if(target && getParty()){
		Player* tmpPlayer = NULL;
		if(target->getPlayer()){
			tmpPlayer = target->getPlayer();
		}
		else if(target->isPlayerSummon()){
			tmpPlayer = target->getPlayerMaster();
		}

		if(isPartner(tmpPlayer)){
			//We have fulfilled a requirement for shared experience
			getParty()->addPlayerHealedMember(this, points);
		}
	}
}

void Player::onKilledCreature(Creature* target)
{
	if(hasFlag(PlayerFlag_NotGenerateLoot)){
		target->setDropLoot(false);
	}

	if(Player* targetPlayer = target->getPlayer()){
		if(targetPlayer->getZone() == ZONE_PVP){
			targetPlayer->setDropLoot(false);
			targetPlayer->setLossSkill(false);
		}
		else if(!hasFlag(PlayerFlag_NotGainInFight)){
			if(!Combat::isInPvpZone(this, targetPlayer) && hasCondition(CONDITION_INFIGHT)){
				if(checkPzBlockOnCombat(targetPlayer)){
					addInFightTicks(g_config.getNumber(ConfigManager::UNJUST_SKULL_DURATION), true);
				}
			}
		}
	}

	if(target->getActor() && !target->isPlayerSummon()){
		Condition* condition = Condition::createCondition(CONDITION_HUNTING, g_config.getNumber(ConfigManager::HUNTING_KILL_DURATION));
		addCondition(condition);
	}

	Creature::onKilledCreature(target);
}

void Player::gainExperience(uint64_t& gainExp, bool fromMonster)
{
	if(!hasFlag(PlayerFlag_NotGainExperience)){
		if(gainExp > 0){
			//soul regeneration
			if((uint32_t)gainExp >= getLevel()){
				/*
				Condition* condition = Condition::createCondition(CONDITIONID_DEFAULT, CONDITION_SOUL, 4 * 60 * 1000, 0);
				//Soul regeneration rate is defined by the vocation
				uint32_t vocSoulTicks = vocation->getSoulGainTicks();
				condition->setParam(CONDITIONPARAM_SOULGAIN, 1);
				condition->setParam(CONDITIONPARAM_SOULTICKS, vocSoulTicks * 1000);
				addCondition(condition);
				*/
			}

			//check stamina, player rate and other values
			getGainExperience(gainExp, fromMonster);

			//add experience
			addExperience(gainExp);
		}
	}
}

void Player::getGainExperience(uint64_t& gainExp, bool fromMonster)
{
	if(fromMonster || g_config.getNumber(ConfigManager::RATES_FOR_PLAYER_KILLING)){
		gainExp = (uint64_t)std::floor(gainExp * getRateValue(LEVEL_EXPERIENCE));
	}

	if(fromMonster){
		if((isPremium() || !g_config.getNumber(ConfigManager::STAMINA_EXTRA_EXPERIENCE_ONLYPREM)) &&
			stamina > MAX_STAMINA - g_config.getNumber(ConfigManager::STAMINA_EXTRA_EXPERIENCE_DURATION)){
			gainExp += uint64_t(gainExp * g_config.getFloat(ConfigManager::STAMINA_EXTRA_EXPERIENCE_RATE));
		}

		if(!hasFlag(PlayerFlag_HasInfiniteStamina)){
			if(getStaminaMinutes() <= 0){
				gainExp = 0;
			}
			else if(getStaminaMinutes() <= 840){
				gainExp = gainExp / 2;
			}
		}
	}
}

void Player::onGainExperience(uint64_t gainExp, bool fromMonster)
{
	if(hasFlag(PlayerFlag_NotGainExperience)){
		gainExp = 0;
	}

	Party* party = getParty();
	if(party && party->isSharedExperienceActive() && party->isSharedExperienceEnabled()){
		party->shareExperience(gainExp, fromMonster);
		//We will get a share of the experience through the sharing mechanism
		gainExp = 0;
	}

	gainExperience(gainExp, fromMonster);
	Creature::onGainExperience(gainExp, fromMonster);
}

void Player::onGainSharedExperience(uint64_t gainExp, bool fromMonster)
{
	gainExperience(gainExp, fromMonster);
	Creature::onGainSharedExperience(gainExp, fromMonster);
}

bool Player::isImmune(CombatType type) const
{
	if(type != COMBAT_NONE && hasFlag(PlayerFlag_CannotBeAttacked)){
		return true;
	}

	return Creature::isImmune(type);
}

bool Player::isImmune(MechanicType type) const
{
	if(type != MECHANIC_NONE && hasFlag(PlayerFlag_CannotBeAttacked)){
		return true;
	}

	return Creature::isImmune(type);
}

bool Player::isCured(Condition* condition) const
{
	Item* item;
	for(SlotType::iterator slot = SLOT_FIRST; slot < SLOT_LAST; ++slot){
		if(!(item = getEquippedItem(*slot)))
			continue;

		const ItemType& it = Item::items[item->getID()];
		switch(condition->getCombatType().value()){
			case enums::COMBAT_ENERGYDAMAGE: return it.abilities.cure[CONDITION_ELECTRIFIED.value()];
			case enums::COMBAT_EARTHDAMAGE: return it.abilities.cure[CONDITION_POISONED.value()];
			case enums::COMBAT_FIREDAMAGE: return it.abilities.cure[CONDITION_BURNING.value()];
			case enums::COMBAT_DROWNDAMAGE: return it.abilities.cure[CONDITION_DROWNING.value()];
			case enums::COMBAT_ICEDAMAGE: return it.abilities.cure[CONDITION_FREEZING.value()];
			case enums::COMBAT_HOLYDAMAGE: return it.abilities.cure[CONDITION_DAZZLED.value()];
			case enums::COMBAT_DEATHDAMAGE: return it.abilities.cure[CONDITION_CURSED.value()];

			default:
				break;
		}

		if(condition->getMechanicType() == MECHANIC_DRUNK){
			return it.abilities.cure[CONDITION_DRUNK.value()];
		}
	}

	return false;
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

PartyShieldType Player::getPartyShield(const Player* player) const
{
	if(!player){
		return SHIELD_NONE;
	}

	Party* party = getParty();
	if(party){
		if(party->getLeader() == player){
			if(party->isSharedExperienceActive()){
				if(party->isSharedExperienceEnabled()){
					return SHIELD_YELLOW_SHAREDEXP;
				}

				if(party->canUseSharedExperience(player)){
					return SHIELD_YELLOW_NOSHAREDEXP;
				}

				return SHIELD_YELLOW_NOSHAREDEXP_BLINK;
			}
			return SHIELD_YELLOW;
		}

		if(party->isPlayerMember(player)){
			if(party->isSharedExperienceActive()){
				if(party->isSharedExperienceEnabled()){
					return SHIELD_BLUE_SHAREDEXP;
				}

				if(party->canUseSharedExperience(player)){
					return SHIELD_BLUE_NOSHAREDEXP;
				}

				return SHIELD_BLUE_NOSHAREDEXP_BLINK;
			}
			return SHIELD_BLUE;
		}

		if(isInviting(player)){
			return SHIELD_WHITEBLUE;
		}
	}

	if(player->isInviting(this)){
		return SHIELD_WHITEYELLOW;
	}

	return SHIELD_NONE;
}

bool Player::isInviting(const Player* player) const
{
	if(!player || !getParty() || getParty()->getLeader() != this){
		return false;
	}

	return getParty()->isPlayerInvited(player);
}

bool Player::isPartner(const Player* player) const
{
	if(!player || !getParty() || !player->getParty()){
		return false;
	}

	return (getParty() == player->getParty());
}

void Player::sendPlayerPartyIcons(Player* player)
{
	sendCreatureShield(player);

#ifdef __SKULLSYSTEM__
	sendCreatureSkull(player);
#endif
}

bool Player::addPartyInvitation(Party* party)
{
	if(!party){
		return false;
	}

	PartyList::iterator it = std::find(invitePartyList.begin(), invitePartyList.end(), party);
	if(it != invitePartyList.end()){
		return false;
	}

	invitePartyList.push_back(party);
	return true;
}

bool Player::removePartyInvitation(Party* party)
{
	if(!party){
		return false;
	}

	PartyList::iterator it = std::find(invitePartyList.begin(), invitePartyList.end(), party);
	if(it != invitePartyList.end()){
		invitePartyList.erase(it);
		return true;
	}

	return false;
}

void Player::clearPartyInvitations()
{
	if(!invitePartyList.empty()){
		PartyList list;
		for(PartyList::iterator it = invitePartyList.begin(); it != invitePartyList.end(); ++it){
			list.push_back(*it);
		}

		invitePartyList.clear();

		for(PartyList::iterator it = list.begin(); it != list.end(); ++it){
			(*it)->removeInvite(this);
		}
	}
}

#ifdef __SKULLSYSTEM__
SkullType Player::getSkull() const
{
	if(hasFlag(PlayerFlag_NotGainInFight)){
		return SKULL_NONE;
	}

	return skullType;
}

SkullType Player::getSkullClient(const Player* player) const
{
	if(!player){
		return SKULL_NONE;
	}

	if(getSkull() != SKULL_NONE && player->getSkull() != SKULL_RED && player->getSkull() != SKULL_BLACK){
		if(player->hasAttacked(this)){
			return SKULL_YELLOW;
		}
	}

	if(player->getSkull() == SKULL_NONE && isPartner(player) && g_game.getWorldType() != WORLD_TYPE_NOPVP){
		return SKULL_GREEN;
	}

	return player->getSkull();
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

	std::stringstream msg;
	msg << "Warning! The murder of " << attacked->getName() << " was not justified.";
	sendTextMessage(MSG_STATUS_WARNING, msg.str());

	SkullType oldSkull = getSkull();
	if(oldSkull == SKULL_RED || oldSkull == SKULL_BLACK){
		lastSkullTime = std::time(NULL);
	}

	//day
	int32_t unjustKills = IOPlayer::instance()->getPlayerUnjustKillCount(this, UNJUST_KILL_PERIOD_DAY);
	if(g_config.getNumber(ConfigManager::KILLS_PER_DAY_BLACK_SKULL) > 0 &&
		g_config.getNumber(ConfigManager::KILLS_PER_DAY_BLACK_SKULL) <= unjustKills){
		setSkull(SKULL_BLACK);
	}
	else if(getSkull() != SKULL_BLACK &&
			g_config.getNumber(ConfigManager::KILLS_PER_DAY_RED_SKULL) > 0 &&
			g_config.getNumber(ConfigManager::KILLS_PER_DAY_RED_SKULL) <= unjustKills){
		setSkull(SKULL_RED);
	}

	//week
	unjustKills = IOPlayer::instance()->getPlayerUnjustKillCount(this, UNJUST_KILL_PERIOD_WEEK);
	if(g_config.getNumber(ConfigManager::KILLS_PER_WEEK_BLACK_SKULL) > 0 &&
		g_config.getNumber(ConfigManager::KILLS_PER_WEEK_BLACK_SKULL) <= unjustKills){
		setSkull(SKULL_BLACK);
	}
	else if(getSkull() != SKULL_BLACK &&
			g_config.getNumber(ConfigManager::KILLS_PER_WEEK_RED_SKULL) > 0 &&
			g_config.getNumber(ConfigManager::KILLS_PER_WEEK_RED_SKULL) <= unjustKills){
		setSkull(SKULL_RED);
	}

	//month
	unjustKills = IOPlayer::instance()->getPlayerUnjustKillCount(this, UNJUST_KILL_PERIOD_MONTH);
	if(g_config.getNumber(ConfigManager::KILLS_PER_MONTH_BLACK_SKULL) > 0 &&
		g_config.getNumber(ConfigManager::KILLS_PER_MONTH_BLACK_SKULL) <= unjustKills){
		setSkull(SKULL_BLACK);
	}
	else if(getSkull() != SKULL_BLACK &&
			g_config.getNumber(ConfigManager::KILLS_PER_MONTH_RED_SKULL) > 0 &&
			g_config.getNumber(ConfigManager::KILLS_PER_MONTH_RED_SKULL) <= unjustKills){
		setSkull(SKULL_RED);
	}

	if(oldSkull != getSkull()){
		lastSkullTime = std::time(NULL);
		g_game.updateCreatureSkull(this);
		if(getSkull() == SKULL_BLACK){
			setAttackedCreature(NULL);
			destroySummons();
		}
	}
}

void Player::checkSkullTicks(int32_t ticks)
{
	if(!hasCondition(CONDITION_INFIGHT) && getSkull() != SKULL_NONE){
		if( (skullType == SKULL_RED && std::time(NULL) >= lastSkullTime + g_config.getNumber(ConfigManager::RED_SKULL_DURATION)) ||
			(skullType == SKULL_BLACK && std::time(NULL) >= lastSkullTime + g_config.getNumber(ConfigManager::BLACK_SKULL_DURATION)) ){
			lastSkullTime = 0;
			setSkull(SKULL_NONE);
			g_game.updateCreatureSkull(this);
		}
	}
}
#endif

void Player::setSex(PlayerSex player_sex)
{
	if(sex != player_sex){
		sex = player_sex;

		// REVSCRIPTSYS TODO how do we fix this change?
	}
}

bool Player::canLogout()
{
	if(isConnecting){
		return false;
	}

	if(isPzLocked()){
		return false;
	}

	if(getParentTile()->hasFlag(TILEPROP_NOLOGOUT)){
		return false;
	}

	return true;
}

bool Player::withdrawMoney(uint32_t amount)
{
	if(!g_config.getNumber(ConfigManager::USE_ACCBALANCE)){
		return false;
	}

	if(getBalance() < amount){
		return false;
	}

	bool ret = g_game.addMoney(NULL, this, amount);
	if(ret){
		setBalance(getBalance() - amount);
	}
	return ret;
}

bool Player::depositMoney(uint32_t amount)
{
	if(!g_config.getNumber(ConfigManager::USE_ACCBALANCE)){
		return false;
	}

	bool ret = g_game.removeMoney(NULL, this, amount);
	if(ret){
		setBalance(getBalance() + amount);
	}

	return ret;
}

bool Player::transferMoneyTo(const std::string& name, uint32_t amount)
{
	if(!g_config.getNumber(ConfigManager::USE_ACCBALANCE)){
		return false;
	}

	Player* target = g_game.getPlayerByNameEx(name);
	if(!target){
		return false;
	}

	bool result = false;
	if(getBalance() >= amount){
		setBalance(getBalance() - amount);
		target->setBalance(target->getBalance() + amount);
		result = true;
	}

	if(target->isOffline()){
		IOPlayer::instance()->savePlayer(target);
		delete target;
	}

	return result;
}

bool Player::isLoginAttackLocked(uint32_t attackerId) const
{
	if(OTSYS_TIME() <= lastLoginMs + g_config.getNumber(ConfigManager::LOGIN_ATTACK_DELAY))
		return !hasBeenAttacked(attackerId);
	return false;
}

void Player::addStamina(int64_t value)
{
	int64_t newstamina = stamina + value;

    //stamina may not be bigger than 42 hours, and not smaller than 0
	if(newstamina > MAX_STAMINA)
		newstamina = MAX_STAMINA;
	if(newstamina < 0)
		newstamina = 0;

	stamina = newstamina;
}

int32_t Player::getStaminaMinutes()
{
    if(hasFlag(PlayerFlag_HasInfiniteStamina)){
        return MAX_STAMINA_MINUTES;
    }

    return std::min(MAX_STAMINA_MINUTES, int32_t(stamina / 60000));
}

void Player::checkIdleTime(uint32_t ticks)
{
	if(g_config.getNumber(ConfigManager::IDLE_TIME) > 0){
		if(!getParentTile()->hasFlag(TILEPROP_NOLOGOUT) && !hasFlag(PlayerFlag_CanAlwaysLogin)){
			idleTime += ticks;
			if(idleTime >= g_config.getNumber(ConfigManager::IDLE_TIME)){
				kickPlayer();
			}
			else if(idleTime >= g_config.getNumber(ConfigManager::IDLE_TIME_WARNING) && !idleWarned){
				int32_t alreadyIdleTime = g_config.getNumber(ConfigManager::IDLE_TIME_WARNING) / 60000;
				int32_t remainingTime = (g_config.getNumber(ConfigManager::IDLE_TIME) - g_config.getNumber(ConfigManager::IDLE_TIME_WARNING)) / 60000;
				std::stringstream message;
				message << "You have been idle for " << alreadyIdleTime << " " << (alreadyIdleTime > 1 ? "minutes" : "minute") << ", you will be disconnected in " << remainingTime << " " << (remainingTime > 1 ? "minutes" : "minute") << " if you are still idle then.";
				sendTextMessage(MSG_STATUS_WARNING, message.str());
				idleWarned = true;
			}
		}
	}
}

void Player::broadcastLoot(Creature* creature, Container* corpse)
{
	std::ostringstream os;
	os << "Loot of " << creature->getNameDescription() << ": " << corpse->getContentDescription();

	//send message to party channel
	if(getParty())
		getParty()->broadcastPartyMessage(MSG_INFO_DESCR, os.str());
	else
		sendTextMessage(MSG_INFO_DESCR, os.str());
}

bool Player::checkPzBlockOnCombat(Player* targetPlayer)
{
	#ifdef __SKULLSYSTEM__
	if(targetPlayer->hasAttacked(this) && !g_config.getNumber(ConfigManager::DEFENSIVE_PZ_LOCK)){
		return false;
	}
	#endif

	if(isPartner(targetPlayer)){
		return false;
	}

	return true;
}

