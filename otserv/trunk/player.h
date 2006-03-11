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

#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "definitions.h"
#include "creature.h"
#include "container.h"
#include "depot.h"
#include "cylinder.h"

#include <vector>
#include <ctime>
#include <algorithm>
#include "templates.h"

class House;
class Protocol;

enum skills_t {
	SKILL_FIST=0,
	SKILL_CLUB=1,
	SKILL_SWORD=2,
	SKILL_AXE=3,
	SKILL_DIST=4,
	SKILL_SHIELD=5,
	SKILL_FISH=6
};

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

//0 = None, 1 = Sorcerer, 2 = Druid, 3 = Paladin, 4 = Knight
enum playervoc_t {
	VOCATION_NONE = 0,
	VOCATION_SORCERER = 1,
	VOCATION_DRUID = 2,
	VOCATION_PALADIN = 3,
	VOCATION_KNIGHT = 4
};

enum freeslot_t {
	SLOT_TYPE_NONE,
	SLOT_TYPE_INVENTORY,
	SLOT_TYPE_CONTAINER
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
	Player(const std::string& name, Protocol* p);
	virtual ~Player();
	
	virtual Player* getPlayer() {return this;};
	virtual const Player* getPlayer() const {return this;};

	const std::string& getName() const {return name;};
	virtual bool isPushable() const;
	virtual int getThrowRange() const {return 1;};

	void setGUID(unsigned long _guid) {guid = _guid;};
	unsigned long getGUID() const { return guid;};
	virtual unsigned long idRange(){ return 0x10000000;}
	static AutoList<Player> listPlayer;
	void removeList();
	void addList();
	void kickPlayer();
	
	const std::string& getGuildName() const {return guildName;};
	unsigned long getGuildId() const {return guildId;};

	void addContainer(uint32_t containerid, Container *container);
	void closeContainer(uint32_t containerid);
	int32_t getContainerID(const Container* container) const;
	Container* getContainer(uint32_t cid);
	
	void addStorageValue(const unsigned long key, const long value);
	bool getStorageValue(const unsigned long key, long &value) const;
	inline StorageMap::const_iterator getStorageIteratorBegin() const {return storageMap.begin();}
	inline StorageMap::const_iterator getStorageIteratorEnd() const {return storageMap.end();}
	
	int getAccount() const {return accountNumber;}
	int getLevel() const {return level;}
	int getHealth() const {return health;}
	int getMana() const {return mana;}
	int getMagicLevel() const {return maglevel;}
	playersex_t getSex() {return sex;}
	bool gainManaTick();
	bool gainHealthTick();
	
	int getPlayerInfo(playerinfo_t playerinfo) const;
	int getSkill(skills_t skilltype, skillsid_t skillinfo) const;
	std::string getSkillName(int skillid);
	void addSkillTry(int skilltry);
	void addSkillShieldTry(int skilltry);
	
	unsigned long getExperience() const {
		return experience;
	}

	double getCapacity() const {
		if(access == 0) {
			return capacity;
		}
		else
			return 0.00;
	}
	
	virtual int getLostExperience() {
		return (int)std::floor(((double)experience * 0.1));
	}
	
	double getFreeCapacity() const {
		if(access == 0) {
			return std::max(0.00, capacity - inventoryWeight);
		}
		else
			return 0.00;
	}
	
	time_t getLastLoginSaved() const { return lastLoginSaved; };
	const Position& getLoginPosition() {return loginPosition;};
	
	void updateInventoryWeigth();
	
	Item* getInventoryItem(slots_t slot) const;
	
	void addManaSpent(unsigned long spent);
	void addExperience(unsigned long exp);
	virtual int getWeaponDamage() const;
	virtual int getArmor() const;
	virtual int getDefense() const;
	unsigned long getMoney();

	bool substractMoney(uint32_t money);
	bool substractMoneyItem(Item* item, uint32_t money);
	bool removeItemTypeCount(uint16_t itemId, uint32_t count);
	uint32_t getItemTypeCount(uint16_t itemId);

		
	unsigned long eventAutoWalk;
	
	//battle functions
	Item* GetDistWeapon() const;
	void removeDistItem();
	fight_t getFightType();
	subfight_t getSubFightType();

