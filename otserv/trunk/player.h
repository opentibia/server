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

#ifndef __OTSERV_PLAYER_H__
#define __OTSERV_PLAYER_H__

#include "definitions.h"
#include "creature.h"
#include "container.h"
#include "depot.h"
#include "cylinder.h"
#include "outfit.h"
#include "enums.h"
#include "vocation.h"
#include "protocol80.h"

#include <vector>
#include <ctime>
#include <algorithm>

class House;
class Weapon;
class Protocol80;

enum skillsid_t {
	SKILL_LEVEL=0,
	SKILL_TRIES=1,
	SKILL_PERCENT=2
};

enum playerinfo_t {
	PLAYERINFO_LEVEL,
	PLAYERINFO_LEVELPERCENT,
	PLAYERINFO_HEALTH,
	PLAYERINFO_MAXHEALTH,
	PLAYERINFO_MANA,
	PLAYERINFO_MAXMANA,
	PLAYERINFO_MANAPERCENT,
	PLAYERINFO_MAGICLEVEL,
	PLAYERINFO_MAGICLEVELPERCENT,
	PLAYERINFO_SOUL,
};

enum freeslot_t {
	SLOT_TYPE_NONE,
	SLOT_TYPE_INVENTORY,
	SLOT_TYPE_CONTAINER
};

enum chaseMode_t {
	CHASEMODE_STANDSTILL,
	CHASEMODE_FOLLOW,
};

enum fightMode_t {
	FIGHTMODE_ATTACK,
	FIGHTMODE_BALANCED,
	FIGHTMODE_DEFENSE
};

enum tradestate_t {
	TRADE_NONE,
	TRADE_INITIATED,
	TRADE_ACCEPT,
	TRADE_ACKNOWLEDGE,
	TRADE_TRANSFER
};

typedef std::pair<uint32_t, Container*> containervector_pair;
typedef std::vector<containervector_pair> ContainerVector;
typedef std::map<uint32_t, Depot*> DepotMap;
typedef std::map<uint32_t, int32_t> StorageMap;
typedef std::set<uint32_t> VIPListSet;
typedef std::map<uint32_t, uint32_t> MuteCountMap;
typedef std::list<std::string> LearnedInstantSpellList;

//////////////////////////////////////////////////////////////////////
// Defines a player...

class Player : public Creature, public Cylinder
{
public:
	Player(const std::string& name, Protocol80* p);
	virtual ~Player();

	virtual Player* getPlayer() {return this;}
	virtual const Player* getPlayer() const {return this;}

	static MuteCountMap muteCountMap;
	static int32_t maxMessageBuffer;

	virtual const std::string& getName() const {return name;}
	virtual const std::string& getNameDescription() const {return name;}
	virtual std::string getDescription(int32_t lookDistance) const;

	void setGUID(uint32_t _guid) {guid = _guid;};
	uint32_t getGUID() const { return guid;};
	virtual uint32_t idRange(){ return 0x10000000;}
	static AutoList<Player> listPlayer;
	void removeList();
	void addList();
	void kickPlayer() {client->logout();}
	
	uint32_t getGuildId() const {return guildId;}
	const std::string& getGuildName() const {return guildName;}
	const std::string& getGuildRank() const {return guildRank;}
	const std::string& getGuildNick() const {return guildNick;}
	
	void setGuildRank(const std::string& rank) {guildRank = rank;}
	void setGuildNick(const std::string& nick) {guildNick = nick;}
	
	void setFlags(uint64_t flags){ groupFlags = flags;}
	bool hasFlag(PlayerFlags value) const { return (0 != (groupFlags & ((uint64_t)1 << value)));}
	
	bool isOnline() {return (client != NULL);}
	uint32_t getIP() const;

	void addContainer(uint32_t containerid, Container* container);
	void closeContainer(uint32_t containerid);
	int32_t getContainerID(const Container* container) const;
	Container* getContainer(uint32_t cid);
	
