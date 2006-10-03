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
#include "enums.h"
#include "vocation.h"

#include <vector>
#include <ctime>
#include <algorithm>

class House;
class Protocol76;
class Weapon;

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

enum playersex_t {
	PLAYERSEX_FEMALE = 0,
	PLAYERSEX_MALE = 1,
	PLAYERSEX_OLDMALE = 2
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

enum tradestate_t {
	TRADE_NONE,
	TRADE_INITIATED,
	TRADE_ACCEPT,
	TRADE_ACKNOWLEDGE,
	TRADE_TRANSFER
};

typedef std::pair<unsigned long, Container*> containervector_pair;
typedef std::vector<containervector_pair> ContainerVector;
typedef std::map<unsigned long, Depot*> DepotMap;
typedef std::map<unsigned long,long> StorageMap;
typedef std::set<unsigned long> VIPListSet;

//////////////////////////////////////////////////////////////////////
// Defines a player...

class Player : public Creature, public Cylinder
{
public:
	Player(const std::string& name, Protocol76* p);
	virtual ~Player();
	
	virtual Player* getPlayer() {return this;};
	virtual const Player* getPlayer() const {return this;};

	virtual const std::string& getName() const {return name;};
	virtual const std::string& getNameDescription() const {return name;};
	virtual std::string getDescription(int32_t lookDistance) const;

	void setGUID(unsigned long _guid) {guid = _guid;};
	unsigned long getGUID() const { return guid;};
	virtual unsigned long idRange(){ return 0x10000000;}
	static AutoList<Player> listPlayer;
	void removeList();
	void addList();
	void kickPlayer();
	
	unsigned long getGuildId() const {return guildId;};
	const std::string& getGuildName() const {return guildName;};
	const std::string& getGuildRank() const {return guildRank;};
	const std::string& getGuildNick() const {return guildNick;};
	
	void setGuildRank(const std::string& rank) {guildRank = rank;};
	void setGuildNick(const std::string& nick) {guildNick = nick;};
	
	bool isOnline() {return (client != NULL);};
	unsigned long getIP() const;

	void addContainer(uint32_t containerid, Container* container);
	void closeContainer(uint32_t containerid);
	int32_t getContainerID(const Container* container) const;
	Container* getContainer(uint32_t cid);
	
	void addStorageValue(const unsigned long key, const long value);
	bool getStorageValue(const unsigned long key, long &value) const;
	inline StorageMap::const_iterator getStorageIteratorBegin() const {return storageMap.begin();}
	inline StorageMap::const_iterator getStorageIteratorEnd() const {return storageMap.end();}
	
	int32_t getAccount() const {return accountNumber;}
	int32_t getLevel() const {return level;}
	int32_t getMagicLevel() const {return magLevel;}
	int32_t getAccessLevel() const {return accessLevel;}

	void setVocation(uint32_t vocId);
	uint32_t getVocationId() const;

	void setSkillsPercents();

	playersex_t getSex() {return sex;}	
	int getPlayerInfo(playerinfo_t playerinfo) const;	
	unsigned long getExperience() const {return experience;}

	time_t getLastLoginSaved() const { return lastLoginSaved; };
	const Position& getLoginPosition() { return loginPosition; };
	const Position& getTemplePosition() { return masterPos; };

	virtual bool isPushable() const;
	virtual int getThrowRange() const {return 1;};

