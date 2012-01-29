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
#include "otsystem.h"
#include "creature.h"
#include "container.h"
#include "depot.h"
#include "cylinder.h"
#include "outfit.h"
#include "enums.h"
#include "vocation.h"
#include "protocolgame.h"
#include <vector>
#include <ctime>
#include <algorithm>

class House;
class Weapon;
class ProtocolGame;
class Npc;
class Party;
class SchedulerTask;
class Guild;

enum skillsid_t
{
	SKILL_LEVEL = 0,
	SKILL_TRIES = 1,
	SKILL_PERCENT = 2
};

enum playerinfo_t
{
	PLAYERINFO_LEVEL,
	PLAYERINFO_LEVELPERCENT,
	PLAYERINFO_HEALTH,
	PLAYERINFO_MAXHEALTH,
	PLAYERINFO_MANA,
	PLAYERINFO_MAXMANA,
	PLAYERINFO_MAGICLEVEL,
	PLAYERINFO_MAGICLEVELPERCENT,
	PLAYERINFO_SOUL
};

enum freeslot_t
{
	SLOT_TYPE_NONE,
	SLOT_TYPE_INVENTORY,
	SLOT_TYPE_CONTAINER
};

enum chaseMode_t
{
	CHASEMODE_STANDSTILL,
	CHASEMODE_FOLLOW
};

enum fightMode_t
{
	FIGHTMODE_ATTACK,
	FIGHTMODE_BALANCED,
	FIGHTMODE_DEFENSE
};

enum tradestate_t
{
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
typedef std::map<uint32_t, std::string> ChannelStatementMap;
typedef std::list<std::string> LearnedInstantSpellList;
typedef std::list<Party*> PartyList;

#define PLAYER_MAX_SPEED 1500
#define PLAYER_MIN_SPEED 10
#define STAMINA_MULTIPLIER (60 * 1000)

const int32_t MAX_STAMINA = 42 * 60 * 60 * 1000;
const int32_t MAX_STAMINA_MINUTES = MAX_STAMINA / 60000;

//////////////////////////////////////////////////////////////////////
// Defines a player...

class Player : public Creature, public Cylinder
{
public:
#ifdef __ENABLE_SERVER_DIAGNOSTIC__
	static uint32_t playerCount;
#endif

	Player(const std::string& name, ProtocolGame* p);
	virtual ~Player();

	virtual Player* getPlayer();
	virtual const Player* getPlayer() const;

	static MuteCountMap muteCountMap;
	static ChannelStatementMap channelStatementMap;
	static uint32_t channelStatementGuid;

	virtual const std::string& getName() const;
	virtual const std::string& getNameDescription() const;
	virtual std::string getDescription(const int32_t& lookDistance) const;

	void setGUID(const uint32_t& _guid);
	const uint32_t& getGUID() const;
	virtual uint32_t idRange();
	static AutoList<Player> listPlayer;
	void removeList();
	void addList();
	void kickPlayer();

	static uint64_t getExpForLevel(uint32_t level);

	void setGuild(Guild* _guild);
	Guild* getGuild() const;
	uint32_t getGuildId() const;

	bool isGuildEnemy(const Player* player) const;
	bool isGuildPartner(const Player* player) const;
	bool isWarPartner(const Player* player) const;
	GuildEmblems_t getGuildEmblem(const Player* player) const;

	std::string guildRank, guildNick;
	uint32_t guildLevel;

	void toogleGmInvisible();
	bool isGmInvisible() const;
	bool canSeeGmInvisible(const Player* player) const;
	bool hasSomeInvisibilityFlag() const;

	void setFlags(const uint64_t& flags);
	bool hasFlag(const PlayerFlags& value) const;

	const uint16_t& getPremiumDays() const;
	bool isPremium() const;

	bool isOffline() const;
	bool isOnline() const;
	void disconnect();
	uint32_t getIP() const;

	void addContainer(const uint32_t& cid, Container* container);
	void closeContainer(const uint32_t& cid);
	int32_t getContainerID(const Container* container) const;
	Container* getContainer(const uint32_t& cid);
	bool canOpenCorpse(const uint32_t& ownerId);