	void addStorageValue(const uint32_t key, const int32_t value);
	bool getStorageValue(const uint32_t key, int32_t& value) const;
	void genReservedStorageRange();
	
	inline StorageMap::const_iterator getStorageIteratorBegin() const {return storageMap.begin();}
	inline StorageMap::const_iterator getStorageIteratorEnd() const {return storageMap.end();}
	
	int32_t getAccount() const {return accountNumber;}
	int32_t getLevel() const {return level;}
	int32_t getMagicLevel() const {return magLevel;}
	int32_t getAccessLevel() const {return accessLevel;}

	void setVocation(uint32_t vocId);
	uint32_t getVocationId() const;

	void setSkillsPercents();

	playersex_t getSex() const {return sex;}	
	void setSex(playersex_t);
	int getPlayerInfo(playerinfo_t playerinfo) const;	
	uint32_t getExperience() const {return experience;}

	time_t getLastLoginSaved() const {return lastLoginSaved;}
	const Position& getLoginPosition() {return loginPosition;}
	const Position& getTemplePosition() {return masterPos;}
	uint32_t getTown() const {return town;}
	void setTown(uint32_t _town) {town = _town;}

	virtual bool isPushable() const;
	virtual int getThrowRange() const {return 1;};
	bool isMuted(uint32_t& muteTime);
	void addMessageBuffer();
	void removeMessageBuffer();

	double getCapacity() const {
		if(!hasFlag(PlayerFlag_HasInfiniteCapacity)){
			return capacity;
		}
		else
			return 0.00;
	}
	
	double getFreeCapacity() const {
		if(accessLevel == 0) {
			return std::max(0.00, capacity - inventoryWeight);
		}
		else
			return 0.00;
	}
	
	virtual int32_t getMaxHealth() const {return healthMax + varStats[STAT_MAXHITPOINTS];}
	virtual int32_t getMaxMana() const {return manaMax + varStats[STAT_MAXMANAPOINTS];}

	Item* getInventoryItem(slots_t slot) const;

	bool isItemAbilityEnabled(slots_t slot) const {return inventoryAbilities[slot];}
	void setItemAbility(slots_t slot, bool enabled) {inventoryAbilities[slot] = enabled;}

	int32_t getVarSkill(skills_t skill) const {return varSkills[skill];}
	void setVarSkill(skills_t skill, int32_t modifier) {varSkills[skill] += modifier;}
	
	int32_t getVarStats(stats_t stat) const {return varStats[stat];}
	void setVarStats(stats_t stat, int32_t modifier);
	void setConditionSuppressions(uint32_t conditions, bool remove);

	Depot* getDepot(uint32_t depotId, bool autoCreateDepot);
	bool addDepot(Depot* depot, uint32_t depotId);
	
	virtual bool canSee(const Position& pos) const;
	virtual bool canSeeCreature(const Creature* creature) const;
	
	virtual RaceType_t getRace() const {return RACE_BLOOD;}

	//safe-trade functions
	void setTradeState(tradestate_t state) {tradeState = state;};
	tradestate_t getTradeState() {return tradeState;};
	Item* getTradeItem() {return tradeItem;};
	
	//V.I.P. functions
	void notifyLogIn(Player* player);
	void notifyLogOut(Player* player);
	bool removeVIP(uint32_t guid);
	bool addVIP(uint32_t guid, std::string& name, bool isOnline, bool interal = false);

	//follow functions
	virtual bool setFollowCreature(Creature* creature);

	//follow events
	virtual void onFollowCreature(const Creature* creature);

	//walk events
	virtual void onWalkAborted();

	void setChaseMode(chaseMode_t mode);
	void setFightMode(fightMode_t mode);

	//combat functions
	virtual bool setAttackedCreature(Creature* creature);
	bool isImmune(CombatType_t type) const;
	bool isImmune(ConditionType_t type) const;
	bool hasShield() const;
	virtual bool isAttackable() const;
	
	virtual void changeHealth(int32_t healthChange);
	virtual void changeMana(int32_t manaChange);
	void changeSoul(int32_t soulChange);