	//items
	ContainerVector containerVec;
	void preSave();

	unsigned long getIP() const;
	Depot* getDepot(uint32_t depotId, bool autoCreateDepot);
	bool addDepot(Depot* depot, uint32_t depotId);

	//depots	
	DepotMap depots;
	uint32_t maxDepotLimit;
	
	bool CanSee(const Position& pos) const;
	bool CanSee(int x, int y, int z) const;
	
	void sendIcons();  
	void sendChangeSpeed(Creature* creature);
	void sendToChannel(Creature *creature, SpeakClasses type, const std::string &text, unsigned short channelId);
	void sendCancelMessage(ReturnValue message) const;
	void sendCancel(const char* msg) const;
	void sendCancelWalk() const;
	void sendStats();
	void sendTextMessage(MessageClasses mclass, const char* message) const;
	void sendTextMessage(MessageClasses mclass, const char* message,const Position &pos, unsigned char type) const;
	void sendPing();
	void sendTextWindow(Item* item,const unsigned short maxlen, const bool canWrite);  
	void sendDistanceShoot(const Position &from, const Position &to, unsigned char type);
	void sendMagicEffect(const Position &pos, unsigned char type);
	void sendAnimatedText(const Position &pos, unsigned char color, std::string text);
	void sendCreatureHealth(const Creature *creature);
	void sendTradeItemRequest(const Player* player, const Item* item, bool ack);
	void sendCloseTrade();
	void sendHouseWindow(House* _house, unsigned long _listid);
	void receivePing();
	void flushMsg();
	
	void die();      //player loses exp/skills/maglevel on death
	
	virtual bool isAttackable() const { return (access == 0); };
	virtual void dropLoot(Container *corpse);
	virtual int getLookCorpse();
	bool NeedUpdateStats();
	
	virtual std::string getDescription(int32_t lookDistance) const;
	
	void setTradeState(tradestate_t state) {tradeState = state;};
	tradestate_t getTradeState() {return tradeState;};
	Item* getTradeItem() {return tradeItem;};
	
	void notifyLogIn(Player* player);
	void notifyLogOut(Player* player);
	bool removeVIP(unsigned long guid);
	bool addVIP(unsigned long guid, std::string &name, bool isOnline, bool interal = false);
	
	VIPListSet VIPList;
	
	virtual void getCreatureLight(LightInfo& light) const;
	
	void updateItemsLight(bool internal = false);
	
	#ifdef __SKULLSYSTEM__
	skulls_t getSkull() const;
	skulls_t getSkullClient(const Player* player) const;
	bool hasAttacked(const Player* attacked) const;
	void addAttacked(const Player* attacked);
	void clearAttacked();
	void addUnjustifiedDead(const Player* attacked);
	void setSkull(skulls_t new_skull);
	void sendCreatureSkull(const Creature* creature) const;
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
	void sendCreatureMove(const Creature* creature, const Position& oldPos, uint32_t oldStackPos, bool teleport);

	void sendCreatureTurn(const Creature* creature, uint32_t stackpos);
	void sendCreatureSay(const Creature* creature, SpeakClasses type, const std::string& text);
	void sendCreatureChangeOutfit(const Creature* creature);
	void sendCreatureLight(const Creature* creature);
	void sendWorldLight(LightInfo& lightInfo);

	//event methods
	virtual void onAddTileItem(const Position& pos, const Item* item);
	virtual void onUpdateTileItem(const Position& pos, uint32_t stackpos, const Item* olditem, const Item* newitem);
	virtual void onRemoveTileItem(const Position& pos, uint32_t stackpos, const Item* item);
	virtual void onUpdateTile(const Position& pos);
	
	virtual void onCreatureAppear(const Creature* creature, bool isLogin);
	virtual void onCreatureDisappear(const Creature* creature, uint32_t stackpos, bool isLogout);
	virtual void onCreatureMove(const Creature* creature, const Position& oldPos, uint32_t oldStackPos, bool teleport);

	virtual void onCreatureTurn(const Creature* creature, uint32_t stackpos);
	virtual void onCreatureSay(const Creature* creature, SpeakClasses type, const std::string& text);
	virtual void onCreatureChangeOutfit(const Creature* creature);