	void addStorageValue(const uint32_t& key, const int32_t& value);
	bool getStorageValue(const uint32_t& key, int32_t& value) const;
	bool eraseStorageValue(const uint32_t& key);
	static bool getStorageValueByName(const std::string& name, const uint32_t& key, int32_t& value);
	static bool setStorageValueByName(const std::string& name, const uint32_t& key, const int32_t& value);
	static bool eraseStorageValueByName(const std::string& name, const uint32_t& key);
	void genReservedStorageRange();

	bool withdrawMoney(const uint32_t& amount);
	bool depositMoney(const uint32_t& amount);
	bool transferMoneyTo(const std::string& name, const uint32_t& amount);
	uint32_t balance;

	StorageMap::const_iterator getStorageIteratorBegin() const;
	StorageMap::const_iterator getStorageIteratorEnd() const;

	const std::string& getAccountName() const;
	const uint32_t& getAccountId() const;
	const uint32_t& getLevel() const;
	uint32_t getMagicLevel() const;
	const int16_t& getAccessLevel() const;
	const int16_t& getViolationLevel() const;
	const std::string& getGroupName() const;

	bool setVocation(const uint32_t& vocId);
	uint32_t getVocationId() const;
	Vocation* getVocation() const;

	const PlayerSex_t& getSex() const;
	bool isMale() const;
	bool isFemale() const;

	void setSex(const PlayerSex_t& player_sex);
	int32_t getPlayerInfo(const playerinfo_t& playerinfo) const;
	const uint64_t& getExperience() const;

	const time_t& getLastLoginSaved() const;
	const Position& getLoginPosition() const;
	const Position& getTemplePosition() const;
	const uint32_t& getTown() const;
	void setTown(const uint32_t& _town);

	bool isLoginAttackLocked(const uint32_t& attackerId) const;

	virtual bool isPushable() const;
	virtual bool canBePushedBy(const Player* player) const;
	virtual int getThrowRange() const;
	virtual bool canSeeInvisibility() const;
	uint32_t getMuteTime();
	void addMessageBuffer();
	void removeMessageBuffer();

	double getCapacity() const;
	double getFreeCapacity() const;

	virtual bool hasHiddenHealth() const;
	virtual int32_t getMaxHealth() const;
	virtual int32_t getMaxMana() const;

	// Returns the inventory item in the slot position
	Item* getInventoryItem(const slots_t& slot) const;
	// As above, but returns NULL if the item can not be weared in that slot (armor in hand for example)
	Item* getEquippedItem(const slots_t& slot) const;

	// Returns the first found item with chosen itemid
	Item* getFirstItemById(const uint32_t& id) const;

	bool isItemAbilityEnabled(const slots_t& slot) const;
	void setItemAbility(const slots_t& slot, bool enabled);

	int32_t getVarSkill(const skills_t& skill) const;
	void setVarSkill(const skills_t& skill, const int32_t& modifier);

	int32_t getVarStats(const stats_t& stat) const;
	void setVarStats(const stats_t& stat, const int32_t& modifier);
	int32_t getDefaultStats(const stats_t& stat);

	void setConditionSuppressions(const uint32_t& conditions, bool remove);

	double getRateValue(const levelTypes_t& rateType) const;
	void setRateValue(const levelTypes_t& rateType, double value);

	uint32_t getLossPercent(const lossTypes_t& lossType) const;
	void setLossPercent(const lossTypes_t& lossType, const uint32_t& newPercent);

	Depot* getDepot(const uint32_t& depotId, bool autoCreateDepot);
	bool addDepot(Depot* depot, const uint32_t& depotId);
	void onReceiveMail(const uint32_t& depotId);
	bool isNearDepotBox(const uint32_t& depotId);

	virtual bool canSee(const Position& pos) const;
	virtual bool canSeeCreature(const Creature* creature) const;
	virtual bool canWalkthrough(const Creature* creature) const;

	virtual RaceType_t getRace() const;

	//safe-trade functions
	void setTradeState(const tradestate_t& state);
	const tradestate_t& getTradeState();
	Item* getTradeItem();