	bool isPzLocked() const { return pzLocked; }
	virtual BlockType_t blockHit(Creature* attacker, CombatType_t combatType, int32_t& damage,
		bool checkDefense = false, bool checkArmor = false);
	virtual void doAttacking(uint32_t interval);
	int32_t getShootRange() const {return shootRange;}

	int getSkill(skills_t skilltype, skillsid_t skillinfo) const;
	bool getAddAttackSkill() const {return addAttackSkillPoint;}
	BlockType_t getLastAttackBlockType() const {return lastAttackBlockType;}

	Item* getWeapon();
	Item* getWeapon() const {return getWeapon();}
	virtual WeaponType_t getWeaponType();
	int32_t getWeaponSkill(const Item* item) const;
	Item* getShield() const;

	virtual void drainHealth(Creature* attacker, CombatType_t combatType, int32_t damage);
	virtual void drainMana(Creature* attacker, int32_t manaLoss);	
	void addManaSpent(uint32_t amount);
	void addSkillAdvance(skills_t skill, uint32_t count);

	virtual int32_t getArmor() const;
	virtual int32_t getDefense() const;

	void addExhaustionTicks(uint32_t ticks);
	void addInFightTicks();
	void addDefaultRegeneration(uint32_t addTicks);

	virtual void die();
	virtual Item* getCorpse();
	virtual int32_t getGainedExperience(Creature* attacker) const;

	//combat event functions
	virtual void onAddCondition(ConditionType_t type);
	virtual void onAddCombatCondition(ConditionType_t type);
	virtual void onEndCondition(ConditionType_t type);
	virtual void onCombatRemoveCondition(const Creature* attacker, Condition* condition);
	virtual void onAttackedCreature(Creature* target);
	virtual void onAttacked();
	virtual void onAttackedCreatureDrainHealth(Creature* target, int32_t points);
	virtual void onKilledCreature(Creature* target);
	virtual void onGainExperience(int32_t gainExperience);
	virtual void onAttackedCreatureBlockHit(Creature* target, BlockType_t blockType);
	virtual void onBlockHit(BlockType_t blockType);
	virtual void onAttackedCreatureEnterProtectionZone(const Creature* creature);

	virtual void getCreatureLight(LightInfo& light) const;

#ifdef __SKULLSYSTEM__
	Skulls_t getSkull() const;
	Skulls_t getSkullClient(const Player* player) const;
	bool hasAttacked(const Player* attacked) const;
	void addAttacked(const Player* attacked);
	void clearAttacked();
	void addUnjustifiedDead(const Player* attacked);
	void setSkull(Skulls_t newSkull) {skull = newSkull;}
	void sendCreatureSkull(const Creature* creature, Skulls_t skull) const
		{client->sendCreatureSkull(creature, skull);}
	void checkRedSkullTicks(int32_t ticks);
#endif	
	const OutfitListType& getPlayerOutfits();
	bool canWear(uint32_t _looktype, uint32_t _addons);
	void addOutfit(uint32_t _looktype, uint32_t _addons);
	bool remOutfit(uint32_t _looktype, uint32_t _addons);
	
	//tile
	//send methods
	void sendAddTileItem(const Position& pos, const Item* item)
		{client->sendAddTileItem(pos, item);}
	void sendUpdateTileItem(const Position& pos, uint32_t stackpos, const Item* olditem, const Item* newitem)
		{client->sendUpdateTileItem(pos, stackpos, newitem);}
	void sendRemoveTileItem(const Position& pos, uint32_t stackpos, const Item* item)
		{client->sendRemoveTileItem(pos, stackpos);}
	void sendUpdateTile(const Position& pos)
		{client->UpdateTile(pos);}

	void sendCreatureAppear(const Creature* creature, bool isLogin)
		{client->sendAddCreature(creature, isLogin);}
	void sendCreatureDisappear(const Creature* creature, uint32_t stackpos, bool isLogout)
		{client->sendRemoveCreature(creature, creature->getPosition(), stackpos, isLogout);}
	void sendCreatureMove(const Creature* creature, const Position& newPos, const Position& oldPos, uint32_t oldStackPos, bool teleport)
		{client->sendMoveCreature(creature, newPos, oldPos, oldStackPos, teleport);}