	//container
	void onAddContainerItem(const Container* container, const Item* item);
	void onUpdateContainerItem(const Container* container, uint8_t slot, const Item* oldItem, const Item* newItem);
	void onRemoveContainerItem(const Container* container, uint8_t slot, const Item* item);
	
	void onCloseContainer(const Container* container);
	void onSendContainer(const Container* container);
	void autoCloseContainers(const Container* container);

	//inventory
	void onAddInventoryItem(slots_t slot, const Item* item);
	void onUpdateInventoryItem(slots_t slot, const Item* oldItem, const Item* newItem);
	void onRemoveInventoryItem(slots_t slot, const Item* item);

	virtual void postAddNotification(Thing* thing, bool hasOwnership = true);
	virtual void postRemoveNotification(Thing* thing, bool isCompleteRemoval, bool hadOwnership = true);

protected:
	void checkTradeState(const Item* item);
	void sendCancelAttacking();
	void addSkillTryInternal(int skilltry,int skill);

	virtual int onThink(int& newThinkTicks);

	bool hasCapacity(const Item* item, uint32_t count) const;

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
	virtual Thing* __getThing(uint32_t index) const;

	virtual void __internalAddThing(Thing* thing);
	virtual void __internalAddThing(uint32_t index, Thing* thing);

protected:
	Protocol* client;
	unsigned long experience;
	
	playervoc_t vocation;
	playersex_t sex;
	int food;
	
	double inventoryWeight;
	double capacity;
	
	bool SendBuffer;
	long internal_ping;
	long npings;
	
	char fightMode, followMode;
	
	//account variables
	int accountNumber;
	std::string password;
	time_t lastlogin;
	time_t lastLoginSaved;
	Position loginPosition;
	unsigned long lastip;
	
	//inventory variables
	Item* items[11]; //equipement of the player
	
	//player advances variables
	unsigned int skills[7][3];
	
	//reminder: 0 = None, 1 = Sorcerer, 2 = Druid, 3 = Paladin, 4 = Knight
	static const int CapGain[5];          //for level advances
	static const int ManaGain[5];
	static const int HPGain[5];
	static const int gainManaVector[5][2];
	static const int gainHealthVector[5][2];
	unsigned short manaTick;
	unsigned short healthTick;
	
	unsigned char level_percent;
	unsigned char maglevel_percent;

	//trade variables
	//unsigned long tradePartner;
	Player* tradePartner;
	tradestate_t tradeState;
	Item* tradeItem;
	
	//autowalking
	std::list<Direction> pathlist;
	
	//cache some data
	struct SkillCache{
		unsigned int tries;
		int level;
		playervoc_t vocation;
	};
	
	SkillCache SkillAdvanceCache[7][2];
	struct SentStats{
		int health;
		int healthmax;
		unsigned long experience;
		int level;
		double freeCapacity;
		int mana;
		int manamax;
		int manaspent;
		int maglevel;
	};
	
	SentStats lastSentStats;

	std::string name;	
	unsigned long guid;
	
	//guild variables
	unsigned long guildId;
	std::string guildName;
	std::string guildRank;
	std::string guildNick;
	
	StorageMap storageMap;
	
	struct MoneyItem{
		Item* item;
		freeslot_t location;
		int slot;
		Container *parent;
	};

	typedef std::multimap<int, struct MoneyItem*, std::less<int> > MoneyMap;
	typedef MoneyMap::value_type moneymap_pair;
	
	LightInfo itemsLight;
	
	#ifdef __SKULLSYSTEM__
	int64_t redSkullTicks;
	skulls_t skull;
	typedef std::set<long> AttackedSet;
	AttackedSet attackedSet;
	#endif
	
	//for skill advances
	unsigned int getReqSkillTries (int skill, int level, playervoc_t voc);
	
	//for magic level advances
	unsigned int getReqMana(int maglevel, playervoc_t voc); 
	
	friend OTSYS_THREAD_RETURN ConnectionHandler(void *dat);
	
	friend class Game;
	friend class ActionScript;
	friend class Commands;
	friend class Map;
	friend class IOPlayerXML;
	friend class IOPlayerSQL;
};


#endif // __PLAYER_H__