	//shop functions
	void setShopOwner(Npc* owner, int32_t onBuy, int32_t onSell);
	Npc* getShopOwner(int32_t& onBuy, int32_t& onSell);
	const Npc* getShopOwner(int32_t& onBuy, int32_t& onSell) const;

	//V.I.P. functions
	void notifyLogIn(Player* player);
	void notifyLogOut(Player* player);
	bool removeVIP(const uint32_t& guid);
	bool addVIP(const uint32_t& guid, std::string& name, bool isOnline, bool interal = false);

	//follow functions
	virtual bool setFollowCreature(Creature* creature, bool fullPathSearch = false);
	virtual void goToFollowCreature();

	//follow events
	virtual void onFollowCreature(const Creature* creature);

	//walk events
	virtual void onWalk(Direction& dir);
	virtual void onWalkAborted();
	virtual void onWalkComplete();

	void checkIdleTime(const uint32_t& ticks);
	void resetIdle();
	void setIdleTime(const uint32_t& value, bool warned);

	void setChaseMode(const chaseMode_t& mode);
	const chaseMode_t& getChaseMode() const;
	void setFightMode(const fightMode_t& mode);
	const fightMode_t& getFightMode() const;
	void setSafeMode(bool _safeMode);
	bool hasSafeMode() const;
	uint16_t getIcons() const;

	//combat functions
	virtual bool setAttackedCreature(Creature* creature);
	bool isImmune(const CombatType_t& type) const;
	bool isImmune(const ConditionType_t& type, bool aggressive = true) const;
	bool hasShield() const;
	virtual bool isAttackable() const;

	virtual void changeHealth(const int32_t& healthChange);
	virtual void changeMana(const int32_t& manaChange);
	void changeSoul(const int32_t& soulChange);

	bool isPzLocked() const;
	virtual BlockType_t blockHit(Creature* attacker, const CombatType_t& combatType, int32_t& damage,
	                             bool checkDefense = false, bool checkArmor = false);
	virtual void doAttacking(const uint32_t& interval);
	virtual bool hasExtraSwing();
	const int32_t& getShootRange() const;

	int32_t getSkill(const skills_t& skilltype, const skillsid_t& skillinfo) const;
	static std::string getSkillName(const int& skillid);

	bool getAddAttackSkill() const;
	const BlockType_t& getLastAttackBlockType() const;

	Item* getWeapon(bool ignoreAmmu = false);
	virtual WeaponType_t getWeaponType();
	int32_t getWeaponSkill(const Item* item) const;
	void getShieldAndWeapon(const Item* &shield, const Item* &weapon) const;

	virtual void drainHealth(Creature* attacker, const CombatType_t& combatType, const int32_t& damage);
	virtual void drainMana(Creature* attacker, const int32_t& points);
	void addManaSpent(const uint32_t& amount, bool useMultiplier = true);
	void addSkillAdvance(const skills_t& skill, const uint32_t& count, bool useMultiplier = true);

	virtual int32_t getArmor() const;
	virtual int32_t getDefense() const;
	virtual float getAttackFactor() const;
	virtual float getDefenseFactor() const;

	void addCombatExhaust(const uint32_t& ticks);
	void addHealExhaust(const uint32_t& ticks);
	void addInFightTicks(const uint32_t& ticks, bool pzlock = false);
	void addDefaultRegeneration(const uint32_t& addTicks);

	virtual uint64_t getGainedExperience(Creature* attacker) const;
	void getGainExperience(uint64_t& gainExp, bool fromMonster);