	void sendCreatureTurn(const Creature* creature, uint32_t stackpos)
		{client->sendCreatureTurn(creature, stackpos);}
	void sendCreatureSay(const Creature* creature, SpeakClasses type, const std::string& text)
		{client->sendCreatureSay(creature, type, text);}
	void sendCreatureSquare(const Creature* creature, SquareColor_t color)
		{client->sendCreatureSquare(creature, color);}
	void sendCreatureChangeOutfit(const Creature* creature, const Outfit_t& outfit)
		{client->sendCreatureOutfit(creature, outfit);}
	void sendCreatureChangeVisible(const Creature* creature, bool visible)
		{
			if(visible)
				client->sendCreatureOutfit(creature, creature->getCurrentOutfit());
			else
				client->sendCreatureInvisible(creature);
		}
	void sendCreatureLight(const Creature* creature)
		{client->sendCreatureLight(creature);}

	//container
	void sendAddContainerItem(const Container* container, const Item* item);
	void sendUpdateContainerItem(const Container* container, uint8_t slot, const Item* oldItem, const Item* newItem);
	void sendRemoveContainerItem(const Container* container, uint8_t slot, const Item* item);

	//inventory
	void sendAddInventoryItem(slots_t slot, const Item* item)
		{client->sendAddInventoryItem(slot, item);}
	void sendUpdateInventoryItem(slots_t slot, const Item* oldItem, const Item* newItem)
		{client->sendUpdateInventoryItem(slot, newItem);}
	void sendRemoveInventoryItem(slots_t slot, const Item* item)
		{client->sendRemoveInventoryItem(slot);}

	//event methods
	virtual void onAddTileItem(const Position& pos, const Item* item);
	virtual void onUpdateTileItem(const Position& pos, uint32_t stackpos, const Item* oldItem, const Item* newItem);
	virtual void onRemoveTileItem(const Position& pos, uint32_t stackpos, const Item* item);
	virtual void onUpdateTile(const Position& pos);
	
	virtual void onCreatureAppear(const Creature* creature, bool isLogin);
	virtual void onCreatureDisappear(const Creature* creature, uint32_t stackpos, bool isLogout);
	virtual void onCreatureMove(const Creature* creature, const Position& newPos, const Position& oldPos,
		uint32_t oldStackPos, bool teleport);

	virtual void onAttackedCreatureDissapear(bool isLogout);
	virtual void onFollowCreatureDissapear(bool isLogout);

	//virtual void onCreatureSay(const Creature* creature, SpeakClasses type, const std::string& text);
	//virtual void onCreatureTurn(const Creature* creature, uint32_t stackpos);
	//virtual void onCreatureChangeOutfit(const Creature* creature, const Outfit_t& outfit);

	//container
	void onAddContainerItem(const Container* container, const Item* item);
	void onUpdateContainerItem(const Container* container, uint8_t slot, const Item* oldItem, const Item* newItem);
	void onRemoveContainerItem(const Container* container, uint8_t slot, const Item* item);
	
	void onCloseContainer(const Container* container);
	void onSendContainer(const Container* container);
	void autoCloseContainers(const Container* container);

	//inventory
	void onAddInventoryItem(slots_t slot, Item* item);
	void onUpdateInventoryItem(slots_t slot, Item* oldItem, Item* newItem);
	void onRemoveInventoryItem(slots_t slot, Item* item);