	double getCapacity() const {
		if(getAccessLevel() == 0) {
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
	
	Item* getInventoryItem(slots_t slot) const;

	Depot* getDepot(uint32_t depotId, bool autoCreateDepot);
	bool addDepot(Depot* depot, uint32_t depotId);
	
	virtual bool canSee(const Position& pos) const;
	
	virtual RaceType_t getRace() const {return RACE_BLOOD;}

	//safe-trade functions
	void setTradeState(tradestate_t state) {tradeState = state;};
	tradestate_t getTradeState() {return tradeState;};
	Item* getTradeItem() {return tradeItem;};
	
	//V.I.P. functions
	void notifyLogIn(Player* player);
	void notifyLogOut(Player* player);
	bool removeVIP(unsigned long guid);
	bool addVIP(unsigned long guid, std::string& name, bool isOnline, bool interal = false);

	//follow functions
	virtual bool internalFollowCreature(const Creature* creature);
	void setChaseMode(uint8_t mode);

	//follow events
	virtual void onFollowCreature(const Creature* creature);

	//bool checkStopAutoWalk(bool pathInvalid = false);

	//walk events
	virtual void onWalkAborted();

	//combat functions
	virtual void setAttackedCreature(Creature* creature);
	bool isImmune(DamageType_t type) const;
	virtual bool isAttackable() const;
	bool isPzLocked() const { return pzLocked; }
	virtual BlockType_t blockHit(Creature* attacker, DamageType_t damageType, int32_t& damage,
		bool checkDefense = false, bool checkArmor = false);
	virtual void doAttacking();

	int getSkill(skills_t skilltype, skillsid_t skillinfo) const;
	uint32_t getSkillPoint() const {return skillPoint;}

	virtual void drainHealth(Creature* attacker, DamageType_t damageType, int32_t damage);
	virtual void drainMana(Creature* attacker, int32_t manaLoss);	
	void addManaSpent(uint32_t amount);
	void addSkillAdvance(skills_t skill, uint32_t count);

	virtual int getArmor() const;
	virtual int getDefense() const;

	virtual void die();
	virtual Item* getCorpse();
	virtual int32_t getGainedExperience(Creature* attacker) const;

	//combat event functions
	virtual void onAddCondition(ConditionType_t type);
	virtual void onEndCondition(ConditionType_t type);
	virtual void onAttackedCreature(Creature* target);
	virtual void onAttacked();
	virtual void onAttackedCreatureDrainHealth(Creature* target, int32_t points);
	virtual void onKilledCreature(Creature* target);
	virtual void onGainExperience(int32_t gainExperience);
	virtual void onAttackedCreatureBlockHit(Creature* target, BlockType_t blockType);
	//virtual void onTargetCreatureDisappear();

	virtual void getCreatureLight(LightInfo& light) const;

#ifdef __SKULLSYSTEM__
	Skulls_t getSkull() const;
	Skulls_t getSkullClient(const Player* player) const;
	bool hasAttacked(const Player* attacked) const;
	void addAttacked(const Player* attacked);
	void clearAttacked();
	void addUnjustifiedDead(const Player* attacked);
	void setSkull(Skulls_t new_skull);
	void sendCreatureSkull(const Creature* creature, Skulls_t skull) const;
	void checkRedSkullTicks(long ticks);
#endif
	
	//tile
	//send methods
	void sendAddTileItem(const Position& pos, const Item* item);
	void sendUpdateTileItem(const Position& pos, uint32_t stackpos, const Item* olditem, const Item* newitem);
	void sendRemoveTileItem(const Position& pos, uint32_t stackpos, const Item* item);
	void sendUpdateTile(const Position& pos);

	void sendCreatureAppear(const Creature* creature, bool isLogin);
	void sendCreatureDisappear(const Creature* creature, uint32_t stackpos, bool isLogout);
	void sendCreatureMove(const Creature* creature, const Position& newPos, const Position& oldPos, uint32_t oldStackPos, bool teleport);

	void sendCreatureTurn(const Creature* creature, uint32_t stackpos);
	void sendCreatureSay(const Creature* creature, SpeakClasses type, const std::string& text);
	void sendCreatureSquare(const Creature* creature, SquareColor_t color);
	void sendCreatureChangeOutfit(const Creature* creature);
	void sendCreatureLight(const Creature* creature);
	void sendWorldLight(LightInfo& lightInfo);

	//container
	void sendAddContainerItem(const Container* container, const Item* item);
	void sendUpdateContainerItem(const Container* container, uint8_t slot, const Item* oldItem, const Item* newItem);
	void sendRemoveContainerItem(const Container* container, uint8_t slot, const Item* item);

	//inventory
	void sendAddInventoryItem(slots_t slot, const Item* item);
	void sendUpdateInventoryItem(slots_t slot, const Item* oldItem, const Item* newItem);
	void sendRemoveInventoryItem(slots_t slot, const Item* item);

	//event methods
	virtual void onAddTileItem(const Position& pos, const Item* item);
	virtual void onUpdateTileItem(const Position& pos, uint32_t stackpos, const Item* oldItem, const Item* newItem);
	virtual void onRemoveTileItem(const Position& pos, uint32_t stackpos, const Item* item);
	virtual void onUpdateTile(const Position& pos);
	
	virtual void onCreatureAppear(const Creature* creature, bool isLogin);
	virtual void onCreatureDisappear(const Creature* creature);
	virtual void onCreatureDisappear(const Creature* creature, uint32_t stackpos, bool isLogout);
	virtual void onCreatureMove(const Creature* creature, const Position& newPos, const Position& oldPos,
		uint32_t oldStackPos, bool teleport);

	//virtual void onCreatureTurn(const Creature* creature, uint32_t stackpos);
	//virtual void onCreatureSay(const Creature* creature, SpeakClasses type, const std::string& text);
	//virtual void onCreatureChangeOutfit(const Creature* creature);

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

	void sendIcons() const;  
	void sendChangeSpeed(const Creature* creature) const;
	void sendToChannel(Creature* creature, SpeakClasses type, const std::string& text, unsigned short channelId) const;
	void sendCancelMessage(ReturnValue message) const;
	void sendCancel(const char* msg) const;
	void sendCancelWalk() const;
	void sendCancelTarget() const;
	void sendStats();
	void sendTextMessage(MessageClasses mclass, const std::string& message) const;
	void sendTextMessage(MessageClasses mclass, const std::string& message, const Position& pos,
		unsigned char type) const;
	void sendPing();
	void sendTextWindow(Item* item,const unsigned short maxlen, const bool canWrite) const;
	void sendDistanceShoot(const Position& from, const Position& to, unsigned char type) const;
	void sendMagicEffect(const Position& pos, unsigned char type) const;
	void sendAnimatedText(const Position& pos, unsigned char color, std::string text) const;
	void sendCreatureHealth(const Creature* creature) const;
	void sendTradeItemRequest(const Player* player, const Item* item, bool ack) const;
	void sendCloseTrade() const;
	void sendHouseWindow(House* _house, unsigned long _listid) const;
	void receivePing();
	void flushMsg();

	virtual void onThink(uint32_t interval);

	virtual void postAddNotification(Thing* thing, int32_t index, bool hasOwnership = true);
	virtual void postRemoveNotification(Thing* thing, int32_t index, bool isCompleteRemoval, bool hadOwnership = true);

	VIPListSet VIPList;

	//items
	ContainerVector containerVec;
	void preSave();

	//depots	
	DepotMap depots;
	uint32_t maxDepotLimit;

protected:
	bool gainManaTick();
	bool gainHealthTick();

	void checkTradeState(const Item* item);
	bool hasCapacity(const Item* item, uint32_t count) const;

	//combat help functions
	bool getCombatItem(Item** tool, const Weapon** weapon);

	std::string getSkillName(int skillid);
	void addExperience(unsigned long exp);

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
	virtual uint32_t __getItemTypeCount(uint16_t itemId) const;
	virtual Thing* __getThing(uint32_t index) const;

	virtual void __internalAddThing(Thing* thing);
	virtual void __internalAddThing(uint32_t index, Thing* thing);

protected:
	Protocol76* client;

	int32_t accessLevel;
	uint32_t experience;
	uint32_t immunities;
	int32_t manaSpent;
	Vocation_t vocation_id;
	Vocation* vocation;
	playersex_t sex;
	int food;
	
	double inventoryWeight;
	double capacity;
	
	bool SendBuffer;
	long internal_ping;
	long npings;

	bool pzLocked;
	uint32_t blockCount;
	uint32_t skillPoint;
	
	chaseMode_t chaseMode;

	//account variables
	int accountNumber;
	std::string password;
	time_t lastlogin;
	time_t lastLoginSaved;
	Position loginPosition;
	unsigned long lastip;
	
	//inventory variables
	Item* inventory[11];
	//Abilities ExtraAbilities;
	
	//player advances variables
	unsigned int skills[SKILL_LAST + 1][3];
	
	//reminder: 0 = None, 1 = Sorcerer, 2 = Druid, 3 = Paladin, 4 = Knight
	static const int CapGain[5];
	static const int ManaGain[5];
	static const int HPGain[5];
	static const int gainManaVector[5][2];
	static const int gainHealthVector[5][2];
	unsigned short manaTick;
	unsigned short healthTick;
	
	unsigned char level_percent;
	unsigned char maglevel_percent;

	//trade variables
	Player* tradePartner;
	tradestate_t tradeState;
	Item* tradeItem;

	struct SentStats{
		int health;
		int healthMax;
		unsigned long experience;
		int level;
		double freeCapacity;
		int mana;
		int manaMax;
		int manaSpent;
		int magLevel;
	};
	
	SentStats lastSentStats;

	std::string name;	
	std::string nameDescription;
	unsigned long guid;
	
	//guild variables
	unsigned long guildId;
	std::string guildName;
	std::string guildRank;
	std::string guildNick;
	
	StorageMap storageMap;
	LightInfo itemsLight;
	
#ifdef __SKULLSYSTEM__
	int64_t redSkullTicks;
	Skulls_t skull;
	typedef std::set<long> AttackedSet;
	AttackedSet attackedSet;
#endif
	
	//for skill advances
	//unsigned int getReqSkillTries(int skill, int level, Vocation_t voc);
	
	//for magic level advances
	//unsigned int getReqMana(int magLevel, Vocation_t voc); 
	
	void updateItemsLight(bool internal = false);

	virtual int32_t getLostExperience() const { return (int32_t)std::floor(((double)experience * 0.1));}	
	virtual void dropLoot(Container* corpse);
	virtual uint32_t getImmunities() const { return immunities; }
	virtual uint16_t getLookCorpse() const;

	friend OTSYS_THREAD_RETURN ConnectionHandler(void *dat);
	
	friend class Game;
	friend class LuaScriptInterface;
	friend class Commands;
	friend class Map;
	friend class IOPlayerXML;
	friend class IOPlayerSQL;
};

#endif