	//combat event functions
	virtual void onAddCondition(const ConditionType_t& type, bool hadCondition);
	virtual void onAddCombatCondition(const ConditionType_t& type, bool hadCondition);
	virtual void onEndCondition(const ConditionType_t& type, bool lastCondition);
	virtual void onCombatRemoveCondition(const Creature* attacker, Condition* condition);
	virtual void onTickCondition(const ConditionType_t& type, const int32_t& interval, bool& bRemove);
	virtual void onAttackedCreature(Creature* target);
	virtual void onSummonAttackedCreature(Creature* summon, Creature* target);
	virtual void onAttacked();
	virtual void onAttackedCreatureDrainHealth(Creature* target, const int32_t& points);
	virtual void onSummonAttackedCreatureDrainHealth(Creature* summon, Creature* target, const int32_t& points);
	virtual void onAttackedCreatureDrainMana(Creature* target, const int32_t& points);
	virtual void onSummonAttackedCreatureDrainMana(Creature* summon, Creature* target, const int32_t& points);
	virtual void onTargetCreatureGainHealth(Creature* target, const int32_t& points);
	virtual void onKilledCreature(Creature* target, bool lastHit);
	virtual void onGainExperience(uint64_t& gainExp, bool fromMonster);
	virtual void onGainSharedExperience(uint64_t& gainExp, bool fromMonster);
	virtual void onAttackedCreatureBlockHit(Creature* target, const BlockType_t& blockType);
	virtual void onBlockHit(const BlockType_t& blockType);
	virtual void onChangeZone(const ZoneType_t& zone);
	virtual void onAttackedCreatureChangeZone(const ZoneType_t& zone);
	virtual void onIdleStatus();
	virtual void onPlacedCreature();
	virtual void sendReLoginWindow();
	virtual void getCreatureLight(LightInfo& light) const;

	void setParty(Party* _party);
	Party* getParty() const;
	PartyShields_t getPartyShield(const Player* player) const;
	bool isInviting(const Player* player) const;
	bool isPartner(const Player* player) const;
	void sendPlayerPartyIcons(Player* player);
	bool addPartyInvitation(Party* party);
	bool removePartyInvitation(Party* party);
	void clearPartyInvitations();

	Skulls_t getSkull() const;
	Skulls_t getSkullClient(const Player* player) const;
	bool hasAttacked(const Player* attacked) const;
	void addAttacked(const Player* attacked);
	void clearAttacked();
	void addUnjustifiedDead(const Player* attacked);
	void setSkull(const Skulls_t& newSkull);
	void sendCreatureSkull(const Creature* creature) const;
	void checkSkullTicks(const int32_t& ticks);

#ifdef __GUILDWARSLUARELOAD__
	void sendCreatureEmblem(const Creature* creature);
#endif

	void checkRecentlyGainedExperience(const uint32_t& interval);
	bool canWearOutfit(const uint32_t& outfitId, const uint32_t& addons);
	bool addOutfit(const uint32_t& outfitId, const uint32_t& addons);
	bool removeOutfit(const uint32_t& outfitId, const uint32_t& addons);
	bool canLogout();
	void broadcastLoot(Creature* creature, Container* corpse);
	bool checkPzBlock(Player* targetPlayer);

	//creature events
	void onAdvanceEvent(const levelTypes_t& type, const uint32_t& oldLevel, const uint32_t& newLevel);
	bool onLookEvent(Thing* target, const uint32_t& itemId);

	//tile
	//send methods
	void sendAddTileItem(const Tile* tile, const Position& pos, const Item* item);
	void sendUpdateTileItem(const Tile* tile, const Position& pos, const Item* olditem, const Item* newitem);
	void sendRemoveTileItem(const Tile* tile, const Position& pos, const uint32_t& stackpos, const Item* item);
	void sendUpdateTile(const Tile* tile, const Position& pos);

	void sendCreatureAppear(const Creature* creature, const Position& pos);
	void sendCreatureDisappear(const Creature* creature, const uint32_t& stackpos, bool isLogout);
	void sendCreatureMove(const Creature* creature, const Tile* newTile, const Position& newPos,
	                      const Tile* oldTile, const Position& oldPos, uint32_t oldStackPos, bool teleport);

	void sendCreatureTurn(const Creature* creature);
	void sendCreatureSay(const Creature* creature, const SpeakClasses& type, const std::string& text);
	void sendCreatureSquare(const Creature* creature, const SquareColor_t& color);
	void sendCreatureChangeOutfit(const Creature* creature, const Outfit_t& outfit);
	void sendCreatureChangeVisible(const Creature* creature, bool visible);
#ifdef __MIN_PVP_LEVEL_APPLIES_TO_SUMMONS__
	void forceClientToReloadCreature(const Creature* creature);
#endif
	void sendCreatureLight(const Creature* creature);
	void sendCreatureShield(const Creature* creature);