	//other send messages
	void sendAnimatedText(const Position& pos, unsigned char color, std::string text) const
		{client->sendAnimatedText(pos,color,text);}
	void sendCancel(const char* msg) const
		{client->sendCancel(msg);}
	void sendCancelMessage(ReturnValue message) const;
	void sendCancelTarget() const
		{client->sendCancelTarget();}
	void sendCancelWalk() const
		{client->sendCancelWalk();}
	void sendChangeSpeed(const Creature* creature, uint32_t newSpeed) const 
		{client->sendChangeSpeed(creature, newSpeed);}
	void sendCreatureHealth(const Creature* creature) const
		{client->sendCreatureHealth(creature);}
	void sendDistanceShoot(const Position& from, const Position& to, unsigned char type) const
		{client->sendDistanceShoot(from, to,type);}
	void sendHouseWindow(House* _house, uint32_t _listid) const;
	void sendClosePrivate(uint16_t channelId) const
		{client->sendClosePrivate(channelId);}
	void sendIcons() const; 
	void sendMagicEffect(const Position& pos, unsigned char type) const
		{client->sendMagicEffect(pos,type);}
	void sendPing(uint32_t interval);
	void sendStats();
	void sendSkills() const
		{client->sendSkills();}
	void sendTextMessage(MessageClasses mclass, const std::string& message) const
		{client->sendTextMessage(mclass, message);}
	void sendTextWindow(Item* item,const uint16_t maxlen, const bool canWrite) const
		{client->sendTextWindow(item,maxlen,canWrite);}
	void sendTextWindow(uint32_t itemid, const std::string& text) const
		{client->sendTextWindow(itemid,text);}
	void sendToChannel(Creature* creature, SpeakClasses type, const std::string& text, uint16_t channelId) const
		{client->sendToChannel(creature, type, text, channelId);}
	void sendTradeItemRequest(const Player* player, const Item* item, bool ack) const
		{client->sendTradeItemRequest(player, item, ack);}
	void sendTradeClose() const 
		{client->sendCloseTrade();}
	void sendWorldLight(LightInfo& lightInfo)
		{client->sendWorldLight(lightInfo);}
	
	void receivePing() {if(npings > 0) npings--;}
	void flushMsg() {client->flushOutputBuffer();}

	virtual void onThink(uint32_t interval);
	virtual void onAttacking(uint32_t interval);

	virtual void postAddNotification(Thing* thing, int32_t index, cylinderlink_t link = LINK_OWNER);
	virtual void postRemoveNotification(Thing* thing, int32_t index, bool isCompleteRemoval, cylinderlink_t link = LINK_OWNER);

	void setLastAction(uint64_t time) {lastAction = time;}
	int64_t getLastAction() const {return lastAction;}

	void learnInstantSpell(const std::string& name);
	bool hasLearnedInstantSpell(const std::string& name) const;

	VIPListSet VIPList;
	uint32_t maxVipLimit;

	//items
	ContainerVector containerVec;
	void preSave();

	//depots	
	DepotMap depots;
	uint32_t maxDepotLimit;

protected:
	void checkTradeState(const Item* item);
	bool hasCapacity(const Item* item, uint32_t count) const;

	std::string getSkillName(int skillid);
	void addExperience(uint32_t exp);

	bool NeedUpdateStats();
	void updateInventoryWeigth();

	//cylinder implementations
	virtual ReturnValue __queryAdd(int32_t index, const Thing* thing, uint32_t count,
		uint32_t flags) const;
	virtual ReturnValue __queryMaxCount(int32_t index, const Thing* thing, uint32_t count, uint32_t& maxQueryCount,
		uint32_t flags) const;
	virtual ReturnValue __queryRemove(const Thing* thing, uint32_t count) const;
	virtual Cylinder* __queryDestination(int32_t& index, const Thing* thing, Item** destItem,
		uint32_t& flags);

	virtual void __addThing(Thing* thing);
	virtual void __addThing(int32_t index, Thing* thing);

	virtual void __updateThing(Thing* thing, uint32_t count);
	virtual void __replaceThing(uint32_t index, Thing* thing);

	virtual void __removeThing(Thing* thing, uint32_t count);

	virtual int32_t __getIndexOfThing(const Thing* thing) const;
	virtual int32_t __getFirstIndex() const;
	virtual int32_t __getLastIndex() const;
	virtual uint32_t __getItemTypeCount(uint16_t itemId, int32_t subType = -1, bool itemCount = true) const;
	virtual Thing* __getThing(uint32_t index) const;