	//container
	void sendAddContainerItem(const Container* container, const Item* item);
	void sendUpdateContainerItem(const Container* container, const uint8_t& slot, const Item* oldItem, const Item* newItem);
	void sendRemoveContainerItem(const Container* container, const uint8_t& slot, const Item* item);
	void sendContainer(const uint32_t& cid, const Container* container, bool hasParent);

	//inventory
	void sendAddInventoryItem(const slots_t& slot, const Item* item);
	void sendUpdateInventoryItem(const slots_t& slot, const Item* oldItem, const Item* newItem);
	void sendRemoveInventoryItem(const slots_t& slot, const Item* item);

	//event methods
	virtual void onAddTileItem(const Tile* tile, const Position& pos, const Item* item);
	virtual void onUpdateTileItem(const Tile* tile, const Position& pos,
	                              const Item* oldItem, const ItemType& oldType, const Item* newItem, const ItemType& newType);
	virtual void onRemoveTileItem(const Tile* tile, const Position& pos,
	                              const ItemType& iType, const Item* item);
	virtual void onUpdateTile(const Tile* tile, const Position& pos);

	virtual void onCreatureAppear(const Creature* creature, bool isLogin);
	virtual void onCreatureDisappear(const Creature* creature, bool isLogout);
	virtual void onCreatureMove(const Creature* creature, const Tile* newTile, const Position& newPos,
	                            const Tile* oldTile, const Position& oldPos, bool teleport);

	virtual void onAttackedCreatureDissapear(bool isLogout);
	virtual void onFollowCreatureDissapear(bool isLogout);

	//container
	void onAddContainerItem(const Container* container, const Item* item);
	void onUpdateContainerItem(const Container* container, const uint8_t& slot,
	                           const Item* oldItem, const ItemType& oldType, const Item* newItem, const ItemType& newType);
	void onRemoveContainerItem(const Container* container, const uint8_t& slot, const Item* item);

	void onCloseContainer(const Container* container);
	void onSendContainer(const Container* container);
	void autoCloseContainers(const Container* container);

	//inventory
	void onAddInventoryItem(const slots_t& slot, Item* item);
	void onUpdateInventoryItem(const slots_t& slot, Item* oldItem, const ItemType& oldType,
	                           Item* newItem, const ItemType& newType);
	void onRemoveInventoryItem(const slots_t& slot, Item* item);

	//other send messages
	void sendAnimatedText(const Position& pos, const uint8_t& color, const std::string& text) const;
	void sendCancel(const std::string& msg) const;
	void sendCancelMessage(const ReturnValue& message) const;
	void sendCancelTarget() const;
	void sendCancelWalk() const;
	void sendChangeSpeed(const Creature* creature, const uint32_t& newSpeed) const;
	void sendCreatureHealth(const Creature* creature) const;
	void sendDistanceShoot(const Position& from, const Position& to, const uint8_t& type) const;
	void sendHouseWindow(House* house, const uint32_t& listId) const;
	void sendOutfitWindow() const;
	void sendCreatePrivateChannel(const uint16_t& channelId, const std::string& channelName);
	void sendClosePrivate(const uint16_t& channelId) const;
	void sendIcons() const;
	void sendMagicEffect(const Position& pos, const uint8_t& type) const;
	void sendStats();
	void sendSkills() const;
	void sendTextMessage(const MessageClasses& mclass, const std::string& message) const;
	void sendTextWindow(Item* item, const uint16_t& maxlen, bool canWrite) const;
	void sendTextWindow(const uint32_t& itemId, const std::string& text) const;
	void sendToChannel(Creature* creature, const SpeakClasses& type, const std::string& text, const uint16_t& channelId, const uint32_t& time = 0) const;
	// new: shop window
	void sendShop();
	void sendSaleItemList() const;
	void sendCloseShop() const;
	void sendTradeItemRequest(const Player* player, const Item* item, bool ack) const;
	void sendTradeClose() const;
	void sendWorldLight(LightInfo& lightInfo);
	void sendChannelsDialog();
	void sendOpenPrivateChannel(const std::string& receiver);
	void sendOutfitWindow();
	void sendCloseContainer(const uint32_t& cid);
	void sendChannel(const uint16_t& channelId, const std::string& channelName);
	void sendRuleViolationsChannel(const uint16_t& channelId);
	void sendRemoveReport(const std::string& name);
	void sendLockRuleViolation();
	void sendRuleViolationCancel(const std::string& name);
	void sendQuestLog();
	void sendQuestLine(const Quest* quest);

	void sendTutorial(const uint8_t& tutorialId);
	void sendAddMarker(const Position& pos, const uint8_t& markType, const std::string& desc);

	void receivePing();

	virtual void onThink(const uint32_t& interval);
	virtual void onAttacking(const uint32_t& interval);

	virtual void postAddNotification(Thing* thing, const Cylinder* oldParent, int32_t index, cylinderlink_t link = LINK_OWNER, bool isNewItem = true);
	virtual void postRemoveNotification(Thing* thing, const Cylinder* newParent, int32_t index, bool isCompleteRemoval, cylinderlink_t link = LINK_OWNER);

	Item* getWriteItem(uint32_t& _windowTextId, uint16_t& _maxWriteLen);
	void setWriteItem(Item* item, const uint16_t& _maxWriteLen = 0);

	void registerMoveItemAsNow();
	bool canMoveItem() const;

	House* getEditHouse(uint32_t& _windowTextId, uint32_t& _listId);
	void setEditHouse(House* house, const uint32_t& listId = 0);

	void setNextAction(const int64_t& time);
	bool canDoAction() const;
	uint32_t getNextActionTime() const;
	virtual const uint32_t& getAttackSpeed() const;
	void setLastAttackAsNow();
	const int64_t& getLastTimeRequestOutfit() const;
	void setLastTimeRequestOutfitAsNow();

	void learnInstantSpell(const std::string& name);
	bool hasLearnedInstantSpell(const std::string& name) const;
	void stopWalk();
	void openShopWindow(const std::list<ShopInfo>& shop);
	void closeShopWindow(bool sendCloseShopWindow = true);
	void updateSaleShopList(const uint32_t& itemId);
	bool hasShopItemForSale(const uint32_t& itemId, const uint8_t& subType);

	VIPListSet VIPList;
	uint32_t maxVipLimit;

	//items
	ContainerVector containerVec;
	void preSave();

	//stamina
	void addStamina(const int64_t& value);
	void removeStamina(const int64_t& value);
	int32_t getStaminaMinutes();
	const int32_t& getStamina();
	int32_t getSpentStamina();
	void setStaminaMinutes(const uint32_t& value);

	//depots
	DepotMap depots;
	uint32_t maxDepotLimit;

protected:
	void checkTradeState(const Item* item);
	bool hasCapacity(const Item* item, uint32_t count) const;

	void gainExperience(uint64_t& gainExp, bool fromMonster);
	void addExperience(const uint64_t& exp);
	void removeExperience(const uint64_t& exp, bool updateStats = true);

	void updateInventoryWeight();

	void setNextWalkActionTask(SchedulerTask* task);
	void setNextActionTask(SchedulerTask* task);

	void onDie();
	void die();
	virtual Item* dropCorpse();
	virtual Item* createCorpse();

	//cylinder implementations
	virtual ReturnValue __queryAdd(int32_t index, const Thing* thing, uint32_t count,
	                               uint32_t flags) const;
	virtual ReturnValue __queryMaxCount(int32_t index, const Thing* thing, uint32_t count, uint32_t& maxQueryCount,
	                                    uint32_t flags) const;
	virtual ReturnValue __queryRemove(const Thing* thing, uint32_t count, uint32_t flags) const;
	virtual Cylinder* __queryDestination(int32_t& index, const Thing* thing, Item** destItem,
	                                     uint32_t& flags);

	virtual void __addThing(Thing* thing);
	virtual void __addThing(int32_t index, Thing* thing);

	virtual void __updateThing(Thing* thing, uint16_t itemId, uint32_t count);
	virtual void __replaceThing(uint32_t index, Thing* thing);

	virtual void __removeThing(Thing* thing, uint32_t count);