	virtual void __internalAddThing(Thing* thing);
	virtual void __internalAddThing(uint32_t index, Thing* thing);

protected:
	Protocol80* client;

	int32_t level;
	int32_t magLevel;
	int32_t accessLevel;
	uint32_t experience;
	uint32_t damageImmunities;
	uint32_t conditionImmunities;
	uint32_t conditionSuppressions;
	uint32_t condition;
	int32_t manaSpent;
	Vocation_t vocation_id;
	Vocation* vocation;
	playersex_t sex;
	int32_t soul, soulMax;
	uint64_t groupFlags;
	uint32_t MessageBufferTicks;
	int32_t MessageBufferCount;

	double inventoryWeight;
	double capacity;
	
	bool SendBuffer;
	uint32_t internal_ping;
	uint32_t npings;
	int64_t lastAction;

	bool pzLocked;
	int32_t bloodHitCount;
	int32_t shieldBlockCount;
	BlockType_t lastAttackBlockType;
	bool addAttackSkillPoint;
	uint32_t attackTicks;
	int32_t shootRange;
	
	chaseMode_t chaseMode;
	fightMode_t fightMode;

	//account variables
	int accountNumber;
	std::string password;
	time_t lastlogin;
	time_t lastLoginSaved;
	Position loginPosition;
	uint32_t lastip;
	
	//inventory variables
	Item* inventory[11];
	bool inventoryAbilities[11];
	
	//player advances variables
	uint32_t skills[SKILL_LAST + 1][3];

	//extra skill modifiers
	int32_t varSkills[SKILL_LAST + 1];

	//extra stat modifiers
	int32_t varStats[STAT_LAST + 1];

	LearnedInstantSpellList learnedInstantSpellList;
	
	ConditionList storedConditionList;
	
	unsigned char level_percent;
	unsigned char maglevel_percent;

	//trade variables
	Player* tradePartner;
	tradestate_t tradeState;
	Item* tradeItem;

	struct SentStats{
		int32_t health;
		int32_t healthMax;
		uint32_t experience;
		int32_t level;
		double freeCapacity;
		int32_t mana;
		int32_t manaMax;
		int32_t manaSpent;
		int32_t magLevel;
	};
	
	SentStats lastSentStats;

	std::string name;	
	std::string nameDescription;
	uint32_t guid;
	
	uint32_t town;
	
	//guild variables
	uint32_t guildId;
	std::string guildName;
	std::string guildRank;
	std::string guildNick;
	uint32_t guildLevel;
	
	StorageMap storageMap;
	LightInfo itemsLight;
	
	OutfitList m_playerOutfits;
	
#ifdef __SKULLSYSTEM__
	int64_t redSkullTicks;
	Skulls_t skull;
	typedef std::set<uint32_t> AttackedSet;
	AttackedSet attackedSet;
#endif
	
	void updateItemsLight(bool internal = false);
	void updateBaseSpeed(){ 
		if(!hasFlag(PlayerFlag_SetMaxSpeed)){
			baseSpeed = 220 + (2* (level - 1));
		}
		else{
			baseSpeed = 900;
		};
	}

	virtual int32_t getLostExperience() const { return (int32_t)std::floor(((double)experience * 0.1));}	
	virtual void dropLoot(Container* corpse);
	virtual uint32_t getDamageImmunities() const { return damageImmunities; }
	virtual uint32_t getConditionImmunities() const { return conditionImmunities; }
	virtual uint32_t getConditionSuppressions() const { return conditionSuppressions; }
	virtual uint16_t getLookCorpse() const;
	virtual uint32_t getAttackSpeed();

	friend OTSYS_THREAD_RETURN ConnectionHandler(void *dat);
	
	friend class Game;
	friend class LuaScriptInterface;
	friend class Commands;
	friend class Map;
	friend class Actions;
	friend class IOPlayerXML;
	friend class IOPlayerSQL;
};

#endif