	virtual int32_t __getIndexOfThing(const Thing* thing) const;
	virtual int32_t __getFirstIndex() const;
	virtual int32_t __getLastIndex() const;
	virtual uint32_t __getItemTypeCount(uint16_t itemId, int32_t subType = -1) const;
	virtual std::map<uint32_t, uint32_t>& __getAllItemTypeCount(std::map<uint32_t, uint32_t>& countMap) const;
	virtual Thing* __getThing(uint32_t index) const;

	virtual void __internalAddThing(Thing* thing);
	virtual void __internalAddThing(uint32_t index, Thing* thing);

protected:
	ProtocolGame* client;

	uint32_t level;
	uint32_t levelPercent;
	uint32_t magLevel;
	uint32_t magLevelPercent;
	int16_t accessLevel;
	int16_t violationLevel;
	std::string groupName;
	uint64_t experience;
	uint32_t damageImmunities;
	uint32_t conditionImmunities;
	uint32_t conditionSuppressions;
	uint32_t condition;
	int32_t stamina;
	uint32_t manaSpent;
	Vocation_t vocation_id;
	Vocation* vocation;
	PlayerSex_t sex;
	int32_t soul, soulMax;
	uint64_t groupFlags;
	uint16_t premiumDays;
	uint32_t MessageBufferTicks;
	int32_t MessageBufferCount;
	uint32_t actionTaskEvent;
	uint32_t walkTaskEvent;
	SchedulerTask* walkTask;

	int32_t idleTime;
	bool idleWarned;
	int64_t lastTimeRequestOutfit;

	double inventoryWeight;
	double capacity;

	int64_t last_ping;
	int64_t last_pong;
	int64_t nextAction;
	int64_t lastMoveItem;

	bool pzLocked;
	bool isConnecting;
	int32_t bloodHitCount;
	int32_t shieldBlockCount;
	BlockType_t lastAttackBlockType;
	bool addAttackSkillPoint;
	uint64_t lastAttack;
	int32_t shootRange;

	chaseMode_t chaseMode;
	fightMode_t fightMode;
	bool safeMode;

	//
	bool gmInvisible;

	//account variables
	uint32_t accountId;
	std::string accountName;
	std::string password;
	time_t lastLoginSaved;
	time_t lastLogout;
	int64_t lastLoginMs;
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

	//loss percent variables
	uint32_t lossPercent[LOSS_LAST + 1];

	//rate value variables
	double rateValue[LEVEL_LAST + 1];

	LearnedInstantSpellList learnedInstantSpellList;

	ConditionList storedConditionList;

	//trade variables
	Player* tradePartner;
	tradestate_t tradeState;
	Item* tradeItem;

	//shop variables
	Npc* shopOwner;
	int32_t purchaseCallback;
	int32_t saleCallback;
	std::list<ShopInfo> shopItemList;

	//party variables
	Party* party;
	PartyList invitePartyList;

	std::string name;
	std::string nameDescription;
	uint32_t guid;

	uint32_t town;

	//Guild
	Guild* guild;

	StorageMap storageMap;
	LightInfo itemsLight;
	std::pair<Container*, int32_t> backpack;

	OutfitMap outfits;
	bool requestedOutfitWindow;

	//read/write storage data
	uint32_t windowTextId;
	Item* writeItem;
	uint16_t maxWriteLen;
	House* editHouse;
	uint32_t editListId;

	Skulls_t skullType;
	int64_t lastSkullTime;
	typedef std::set<uint32_t> AttackedSet;
	AttackedSet attackedSet;

	void updateItemsLight(bool internal = false);
	virtual int32_t getStepSpeed() const;
	void updateBaseSpeed();

	static uint32_t getPercentLevel(const uint64_t& count, const uint32_t& nextLevelCount);
	virtual uint64_t getLostExperience() const;

	virtual void dropLoot(Container* corpse);
	virtual const uint32_t& getDamageImmunities() const;
	virtual const uint32_t& getConditionImmunities() const;
	virtual const uint32_t& getConditionSuppressions() const;
	virtual const uint16_t& getLookCorpse() const;
	virtual void getPathSearchParams(const Creature* creature, FindPathParams& fpp) const;

	friend class Game;
	friend class Npc;
	friend class LuaScriptInterface;
	friend class Map;
	friend class Actions;
	friend class IOPlayer;
	friend class ProtocolGame;
};

#endif
