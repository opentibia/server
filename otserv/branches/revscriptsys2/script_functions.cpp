/////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
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

#include "lua_manager.h"
#include "script_manager.h"
#include "script_event.h"
#include "script_listener.h"
#include "configmanager.h"
#include "housetile.h"
#include "player.h"
#include "ioplayer.h"
#include "game.h"
#include "actor.h"
#include "town.h"
#include "chat.h"
#include "house.h"
#include "spawn.h"
#include "vocation.h"
#include "status.h"

extern ConfigManager g_config;
extern Game g_game;
extern Vocations g_vocations;

using namespace Script;

///////////////////////////////////////////////////////////////////////////////
// Bit lib declarations

int lua_BitNot(lua_State* L);
int lua_BitAnd(lua_State* L);
int lua_BitOr(lua_State* L);
int lua_BitXor(lua_State* L);
int lua_BitLeftShift(lua_State* L);
int lua_BitRightShift(lua_State* L);
// Unsigned
int lua_BitUNot(lua_State* L);
int lua_BitUAnd(lua_State* L);
int lua_BitUOr(lua_State* L);
int lua_BitUXor(lua_State* L);
int lua_BitULeftShift(lua_State* L);
int lua_BitURightShift(lua_State* L);

const luaL_Reg lua_BitReg[] =
{
	{"bnot", lua_BitNot},
	{"band", lua_BitAnd},
	{"bor", lua_BitOr},
	{"bxor", lua_BitXor},
	{"lshift", lua_BitLeftShift},
	{"rshift", lua_BitRightShift},
	// Unsigned
	{"ubnot", lua_BitUNot},
	{"uband", lua_BitUAnd},
	{"ubor", lua_BitUOr},
	{"ubxor", lua_BitUXor},
	{"ulshift", lua_BitULeftShift},
	{"urshift", lua_BitURightShift},
	{NULL,NULL}
};

///////////////////////////////////////////////////////////////////////////////
// Function list

void Manager::registerClasses() {
	// Enums
	registerClass("Enum");
	registerEnum<RaceType>();
	registerEnum<Direction>();
	registerEnum<CombatType>();
	registerEnum<BlockType>();
	registerEnum<ViolationAction>();
	registerEnum<SkillType>();
	registerEnum<LevelType>();
	registerEnum<PlayerStatType>();
	registerEnum<LossType>();
	registerEnum<PlayerSex>();
	registerEnum<ChaseMode>();
	registerEnum<FightMode>();
	registerEnum<TradeState>();
	registerEnum<SlotType>();
	registerEnum<SlotPosition>();
	registerEnum<TileProp>();
	registerEnum<ZoneType>();
	registerEnum<WorldType>();
	registerEnum<Script::ListenerType>();
	registerEnum<MechanicType>();
	registerEnum<ConditionId>();
	registerEnum<ConditionEnd>();
	registerEnum<ConditionAttribute>();
	registerEnum<PlayerFlag>();
	registerEnum<MagicEffect>();
	registerEnum<ShootEffect>();
	registerEnum<SpeakClass>();
	registerEnum<MessageClass>();
	registerEnum<FluidType>();
	registerEnum<TextColor>();
	registerEnum<IconType>();
	registerEnum<WeaponType>();
	registerEnum<AmmunitionType>();
	registerEnum<AmmunitionAction>();
	registerEnum<WieldInformation>();
	registerEnum<SkullType>();
	registerEnum<ReturnValue>();

	registerClass("Event");
	registerClass("OnSayEvent", "Event");
	registerClass("OnUseItemEvent", "Event");
	registerClass("OnJoinChannelEvent", "Event");
	registerClass("OnLeaveChannelEvent", "Event");
	registerClass("OnLoginEvent", "Event");
	registerClass("OnLogoutEvent", "Event");
	registerClass("OnLookEvent", "Event");
	registerClass("OnTurnEvent", "Event");
	registerClass("OnServerLoadEvent", "Event");
	registerClass("OnServerUnloadEvent", "Event");
	registerClass("OnSpotCreatureEvent", "Event");
	registerClass("OnLoseCreatureEvent", "Event");
	registerClass("OnSpawnEvent", "Event");
	registerClass("OnThinkEvent", "Event");
	registerClass("OnAdvanceEvent", "Event");
	registerClass("OnKillEvent", "Event");
	registerClass("OnDeathEvent", "Event");
	registerClass("OnHearEvent", "Event");
	registerClass("OnEquipItemEvent", "Event");
	registerClass("OnMoveCreatureEvent", "Event");
	registerClass("OnMoveItemEvent", "Event");
	registerClass("OnAccountLoginEvent", "Event");
	registerClass("OnChangeOutfitEvent", "Event");
	registerClass("OnShopPurchaseEvent", "Event");
	registerClass("OnShopSellEvent", "Event");
	registerClass("OnShopCloseEvent", "Event");
	registerClass("OnTradeBeginEvent", "Event");
	registerClass("OnTradeEndEvent", "Event");
	registerClass("OnConditionEffectEvent", "Event");
	registerClass("OnAttackEvent", "Event");
	registerClass("OnDamageEvent", "Event");

	registerClass("Thing");
	registerClass("Creature", "Thing");
	registerClass("Actor", "Creature");
	registerClass("Player", "Creature");
	registerClass("Item", "Thing");
	registerClass("Teleport", "Item");
	registerClass("Container", "Item");

	registerClass("Tile");

	registerClass("Town");
	registerClass("House");
	registerClass("Waypoint");

	registerClass("Channel");

	registerClass("Condition");

	// Event classes
	registerMemberFunction("Event", "skip()", &Manager::lua_Event_skip);
	registerMemberFunction("Event", "propagate()", &Manager::lua_Event_propagate);

	// Game classes
	registerMemberFunction("Thing", "getPosition()", &Manager::lua_Thing_getPosition);
	registerMemberFunction("Thing", "getX()", &Manager::lua_Thing_getX);
	registerMemberFunction("Thing", "getY()", &Manager::lua_Thing_getY);
	registerMemberFunction("Thing", "getZ()", &Manager::lua_Thing_getZ);
	registerMemberFunction("Thing", "getParentTile()", &Manager::lua_Thing_getParentTile);
	registerMemberFunction("Thing", "getParent()", &Manager::lua_Thing_getParent);
	registerMemberFunction("Thing", "isMoveable()", &Manager::lua_Thing_isMoveable);
	registerMemberFunction("Thing", "getName()", &Manager::lua_Thing_getName);
	registerMemberFunction("Thing", "getDescription([int lookdistance])", &Manager::lua_Thing_getDescription);
	registerMemberFunction("Thing", "moveTo(table pos)", &Manager::lua_Thing_moveToPosition);
	registerMemberFunction("Thing", "destroy()", &Manager::lua_Thing_destroy);

	registerGlobalFunction("getThingByID(int id)", &Manager::lua_getThingByID);

	// Condition
	registerMemberFunction("Condition", "setName(string name)", &Manager::lua_Condition_setName);
	registerMemberFunction("Condition", "setCombatType(CombatType type)", &Manager::lua_Condition_setCombatType);
	registerMemberFunction("Condition", "setMechanicType(MechanicType type)", &Manager::lua_Condition_setMechanicType);
	registerMemberFunction("Condition", "setTicks(int ticks)", &Manager::lua_Condition_setTicks);
	registerMemberFunction("Condition", "setFlags(int flags)", &Manager::lua_Condition_setFlags);
	registerMemberFunction("Condition", "destroy()", &Manager::lua_Condition_destroy);

	registerGlobalFunction("createCondition()", &Manager::lua_createCondition);

	registerMemberFunction("Condition", "addPeriodicHeal(int interval, int value, int rounds)", &Manager::lua_Condition_addPeriodicHeal);
	registerMemberFunction("Condition", "addPeriodicDamage(int interval, CombatType type, int value, int rounds)", &Manager::lua_Condition_addPeriodicDamage);
	registerMemberFunction("Condition", "addAveragePeriodicDamage(int interval, CombatType type, int totalDamage, int startDamage)", &Manager::lua_Condition_addAveragePeriodicDamage);
	registerMemberFunction("Condition", "addModStamina(int interval, int value)", &Manager::lua_Condition_addModStamina);
	registerMemberFunction("Condition", "addRegenHealth(int interval, int value)", &Manager::lua_Condition_addRegenHealth);
	registerMemberFunction("Condition", "addRegenPercentHealth(int interval, PlayerStatType type, int percent)", &Manager::lua_Condition_addRegenPercentHealth);
	registerMemberFunction("Condition", "addRegenMana(int interval, int value)", &Manager::lua_Condition_addRegenMana);
	registerMemberFunction("Condition", "addRegenPercentMana(int interval, PlayerStatType type, int percent)", &Manager::lua_Condition_addRegenPercentMana);
	registerMemberFunction("Condition", "addRegenSoul(int interval, int value)", &Manager::lua_Condition_addRegenSoul);
	registerMemberFunction("Condition", "addRegenPercentSoul(int interval, PlayerStatType type, int percent)", &Manager::lua_Condition_addRegenPercentSoul);
	registerMemberFunction("Condition", "addModSpeed(int percent, int value)", &Manager::lua_Condition_addSpeed);
	registerMemberFunction("Condition", "addModStat(PlayerStatType type, int value)", &Manager::lua_Condition_addModStat);
	registerMemberFunction("Condition", "addModPercentStat(PlayerStatType type, int percent)", &Manager::lua_Condition_addModPercentStat);
	registerMemberFunction("Condition", "addModSkill(SkillType type, int value)", &Manager::lua_Condition_addModSkill);
	registerMemberFunction("Condition", "addModPercentSkill(SkillType type, int percent)", &Manager::lua_Condition_addModPercentSkill);
	registerMemberFunction("Condition", "addShapeShift(table outfit)", &Manager::lua_Condition_addShapeShift);
	registerMemberFunction("Condition", "addLight(int level, int color)", &Manager::lua_Condition_addLight);
	registerMemberFunction("Condition", "addDispel(string name)", &Manager::lua_Condition_addDispel);
	registerMemberFunction("Condition", "addScript(string name [, int interval = nil])", &Manager::lua_Condition_addScript);

	// Creature
	registerMemberFunction("Creature", "setRawCustomValue(string key, mixed value)", &Manager::lua_Creature_setRawCustomValue);
	registerMemberFunction("Creature", "getRawCustomValue(string key)", &Manager::lua_Creature_getRawCustomValue);

	registerMemberFunction("Creature", "addSummon(Actor other)", &Manager::lua_Creature_addSummon);
	registerMemberFunction("Creature", "getDirection()", &Manager::lua_Creature_getOrientation);
	registerMemberFunction("Creature", "getHealth()", &Manager::lua_Creature_getHealth);
	registerMemberFunction("Creature", "getHealthMax()", &Manager::lua_Creature_getHealthMax);
	registerMemberFunction("Creature", "setHealth(integer newval)", &Manager::lua_Creature_setHealth);
	registerMemberFunction("Creature", "getID()", &Manager::lua_Creature_getID);
	registerMemberFunction("Creature", "getOrientation()", &Manager::lua_Creature_getOrientation);
	registerMemberFunction("Creature", "getOutfit()", &Manager::lua_Creature_getOutfit);
	registerMemberFunction("Creature", "say(string msg)", &Manager::lua_Creature_say);
	registerMemberFunction("Creature", "setOutfit(table outfit)", &Manager::lua_Creature_setOutfit);
	registerMemberFunction("Creature", "walk(Direction direction)", &Manager::lua_Creature_walk);
	registerMemberFunction("Creature", "getSpeed()", &Manager::lua_Creature_getSpeed);
	registerMemberFunction("Creature", "getArmor()", &Manager::lua_Creature_getArmor);
	registerMemberFunction("Creature", "getDefense()", &Manager::lua_Creature_getDefense);
	registerMemberFunction("Creature", "getBaseSpeed()", &Manager::lua_Creature_getBaseSpeed);
	registerMemberFunction("Creature", "isPushable()", &Manager::lua_Creature_isPushable);
	registerMemberFunction("Creature", "getTarget()", &Manager::lua_Creature_getTarget);
	registerMemberFunction("Creature", "isImmuneToCombat(CombatType combattype)", &Manager::lua_Creature_isImmuneToCombat);
	registerMemberFunction("Creature", "isImmuneToMechanic(MechanicType conditiontype)", &Manager::lua_Creature_isImmuneToMechanic);
	registerMemberFunction("Creature", "getMechanicImmunities()", &Manager::lua_Creature_getMechanicImmunities);
	registerMemberFunction("Creature", "getDamageImmunities()", &Manager::lua_Creature_getDamageImmunities);
	registerMemberFunction("Creature", "internalAddCondition(Condition condition)", &Manager::lua_Creature_internalAddCondition);

	registerGlobalFunction("internalCastSpell(CombatType type, Creature caster, Creature target, int amount \
		[, boolean blockedByShield [, boolean blockedByArmor \
		[, boolean showEffect [, MagicEffect hitEffect [, TextColor hitTextColor]]]]])", &Manager::lua_internalCastSpell);
	registerGlobalFunction("getCreatureByName(string name)", &Manager::lua_getCreatureByName);
	registerGlobalFunction("getCreaturesByName(string name)", &Manager::lua_getCreaturesByName);

	// Actor
	registerMemberFunction("Actor", "setShouldReload(boolean shouldreload)", &Manager::lua_Actor_setShouldReload);
	registerMemberFunction("Actor", "getShouldReload()", &Manager::lua_Actor_getShouldReload);
	registerMemberFunction("Actor", "setAlwaysThink(boolean shouldreload)", &Manager::lua_Actor_setAlwaysThink);
	registerMemberFunction("Actor", "getAlwaysThink()", &Manager::lua_Actor_getAlwaysThink);

	registerMemberFunction("Actor", "setArmor(int newarmor)", &Manager::lua_Actor_setArmor);
	registerMemberFunction("Actor", "setDefense(int newdefense)", &Manager::lua_Actor_setDefense);
	registerMemberFunction("Actor", "setExperienceWorth(int newexp)", &Manager::lua_Actor_setExperienceWorth);
	registerMemberFunction("Actor", "getExperienceWorth()", &Manager::lua_Actor_getExperienceWorth);
	registerMemberFunction("Actor", "setCanPushItems(boolean canpushitems)", &Manager::lua_Actor_setCanPushItems);
	registerMemberFunction("Actor", "getCanPushItems()", &Manager::lua_Actor_getCanPushItems);
	registerMemberFunction("Actor", "setCanPushCreatures(boolean canpushcreatures)", &Manager::lua_Actor_setCanPushCreatures);
	registerMemberFunction("Actor", "getCanPushCreatures()", &Manager::lua_Actor_getCanPushCreatures);
	registerMemberFunction("Actor", "setSpeed(int newspeed)", &Manager::lua_Actor_setSpeed);
	registerMemberFunction("Actor", "setTargetDistance(int newtargetdistance)", &Manager::lua_Actor_setTargetDistance);
	registerMemberFunction("Actor", "getTargetDistance()", &Manager::lua_Actor_getTargetDistance);
	registerMemberFunction("Actor", "setMaxSummons(int newsummons)", &Manager::lua_Actor_setMaxSummons);
	registerMemberFunction("Actor", "getMaxSummons()", &Manager::lua_Actor_getMaxSummons);
	registerMemberFunction("Actor", "setName(string newname)", &Manager::lua_Actor_setName);
	registerMemberFunction("Actor", "setNameDescription(string newname)", &Manager::lua_Actor_setNameDescription);
	registerMemberFunction("Actor", "setStaticAttackChance(int chance)", &Manager::lua_Actor_setStaticAttackChance);
	registerMemberFunction("Actor", "getStaticAttackChance()", &Manager::lua_Actor_getStaticAttackChance);
	registerMemberFunction("Actor", "setFleeHealth(int health)", &Manager::lua_Actor_setFleeHealth);
	registerMemberFunction("Actor", "getFleeHealth()", &Manager::lua_Actor_getFleeHealth);
	registerMemberFunction("Actor", "setPushable(bool pushable)", &Manager::lua_Actor_setPushable);
	registerMemberFunction("Actor", "setBaseSpeed(int newbasespeed)", &Manager::lua_Actor_setBaseSpeed);
	registerMemberFunction("Actor", "setMaxHealth(int newmaxhealth)", &Manager::lua_Actor_setMaxHealth);
	registerMemberFunction("Actor", "getCorpseId()", &Manager::lua_Actor_getCorpseId);
	registerMemberFunction("Actor", "setRace(int racetype)", &Manager::lua_Actor_setRace);
	registerMemberFunction("Actor", "getRace()", &Manager::lua_Actor_getRace);
	registerMemberFunction("Actor", "isSummonable()", &Manager::lua_Actor_isSummonable);
	registerMemberFunction("Actor", "isConvinceable()", &Manager::lua_Actor_isConvinceable);
	registerMemberFunction("Actor", "isIllusionable()", &Manager::lua_Actor_isIllusionable);
	registerMemberFunction("Actor", "setCanBeAttacked(boolean isAttackable)", &Manager::lua_Actor_setCanBeAttacked);
	registerMemberFunction("Actor", "getCanBeAttacked()", &Manager::lua_Actor_getCanBeAttacked);
	registerMemberFunction("Actor", "setCanBeLured(boolean isLureable)", &Manager::lua_Actor_setCanBeLured);
	registerMemberFunction("Actor", "getCanBeLured()", &Manager::lua_Actor_getCanBeLured);
	registerMemberFunction("Actor", "setLightLevel(int newlightlevel)", &Manager::lua_Actor_setLightLevel);
	registerMemberFunction("Actor", "getLightLevel()", &Manager::lua_Actor_getLightLevel);
	registerMemberFunction("Actor", "setLightColor(int newlightcolor)", &Manager::lua_Actor_setLightColor);
	registerMemberFunction("Actor", "getLightColor()", &Manager::lua_Actor_getLightColor);
	registerMemberFunction("Actor", "getManaCost()", &Manager::lua_Actor_getManaCost);
	registerMemberFunction("Actor", "setTarget(Creature target)", &Manager::lua_Actor_setTarget);
	registerMemberFunction("Actor", "setMechanicImmunities(table immunities)", &Manager::lua_Actor_setMechanicImmunities);
	registerMemberFunction("Actor", "setDamageImmunities(table immunities)", &Manager::lua_Actor_setDamageImmunities);
	registerMemberFunction("Actor", "openShop(Player who, table list)", &Manager::lua_Actor_openShop);
	registerMemberFunction("Actor", "closeShop(Player who)", &Manager::lua_Actor_closeShop);

	registerGlobalFunction("createMonster(string monstertypename, position pos)", &Manager::lua_createMonster);
	registerGlobalFunction("createActor(string name, position pos)", &Manager::lua_createActor);

	// Player
	registerMemberFunction("Player", "getFood()", &Manager::lua_Player_getFood);
	registerMemberFunction("Player", "getMana()", &Manager::lua_Player_getMana);
	registerMemberFunction("Player", "getLevel()", &Manager::lua_Player_getLevel);
	registerMemberFunction("Player", "getMagicLevel()", &Manager::lua_Player_getMagicLevel);
	registerMemberFunction("Player", "getSkill(SkillType skill)", &Manager::lua_Player_getSkill);
	registerMemberFunction("Player", "advanceSkill(SkillType skill, int count)", &Manager::lua_Player_advanceSkill);
	registerMemberFunction("Player", "isPremium()", &Manager::lua_Player_isPremium);
	registerMemberFunction("Player", "isAutoWalking()", &Manager::lua_Player_isAutoWalking);
	registerMemberFunction("Player", "getManaMax()", &Manager::lua_Player_getManaMax);
	registerMemberFunction("Player", "setMana(integer newval)", &Manager::lua_Player_setMana);
	registerMemberFunction("Player", "addManaSpent(integer howmuch)", &Manager::lua_Player_addManaSpent);
	registerMemberFunction("Player", "getSoulPoints()", &Manager::lua_Player_getSoulPoints);
	registerMemberFunction("Player", "setSoulPoints(int newval)", &Manager::lua_Player_setSoulPoints);
	registerMemberFunction("Player", "getFreeCap()", &Manager::lua_Player_getFreeCap);
	registerMemberFunction("Player", "getMaximumCap()", &Manager::lua_Player_getMaximumCap);
	registerMemberFunction("Player", "getSex()", &Manager::lua_Player_getSex);
	registerMemberFunction("Player", "getAccess()", &Manager::lua_Player_getAccess);
	registerMemberFunction("Player", "getVocationID()", &Manager::lua_Player_getVocationID);
	registerMemberFunction("Player", "getVocationName()", &Manager::lua_Player_getVocationName);
	registerMemberFunction("Player", "getTownID()", &Manager::lua_Player_getTownID);
	registerMemberFunction("Player", "getGUID()", &Manager::lua_Player_getGUID);
	registerMemberFunction("Player", "getAccessGroup()", &Manager::lua_Player_getGroup);
	registerMemberFunction("Player", "getPremiumDays()", &Manager::lua_Player_getPremiumDays);
	registerMemberFunction("Player", "getSkull()", &Manager::lua_Player_getSkullType);
	registerMemberFunction("Player", "getLastLogin()", &Manager::lua_Player_getLastLogin);
	registerMemberFunction("Player", "getGuildID()", &Manager::lua_Player_getGuildID);
	registerMemberFunction("Player", "getGuildName()", &Manager::lua_Player_getGuildName);
	registerMemberFunction("Player", "getGuildRank()", &Manager::lua_Player_getGuildRank);
	registerMemberFunction("Player", "getGuildNick()", &Manager::lua_Player_getGuildNick);

	registerMemberFunction("Player", "getItemCount(int itemid)", &Manager::lua_Player_getItemCount);
	registerMemberFunction("Player", "addItem(Item item [, SlotType slot = nil [, boolean canDropOnMap = nil]])", &Manager::lua_Player_addItem);
	registerMemberFunction("Player", "removeItem(int id [, int type [,int count]])", &Manager::lua_Player_removeItem);
	registerMemberFunction("Player", "getInventoryItem(SlotType slot)", &Manager::lua_Player_getInventoryItem);
	registerMemberFunction("Player", "getSlot(Item item)", &Manager::lua_Player_getSlot);
	registerMemberFunction("Player", "addExperience(int experience)", &Manager::lua_Player_addExperience);
	registerMemberFunction("Player", "setTown(Town town)", &Manager::lua_Player_setTown);
	registerMemberFunction("Player", "setVocation(int vocationid)", &Manager::lua_Player_setVocation);
	registerMemberFunction("Player", "hasGroupFlag(integer flag)", &Manager::lua_Player_hasGroupFlag);
	registerMemberFunction("Player", "internalWalkTo(position pos)", &Manager::lua_Player_internalWalkTo);
	registerMemberFunction("Player", "internalPickup(Item item)", &Manager::lua_Player_internalPickup);

	registerMemberFunction("Player", "countMoney()", &Manager::lua_Player_countMoney);
	registerMemberFunction("Player", "addMoney(int amount)", &Manager::lua_Player_addMoney);
	registerMemberFunction("Player", "removeMoney(int amount)", &Manager::lua_Player_removeMoney);

	registerMemberFunction("Player", "sendMessage(MessageClass type, mixed msg)", &Manager::lua_Player_sendMessage);

	registerGlobalFunction("getOnlinePlayers()", &Manager::lua_getOnlinePlayers);
	registerGlobalFunction("getPlayerByName(string name)", &Manager::lua_getPlayerByName);
	registerGlobalFunction("getPlayerByNameWildcard(string wild)", &Manager::lua_getPlayerByNameWildcard);
	registerGlobalFunction("getPlayersByName(string name)", &Manager::lua_getPlayersByName);
	registerGlobalFunction("getPlayersByNameWildcard(string wild)", &Manager::lua_getPlayersByNameWildcard);

	// Item
	registerGlobalFunction("createItem(int newid[, int count = nil])", &Manager::lua_createItem);
	registerMemberFunction("Item", "getItemID()", &Manager::lua_Item_getItemID);
	registerMemberFunction("Item", "getLongName()", &Manager::lua_Item_getLongName);
	registerMemberFunction("Item", "getCount()", &Manager::lua_Item_getCount);
	registerMemberFunction("Item", "getWeight()", &Manager::lua_Item_getWeight);
	registerMemberFunction("Item", "isPickupable()", &Manager::lua_Item_isPickupable);
	registerMemberFunction("Item", "getSubtype()", &Manager::lua_Item_getSubtype);

	registerMemberFunction("Item", "setItemID(int newid [, int newtype])", &Manager::lua_Item_setItemID);
	registerMemberFunction("Item", "setCount(int newcount)", &Manager::lua_Item_setCount);
	registerMemberFunction("Item", "startDecaying()", &Manager::lua_Item_startDecaying);
	registerMemberFunction("Item", "setSubtype(int newtype)", &Manager::lua_Item_setSubtype);

	registerMemberFunction("Item", "getRawAttribute(string key)", &Manager::lua_Item_getRawAttribute);
	registerMemberFunction("Item", "eraseAttribute(string key)", &Manager::lua_Item_eraseAttribute);
	// We expose type-safe versions as type is important for internal state
	// The generic setAttribute is defined in item.lua
	registerMemberFunction("Item", "setStringAttribute(string key, string value)", &Manager::lua_Item_setStringAttribute);
	registerMemberFunction("Item", "setIntegerAttribute(string key, int value)", &Manager::lua_Item_setIntegerAttribute);
	registerMemberFunction("Item", "setFloatAttribute(string key, float value)", &Manager::lua_Item_setFloatAttribute);
	registerMemberFunction("Item", "setBooleanAttribute(string key, boolean value)", &Manager::lua_Item_setBooleanAttribute);

	// Container
	registerMemberFunction("Container", "addItem(Item item)", &Manager::lua_Container_addItem);
	registerMemberFunction("Container", "getItem(int index)", &Manager::lua_Container_getItem);
	registerMemberFunction("Container", "size()", &Manager::lua_Container_getSize);
	registerMemberFunction("Container", "capacity()", &Manager::lua_Container_getCapacity);
	registerMemberFunction("Container", "getItems()", &Manager::lua_Container_getItems);
	registerMemberFunction("Container", "getContentDescription()", &Manager::lua_Container_getContentDescription);

	registerGlobalFunction("getItemType(int itemid)", &Manager::lua_getItemType);
	registerGlobalFunction("getMaxItemType()", &Manager::lua_getMaxItemType);
	registerGlobalFunction("getItemIDByName(string name)", &Manager::lua_getItemIDByName);
	registerGlobalFunction("isValidItemID(int id)", &Manager::lua_isValidItemID);

	// Tile
	registerMemberFunction("Tile", "getThing(int index)", &Manager::lua_Tile_getThing);
	registerMemberFunction("Tile", "getCreatures()", &Manager::lua_Tile_getCreatures);

	registerMemberFunction("Tile", "addItem(Item item)", &Manager::lua_Tile_addItem);
	registerMemberFunction("Tile", "getItemCount(int itemid)", &Manager::lua_Tile_getItemCount);
	registerMemberFunction("Tile", "getItem(int index)", &Manager::lua_Tile_getItem);
	registerMemberFunction("Tile", "getItems()", &Manager::lua_Tile_getItems);
	registerMemberFunction("Tile", "getMoveableItems()", &Manager::lua_Tile_getMoveableItems);
	registerMemberFunction("Tile", "getItemsWithItemID()", &Manager::lua_Tile_getItemsWithItemID);
	registerMemberFunction("Tile", "getItemsWithActionID()", &Manager::lua_Tile_getItemsWithActionID);
	registerMemberFunction("Tile", "getItemWithItemID()", &Manager::lua_Tile_getItemWithItemID);
	registerMemberFunction("Tile", "getItemWithActionID()", &Manager::lua_Tile_getItemWithActionID);
	registerMemberFunction("Tile", "queryAdd()", &Manager::lua_Tile_queryAdd);
	registerMemberFunction("Tile", "hasProperty(TileProp prop)", &Manager::lua_Tile_hasProperty);

	// Town
	registerMemberFunction("Town", "getTemplePosition()", &Manager::lua_Town_getTemplePosition);
	registerMemberFunction("Town", "getName()", &Manager::lua_Town_getName);
	registerMemberFunction("Town", "getID()", &Manager::lua_Town_getID);

	registerGlobalFunction("getAllTowns()", &Manager::lua_getTowns);
	registerGlobalFunction("sendMailTo(Item item, string playername [, Town town])", &Manager::lua_sendMailTo);

	// Waypoint
	registerMemberFunction("Waypoint", "getPosition()", &Manager::lua_Waypoint_getPosition);
	registerMemberFunction("Waypoint", "getName()", &Manager::lua_Waypoint_getName);

	registerGlobalFunction("getWaypointByName(string name)", &Manager::lua_getWaypointByName);

	// House
	registerMemberFunction("House", "cleanHouse()", &Manager::lua_House_cleanHouse);
	registerMemberFunction("House", "getDoors()", &Manager::lua_House_getDoors);
	registerMemberFunction("House", "getExitPosition()", &Manager::lua_House_getExitPosition);
	registerMemberFunction("House", "getID()", &Manager::lua_House_getID);
	registerMemberFunction("House", "getInvitedList()", &Manager::lua_House_getInvitedList);
	registerMemberFunction("House", "getName()", &Manager::lua_House_getName);
	registerMemberFunction("House", "getPaidUntil()", &Manager::lua_House_getPaidUntil);
	registerMemberFunction("House", "getRent()", &Manager::lua_House_getRent);
	registerMemberFunction("House", "getSubownerList()", &Manager::lua_House_getSubownerList);
	registerMemberFunction("House", "getTiles()", &Manager::lua_House_getTiles);
	registerMemberFunction("House", "getTown()", &Manager::lua_House_getTown);
	registerMemberFunction("House", "kickPlayer(Player who)", &Manager::lua_House_kickPlayer);
	registerMemberFunction("House", "isInvited(Player who)", &Manager::lua_House_isInvited);
	registerMemberFunction("House", "setInviteList(table list)", &Manager::lua_House_setInviteList);
	registerMemberFunction("House", "setOwner(int guid)", &Manager::lua_House_setOwner);
	registerMemberFunction("House", "setSubownerList(table list)", &Manager::lua_House_setSubownerList);
	registerMemberFunction("House", "setPaidUntil(int until)", &Manager::lua_House_setPaidUntil);

	registerGlobalFunction("getAllHouses()", &Manager::lua_getHouses);

	// Channel
	registerMemberFunction("Channel", "getID()", &Manager::lua_Channel_getID);
	registerMemberFunction("Channel", "getName()", &Manager::lua_Channel_getName);
	registerMemberFunction("Channel", "getUsers()", &Manager::lua_Channel_getUsers);
	registerMemberFunction("Channel", "addUser(Player player)", &Manager::lua_Channel_addUser);
	registerMemberFunction("Channel", "removeUser(Player player)", &Manager::lua_Channel_removeUser);
	registerMemberFunction("Channel", "talk(Player speaker, SpeakClass type, string msg)", &Manager::lua_Channel_talk);
}

void Manager::registerFunctions() {
	// General functions
	registerGlobalFunction("wait(int delay)", &Manager::lua_wait);
	registerGlobalFunction("stacktrace(thread thread)", &Manager::lua_stacktrace);
	registerGlobalFunction("require_directory(string path)", &Manager::lua_require_directory);
	registerGlobalFunction("get_thread_id(thread t)", &Manager::lua_get_thread_id);

	registerGlobalFunction("getConfigValue(string key)", &Manager::lua_getConfigValue);

	// Register different events

	// OnSay/OnHear
	registerGlobalFunction("registerOnSay(string method, boolean case_sensitive, string filter, function callback)", &Manager::lua_registerGenericEvent_OnSay);
	registerGlobalFunction("registerOnCreatureSay(Creature who, string method, boolean case_sensitive, string filter, function callback)", &Manager::lua_registerSpecificEvent_OnSay);
	registerGlobalFunction("registerOnHear(Creature who, function callback)", &Manager::lua_registerSpecificEvent_OnHear);

	// OnUse
	registerGlobalFunction("registerOnUseItem(string method, int filter, function callback)", &Manager::lua_registerGenericEvent_OnUseItem);

	// OnMove
	registerGlobalFunction("registerOnCreatureMove(Creature who, function callback)", &Manager::lua_registerSpecificEvent_CreatureMove);
	registerGlobalFunction("registerOnCreatureMoveIn(Creature who, string method, int itemid, function callback)", &Manager::lua_registerSpecificEvent_CreatureMoveIn);
	registerGlobalFunction("registerOnCreatureMoveOut(Creature who, string method, int itemid, function callback)", &Manager::lua_registerSpecificEvent_CreatureMoveOut);
	registerGlobalFunction("registerOnAnyCreatureMoveIn(string method, int itemid, function callback)", &Manager::lua_registerGenericEvent_CreatureMoveIn);
	registerGlobalFunction("registerOnAnyCreatureMoveOut(string method, int itemid, function callback)", &Manager::lua_registerGenericEvent_CreatureMoveOut);

	// OnTurn
	registerGlobalFunction("registerOnAnyCreatureTurn(function callback)", &Manager::lua_registerGenericEvent_OnCreatureTurn);
	registerGlobalFunction("registerOnCreatureTurn(Creature who, function callback)", &Manager::lua_registerSpecificEvent_OnCreatureTurn);

	// On(De)Equip
	registerGlobalFunction("registerOnEquipItem([string when = nil], string method, int filter, SlotPosition slot, function callback)", &Manager::lua_registerGenericEvent_OnEquipItem);
	registerGlobalFunction("registerOnDeEquipItem([string when = nil], string method, int filter, SlotPosition slot, function callback)", &Manager::lua_registerGenericEvent_OnDeEquipItem);

	// OnServerLoad
	registerGlobalFunction("registerOnServerLoad(function callback)", &Manager::lua_registerGenericEvent_OnServerLoad);

	// OnMoveItem
	// Registering other OnMoveItem events are done through lua
	registerGlobalFunction("registerOnMoveItem([string when = nil], string method, int filter, boolean isadd, boolean isontile, function callback)", &Manager::lua_registerGenericEvent_OnMoveItem);

	// OnSpot / OnLose
	registerGlobalFunction("registerOnSpotCreature(Creature creature, function callback)", &Manager::lua_registerSpecificEvent_OnSpotCreature);
	registerGlobalFunction("registerOnLoseCreature(Creature creature, function callback)", &Manager::lua_registerSpecificEvent_OnLoseCreature);

	// OnThink
	registerGlobalFunction("registerOnCreatureThink(Creature creature, function callback)", &Manager::lua_registerSpecificEvent_OnCreatureThink);

	// OnSpawn
	registerGlobalFunction("registerOnSpawn(string cname, function callback)", &Manager::lua_registerGenericEvent_OnSpawn);

	// OnJoin / OnLeave (channel)
	registerGlobalFunction("registerOnJoinChannel(function callback)", &Manager::lua_registerGenericEvent_OnJoinChannel);
	registerGlobalFunction("registerOnPlayerJoinChannel(Player player, function callback)", &Manager::lua_registerSpecificEvent_OnJoinChannel);
	registerGlobalFunction("registerOnLeaveChannel(function callback)", &Manager::lua_registerGenericEvent_OnLeaveChannel);
	registerGlobalFunction("registerOnPlayerLeaveChannel(Player player, function callback)", &Manager::lua_registerSpecificEvent_OnLeaveChannel);

	// OnLogin / OnLogout
	registerGlobalFunction("registerOnLogin(function callback)", &Manager::lua_registerGenericEvent_OnLogin);
	registerGlobalFunction("registerOnLogout(function callback)", &Manager::lua_registerGenericEvent_OnLogout);
	registerGlobalFunction("registerOnPlayerLogout(Player player, function callback)", &Manager::lua_registerSpecificEvent_OnLogout);

	// OnChangeOutfit
	registerGlobalFunction("registerOnChangeOutfit(function callback)", &Manager::lua_registerGenericEvent_OnChangeOutfit);
	registerGlobalFunction("registerOnPlayerChangeOutfit(Player player, function callback)", &Manager::lua_registerSpecificEvent_OnChangeOutfit);

	// OnLook
	registerGlobalFunction("registerOnLookAtItem(string method, int filter, function callback)", &Manager::lua_registerGenericEvent_OnLookAtItem);
	registerGlobalFunction("registerOnLookAtCreature(Creature creature, function callback)", &Manager::lua_registerSpecificEvent_OnLookAtCreature);
	registerGlobalFunction("registerOnPlayerLookAt(Creature creature, function callback)", &Manager::lua_registerSpecificEvent_OnLook);

	// OnAdvance
	registerGlobalFunction("registerOnAdvance([LevelType skillid = nil], function callback)", &Manager::lua_registerGenericEvent_OnAdvance);
	registerGlobalFunction("registerOnPlayerAdvance(Player player [, LevelType skillid = nil], function callback)", &Manager::lua_registerSpecificEvent_OnAdvance);

	// OnShopPurchase/OnShopSell/OnShopClose
	registerGlobalFunction("registerOnShopPurchase(function callback)", &Manager::lua_registerGenericEvent_OnShopPurchase);
	registerGlobalFunction("registerOnShopSell(function callback)", &Manager::lua_registerGenericEvent_OnShopSell);
	registerGlobalFunction("registerOnShopClose(function callback)", &Manager::lua_registerGenericEvent_OnShopClose);

	//OnTradeBegin/onTradeEnd
	registerGlobalFunction("registerOnTradeBegin(string method [, int filter = nil], function callback)", &Manager::lua_registerGenericEvent_OnTradeBegin);
	registerGlobalFunction("registerOnPlayerTradeBegin(Player who, string method [, int filter = nil], function callback)", &Manager::lua_registerSpecificEvent_OnTradeBegin);
	registerGlobalFunction("registerOnTradeEnd(string method [, int filter = nil], function callback)", &Manager::lua_registerGenericEvent_OnTradeEnd);
	registerGlobalFunction("registerOnPlayerTradeEnd(Player who, string method [, int filter = nil], function callback)", &Manager::lua_registerSpecificEvent_OnTradeEnd);

	// OnCondition
	registerGlobalFunction("registerOnConditionEffect(string name, string method, function callback)", &Manager::lua_registerGenericEvent_OnConditionEffect);
	//registerGlobalFunction("registerOnCreatureCondition(Creature creature, string name, string method, function callback)", &Manager::lua_registerSpecificEvent_OnConditionEffect);

	// OnAttack
	registerGlobalFunction("registerOnAttack([string what = nil], string method, function callback)", &Manager::lua_registerGenericEvent_OnAttack);
	registerGlobalFunction("registerOnCreatureAttack(Creature creature, string method, function callback)", &Manager::lua_registerSpecificEvent_OnAttack);

	// OnDamage
	registerGlobalFunction("registerOnDamage([mixed what = nil], string method, function callback)", &Manager::lua_registerGenericEvent_OnDamage);
	registerGlobalFunction("registerOnCreatureDamage(Creature creature [, mixed what = nil], string method, function callback)", &Manager::lua_registerSpecificEvent_OnDamage);

	// OnKill
	registerGlobalFunction("registerOnKill([string what = nil], string method, function callback)", &Manager::lua_registerGenericEvent_OnKill);
	registerGlobalFunction("registerOnCreatureKill(Creature killer [, string what = nil], string method, function callback)", &Manager::lua_registerSpecificEvent_OnKill);
	registerGlobalFunction("registerOnKilled([string what = nil], string method, function callback)", &Manager::lua_registerGenericEvent_OnKilled);
	registerGlobalFunction("registerOnCreatureKilled(Creature creature [, string what = nil], string method, function callback)", &Manager::lua_registerSpecificEvent_OnKilled);

	// OnDeath
	registerGlobalFunction("registerOnDeathBy([string what = nil], string method, function callback)", &Manager::lua_registerGenericEvent_OnDeathBy);
	registerGlobalFunction("registerOnCreatureDeathBy(Creature killer [, string what = nil], string method, function callback)", &Manager::lua_registerSpecificEvent_OnDeathBy);
	registerGlobalFunction("registerOnDeath([string what = nil], string method, function callback)", &Manager::lua_registerGenericEvent_OnDeath);
	registerGlobalFunction("registerOnCreatureDeath(Creature creature [, string what = nil], string method, function callback)", &Manager::lua_registerSpecificEvent_OnDeath);


	registerGlobalFunction("stopListener(string listener_id)", &Manager::lua_stopListener);

	// Game/Map functions
	registerGlobalFunction("getParentTile(int x, int y, int z)", &Manager::lua_getTile);
	registerGlobalFunction("sendMagicEffect(position where, MagicEffect type)", &Manager::lua_sendMagicEffect);
	registerGlobalFunction("sendDistanceEffect([Creature c = nil], position from, position to, ShootEffect type)", &Manager::lua_sendDistanceEffect);
	registerGlobalFunction("sendAnimatedText(position where, int color, string text)", &Manager::lua_sendAnimatedText);

	registerGlobalFunction("sendMailTo(Item item, string player [, Town town])", &Manager::lua_sendMailTo);

	registerGlobalFunction("setGlobalValue(string key, string text)", &Manager::lua_setGlobalValue);
	registerGlobalFunction("getGlobalValue(string key)", &Manager::lua_getGlobalValue);

	registerGlobalFunction("getWorldType()", &Manager::lua_getWorldType);
	registerGlobalFunction("getWorldTime()", &Manager::lua_getWorldTime);
	registerGlobalFunction("getWorldUpTime()", &Manager::lua_getWorldUpTime);

	// Register the bitlib
	luaL_register(state, "bit", lua_BitReg);
}

///////////////////////////////////////////////////////////////////////////////
// Utility functions

int LuaState::lua_wait()
{
	// integer delay is ontop of stack
	pushString("WAIT");
	return lua_yield(state, 2);
}

int LuaState::lua_getConfigValue()
{
	std::string key = popString();

	if(key == "sql_user" || key == "sql_pass" || key == "_G"){
		throw Script::Error("Read-protected config value " + key + " accessed.");
	}

	g_config.getConfigValue(key, state);
	return 1;
}

int LuaState::lua_require_directory()
{
	std::string script_dir = g_config.getString(ConfigManager::DATA_DIRECTORY) + "scripts/";
	std::string dir = popString();
	if(!manager->loadDirectory(script_dir + dir)){
		throw Error("Failed to load directory " + dir + ".");
	}
	pushBoolean(true);
	return 1;
}

int LuaState::lua_get_thread_id()
{
	lua_State* L = lua_tothread(state, -1);
	lua_pop(state, 1);
	std::ostringstream os;
	os << L;
	pushString(os.str());
	return 1;
}

int LuaState::lua_stacktrace()
{
	lua_State* L = lua_tothread(state, -1);
	lua_pop(state, 1);

	std::string report;
	{
		// Local thread is OK here
		LuaThread lt(manager, L);
		report = lt.report();
	}
	pushString(report);
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Register Events

int LuaState::lua_registerGenericEvent_OnServerLoad()
{
	Listener_ptr listener(new Listener(ON_LOAD_LISTENER, boost::any(), *manager));

	environment->Generic.OnLoad.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnSay() {
	// Store callback
	insert(-4);

	std::string filter = popString();
	bool case_sensitive = popBoolean();
	std::string method = popString();

	OnSay::ScriptInformation si_onsay;
	if(method == "all") {
		si_onsay.method = OnSay::FILTER_ALL;
	}
	else if(method == "substring") {
		si_onsay.method = OnSay::FILTER_SUBSTRING;
	}
	else if(method == "beginning") {
		si_onsay.method = OnSay::FILTER_MATCH_BEGINNING;
	}
	else if(method == "exact") {
		si_onsay.method = OnSay::FILTER_EXACT;
	}
	else {
		throw Error("Invalid argument (2) 'method'");
	}
	si_onsay.filter = filter;
	si_onsay.case_sensitive = case_sensitive;

	boost::any p(si_onsay);
	Listener_ptr listener(new Listener(ON_SAY_LISTENER, p, *manager));

	// OnSay event list is sorted by length, from longest to shortest
	if(si_onsay.method == OnSay::FILTER_EXACT){
		// Insert at beginning
		environment->Generic.OnSay.insert(environment->Generic.OnSay.begin(), listener);
	}
	else if(si_onsay.method == OnSay::FILTER_ALL){
		// All comes very last
		environment->Generic.OnSay.push_back(listener);
	}
	else{
		if(environment->Generic.OnSay.empty()){
			environment->Generic.OnSay.push_back(listener);
		}
		else{
			bool registered = false;
			for(ListenerList::iterator listener_iter = environment->Generic.OnSay.begin(),
				end = environment->Generic.OnSay.end();
				listener_iter != end; ++listener_iter)
			{
				OnSay::ScriptInformation info = boost::any_cast<OnSay::ScriptInformation>((*listener_iter)->getData());

				if(si_onsay.method == OnSay::FILTER_MATCH_BEGINNING){
					// We should be inserted before substrings...
					if(info.method == OnSay::FILTER_SUBSTRING || info.method == OnSay::FILTER_ALL){
						environment->Generic.OnSay.insert(listener_iter, listener);
						registered = true;
						break;
					}

					if(info.filter.length() < si_onsay.filter.length()){
						environment->Generic.OnSay.insert(listener_iter, listener);
						registered = true;
						break;
					}
				}
				else{
					assert(si_onsay.method == OnSay::FILTER_SUBSTRING);
					// We should be inserted before generic...
					if(info.method == OnSay::FILTER_ALL){
						environment->Generic.OnSay.insert(listener_iter, listener);
						registered = true;
						break;
					}

					if(info.filter.length() < si_onsay.filter.length()){
						environment->Generic.OnSay.insert(listener_iter, listener);
						registered = true;
						break;
					}
				}
			}
			if(!registered)
				environment->Generic.OnSay.push_back(listener);
		}
	}
	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_OnSay() {
	// Store callback
	insert(-5);

	std::string filter = popString();
	bool case_sensitive = popBoolean();
	std::string method = popString();
	Creature* who = popCreature();

	// Callback is no the top of the stack

	OnSay::ScriptInformation si_onsay;
	if(method == "all") {
		si_onsay.method = OnSay::FILTER_ALL;
	}
	else if(method == "substring") {
		si_onsay.method = OnSay::FILTER_SUBSTRING;
	}
	else if(method == "beginning") {
		si_onsay.method = OnSay::FILTER_MATCH_BEGINNING;
	}
	else if(method == "exact") {
		si_onsay.method = OnSay::FILTER_EXACT;
	}
	else {
		throw Error("Invalid argument (2) 'method'");
	}
	si_onsay.filter = filter;
	si_onsay.case_sensitive = case_sensitive;

	// This here explains why boost is so awesome, thanks to our custom
	// delete function, the listener is cleanly stopped when all references
	// to it is removed. :)
	boost::any p(si_onsay);
	Listener_ptr listener(
		new Listener(ON_SAY_LISTENER, p, *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_OnHear() {
	// Store callback
	insert(-2);

	Creature* who = popCreature();

	// Callback is no the top of the stack

	OnHear::ScriptInformation si_onhear;

	// This here explains why boost is so awesome, thanks to our custom
	// delete function, the listener is cleanly stopped when all references
	// to it is removed. :)
	boost::any p(si_onhear);
	Listener_ptr listener(
		new Listener(ON_HEAR_LISTENER, p, *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnUseItem() {
	// Store callback
	insert(-3);

	int32_t id = popInteger();
	std::string method = popString();

	OnUseItem::ScriptInformation si_onuse;
	if(method == "itemid") {
		si_onuse.method = OnUseItem::FILTER_ITEMID;
	}
	else if(method == "actionid") {
		si_onuse.method = OnUseItem::FILTER_ACTIONID;
	}
	else {
		throw Error("Invalid argument (1) 'method'");
	}
	si_onuse.id = id;

	boost::any p(si_onuse);
	Listener_ptr listener(new Listener(ON_USE_ITEM_LISTENER, p, *manager));

	ListenerList* list = NULL;
	switch(si_onuse.method){
		case OnUseItem::FILTER_ITEMID: list = &environment->Generic.OnUseItem.ItemId[si_onuse.id]; break;
		case OnUseItem::FILTER_ACTIONID: list = &environment->Generic.OnUseItem.ActionId[si_onuse.id]; break;
		default: break; // impossible, crash
	}
	list->push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnJoinChannel() {
	boost::any p(0);
	Listener_ptr listener(new Listener(ON_OPEN_CHANNEL_LISTENER, p, *manager));

	environment->Generic.OnJoinChannel.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_OnJoinChannel() {
	// Store callback
	insert(-2);

	Player* who = popPlayer();

	boost::any p(0);
	Listener_ptr listener(
		new Listener(ON_OPEN_CHANNEL_LISTENER, p, *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnLeaveChannel() {
	boost::any p(0);
	Listener_ptr listener(new Listener(ON_CLOSE_CHANNEL_LISTENER, p, *manager));

	environment->Generic.OnLeaveChannel.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_OnLeaveChannel() {
	// Store callback
	insert(-2);

	Player* who = popPlayer();

	boost::any p(0);
	Listener_ptr listener(
		new Listener(ON_CLOSE_CHANNEL_LISTENER, p, *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnLogin() {
	boost::any p(0);
	Listener_ptr listener(new Listener(ON_LOGIN_LISTENER, p, *manager));

	environment->Generic.OnLogin.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnLogout() {
	boost::any p(0);
	Listener_ptr listener(new Listener(ON_LOGOUT_LISTENER, p, *manager));

	environment->Generic.OnLogout.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_OnLogout() {
	// Store callback
	insert(-2);

	Player* who = popPlayer();

	boost::any p(0);
	Listener_ptr listener(
		new Listener(ON_LOGOUT_LISTENER, p, *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnChangeOutfit() {
	boost::any p(0);
	Listener_ptr listener(new Listener(ON_CHANGE_OUTFIT_LISTENER, p, *manager));

	environment->Generic.OnChangeOutfit.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_OnChangeOutfit() {
	// Store callback
	insert(-2);

	Player* who = popPlayer();

	boost::any p(0);
	Listener_ptr listener(
		new Listener(ON_CHANGE_OUTFIT_LISTENER, p, *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnLookAtItem() {
	// Store callback
	insert(-3);

	uint32_t id = popInteger();
	std::string method = popString();

	OnLook::ScriptInformation si_onlook;
	if(method == "itemid") {
		si_onlook.method = OnLook::FILTER_ITEMID;
	}
	else if(method == "actionid") {
		si_onlook.method = OnLook::FILTER_ACTIONID;
	}
	else {
		throw Error("Invalid argument (1) 'method'");
	}
	si_onlook.id = id;

	boost::any p(si_onlook);
	Listener_ptr listener(new Listener(ON_LOOK_LISTENER, p, *manager));

	ListenerList* list = NULL;
	switch(si_onlook.method){
		case OnLook::FILTER_ITEMID: list = &environment->Generic.OnLook.ItemId[si_onlook.id]; break;
		case OnLook::FILTER_ACTIONID: list = &environment->Generic.OnLook.ActionId[si_onlook.id]; break;
		default: break; // impossible, crash
	}
	list->push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_OnLookAtCreature() {
	// Store callback
	insert(-2);

	Creature* who = popCreature();

	OnLook::ScriptInformation si_onlook;
	si_onlook.method = OnLook::FILTER_NONE;
	si_onlook.id = 0;

	// Listener is unbound automatically
	boost::any p(si_onlook);
	Listener_ptr listener(
		new Listener(ON_LOOKED_AT_LISTENER, p, *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_OnLook() {
	// Store callback
	insert(-2);

	Creature* who = popCreature();

	OnLook::ScriptInformation si_onlook;
	si_onlook.method = OnLook::FILTER_NONE;
	si_onlook.id = 0;

	// This here explains why boost is so awesome, thanks to our custom
	// delete function, the listener is cleanly stopped when all references
	// to it is removed. :)
	boost::any p(si_onlook);
	Listener_ptr listener(
		new Listener(ON_LOOK_LISTENER, p, *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnEquipItem() {
	// Store callback
	insert(-5);

	SlotPosition slotPos = popEnum<SlotPosition>();
	int32_t id = popInteger();
	std::string method = popString();
	bool postEvent = true;

	// nil means its triggered after the item has been equipped
	if(isNil()){
		pop();
	}
	else{
		std::string when = popString();
		if(when == "before"){
			postEvent = false;
		}
		else if(when == "after"){
			postEvent = true;
		}
		else {
			throw Error("Invalid argument (1) 'when'");
		}
	}

	OnEquipItem::ScriptInformation si_onequip;

	if(method == "itemid") {
		si_onequip.method = OnEquipItem::FILTER_ITEMID;
	}
	else if(method == "actionid") {
		si_onequip.method = OnEquipItem::FILTER_ACTIONID;
	}
	else {
		throw Error("Invalid argument (2) 'method'");
	}

	si_onequip.id = id;
	si_onequip.slotPos = slotPos;
	si_onequip.equip = true;
	si_onequip.postEvent = postEvent;

	boost::any p(si_onequip);
	Listener_ptr listener(new Listener(ON_EQUIP_ITEM_LISTENER, p, *manager));

	environment->Generic.OnEquipItem.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnDeEquipItem() {
	// Store callback
	insert(-5);

	SlotPosition slotPos = popEnum<SlotPosition>();
	int32_t id = popInteger();
	std::string method = popString();
	bool postEvent = true;

	// nil means its triggered after the item has been de-equipped
	if(isNil()){
		pop();
	}
	else{
		std::string when = popString();
		if(when == "before"){
			postEvent = false;
		}
		else if(when == "after"){
			postEvent = true;
		}
		else {
			throw Error("Invalid argument (1) 'when'");
		}
	}

	OnEquipItem::ScriptInformation si_ondeequip;

	if(method == "itemid") {
		si_ondeequip.method = OnEquipItem::FILTER_ITEMID;
	}
	else if(method == "actionid") {
		si_ondeequip.method = OnEquipItem::FILTER_ACTIONID;
	}
	else {
		throw Error("Invalid argument (2) 'method'");
	}

	si_ondeequip.id = id;
	si_ondeequip.slotPos = slotPos;
	si_ondeequip.equip = false;
	si_ondeequip.postEvent = postEvent;

	boost::any p(si_ondeequip);
	Listener_ptr listener(new Listener(ON_EQUIP_ITEM_LISTENER, p, *manager));

	environment->Generic.OnEquipItem.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_CreatureMove() {
	// Store callback
	insert(-2);

	Creature* who = popCreature();

	OnMoveCreature::ScriptInformation si_onmovecreature;
	si_onmovecreature.method = OnMoveCreature::FILTER_NONE;
	si_onmovecreature.id = 0;
	si_onmovecreature.slot = 0;
	si_onmovecreature.moveType = OnMoveCreature::TYPE_MOVE;

	boost::any p(si_onmovecreature);
	Listener_ptr listener(
		new Listener(ON_MOVE_CREATURE_LISTENER, p, *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_CreatureMoveIn() {
	// Store callback
	insert(-4);

	int32_t id = popInteger();
	std::string method = popString();
	Creature* who = popCreature();

	OnMoveCreature::ScriptInformation si_onmovecreature;
	if(method == "itemid") {
		si_onmovecreature.method = OnMoveCreature::FILTER_ITEMID;
	}
	else if(method == "actionid") {
		si_onmovecreature.method = OnMoveCreature::FILTER_ACTIONID;
	}
	else {
		throw Error("Invalid argument (1) 'method'");
	}
	si_onmovecreature.slot = 0;
	si_onmovecreature.id = id;
	si_onmovecreature.moveType = OnMoveCreature::TYPE_STEPIN;

	boost::any p(si_onmovecreature);
	Listener_ptr listener(
		new Listener(ON_MOVE_CREATURE_LISTENER, p, *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_CreatureMoveOut() {
	// Store callback
	insert(-4);

	int32_t id = popInteger();
	std::string method = popString();
	Creature* who = popCreature();

	OnMoveCreature::ScriptInformation si_onmovecreature;
	if(method == "itemid") {
		si_onmovecreature.method = OnMoveCreature::FILTER_ITEMID;
	}
	else if(method == "actionid") {
		si_onmovecreature.method = OnMoveCreature::FILTER_ACTIONID;
	}
	else {
		throw Error("Invalid argument (1) 'method'");
	}
	si_onmovecreature.slot = 0;
	si_onmovecreature.id = id;
	si_onmovecreature.moveType = OnMoveCreature::TYPE_STEPOUT;

	boost::any p(si_onmovecreature);
	Listener_ptr listener(
		new Listener(ON_MOVE_CREATURE_LISTENER, p, *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_CreatureMoveIn() {
	// Store callback
	insert(-3);

	int32_t id = popInteger();
	std::string method = popString();

	OnMoveCreature::ScriptInformation si_onmovecreature;
	if(method == "itemid") {
		si_onmovecreature.method = OnMoveCreature::FILTER_ITEMID;
	}
	else if(method == "actionid") {
		si_onmovecreature.method = OnMoveCreature::FILTER_ACTIONID;
	}
	else {
		throw Error("Invalid argument (1) 'method'");
	}
	si_onmovecreature.slot = 0;
	si_onmovecreature.id = id;
	si_onmovecreature.moveType = OnMoveCreature::TYPE_STEPIN;

	boost::any p(si_onmovecreature);
	Listener_ptr listener(new Listener(ON_MOVE_CREATURE_LISTENER, p, *manager));

	environment->Generic.OnMoveCreature.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_CreatureMoveOut() {
	// Store callback
	insert(-3);

	int32_t id = popInteger();
	std::string method = popString();

	OnMoveCreature::ScriptInformation si_onmovecreature;
	if(method == "itemid") {
		si_onmovecreature.method = OnMoveCreature::FILTER_ITEMID;
	}
	else if(method == "actionid") {
		si_onmovecreature.method = OnMoveCreature::FILTER_ACTIONID;
	}
	else {
		throw Error("Invalid argument (1) 'method'");
	}
	si_onmovecreature.slot = 0;
	si_onmovecreature.id = id;
	si_onmovecreature.moveType = OnMoveCreature::TYPE_STEPOUT;

	boost::any p(si_onmovecreature);
	Listener_ptr listener(new Listener(ON_MOVE_CREATURE_LISTENER, p, *manager));

	environment->Generic.OnMoveCreature.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnSpawn() {
	// Store callback
	insert(-2);

	std::string cname = popString();
	toLowerCaseString(cname);

	boost::any p(0);
	Listener_ptr listener(
		new Listener(ON_SPAWN_LISTENER, p, *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->Generic.OnSpawn[cname].push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnCreatureTurn() {
	Listener_ptr listener(new Listener(ON_LOOK_LISTENER, boost::any(), *manager));

	environment->Generic.OnTurn.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_OnCreatureTurn() {
	// Store callback
	insert(-2);

	Creature* who = popCreature();

	Listener_ptr listener(
		new Listener(ON_TURN_LISTENER, boost::any(), *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnMoveItem() {
	// Store callback
	insert(-6);

	bool isItemOnTile = popBoolean();
	bool isAddItem = popBoolean();
	int32_t id = popInteger();
	std::string method = popString();
	bool postEvent = true;

	// nil means its triggered after the item has been moved
	if(isNil()){
		pop();
	}
	else{
		std::string when = popString();
		if(when == "before"){
			postEvent = false;
		}
		else if(when == "after"){
			postEvent = true;
		}
		else {
			throw Error("Invalid argument (1) 'when'");
		}
	}

	OnMoveItem::ScriptInformation si_onmoveitem;

	if(method == "itemid") {
		si_onmoveitem.method = OnMoveItem::FILTER_ITEMID;
	}
	else if(method == "actionid") {
		si_onmoveitem.method = OnMoveItem::FILTER_ACTIONID;
	}
	else {
		throw Error("Invalid argument (2) 'method'");
	}

	si_onmoveitem.id = id;
	si_onmoveitem.postEvent = postEvent;
	si_onmoveitem.addItem = isAddItem;
	si_onmoveitem.isItemOnTile = isItemOnTile;

	boost::any p(si_onmoveitem);
	Listener_ptr listener(new Listener(ON_MOVE_ITEM_LISTENER, p, *manager));

	ListenerList* list = NULL;
	if(isItemOnTile){
		list = &environment->Generic.OnMoveItemOnItem;
	}
	else{
		switch(si_onmoveitem.method){
			case OnMoveItem::FILTER_ITEMID:   list = &environment->Generic.OnMoveItem.ItemId[id]; break;
			case OnMoveItem::FILTER_ACTIONID: list = &environment->Generic.OnMoveItem.ActionId[id]; break;
			default: break;
		}
	}
	list->push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_OnSpotCreature() {
	// Store callback
	insert(-2);

	Creature* who = popCreature();

	Listener_ptr listener(
		new Listener(ON_SPOT_CREATURE_LISTENER, boost::any(), *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_OnLoseCreature() {
	// Store callback
	insert(-2);

	Creature* who = popCreature();

	Listener_ptr listener(
		new Listener(ON_LOSE_CREATURE_LISTENER, boost::any(), *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_OnCreatureThink() {
	// Store callback
	insert(-2);

	Creature* who = popCreature();

	Listener_ptr listener(
		new Listener(ON_THINK_LISTENER, boost::any(), *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnAdvance() {
	OnAdvance::ScriptInformation si_onadvance;

	// We always receive to arguments
	// store callback
	insert(-2);

	// nil as first means no skill
	if(isNil()){
		si_onadvance.method = OnAdvance::FILTER_ALL;
		si_onadvance.skill = LEVEL_EXPERIENCE; // Unused, just don't leave it hanging
		pop();
	}
	else{
		si_onadvance.method = OnAdvance::FILTER_SKILL;
		si_onadvance.skill = popEnum<LevelType>();
	}

	boost::any p(si_onadvance);
	Listener_ptr listener(new Listener(ON_ADVANCE_LISTENER, p, *manager));

	// Add it to the listener list
	environment->Generic.OnAdvance.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener tag
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_OnAdvance() {
	OnAdvance::ScriptInformation si_onadvance;

	// store callback
	insert(-3);

	// Then comes the skill, nil means all
	if(isNil()){
		si_onadvance.method = OnAdvance::FILTER_ALL;
		si_onadvance.skill = LEVEL_EXPERIENCE; // Unused, just don't leave it hanging
		pop();
	}
	else{
		si_onadvance.method = OnAdvance::FILTER_SKILL;
		si_onadvance.skill = popEnum<LevelType>();
	}

	// Finally player
	Player* who = popPlayer();

	// Callback is now the top of the stack
	boost::any p(si_onadvance);
	Listener_ptr listener(
		new Listener(ON_ADVANCE_LISTENER, p, *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnShopPurchase() {
	boost::any p(0);
	Listener_ptr listener(new Listener(ON_SHOP_PURCHASE_LISTENER, p, *manager));

	environment->Generic.OnShopPurchase.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnShopSell() {
	boost::any p(0);
	Listener_ptr listener(new Listener(ON_SHOP_SELL_LISTENER, p, *manager));

	environment->Generic.OnShopSell.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnShopClose() {
	boost::any p(0);
	Listener_ptr listener(new Listener(ON_SHOP_CLOSE_LISTENER, p, *manager));

	environment->Generic.OnShopClose.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnTradeBegin() {
	// store callback
	insert(-3);

	OnTradeBegin::ScriptInformation si_ontrade;

	if(isNil()){
		si_ontrade.method = OnTradeBegin::FILTER_ALL;
		si_ontrade.id = 0; // Unused, just don't leave it hanging
		pop();
	}
	else{
		si_ontrade.id = popInteger();
	}

	std::string method = popString();

	if(method == "itemid") {
		si_ontrade.method = OnTradeBegin::FILTER_ITEMID;
	}
	else if(method == "actionid") {
		si_ontrade.method = OnTradeBegin::FILTER_ACTIONID;
	}
	else if(si_ontrade.method != OnTradeBegin::FILTER_ALL) {
		throw Error("Invalid argument (1) 'method'");
	}

	boost::any p(si_ontrade);
	Listener_ptr listener(new Listener(ON_TRADE_BEGIN_LISTENER, p, *manager));

	environment->Generic.OnTradeBegin.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_OnTradeBegin() {
	// store callback
	insert(-4);

	OnTradeBegin::ScriptInformation si_ontrade;

	if(isNil()){
		si_ontrade.method = OnTradeBegin::FILTER_ALL;
		si_ontrade.id = 0; // Unused, just don't leave it hanging
		pop();
	}
	else{
		si_ontrade.id = popInteger();
	}

	std::string method = popString();

	if(method == "itemid") {
		si_ontrade.method = OnTradeBegin::FILTER_ITEMID;
	}
	else if(method == "actionid") {
		si_ontrade.method = OnTradeBegin::FILTER_ACTIONID;
	}
	else if(si_ontrade.method != OnTradeBegin::FILTER_ALL) {
		throw Error("Invalid argument (1) 'method'");
	}

	Player* who = popPlayer();

	boost::any p(si_ontrade);
	Listener_ptr listener(
		new Listener(ON_TRADE_BEGIN_LISTENER, p, *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnTradeEnd() {
	// store callback
	insert(-3);

	OnTradeEnd::ScriptInformation si_ontrade;

	if(isNil()){
		si_ontrade.method = OnTradeEnd::FILTER_ALL;
		si_ontrade.id = 0; // Unused, just don't leave it hanging
		pop();
	}
	else{
		si_ontrade.id = popInteger();
	}

	std::string method = popString();

	if(method == "itemid") {
		si_ontrade.method = OnTradeEnd::FILTER_ITEMID;
	}
	else if(method == "actionid") {
		si_ontrade.method = OnTradeEnd::FILTER_ACTIONID;
	}
	else if(si_ontrade.method != OnTradeEnd::FILTER_ALL) {
		throw Error("Invalid argument (1) 'method'");
	}

	boost::any p(si_ontrade);
	Listener_ptr listener(new Listener(ON_TRADE_END_LISTENER, p, *manager));

	environment->Generic.OnTradeEnd.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_OnTradeEnd() {
	// store callback
	insert(-4);

	OnTradeEnd::ScriptInformation si_ontrade;

	if(isNil()){
		si_ontrade.method = OnTradeEnd::FILTER_ALL;
		si_ontrade.id = 0; // Unused, just don't leave it hanging
		pop();
	}
	else{
		si_ontrade.id = popInteger();
	}

	std::string method = popString();

	if(method == "itemid") {
		si_ontrade.method = OnTradeEnd::FILTER_ITEMID;
	}
	else if(method == "actionid") {
		si_ontrade.method = OnTradeEnd::FILTER_ACTIONID;
	}
	else if(si_ontrade.method != OnTradeEnd::FILTER_ALL) {
		throw Error("Invalid argument (1) 'method'");
	}

	Player* who = popPlayer();

	boost::any p(si_ontrade);
	Listener_ptr listener(
		new Listener(ON_TRADE_BEGIN_LISTENER, p, *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnConditionEffect() {

	// Tied to a condition effect by name and type (begin/tick/end)
	// Store callback
	insert(-3);

	std::string method = popString();
	std::string name = popString();

	OnConditionEffect::ScriptInformation si_onconditioneffect;
	si_onconditioneffect.name = name;

	if(method == "begin"){
		si_onconditioneffect.method = OnConditionEffect::FILTER_BEGIN;
	}
	else if(method == "end"){
		si_onconditioneffect.method = OnConditionEffect::FILTER_END;
	}
	else if(method == "tick"){
		si_onconditioneffect.method = OnConditionEffect::FILTER_TICK;
	}
	else{
		throw Error("Invalid argument (2) 'method'");
	}

	boost::any p(si_onconditioneffect);
	Listener_ptr listener(new Listener(ON_CONDITION_LISTENER, p, *manager));

	environment->Generic.OnConditionEffect.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnAttack() {
	// Tied to attacked creature, either by name, creature type or all attacks
	// Store callback
	insert(-3);

	std::string method = popString();
	std::string name = "";
	if(isNil(-1)){
		pop();
	}
	else{
		name = popString();
	}

	OnAttack::ScriptInformation si_onattack;

	if(method == "all"){
		si_onattack.method = OnAttack::FILTER_ALL;
	}
	else if(method == "name"){
		si_onattack.method = OnAttack::FILTER_NAME;
		si_onattack.name = name;
	}
	else if(method == "player"){
		si_onattack.method = OnAttack::FILTER_PLAYER;
	}
	else if(method == "attacked_name"){
		si_onattack.method = OnAttack::FILTER_ATTACKED_NAME;
		si_onattack.name = name;
	}
	else if(method == "attacked_player"){
		si_onattack.method = OnAttack::FILTER_ATTACKED_PLAYER;
	}
	else if(method == "attacked_actor"){
		si_onattack.method = OnAttack::FILTER_ATTACKED_ACTOR;
	}
	else if(method == "player_attack_player"){
		si_onattack.method = OnAttack::FILTER_PLAYER_ATTACK_PLAYER;
	}
	else if(method == "player_attack_actor"){
		si_onattack.method = OnAttack::FILTER_PLAYER_ATTACK_ACTOR;
	}
	else if(method == "actor_attack_actor"){
		si_onattack.method = OnAttack::FILTER_ACTOR_ATTACK_ACTOR;
	}
	else if(method == "actor_attack_player"){
		si_onattack.method = OnAttack::FILTER_ACTOR_ATTACK_PLAYER;
	}
	else {
		throw Error("Invalid argument (2) 'method'");
	}

	boost::any p(si_onattack);
	Listener_ptr listener(new Listener(ON_ATTACK_LISTENER, p, *manager));

	environment->Generic.OnAttack.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_OnAttack() {
	// Tied to a specific creature, and run each time that creature attacks something
	// Store callback
	insert(-3);

	std::string method = popString();
	Creature* who = popCreature();

	OnAttack::ScriptInformation si_onattack;
	si_onattack.method = OnAttack::FILTER_ALL;

	if(method == "all"){
		si_onattack.method = OnAttack::FILTER_ALL;
	}
	else if(method == "player"){
		si_onattack.method = OnAttack::FILTER_PLAYER;
	}
	else if(method == "attacked_player"){
		si_onattack.method = OnAttack::FILTER_ATTACKED_PLAYER;
	}
	else if(method == "attacked_actor"){
		si_onattack.method = OnAttack::FILTER_ATTACKED_ACTOR;
	}
	else if(method == "player_attack_player"){
		si_onattack.method = OnAttack::FILTER_PLAYER_ATTACK_PLAYER;
	}
	else if(method == "player_attack_actor"){
		si_onattack.method = OnAttack::FILTER_PLAYER_ATTACK_ACTOR;
	}
	else if(method == "actor_attack_actor"){
		si_onattack.method = OnAttack::FILTER_ACTOR_ATTACK_ACTOR;
	}
	else if(method == "actor_attack_player"){
		si_onattack.method = OnAttack::FILTER_ACTOR_ATTACK_PLAYER;
	}
	else {
		throw Error("Invalid argument (3) 'method'");
	}

	boost::any p(si_onattack);
	Listener_ptr listener(
		new Listener(ON_ATTACK_LISTENER, p, *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnDamage() {
	// Tied to damaged creature, either by name, damage type, creature type or all damage
	// Store callback
	insert(-3);

	std::string method = popString();
	std::string name;
	CombatType combatType;

	if(isNil(-1)) {
		pop();
	}
	if(isString(-1)){
		name = popString();
	}
	else if(isTable(-1)){
		combatType = popEnum<CombatType>();
	}
	else{
		throw Error("Mixed variable must be either string or table (was " + typeName() + ").");
	}

	OnDamage::ScriptInformation si_ondamage;
	si_ondamage.combatType = COMBAT_NONE;

	if(method == "all"){
		si_ondamage.method = OnDamage::FILTER_ALL;
	}
	else if(method == "name"){
		si_ondamage.method = OnDamage::FILTER_NAME;
		si_ondamage.name = name;
	}
	else if(method == "player"){
		si_ondamage.method = OnDamage::FILTER_PLAYER;
	}
	else if(method == "attacker_name"){
		si_ondamage.method = OnDamage::FILTER_ATTACKER_NAME;
		si_ondamage.name = name;
	}
	else if(method == "attacker_player"){
		si_ondamage.method = OnDamage::FILTER_ATTACKER_PLAYER;
	}
	else if(method == "attacker_actor"){
		si_ondamage.method = OnDamage::FILTER_ATTACKER_ACTOR;
	}
	else if(method == "player_damage_player"){
		si_ondamage.method = OnDamage::FILTER_PLAYER_DAMAGE_PLAYER;
	}
	else if(method == "player_damage_actor"){
		si_ondamage.method = OnDamage::FILTER_PLAYER_DAMAGE_ACTOR;
	}
	else if(method == "actor_damage_actor"){
		si_ondamage.method = OnDamage::FILTER_ACTOR_DAMAGE_ACTOR;
	}
	else if(method == "actor_damage_player"){
		si_ondamage.method = OnDamage::FILTER_ACTOR_DAMAGE_PLAYER;
	}
	else if(method == "type"){
		si_ondamage.method = OnDamage::FILTER_TYPE;
		si_ondamage.combatType = combatType;
	}
	else {
		throw Error("Invalid argument (2) 'method'");
	}

	boost::any p(si_ondamage);
	Listener_ptr listener(new Listener(ON_DAMAGE_LISTENER, p, *manager));

	environment->Generic.OnDamage.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_OnDamage() {
	// Tied to a specific creature, and run each time that creature is taking damage (or heals)
	// Store callback
	insert(-4);

	std::string method = popString();
	std::string name;
	CombatType combatType;

	if(isNil(-1)) {
		pop();
	}
	if(isString(-1)){
		name = popString();
	}
	else if(isTable(-1)){
		combatType = popEnum<CombatType>();
	}
	else{
		throw Error("Mixed variable value must be either string or table (was " + typeName() + ").");
	}

	Creature* who = popCreature();

	OnDamage::ScriptInformation si_ondamage;
	si_ondamage.combatType = COMBAT_NONE;

	if(method == "all"){
		si_ondamage.method = OnDamage::FILTER_ALL;
	}
	else if(method == "attacker_name"){
		si_ondamage.method = OnDamage::FILTER_ATTACKER_NAME;
		si_ondamage.name = name;
	}
	else if(method == "attacker_player"){
		si_ondamage.method = OnDamage::FILTER_ATTACKER_PLAYER;
	}
	else if(method == "attacker_actor"){
		si_ondamage.method = OnDamage::FILTER_ATTACKER_ACTOR;
	}
	else if(method == "type"){
		si_ondamage.method = OnDamage::FILTER_TYPE;
		si_ondamage.combatType = combatType;
	}
	else {
		throw Error("Invalid argument (3) 'method'");
	}

	boost::any p(si_ondamage);
	Listener_ptr listener(
		new Listener(ON_DAMAGE_LISTENER, p, *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnKill() {
	// Tied to killing creature, either by name, creature type or all kills
	// Store callback
	insert(-3);

	std::string method = popString();
	std::string name = "";
	if(isNil(-1)){
		pop();
	}
	else{
		name = popString();
	}

	OnKill::ScriptInformation si_onkill;

	if(method == "all"){
		si_onkill.method = OnKill::FILTER_ALL;
	}
	else if(method == "name"){
		si_onkill.method = OnKill::FILTER_NAME;
		si_onkill.name = name;
	}
	else if(method == "player"){
		si_onkill.method = OnKill::FILTER_PLAYER;
	}
	else if(method == "killer_name"){
		si_onkill.method = OnKill::FILTER_KILLER_NAME;
		si_onkill.name = name;
	}
	else if(method == "killer_player"){
		si_onkill.method = OnKill::FILTER_KILLER_PLAYER;
	}
	else if(method == "killer_actor"){
		si_onkill.method = OnKill::FILTER_KILLER_ACTOR;
	}
	else if(method == "player_kill_player"){
		si_onkill.method = OnKill::FILTER_PLAYER_KILL_PLAYER;
	}
	else if(method == "player_kill_actor"){
		si_onkill.method = OnKill::FILTER_PLAYER_KILL_ACTOR;
	}
	else if(method == "actor_kill_player"){
		si_onkill.method = OnKill::FILTER_ACTOR_KILL_PLAYER;
	}
	else if(method == "actor_kill_actor"){
		si_onkill.method = OnKill::FILTER_ACTOR_KILL_ACTOR;
	}
	else {
		throw Error("Invalid argument (2) 'method'");
	}

	boost::any p(si_onkill);
	Listener_ptr listener(new Listener(ON_KILL_LISTENER, p, *manager));

	environment->Generic.OnKill.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_OnKill() {
	// Tied to a specific creature, and run each time that creature kills something
	// Store callback
	insert(-4);

	std::string method = popString();
	std::string name;

	if(!isNil(-1)) {
		name = popString();
	}
	else{
		pop();
	}

	Creature* who = popCreature();

	OnKill::ScriptInformation si_onkill;

	if(method == "all"){
		si_onkill.method = OnKill::FILTER_ALL;
	}
	else if(method == "killer_name"){
		si_onkill.method = OnKill::FILTER_KILLER_NAME;
		si_onkill.name = name;
	}
	else if(method == "killer_player"){
		si_onkill.method = OnKill::FILTER_KILLER_PLAYER;
	}
	else if(method == "killer_actor"){
		si_onkill.method = OnKill::FILTER_KILLER_ACTOR;
	}
	else {
		throw Error("Invalid argument (3) 'method'");
	}

	boost::any p(si_onkill);
	Listener_ptr listener(
		new Listener(ON_KILL_LISTENER, p, *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnKilled() {
	// Tied to any dying creature, by name (or all dying creatures)
	// Store callback
	insert(-3);

	std::string method = popString();
	std::string name = "";
	if(isNil(-1)){
		pop();
	}
	else{
		name = popString();
	}

	OnKill::ScriptInformation si_onkill;

	if(method == "all"){
		si_onkill.method = OnKill::FILTER_ALL;
	}
	else if(method == "name"){
		si_onkill.method = OnKill::FILTER_NAME;
		si_onkill.name = name;
	}
	else if(method == "player"){
		si_onkill.method = OnKill::FILTER_PLAYER;
	}
	else if(method == "killer_name"){
		si_onkill.method = OnKill::FILTER_KILLER_NAME;
		si_onkill.name = name;
	}
	else if(method == "killer_player"){
		si_onkill.method = OnKill::FILTER_KILLER_PLAYER;
	}
	else if(method == "killer_actor"){
		si_onkill.method = OnKill::FILTER_KILLER_ACTOR;
	}
	else if(method == "player_kill_player"){
		si_onkill.method = OnKill::FILTER_PLAYER_KILL_PLAYER;
	}
	else if(method == "player_kill_actor"){
		si_onkill.method = OnKill::FILTER_PLAYER_KILL_ACTOR;
	}
	else if(method == "actor_kill_player"){
		si_onkill.method = OnKill::FILTER_ACTOR_KILL_PLAYER;
	}
	else if(method == "actor_kill_actor"){
		si_onkill.method = OnKill::FILTER_ACTOR_KILL_ACTOR;
	}
	else {
		throw Error("Invalid argument (2) 'method'");
	}

	boost::any p(si_onkill);
	Listener_ptr listener(new Listener(ON_KILLED_LISTENER, p, *manager));

	environment->Generic.OnKilled.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_OnKilled() {
	// Tied to a specific dying creature
	// Store callback
	insert(-4);

	std::string method = popString();
	std::string name;

	if(!isNil(-1)) {
		name = popString();
	}
	else{
		pop();
	}

	Creature* who = popCreature();

	OnKill::ScriptInformation si_onkill;

	if(method == "all"){
		si_onkill.method = OnKill::FILTER_ALL;
	}
	else if(method == "killer_name"){
		si_onkill.method = OnKill::FILTER_KILLER_NAME;
		si_onkill.name = name;
	}
	else if(method == "killer_player"){
		si_onkill.method = OnKill::FILTER_KILLER_PLAYER;
	}
	else if(method == "killer_actor"){
		si_onkill.method = OnKill::FILTER_KILLER_ACTOR;
	}
	else {
		throw Error("Invalid argument (3) 'method'");
	}

	boost::any p(si_onkill);
	Listener_ptr listener(
		new Listener(ON_KILLED_LISTENER, p, *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnDeathBy() {
	// Store callback
	insert(-3);

	std::string method = popString();
	std::string name = "";
	if(isNil(-1)){
		pop();
	}
	else{
		name = popString();
	}

	OnDeath::ScriptInformation si_ondeath;

	if(method == "all"){
		si_ondeath.method = OnDeath::FILTER_ALL;
	}
	else if(method == "name"){
		si_ondeath.method = OnDeath::FILTER_NAME;
		si_ondeath.name = name;
	}
	else if(method == "player"){
		si_ondeath.method = OnDeath::FILTER_PLAYER;
	}
	else if(method == "killer_name"){
		si_ondeath.method = OnDeath::FILTER_KILLER_NAME;
		si_ondeath.name = name;
	}
	else if(method == "killer_player"){
		si_ondeath.method = OnDeath::FILTER_KILLER_PLAYER;
	}
	else if(method == "killer_actor"){
		si_ondeath.method = OnDeath::FILTER_KILLER_ACTOR;
	}
	else if(method == "player_death_by_player"){
		si_ondeath.method = OnDeath::FILTER_PLAYER_DEATH_BY_PLAYER;
	}
	else if(method == "player_death_by_actor"){
		si_ondeath.method = OnDeath::FILTER_PLAYER_DEATH_BY_ACTOR;
	}
	else if(method == "actor_death_by_player"){
		si_ondeath.method = OnDeath::FILTER_ACTOR_DEATH_BY_PLAYER;
	}
	else if(method == "actor_death_by_actor"){
		si_ondeath.method = OnDeath::FILTER_ACTOR_DEATH_BY_ACTOR;
	}
	else {
		throw Error("Invalid argument (2) 'method'");
	}

	boost::any p(si_ondeath);
	Listener_ptr listener(new Listener(ON_DEATH_BY_LISTENER, p, *manager));

	environment->Generic.OnDeathBy.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_OnDeathBy() {
	// Store callback
	insert(-4);

	std::string method = popString();
	std::string name;

	if(!isNil(-1)) {
		name = popString();
	}
	else{
		pop();
	}

	Creature* who = popCreature();

	OnDeath::ScriptInformation si_ondeath;

	if(method == "all"){
		si_ondeath.method = OnDeath::FILTER_ALL;
	}
	else if(method == "killer_name"){
		si_ondeath.method = OnDeath::FILTER_KILLER_NAME;
		si_ondeath.name = name;
	}
	else if(method == "killer_player"){
		si_ondeath.method = OnDeath::FILTER_KILLER_PLAYER;
	}
	else if(method == "killer_actor"){
		si_ondeath.method = OnDeath::FILTER_KILLER_ACTOR;
	}
	else {
		throw Error("Invalid argument (3) 'method'");
	}

	boost::any p(si_ondeath);
	Listener_ptr listener(
		new Listener(ON_DEATH_BY_LISTENER, p, *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerGenericEvent_OnDeath() {
	// Store callback
	insert(-3);

	std::string method = popString();
	std::string name = "";
	if(isNil(-1)){
		pop();
	}
	else{
		name = popString();
	}

	OnDeath::ScriptInformation si_ondeath;

	if(method == "all"){
		si_ondeath.method = OnDeath::FILTER_ALL;
	}
	else if(method == "name"){
		si_ondeath.method = OnDeath::FILTER_NAME;
		si_ondeath.name = name;
	}
	else if(method == "player"){
		si_ondeath.method = OnDeath::FILTER_PLAYER;
	}
	else if(method == "killer_name"){
		si_ondeath.method = OnDeath::FILTER_KILLER_NAME;
		si_ondeath.name = name;
	}
	else if(method == "killer_player"){
		si_ondeath.method = OnDeath::FILTER_KILLER_PLAYER;
	}
	else if(method == "killer_actor"){
		si_ondeath.method = OnDeath::FILTER_KILLER_ACTOR;
	}
	else if(method == "player_death_by_player"){
		si_ondeath.method = OnDeath::FILTER_PLAYER_DEATH_BY_PLAYER;
	}
	else if(method == "player_death_by_actor"){
		si_ondeath.method = OnDeath::FILTER_PLAYER_DEATH_BY_ACTOR;
	}
	else if(method == "actor_death_by_player"){
		si_ondeath.method = OnDeath::FILTER_ACTOR_DEATH_BY_PLAYER;
	}
	else if(method == "actor_death_by_actor"){
		si_ondeath.method = OnDeath::FILTER_ACTOR_DEATH_BY_ACTOR;
	}
	else {
		throw Error("Invalid argument (2) 'method'");
	}

	boost::any p(si_ondeath);
	Listener_ptr listener(new Listener(ON_DEATH_LISTENER, p, *manager));

	environment->Generic.OnDeath.push_back(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

int LuaState::lua_registerSpecificEvent_OnDeath() {
	// Store callback
	insert(-3);

	std::string method = popString();
	std::string name;

	if(!isNil(-1)) {
		name = popString();
	}
	else{
		pop();
	}

	Creature* who = popCreature();

	OnDeath::ScriptInformation si_ondeath;

	if(method == "all"){
		si_ondeath.method = OnDeath::FILTER_ALL;
	}
	else if(method == "killer_name"){
		si_ondeath.method = OnDeath::FILTER_KILLER_NAME;
		si_ondeath.name = name;
	}
	else if(method == "killer_player"){
		si_ondeath.method = OnDeath::FILTER_KILLER_PLAYER;
	}
	else if(method == "killer_actor"){
		si_ondeath.method = OnDeath::FILTER_KILLER_ACTOR;
	}
	else {
		throw Error("Invalid argument (3) 'method'");
	}

	boost::any p(si_ondeath);
	Listener_ptr listener(
		new Listener(ON_DEATH_LISTENER, p, *manager),
		boost::bind(&Listener::deactivate, _1));

	environment->registerSpecificListener(listener);
	who->addListener(listener);

	// Register event
	setRegistryItem(listener->getLuaTag());

	// Return listener
	pushString(listener->getLuaTag());
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Stop listener function

int LuaState::lua_stopListener() {
	std::string listener_id = popString();

	size_t _pos = listener_id.find("_") + 1;
	if(_pos == std::string::npos) {
		throw Error("Invalid argument to stopListener");
	}
	size_t _2pos = listener_id.find("_", _pos);
	if(_2pos == std::string::npos) {
		throw Error("Invalid argument to stopListener");
	}

	std::istringstream is(listener_id.substr(_pos, _2pos));
	uint32_t id, type;
	is >> type;
	if(!is) {
		pushBoolean(false);
		return 1;
	}
	is.str(listener_id.substr(_2pos+1));
	is >> id;
	if(!is) {
		pushBoolean(false);
		return 1;
	}
	pushBoolean(environment->stopListener((ListenerType)type, id));
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Member functions
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Class Event

int LuaState::lua_Event_skip() {
	// Event table is ontop of stack
	setField(-1, "skipped", false);
	return 1;
}

int LuaState::lua_Event_propagate() {
	// Event table is ontop of stack
	setField(-1, "skipped", true);
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Class Thing

int LuaState::lua_Thing_getPosition()
{
	Thing* thing = popThing();
	pushPosition(thing->getPosition());
	return 1;
}

int LuaState::lua_Thing_getX()
{
	Thing* thing = popThing();
	pushInteger(thing->getPosition().x);
	return 1;
}

int LuaState::lua_Thing_getY()
{
	Thing* thing = popThing();
	pushInteger(thing->getPosition().y);
	return 1;
}

int LuaState::lua_Thing_getZ()
{
	Thing* thing = popThing();
	pushInteger(thing->getPosition().z);
	return 1;
}

int LuaState::lua_Thing_getParentTile()
{
	Thing* thing = popThing();
	pushTile(thing->getParentTile());
	return 1;
}

int LuaState::lua_Thing_getParent()
{
	Thing* thing = popThing();
	Cylinder* parent = thing->getParent();
	if(parent){
		if(parent->getItem()){
			pushThing(parent->getItem());
		} else if(parent->getCreature()){
			pushThing(parent->getCreature());
		}
		else if(parent->getParentTile()) {
			pushTile(static_cast<Tile*>(parent));
		}
		else{
			//impossible
			pushNil();
		}
	}
	else {
		pushNil();
	}
	return 1;
}

int LuaState::lua_Thing_moveToPosition()
{
	Position pos = popPosition();
	Thing* thing = popThing();

	pushBoolean(g_game.internalTeleport(NULL, thing, pos) == RET_NOERROR);

	return 1;
}

int LuaState::lua_Thing_isMoveable()
{
	Thing* thing = popThing();
	pushBoolean(thing->isPushable());
	return 1;
}

int LuaState::lua_Thing_getName() {
	Thing* t = popThing();
	if(Creature* c = t->getCreature()) {
		pushString(c->getName());
	} else if(Item* i = t->getItem()) {
		pushString(i->getName());
	} else {
		pushString("");
	}
	return 1;
}

int LuaState::lua_Thing_getDescription()
{
	int32_t lookdistance = 0;
	if(getStackSize() > 1) {
		lookdistance = popInteger();
	}
	Thing* thing = popThing();
	pushString(thing->getDescription(lookdistance));
	return 1;
}

int LuaState::lua_Thing_destroy()
{
	Thing* thing = popThing();
	if(Item* item = thing->getItem()) {
		pushBoolean(g_game.internalRemoveItem(NULL, item) == RET_NOERROR);
	} else if(Creature* creature = thing->getCreature()) {
		if(Player* player = creature->getPlayer()) {
			player->kickPlayer();
			pushBoolean(true);
		} else {
			pushBoolean(g_game.removeCreature(creature));
		}
	} else {
		pushBoolean(false);
	}
	return 1;
}

int LuaState::lua_getThingByID()
{
	Script::ObjectID objid = (Script::ObjectID)popFloat();
	Thing* thing = environment->getThing(objid);
	pushThing(thing);
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Class Tile

int LuaState::lua_Tile_getThing()
{
	int32_t index = popInteger(); // lua indices start at 1, but we don't give a crap!
	Tile* tile = popTile();

	// -1 is top item
	if(index < 0) {
		index = tile->__getLastIndex() + index;
	}
	if(index > tile->__getLastIndex()) {
		throw Error("Tile:getThing : Index out of range!");
	}

	pushThing(tile->__getThing(index));
	return 1;
}

int LuaState::lua_Tile_getCreatures()
{
	Tile* tile = popTile();

	newTable();
	int n = 1;
	for(CreatureIterator it = tile->creatures_begin(), end_iter = tile->creatures_end(); it != end_iter; ++it, ++n){
		pushThing(*it);
		setField(-2, n);
	}
	return 1;
}

int LuaState::lua_Tile_getMoveableItems()
{
	Tile* tile = popTile();

	newTable();
	int n = 1;
	for(TileItemIterator iter = tile->items_begin(), end_iter = tile->items_end(); iter != end_iter; ++iter){
		if((*iter)->isMoveable()) {
			pushThing(*iter);
			setField(-2, n++);
		}
	}
	return 1;
}

int LuaState::lua_Tile_getItems()
{
	Tile* tile = popTile();

	newTable();
	int n = 1;
	if(tile->ground) {
		pushThing(tile->ground);
		setField(-2, n++);
	}

	for(TileItemIterator iter = tile->items_begin(), end_iter = tile->items_end(); iter != end_iter; ++iter, ++n){
		pushThing(*iter);
		setField(-2, n);
	}

	return 1;
}

int LuaState::lua_Tile_getItem()
{
	int32_t index = popInteger(); // lua indices start at 1, but we don't give a crap!
	Tile* tile = popTile();

	// -1 is top item
	int lastindex = tile->items_count() + (tile->ground != NULL ? 1 : 0);
	if(index < 0) {
		index = lastindex + index;
	}
	if(index > lastindex) {
		throw Error("Tile:getItem: Index out of range!");
	}
	assert(index >= 0);

	if(tile->ground){
		if(index == 0){
			pushThing(tile->ground);
			return 1;
		}

		--index;
	}

	uint32_t topItemSize = tile->items_topCount();
	if(uint32_t(index) < topItemSize){
		pushThing(tile->items_get(tile->items_downCount() + index));
		return 1;
	}

	index -= topItemSize;
	index -= uint32_t(tile->creatures_count());

	if(uint32_t(index) < tile->items_downCount()){
		pushThing(tile->items_get(index));
		return 1;
	}

	pushNil();
	return 1;
}

int LuaState::lua_Tile_getItemsWithActionID()
{
	int32_t aid = popInteger();
	Tile* tile = popTile();

	newTable();
	int n = 1;
	ItemVector v = tile->items_getListWithActionId(aid);
	for(ItemVector::iterator iter = v.begin(), end_iter = v.end(); iter != end_iter; ++iter, ++n){
		pushThing(*iter);
		setField(-2, n);
	}

	return 1;
}

int LuaState::lua_Tile_getItemsWithItemID()
{
	int32_t id = popInteger();
	Tile* tile = popTile();

	newTable();
	int n = 1;
	ItemVector v = tile->items_getListWithItemId(id);
	for(ItemVector::iterator iter = v.begin(), end_iter = v.end(); iter != end_iter; ++iter, ++n){
		pushThing(*iter);
		setField(-2, n);
	}

	return 1;
}

int LuaState::lua_Tile_getItemWithActionID()
{
	int32_t aid = popInteger();
	Tile* tile = popTile();

	pushThing(tile->items_getItemWithActionId(aid));
	return 1;
}

int LuaState::lua_Tile_getItemWithItemID()
{
	int32_t id = popInteger();
	Tile* tile = popTile();

	pushThing(tile->items_getItemWithItemId(id));
	return 1;
}

int LuaState::lua_Tile_getItemCount() {
	int32_t type = popInteger();
	Tile* tile = popTile();
	push(tile->__getItemTypeCount(type));
	return 1;
}

int LuaState::lua_Tile_queryAdd()
{
	int32_t flags = 0;
	if(getStackSize() > 2) {
		flags = popInteger();
	}
	Thing* thing = popThing();
	Tile* tile = popTile();

	pushEnum(tile->__queryAdd(0, thing, 1, flags));
	return 1;
}

int LuaState::lua_Tile_addItem()
{
	Item* item = popItem(Script::ERROR_PASS);
	if(item == NULL) {
		pushEnum(RET_NOTPOSSIBLE);
		pushBoolean(false);
		return 2;
	}

	Tile* tile = popTile();

	ReturnValue ret = g_game.internalMoveItem(NULL, item->getParent(), tile, INDEX_WHEREEVER, item, item->getItemCount(), NULL, FLAG_IGNOREBLOCKITEM | FLAG_IGNOREBLOCKCREATURE);
	pushEnum(ret);
	pushBoolean(ret == RET_NOERROR);
	return 2;
}

int LuaState::lua_Tile_hasProperty()
{
	TileProp prop = popEnum<TileProp>();
	Tile* tile = popTile();
	pushBoolean(tile && tile->hasFlag(prop));
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Class Condition

int LuaState::lua_createCondition()
{
	Condition* condition = Condition::createCondition("", 0, MECHANIC_NONE, COMBAT_NONE, 0);
	pushCondition(condition);
	return 1;
}

int LuaState::lua_Condition_destroy()
{
	Condition* condition = popCondition();
	/* REVSCRIPT TODO
	 Check if condition is attached to a creature and prevent destruction in that case
	if(condition->attached()){
		throw Error("Can not destroy ");
	}
	*/
	environment->removeObject(condition);
	delete condition;
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Condition_setName()
{
	std::string name = popString();
	Condition* condition = popCondition();
	condition->setName(name);
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Condition_setCombatType()
{
	CombatType combatType = popEnum<CombatType>();
	Condition* condition = popCondition();
	condition->setCombatType(combatType);
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Condition_setMechanicType()
{
	MechanicType mechanicType = popEnum<MechanicType>();
	Condition* condition = popCondition();
	condition->setMechanicType(mechanicType);
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Condition_setTicks()
{
	uint32_t ticks = popUnsignedInteger();
	Condition* condition = popCondition();
	condition->setTicks(ticks);
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Condition_setFlags()
{
	uint32_t flags = popUnsignedInteger();
	Condition* condition = popCondition();
	condition->setFlags(flags);
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Condition_addPeriodicHeal()
{
	int32_t rounds = popInteger();
	int32_t value = std::abs(popInteger());
	uint32_t interval = popUnsignedInteger();
	Condition* condition = popCondition();
	condition->addEffect(ConditionEffect::createPeriodicHeal(interval, value, rounds));
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Condition_addPeriodicDamage()
{
	int32_t rounds = popInteger();
	int32_t value = std::abs(popInteger());
	CombatType combatType = popEnum<CombatType>();
	uint32_t interval = popUnsignedInteger();
	Condition* condition = popCondition();
	condition->addEffect(ConditionEffect::createPeriodicDamage(interval, combatType, 0, 0, value, rounds));
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Condition_addAveragePeriodicDamage()
{
	int32_t startDamage = std::abs(popInteger());
	int32_t totalDamage = std::abs(popInteger());
	CombatType combatType = popEnum<CombatType>();
	uint32_t interval = popUnsignedInteger();
	Condition* condition = popCondition();
	condition->addEffect(ConditionEffect::createPeriodicDamage(interval, combatType, totalDamage, (totalDamage / startDamage), startDamage, 0));
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Condition_addModStamina()
{
	int32_t value = popInteger();
	uint32_t interval = popUnsignedInteger();
	Condition* condition = popCondition();
	condition->addEffect(ConditionEffect::createModStamina(interval, value));
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Condition_addRegenHealth()
{
	int32_t value = popInteger();
	uint32_t interval = popUnsignedInteger();
	Condition* condition = popCondition();
	condition->addEffect(ConditionEffect::createRegenHealth(interval, value));
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Condition_addRegenPercentHealth()
{
	int32_t percent = popInteger();
	PlayerStatType type = popEnum<PlayerStatType>();
	uint32_t interval = popUnsignedInteger();
	Condition* condition = popCondition();
	condition->addEffect(ConditionEffect::createRegenPercentHealth(interval, type, percent));
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Condition_addRegenMana()
{
	int32_t value = popInteger();
	uint32_t interval = popUnsignedInteger();
	Condition* condition = popCondition();
	condition->addEffect(ConditionEffect::createRegenMana(interval, value));
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Condition_addRegenPercentMana()
{
	int32_t percent = popInteger();
	PlayerStatType type = popEnum<PlayerStatType>();
	uint32_t interval = popUnsignedInteger();
	Condition* condition = popCondition();
	condition->addEffect(ConditionEffect::createRegenPercentMana(interval, type, percent));
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Condition_addRegenSoul()
{
	int32_t value = popInteger();
	uint32_t interval = popUnsignedInteger();
	Condition* condition = popCondition();
	condition->addEffect(ConditionEffect::createRegenSoul(interval, value));
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Condition_addRegenPercentSoul()
{
	int32_t percent = popInteger();
	PlayerStatType type = popEnum<PlayerStatType>();
	uint32_t interval = popUnsignedInteger();
	Condition* condition = popCondition();
	condition->addEffect(ConditionEffect::createRegenPercentSoul(interval, type, percent));
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Condition_addSpeed()
{
	int32_t value = popInteger();
	int32_t percent = popInteger();
	Condition* condition = popCondition();
	condition->addEffect(ConditionEffect::createModSpeed(percent, value));
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Condition_addModStat()
{
	int32_t value = popInteger();
	PlayerStatType type = popEnum<PlayerStatType>();
	Condition* condition = popCondition();
	condition->addEffect(ConditionEffect::createModStat(type, value));
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Condition_addModPercentStat()
{
	int32_t percent = popInteger();
	PlayerStatType type = popEnum<PlayerStatType>();
	Condition* condition = popCondition();
	condition->addEffect(ConditionEffect::createModPercentStat(type, percent));
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Condition_addModSkill()
{
	int32_t value = popInteger();
	SkillType type = popEnum<SkillType>();
	Condition* condition = popCondition();
	condition->addEffect(ConditionEffect::createModSkill(type, value));
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Condition_addModPercentSkill()
{
	int32_t percent = popInteger();
	SkillType type = popEnum<SkillType>();
	Condition* condition = popCondition();
	condition->addEffect(ConditionEffect::createModPercentSkill(type, percent));
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Condition_addShapeShift()
{
	OutfitType outfit = popOutfit();
	Condition* condition = popCondition();
	condition->addEffect(ConditionEffect::createModShapeShift(outfit));
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Condition_addLight()
{
	int32_t color = popInteger();
	int32_t level = popInteger();
	Condition* condition = popCondition();
	condition->addEffect(ConditionEffect::createModLight(level, color));
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Condition_addDispel()
{
	std::string name = popString();
	Condition* condition = popCondition();
	condition->addEffect(ConditionEffect::createModDispel(name));
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Condition_addScript()
{
	uint32_t interval = 0;
	if(!isNil(-1))
		interval = popUnsignedInteger();
	else{
		pop();
	}
	std::string name = popString();
	Condition* condition = popCondition();
	condition->addEffect(ConditionEffect::createModScript(name, interval));
	pushBoolean(true);
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Class Creature

int LuaState::lua_Creature_getID()
{
	Creature* creature = popCreature();
	pushUnsignedInteger(creature->getID());
	return 1;
}

int LuaState::lua_Creature_getHealth()
{
	Creature* creature = popCreature();
	pushInteger(creature->getHealth());
	return 1;
}

int LuaState::lua_Creature_getHealthMax()
{
	Creature* creature = popCreature();
	pushInteger(creature->getMaxHealth());
	return 1;
}

int LuaState::lua_Creature_setHealth()
{
	int32_t newval = popInteger();
	Creature* c = popCreature();

	if(newval >= c->getHealth()){
		g_game.combatChangeHealth(COMBAT_HEALING, NULL, c, newval - c->getHealth(), false);
	}
	else{
		g_game.combatChangeHealth(COMBAT_UNDEFINEDDAMAGE, NULL, c, newval - c->getHealth(), false);
	}

	push(true);
	return 1;
}

int LuaState::lua_Creature_addSummon()
{
	Actor* summon = popActor();
	Creature* creature = popCreature();

	if(summon == creature)
		throw Script::Error("A creature can not be a summon of itself.");
	if(creature != summon->getMaster()){
		// Fix for being able to add summons belonging to a player already
		if(summon->isPlayerSummon())
			summon->getMaster()->removeSummon(summon);

		summon->convinceCreature(creature);
	}
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Creature_getOrientation()
{
	Creature* creature = popCreature();
	pushEnum(creature->getDirection());
	return 1;
}

int LuaState::lua_Creature_getNameDescription()
{
	Creature* creature = popCreature();
	pushString(creature->getNameDescription());
	return 1;
}

int LuaState::lua_Creature_walk()
{
	Direction ndir = popEnum<Direction>();
	Creature* creature = popCreature();

	switch(ndir.value()){
		case ::enums::NORTH:
		case ::enums::SOUTH:
		case ::enums::WEST:
		case ::enums::EAST:
		case ::enums::SOUTHWEST:
		case ::enums::NORTHWEST:
		case ::enums::NORTHEAST:
		case ::enums::SOUTHEAST:
			break;
		default:
			throw Error("Creature:walk : Invalid direction");
	}

	ReturnValue ret = g_game.internalMoveCreature(NULL, creature, (Direction)ndir);
	pushBoolean(ret == RET_NOERROR);
	return 1;
}

int LuaState::lua_Creature_getSpeed()
{
	Creature* creature = popCreature();
	pushInteger(creature->getSpeed());
	return 1;
}

int LuaState::lua_Creature_getArmor()
{
	Creature* creature = popCreature();
	pushInteger(creature->getArmor());
	return 1;
}

int LuaState::lua_Creature_getDefense()
{
	Creature* creature = popCreature();
	pushInteger(creature->getDefense());
	return 1;
}

int LuaState::lua_Creature_getBaseSpeed()
{
	Creature* creature = popCreature();
	pushInteger(creature->getBaseSpeed());
	return 1;
}

int LuaState::lua_Creature_isPushable()
{
	Creature* creature = popCreature();
	pushInteger(creature->isPushable());
	return 1;
}

int LuaState::lua_Creature_getTarget()
{
	Creature* creature = popCreature();
	pushThing(creature->getAttackedCreature());
	return 1;
}

int LuaState::lua_Creature_isImmuneToMechanic()
{
	Creature* creature = popCreature();
	MechanicType mechanicType = popEnum<MechanicType>();

	pushBoolean(creature->isImmune(mechanicType));
	return 1;
}

int LuaState::lua_Creature_isImmuneToCombat()
{
	Creature* creature = popCreature();
	CombatType combatType = popEnum<CombatType>();

	pushBoolean(creature->isImmune(combatType));
	return 1;
}

int LuaState::lua_Creature_getMechanicImmunities()
{
	Creature* creature = popCreature();

	newTable();
	for(MechanicType::iterator i = MechanicType::begin(); i != MechanicType::end(); ++i){
		pushBoolean(creature->isImmune(*i));
		setField(-2, i->toString());
	}
	return 1;
}

int LuaState::lua_Creature_getDamageImmunities()
{
	Creature* creature = popCreature();

	newTable();
	for(CombatType::iterator i = COMBAT_PHYSICALDAMAGE; i != CombatType::end(); ++i){
		pushBoolean(creature->isImmune(*i));
		setField(-2, i->toString());
	}
	return 1;
}

int LuaState::lua_Creature_setRawCustomValue()
{
	bool remove = false;
	std::string value;
	if(isNil(-1)) {
		pop();
		remove = true;
	}
	else if(isString(-1) || isNumber(-1)){
		value = popString();
	}
	else{
		throw Error("Raw creature custom value must be either string or number (was " + typeName() + ").");
	}
	std::string key = popString();
	Creature* creature = popCreature();

	if(remove)
		creature->eraseCustomValue(key);
	else
		creature->setCustomValue(key, value);

	pushBoolean(true);
	return 1;
}

int LuaState::lua_Creature_getRawCustomValue()
{
	std::string key = popString();
	Creature* creature = popCreature();
	std::string value;
	if(creature->getCustomValue(key, value)) {
		pushString(value);
	} else {
		pushNil();
	}
	return 1;
}

int LuaState::lua_Creature_internalAddCondition()
{
	Condition* condition = popCondition();
	Creature* creature = popCreature();
	bool ret = creature->addCondition(condition);
	pushBoolean(ret);
	return 1;
}

int LuaState::lua_internalCastSpell()
{
	bool blockedByShield = false;
	bool blockedByArmor = false;

	CombatEffect combatEffect;
	if(getStackSize() >= 9) {
		combatEffect.hitTextColor = popEnum<TextColor>();
	}
	if(getStackSize() >= 8) {
		combatEffect.hitEffect = popEnum<MagicEffect>();
	}
	if(getStackSize() >= 7) {
		combatEffect.showEffect = popBoolean();
	}
	if(getStackSize() >= 6) {
		blockedByArmor = popBoolean();
	}
	if(getStackSize() >= 5) {
		blockedByShield = popBoolean();
	}
	int32_t amount = popInteger();
	Creature* target = popCreature(ERROR_PASS);
	Creature* caster = popCreature(ERROR_PASS);
	CombatType combatType = popEnum<CombatType>();
	
	CombatSource combatSource(caster);
	if(g_game.combatBlockHit(combatType, combatSource, target, amount, blockedByShield, blockedByArmor)){
		pushBoolean(false);
		return 1;
	}

	bool result = false;
	if(combatType != COMBAT_MANADRAIN){
		result = g_game.combatChangeHealth(combatType, combatSource, combatEffect, target, amount);
	}
	else{
		result = g_game.combatChangeMana(combatSource, combatEffect, target, amount);
	}

	pushBoolean(result);
	return 1;
}

int LuaState::lua_Creature_say()
{
	std::string msg = popString();
	Creature* creature = popCreature();

	bool ret = g_game.internalCreatureSay(creature, SPEAK_SAY, msg);
	pushBoolean(ret);
	return 1;
}

int LuaState::lua_Creature_getOutfit()
{
	Creature* creature = popCreature();
	pushOutfit(creature->getCurrentOutfit());
	return 1;
}

int LuaState::lua_Creature_setOutfit()
{
	OutfitType outfit = popOutfit();
	Creature* creature = popCreature();
	g_game.internalCreatureChangeOutfit(creature, outfit);
	pushBoolean(true);
	return 1;
}

int LuaState::lua_getCreatureByName()
{
	std::string name = popString();
	pushThing(g_game.getCreatureByName(name));
	return 1 ;
}

int LuaState::lua_getCreaturesByName()
{
	std::string name = popString();
	std::vector<Creature*> creatures = g_game.getCreaturesByName(name);

	int n = 1;
	newTable();
	for(std::vector<Creature*>::iterator i = creatures.begin(); i != creatures.end(); ++i){
		pushThing(*i);
		setField(-2, n++);
	}
	return 1 ;
}

///////////////////////////////////////////////////////////////////////////////
// Class Actor

int LuaState::lua_createActor()
{
	Position p = popPosition();
	std::string name = popString();

	// Create an empty actor, with some default values
	CreatureType ct;
	OutfitType ot;
	ot.lookType = 130;
	ct.outfit(ot);

	Actor* a = Actor::create(ct);

	// Set some default attributes, so the actor can be spawned without issues
	a->getType().name(name);

	if(!g_game.placeCreature(a, p)){
		a->unRef();
		pushNil();
	}
	else
		pushThing(a);
	return 1;
}

int LuaState::lua_createMonster()
{
	Position p = popPosition();

	// Create a monster from a monster type
	Actor* a = Actor::create(popString());
	if(a)
		g_game.placeCreature(a, p);
	pushThing(a);
	return 1;
}

int LuaState::lua_Actor_setShouldReload()
{
	bool b = popBoolean();
	popActor()->shouldReload(b);
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Actor_getShouldReload()
{
	pushBoolean(popActor()->shouldReload());
	return 1;
}

int LuaState::lua_Actor_setAlwaysThink()
{
	bool b = popBoolean();
	popActor()->alwaysThink(b);
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Actor_getAlwaysThink()
{
	pushBoolean(popActor()->alwaysThink());
	return 1;
}

template <class T>
int Actor_modAttribute(LuaState* l, void (CreatureType::*mfp)(const T&)){
	T value = l->popValue<T>();
	Actor* actor = l->popActor();
	((actor->getType()).*(mfp))(value);
	l->pushBoolean(true);
	return 1;
}

template <class E, int32_t size_>
int Actor_modAttribute(LuaState* l, void (CreatureType::*mfp)(const Enum<E, size_>&)){
	Enum<E, size_> value = l->popEnum<Enum<E, size_> >();
	Actor* actor = l->popActor();
	((actor->getType()).*(mfp))(value);
	l->pushBoolean(true);
	return 1;
}

#define Actor_getAttribute(mfp) \
	push(popActor()->getType().mfp()), 1


int LuaState::lua_Actor_setArmor()
{
	return Actor_modAttribute(this, &CreatureType::armor);
}

int LuaState::lua_Actor_setDefense()
{
	return Actor_modAttribute(this, &CreatureType::defense);
}

int LuaState::lua_Actor_getExperienceWorth()
{
	return Actor_getAttribute(experience);
}

int LuaState::lua_Actor_setExperienceWorth()
{
	return Actor_modAttribute(this, &CreatureType::experience);
}

int LuaState::lua_Actor_getCanPushItems()
{
	return Actor_getAttribute(canPushItems);
}

int LuaState::lua_Actor_setCanPushItems()
{
	return Actor_modAttribute(this, &CreatureType::canPushItems);
}

int LuaState::lua_Actor_getCanPushCreatures()
{
	return Actor_getAttribute(canPushCreatures);
}

int LuaState::lua_Actor_setCanPushCreatures()
{
	return Actor_modAttribute(this, &CreatureType::canPushCreatures);
}

int LuaState::lua_Actor_setSpeed()
{
	int32_t value = popInteger();
	Actor* actor = popActor();
	g_game.changeSpeed(actor, value - actor->getSpeed());

	pushBoolean(true);
	return 1;
}

int LuaState::lua_Actor_getTargetDistance()
{
	return Actor_getAttribute(targetDistance);
}

int LuaState::lua_Actor_setTargetDistance()
{
	return Actor_modAttribute(this, &CreatureType::targetDistance);
}

int LuaState::lua_Actor_setMechanicImmunities()
{
	if(!isTable(-1)) {
		HandleError(Script::ERROR_THROW, "Attempt to treat non-table value as an immunity-table.");
		pop();
		pushBoolean(false);
		return 1;
	}

	MechanicType immunities = MECHANIC_NONE;
	for(MechanicType::iterator i = MechanicType::begin(); i != MechanicType::end(); ++i){
		getField(-1, i->toString());
		immunities |= (MechanicType)popInteger();
	}

	pop();

	Actor* actor = popActor();
	actor->getType().mechanicImmunities(immunities);
	//TODO:
	//actor->updateMapCache();

	pushBoolean(true);
	return 1;
}

int LuaState::lua_Actor_setDamageImmunities()
{
	if(!isTable(-1)) {
		HandleError(Script::ERROR_THROW, "Attempt to treat non-table value as an immunity-table.");
		pop();
		pushBoolean(false);
		return 1;
	}

	CombatType immunities = COMBAT_NONE;
	for(CombatType::iterator i = COMBAT_PHYSICALDAMAGE; i != CombatType::end(); ++i){
		getField(-1, i->toString());
		immunities |= (CombatType)popInteger();
	}

	pop();

	Actor* actor = popActor();
	actor->getType().damageImmunities(immunities);
	//actor->updateMapCache();

	pushBoolean(true);
	return 1;
}

int LuaState::lua_Actor_openShop()
{
	if(!isTable(-1)) {
		HandleError(Script::ERROR_THROW, "Attempt to treat non-table value as a shop list.");
		pop();
		pushBoolean(false);
		return 1;
	}

	ShopItemList list;

	// iterate over the table
	pushNil();
	while(iterateTable(-2)){
		ShopItem shopItem;
		getField(-1, "itemId");
		shopItem.itemId = popUnsignedInteger();
		getField(-1, "subType");
		shopItem.subType = popInteger();
		getField(-1, "buyPrice");
		shopItem.buyPrice = popUnsignedInteger();
		getField(-1, "sellPrice");
		shopItem.sellPrice = popUnsignedInteger();

		list.push_back(shopItem);

		pop(); // pop value
	}
	// Pop the 'list' table
	pop();

	Player* player = popPlayer();
	Actor* actor = popActor();

	player->sendShopWindow(list);
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Actor_closeShop()
{
	Player* player = popPlayer();
	Actor* actor = popActor();

	player->sendShopClose();
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Actor_getMaxSummons()
{
	return Actor_getAttribute(maxSummons);
}

int LuaState::lua_Actor_setMaxSummons()
{
	return Actor_modAttribute(this, &CreatureType::maxSummons);
}

int LuaState::lua_Actor_setName()
{
	std::string value = popString();
	Actor* actor = popActor();
	actor->getType().name(value);
	//TODO: notify all clients about the name change

	pushBoolean(true);
	return 1;
}

int LuaState::lua_Actor_setNameDescription()
{
	std::string value = popString();
	Actor* actor = popActor();
	actor->getType().nameDescription(value);
	actor->updateNameDescription();
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Actor_setStaticAttackChance()
{
	return Actor_modAttribute(this, &CreatureType::staticAttackChance);
}

int LuaState::lua_Actor_getStaticAttackChance()
{
	return Actor_getAttribute(staticAttackChance);
}

int LuaState::lua_Actor_setFleeHealth()
{
	return Actor_modAttribute(this, &CreatureType::fleeHealth);
}

int LuaState::lua_Actor_getFleeHealth()
{
	return Actor_getAttribute(fleeHealth);
}

int LuaState::lua_Actor_setPushable()
{
	return Actor_modAttribute(this, &CreatureType::pushable);
}

int LuaState::lua_Actor_setBaseSpeed()
{
	int32_t value = popInteger();
	Actor* actor = popActor();
	actor->getType().base_speed(value);
	actor->updateBaseSpeed();
	g_game.changeSpeed(actor, 0);

	pushBoolean(true);
	return 1;
}

int LuaState::lua_Actor_setMaxHealth()
{
	int32_t value = popInteger();
	Actor* actor = popActor();
	actor->getType().health_max(value);
	actor->updateMaxHealth();
	g_game.addCreatureHealth(actor);

	pushBoolean(true);
	return 1;
}

int LuaState::lua_Actor_getCorpseId()
{
	return Actor_getAttribute(corpseId);
}

int LuaState::lua_Actor_setRace()
{
	return Actor_modAttribute(this, &CreatureType::race);
}

int LuaState::lua_Actor_getRace()
{
	return Actor_getAttribute(race);
}

int LuaState::lua_Actor_isSummonable()
{
	return Actor_getAttribute(isSummonable);
}

int LuaState::lua_Actor_isConvinceable()
{
	return Actor_getAttribute(isConvinceable);
}

int LuaState::lua_Actor_isIllusionable()
{
	return Actor_getAttribute(isIllusionable);
}

int LuaState::lua_Actor_setCanBeAttacked()
{
	return Actor_modAttribute(this, &CreatureType::isAttackable);
}

int LuaState::lua_Actor_getCanBeAttacked()
{
	return Actor_getAttribute(isAttackable);
}

int LuaState::lua_Actor_setCanBeLured()
{
	return Actor_modAttribute(this, &CreatureType::isLureable);
}

int LuaState::lua_Actor_getCanBeLured()
{
	return Actor_getAttribute(isLureable);
}

int LuaState::lua_Actor_setLightLevel()
{
	int32_t value = popInteger();
	Actor* actor = popActor();
	actor->getType().lightLevel(value);
	actor->updateLightLevel();
	g_game.changeLight(actor);

	pushBoolean(true);
	return 1;
}

int LuaState::lua_Actor_getLightLevel()
{
	return Actor_getAttribute(lightLevel);
}

int LuaState::lua_Actor_setLightColor()
{
	int32_t value = popInteger();
	Actor* actor = popActor();
	actor->getType().lightColor(value);
	actor->updateLightColor();
	g_game.changeLight(actor);

	pushBoolean(true);
	return 1;
}

int LuaState::lua_Actor_getLightColor()
{
	return Actor_getAttribute(lightColor);
}

int LuaState::lua_Actor_getManaCost()
{
	return Actor_getAttribute(manaCost);
}

int LuaState::lua_Actor_setTarget()
{
	Creature* target = popCreature(ERROR_PASS);
	Actor* actor = popActor();
	pushBoolean(actor->setAttackedCreature(target));
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Class Player

int LuaState::lua_Player_getFood()
{
	Player* p = popPlayer();
	pushInteger(p->getFood());
	return 1;
}

int LuaState::lua_Player_getLevel()
{
	Player* p = popPlayer();
	push(p->getLevel());
	return 1;
}

int LuaState::lua_Player_getMagicLevel()
{
	Player* p = popPlayer();
	push(p->getMagicLevel());
	return 1;
}

int LuaState::lua_Player_getSkill()
{
	SkillType skill = popEnum<SkillType>();
	Player* p = popPlayer();
	push(p->getSkill(skill, SKILL_LEVEL));
	return 1;
}

int LuaState::lua_Player_advanceSkill()
{
	int32_t count = popInteger();
	SkillType skill = popEnum<SkillType>();
	Player* p = popPlayer();
	p->addSkillAdvance(skill, count);
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Player_isAutoWalking()
{
	Player* player = popPlayer();
	push(player->isAutoWalking());
	return 1;
}

int LuaState::lua_Player_isPremium()
{
	Player* player = popPlayer();
	push(player->isPremium());
	return 1;
}

int LuaState::lua_Player_getAccess()
{
	Player* p = popPlayer();
	push(p->getAccessLevel());
	return 1;
}

int LuaState::lua_Player_getMana()
{
	Player* p = popPlayer();
	push(p->getMana());
	return 1;
}

int LuaState::lua_Player_getManaMax()
{
	Player* p = popPlayer();
	push(p->getMaxMana());
	return 1;
}

int LuaState::lua_Player_setMana()
{
	int32_t newval = popInteger();
	Player* p = popPlayer();
	g_game.combatChangeMana(NULL, p, newval - p->getMana(), false);
	push(true);
	return 1;
}

int LuaState::lua_Player_addManaSpent()
{
	int32_t mana = popInteger();
	Player* p = popPlayer();
	p->addManaSpent(mana);
	push(true);
	return 1;
}

int LuaState::lua_Player_getVocationID()
{
	Player* p = popPlayer();
	push(p->getVocationId());
	return 1;
}

int LuaState::lua_Player_getVocationName()
{
	Player* p = popPlayer();
	Vocation* vocation = p->getVocation();
	push(vocation->getVocDescription());
	return 1;
}

int LuaState::lua_Player_getSoulPoints()
{
	Player* p = popPlayer();
	push(p->getPlayerInfo(PLAYERINFO_SOUL));
	return 1;
}

int LuaState::lua_Player_setSoulPoints()
{
	int32_t soul = popInteger();
	Player* p = popPlayer();
	p->changeSoul(soul - p->getPlayerInfo(PLAYERINFO_SOUL));
	push(true);
	return 1;
}

int LuaState::lua_Player_getFreeCap()
{
	Player* p = popPlayer();
	push(p->getFreeCapacity());
	return 1;
}

int LuaState::lua_Player_getMaximumCap()
{
	Player* p = popPlayer();
	push(p->getCapacity());
	return 1;
}

int LuaState::lua_Player_getGuildID()
{
	Player* p = popPlayer();
	push(p->getGuildId());
	return 1;
}

int LuaState::lua_Player_getGuildName()
{
	Player* p = popPlayer();
	push(p->getGuildName());
	return 1;
}

int LuaState::lua_Player_getGuildRank()
{
	Player* p = popPlayer();
	push(p->getGuildRank());
	return 1;
}

int LuaState::lua_Player_getGuildNick()
{
	Player* p = popPlayer();
	push(p->getGuildNick());
	return 1;
}

int LuaState::lua_Player_getSex()
{
	Player* p = popPlayer();
	push(p->getSex());
	return 1;
}

int LuaState::lua_Player_getTownID()
{
	Player* p = popPlayer();
	push(p->getTown());
	return 1;
}

int LuaState::lua_Player_getGUID()
{
	Player* p = popPlayer();
	push(p->getGUID());
	return 1;
}

int LuaState::lua_Player_getGroup()
{
	Player* p = popPlayer();
	push(p->getAccessGroup());
	return 1;
}

int LuaState::lua_Player_getPremiumDays()
{
	Player* p = popPlayer();
	push(p->getPremiumDays());
	return 1;
}

int LuaState::lua_Player_getLastLogin()
{
	Player* p = popPlayer();
	pushUnsignedInteger(p->getLastLoginSaved());
	return 1;
}

int LuaState::lua_Player_getSkullType()
{
	Player* p = popPlayer();
#ifdef __SKULLSYSTEM__
	push(p->getSkull());
#else
	pushInteger(0);
#endif
	return 1;
}
int LuaState::lua_Player_hasGroupFlag()
{
	PlayerFlag f = popEnum<PlayerFlag>();
	Player* player = popPlayer();
	if(!f.exists())
		throw Error("Invalid player flag passed to function Player.hasGroupFlag!");
	pushBoolean(player->hasFlag((PlayerFlags)f));
	return 1;
}

int LuaState::lua_Player_addExperience()
{
	int64_t exp = (int64_t)popFloat();
	Player* p = popPlayer();
	p->addExperience(exp);
	return 1;
}

int LuaState::lua_Player_getInventoryItem()
{
	SlotType slot(popEnum<SlotType>());
	Player* player = popPlayer();

	Item* i = player->getInventoryItem(slot);
	if(i)
		pushThing(i);
	else
		pushNil();

	return 1;
}

int LuaState::lua_Player_getSlot()
{
	Item* item = popItem();
	Player* player = popPlayer();
	int32_t index = player->__getIndexOfThing(item);
	if(index == -1){
		pushNil();
	}
	else{
		pushEnum(SlotType::fromInteger(index));
	}
	return 1;
}

int LuaState::lua_Player_getItemCount()
{
	int32_t type = popInteger();
	Player* player = popPlayer();
	push(player->__getItemTypeCount(type));
	return 1;
}

int LuaState::lua_Player_internalWalkTo()
{
	Position pos = popPosition();
	Player* player = popPlayer();

	std::list<Direction> listDir;
	if(g_game.getPathTo(player, pos, listDir)){
		g_dispatcher.addTask(createTask(boost::bind(&Game::playerAutoWalk,
			&g_game, player->getID(), listDir)));

		pushBoolean(true);
		return 1;
	}

	pushBoolean(false);
	return 1;
}

int LuaState::lua_Player_internalPickup()
{
	Item* item = popItem();
	Player* player = popPlayer();

	ReturnValue ret = g_game.internalMoveItem(NULL, item->getParent(), player, INDEX_WHEREEVER, item, item->getItemCount(), NULL);
	pushEnum(ret);
	pushBoolean(ret == RET_NOERROR);
	return 2;
}

int LuaState::lua_Player_countMoney()
{
	Player* player = popPlayer();
	pushInteger(g_game.getMoney(player));
	return 1;
}

int LuaState::lua_Player_addMoney()
{
	int32_t amount = popInteger();
	Player* player = popPlayer();
	pushBoolean(g_game.addMoney(NULL, player, amount));
	return 1;
}

int LuaState::lua_Player_removeMoney()
{
	int32_t amount = popInteger();
	Player* player = popPlayer();
	pushBoolean(g_game.removeMoney(NULL, player, amount));
	return 1;
}

int LuaState::lua_Player_getItemTypeCount() {
	int32_t type = popInteger();
	Player* player = popPlayer();
	push(player->__getItemTypeCount(type));
	return 1;
}

int LuaState::lua_Player_setVocation()
{
	int32_t vocationID = popInteger();
	Player* player = popPlayer();

	if(g_vocations.getVocation(vocationID)) {
		player->setVocation(vocationID);
		pushBoolean(true);
	} else {
		pushBoolean(false);
	}
	return 1;
}

int LuaState::lua_Player_setTown()
{
	Town* town = popTown();
	Player* player = popPlayer();

	player->setMasterPos(town->getTemplePosition());
	player->setTown(town->getTownID());
	pushBoolean(true);

	return 1;
}

int LuaState::lua_Player_sendMessage()
{
	if(isString(-1)){
		std::string text = popString();
		MessageClass messageClass = popEnum<MessageClass>();
		Player* player = popPlayer();
		player->sendTextMessage(messageClass, text);
	}
	else if(isTable(-1)){
		ReturnValue ret = popEnum<ReturnValue>();
		MessageClass messageClass = popEnum<MessageClass>();
		Player* player = popPlayer();
		player->sendCancelMessage(ret);
	}
	else{
		throw Error("msg must be either a string or ReturnValue (was " + typeName() + ").");
	}
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Player_addItem()
{
	SlotType slot = SLOT_WHEREEVER;
	bool canDropOnMap = true;

	if(getStackSize() > 3)
		canDropOnMap = popBoolean();
	if(getStackSize() > 2)
		slot = popEnum<SlotType>();

	Item* item = popItem(ERROR_PASS);
	if(item == NULL) {
		pushEnum(RET_NOTPOSSIBLE);
		pushBoolean(false);
		return 2;
	}

	Player* player = popPlayer();

	ReturnValue ret = g_game.internalMoveItem(NULL, item->getParent(), player, (slot == SLOT_WHEREEVER ? INDEX_WHEREEVER : slot.value() ), item, item->getItemCount(), NULL);
	if(ret != RET_NOERROR && canDropOnMap){
		ret = g_game.internalMoveItem(NULL, item->getParent(), player->getParentTile(), INDEX_WHEREEVER, item, item->getItemCount(), NULL);
	}

	pushEnum(ret);
	pushBoolean(ret == RET_NOERROR);
	return 2;
}

int LuaState::lua_Player_removeItem()
{
	int32_t subtype = -1;
	int32_t count = 1;
	int32_t itemid = 0;
	if(getStackSize() > 3)
		count = popInteger();
	if(getStackSize() > 2)
		subtype = popInteger();
	itemid = popInteger();
	Player* player = popPlayer();

	const ItemType& it = Item::items[itemid];
	if(it.id == 0){
		pushBoolean(false);
		return 1;
	}
	if(it.stackable && subtype != -1 && count == 1)
		count = subtype;

	pushBoolean(g_game.removeItemOfType(NULL, player, itemid, count, subtype));
	return 1;
}

int LuaState::lua_getOnlinePlayers()
{
	newTable();
	int n = 1;
	for(AutoList<Player>::listiterator it = Player::listPlayer.list.begin(); it != Player::listPlayer.list.end(); ++it){
		if(!it->second->isRemoved()){
			pushThing(it->second);
			setField(-2, n++);
		}
	}
	return 1 ;
}

int LuaState::lua_getPlayerByName()
{
	std::string name = popString();
	pushThing(g_game.getPlayerByName(name));
	return 1 ;
}

int LuaState::lua_getPlayersByName()
{
	std::string name = popString();
	std::vector<Player*> players = g_game.getPlayersByName(name);

	int n = 1;
	newTable();
	for(std::vector<Player*>::iterator i = players.begin(); i != players.end(); ++i){
		pushThing(*i);
		setField(-2, n++);
	}
	return 1 ;
}

int LuaState::lua_getPlayerByNameWildcard()
{
	std::string name = popString();

	Player* p = NULL;
	ReturnValue ret = g_game.getPlayerByNameWildcard(name, p);

	pushEnum(ret);
	pushThing(p);
	return 2;
}

int LuaState::lua_getPlayersByNameWildcard()
{
	std::string name = popString();
	std::vector<Player*> players = g_game.getPlayersByNameWildcard(name);

	int n = 1;
	newTable();
	for(std::vector<Player*>::iterator i = players.begin(); i != players.end(); ++i){
		pushThing(*i);
		setField(-2, n++);
	}
	return 1 ;
}

///////////////////////////////////////////////////////////////////////////////
// Class Item

int LuaState::lua_createItem()
{
	int32_t count = -1;
	if(getStackSize() > 1) {
		if(!isNil(-1))
			count = popInteger();
		else
			pop();
	}
	int32_t id = popUnsignedInteger();

	Item* item = Item::CreateItem((uint16_t)id, (count < 0? 0 : count));
	item->addRef();
	pushThing(item);
	// It will be freed if not assigned to any parent
	g_game.FreeThing(item);
	return 1;
}

int LuaState::lua_Item_getItemID()
{
	Item* item = popItem();
	pushInteger(item->getID());
	return 1;
}

int LuaState::lua_Item_getLongName()
{
	Item* item = popItem();
	pushString(item->getLongName());
	return 1;
}

int LuaState::lua_Item_getCount()
{
	Item* item = popItem();
	if(item->isStackable())
		pushInteger(item->getItemCount());
	else if(item->isRune())
		pushInteger(item->getCharges());
	else
		pushInteger(1);
	return 1;
}

int LuaState::lua_Item_getSubtype()
{
	Item* item = popItem();
	if(item->isRune() || item->isFluidContainer() || item->isSplash())
		pushInteger(item->getSubType());
	else
		pushNil();
	return 1;
}

int LuaState::lua_Item_getWeight()
{
	Item* item = popItem();
	pushFloat(item->getWeight());
	return 1;
}

int LuaState::lua_Item_isPickupable()
{
	Item* item = popItem();
	pushBoolean(item->isPickupable());
	return 1;
}

// Attributes!

// Template function to prevent code duplication
template<typename T> inline void updateActionID(const std::string& key, Item* item, T value) {}

template<> inline void updateActionID<int32_t>(const std::string& key, Item* item, int32_t value)
{
	//This is to re-index the item if the item is placed on an IndexedTile.
	if(key == "aid")
		item->setActionId(value);
}

template<typename T>
int setItemAttribute(LuaState* state)
{
	T value = state->popValue<T>();
	std::string key = state->popString();
	Item* item = state->popItem();

	item->setAttribute(key, value);
	// Update any intrinistic attributes
	updateActionID<T>(key, item, value);

	state->pushBoolean(true);
	return 1;
}

#define exposeItemAttribute(Name, Type) \
int LuaState::lua_Item_set ## Name ## Attribute()                                   \
{                                                                                   \
	return setItemAttribute<Type>(this);                                            \
}

exposeItemAttribute(String, std::string)
exposeItemAttribute(Integer, int32_t)
exposeItemAttribute(Float, float)
exposeItemAttribute(Boolean, bool)


int LuaState::lua_Item_getRawAttribute()
{
	std::string key = popString();
	Item* item = popItem();

	boost::any value = item->getAttribute(key);

	if(value.empty())
		pushNil();
	else if(value.type() == typeid(std::string))
		push(boost::any_cast<std::string>(value));
	else if(value.type() == typeid(int32_t))
		push(boost::any_cast<int32_t>(value));
	else if(value.type() == typeid(float))
		push(boost::any_cast<float>(value));
	else if(value.type() == typeid(bool))
		push(boost::any_cast<bool>(value));
	else
		pushNil();
	return 1;
}

int LuaState::lua_Item_eraseAttribute()
{
	std::string key = popString();
	Item* item = popItem();

	item->eraseAttribute(key);

	pushBoolean(true);
	return 1;
}


int LuaState::lua_Item_setItemID()
{
	int32_t newcount = -1;
	if(getStackSize() > 2) {
		newcount = popInteger();
	}

	int32_t newid = popInteger();
	Item* item = popItem();

	const ItemType& it = Item::items[newid];
	if(it.group == ITEM_GROUP_DEPRECATED || it.id == 0){
		throw Error("Item.setItemID : item ID provided");
	}

	if((it.stackable && newcount > 100) || newcount < -1){
		throw Error("Item.setItemID : Stack count is out of range.");
	}

	g_game.transformItem(NULL, item, newid, newcount);

	pushBoolean(true);
	return 1;
}

int LuaState::lua_Item_startDecaying()
{
	Item* item = popItem();
	g_game.startDecay(item);
	pushBoolean(true);
	return 1;
}

int LuaState::lua_Item_setCount()
{
	int32_t newcount = popInteger();
	Item* item = popItem();
	if(!item->isStackable() && !item->isRune()) {
		throw Error("Item.setCount: Item is not stackable!");
	}
	if(newcount < 1 || newcount > 100) {
		throw Error("Item.setCount: New count out of range!");
	}

	pushBoolean(g_game.transformItem(NULL, item, item->getID(), newcount) != NULL);
	return 1;
}

int LuaState::lua_Item_setSubtype()
{
	int32_t newtype = popInteger();
	Item* item = popItem();
	if(!item->isRune() && !item->isFluidContainer() && !item->isSplash()) {
		throw Error("Item.setSubtype: Item does not have a subtype!");
	}
	if(newtype < 0 || newtype > 100) {
		throw Error("Item.setSubtype: New subtype out of range!");
	}

	pushBoolean(g_game.transformItem(NULL, item, item->getID(), newtype) != NULL);
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Class Container

int LuaState::lua_Container_addItem()
{
	Item* item = popItem(ERROR_PASS);
	if(item == NULL) {
		pushEnum(RET_NOTPOSSIBLE);
		pushBoolean(false);
		return 2;
	}
	Container* container = popContainer();

	ReturnValue ret = g_game.internalMoveItem(NULL, item->getParent(), container, INDEX_WHEREEVER, item, item->getItemCount(), NULL, FLAG_IGNOREBLOCKITEM | FLAG_IGNOREBLOCKCREATURE);
	pushEnum(ret);
	pushBoolean(ret == RET_NOERROR);
	return 2;
}

int LuaState::lua_Container_getItem()
{
	int32_t index = popInteger();
	Container* container = popContainer();
	push(container->getItem(index));
	return 1;
}

int LuaState::lua_Container_getSize()
{
	Container* container = popContainer();
	pushInteger(container->size());
	return 1;
}

int LuaState::lua_Container_getCapacity()
{
	Container* container = popContainer();
	pushInteger(container->capacity());
	return 1;
}

int LuaState::lua_Container_getItems()
{
	Container* container = popContainer();

	newTable();
	int n = 1;
	for(ItemList::const_iterator it = container->getItems(); it != container->getEnd(); ++it){
		pushThing(*it);
		setField(-2, n++);
	}
	return 1;
}

int LuaState::lua_Container_getContentDescription()
{
	Container* container = popContainer();

	pushString(container->getContentDescription());
	return 1;
}

int LuaState::lua_getItemIDByName()
{
	std::string name = popString();

	int32_t itemid = Item::items.getItemIdByName(name);
	if(itemid == -1){
		pushNil();
	} else {
		pushUnsignedInteger(itemid);
	}
	return 1;
}

int LuaState::lua_getItemType()
{
	int32_t itemid = popInteger();

	newTable();

	const ItemType& it = Item::items[itemid];

	if(it.id == 0){
		pushNil();
		return 1;
	}

	setField(-1, "id", it.id);
	setField(-1, "clientID", it.clientId);

	setField(-1, "isGround", it.isGroundTile());
	setField(-1, "isContainer", it.isContainer());
	setField(-1, "isSplash", it.isSplash());
	setField(-1, "isFluidContainer", it.isFluidContainer());
	setField(-1, "isDoor", it.isDoor());
	setField(-1, "isMagicField", it.isMagicField());
	setField(-1, "isTeleport", it.isTeleport());
	setField(-1, "isKey", it.isKey());
	setField(-1, "isDepot", it.isDepot());
	setField(-1, "isMailbox", it.isMailbox());
	setField(-1, "isTrashHolder", it.isTrashHolder());
	setField(-1, "isRune", it.isRune());
	setField(-1, "isBed", it.isBed());
	setField(-1, "hasSpecialType", it.isRune() || it.isFluidContainer() || it.isSplash());

	setField(-1, "name", it.name);
	setField(-1, "article", it.article);
	setField(-1, "pluralName", it.pluralName);
	setField(-1, "description", it.description);
	setField(-1, "maxItems", it.maxItems);
	setField(-1, "weight", it.weight);
	setField(-1, "showCount", it.showCount);
	setField(-1, "weaponType", it.weaponType);
	setField(-1, "ammoType", it.ammoType);
	setField(-1, "shootType", it.shootType);
	setField(-1, "magicEffect", it.magicEffect);
	setField(-1, "attack", it.attack);
	setField(-1, "defense", it.defense);
	setField(-1, "extraDefense", it.extraDefense);
	setField(-1, "armor", it.armor);
	//setField(-1, "slotPosition", it.slotPosition);
	setField(-1, "isVertical", it.isVertical);
	setField(-1, "isHorizontal", it.isHorizontal);
	setField(-1, "isHangable", it.isHangable);
	setField(-1, "allowDistRead", it.allowDistRead);
	setField(-1, "clientCharges", it.clientCharges);
	setField(-1, "speed", it.speed);
	setField(-1, "decayTo", it.decayTo);
	setField(-1, "decayTime", it.decayTime);
	setField(-1, "stopTime", it.stopTime);
	setField(-1, "corpseType", it.corpseType);

	setField(-1, "canReadText", it.canReadText);
	setField(-1, "canWriteText", it.canWriteText);
	setField(-1, "maxTextLen", it.maxTextLen);
	setField(-1, "writeOnceItemID", it.writeOnceItemId);

	setField(-1, "stackable", it.stackable);
	setField(-1, "useable", it.useable);
	setField(-1, "moveable", it.moveable);
	setField(-1, "alwaysOnTop", it.alwaysOnTop);
	setField(-1, "alwaysOnTopOrder", it.alwaysOnTopOrder);
	setField(-1, "pickupable", it.pickupable);
	setField(-1, "rotateable", it.rotateable);
	setField(-1, "rotateTo", it.rotateTo);

	setField(-1, "runeMagicLevel", it.runeMagicLevel);
	setField(-1, "runeLevel", it.runeLevel);
	setField(-1, "runeSpellName", it.runeSpellName);

	//setField(-1, "wieldInfo", it.wieldInfo);
	setField(-1, "vocationString", it.vocationString);
	setField(-1, "minRequiredLevel", it.minRequiredLevel);
	setField(-1, "minRequiredMagicLevel", it.minRequiredMagicLevel);

	setField(-1, "lightLevel", it.lightLevel);
	setField(-1, "lightColor", it.lightColor);

	setField(-1, "floorChangeDown", it.floorChangeDown);
	setField(-1, "floorChangeNorth", it.floorChangeNorth);
	setField(-1, "floorChangeSouth", it.floorChangeSouth);
	setField(-1, "floorChangeEast", it.floorChangeEast);
	setField(-1, "floorChangeWest", it.floorChangeWest);
	setField(-1, "hasHeight", it.hasHeight);

	setField(-1, "blockSolid", it.blockSolid);
	setField(-1, "blockProjectile", it.blockProjectile);
	setField(-1, "blockPathFind", it.blockPathFind);
	setField(-1, "allowPickupable", it.allowPickupable);

	setField(-1, "bedPartnerDirection", it.bedPartnerDirection);
	setField(-1, "maleSleeperID", it.maleSleeperID);
	setField(-1, "femaleSleeperID", it.femaleSleeperID);
	setField(-1, "noSleeperID", it.noSleeperID);

	setField(-1, "transformEquipTo", it.transformEquipTo);
	setField(-1, "transformDeEquipTo", it.transformDeEquipTo);
	setField(-1, "showDuration", it.showDuration);
	setField(-1, "showCharges", it.showCharges);
	setField(-1, "charges", it.charges);
	setField(-1, "breakChance", it.breakChance);
	setField(-1, "hitChance", it.hitChance);
	setField(-1, "maxHitChance", it.maxHitChance);
	setField(-1, "shootRange", it.shootRange);
	setField(-1, "ammoAction", it.ammoAction);
	setField(-1, "fluidSource", it.fluidSource);

	return 1;
}

int LuaState::lua_getMaxItemType()
{
	push(Item::items.size());
	return 1;
}

int LuaState::lua_isValidItemID()
{
	int32_t id = popInteger();

	const ItemType& it = Item::items[(uint16_t)id];
	pushBoolean(id >= 100 && it.id != 0);
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Town

int LuaState::lua_Town_getTemplePosition()
{
	Town* town = popTown();
	pushPosition(town->getTemplePosition());
	return 1;
}

int LuaState::lua_Town_getID()
{
	Town* town = popTown();
	pushInteger(town->getTownID());
	return 1;
}

int LuaState::lua_Town_getName()
{
	Town* town = popTown();
	pushString(town->getName());
	return 1;
}

int LuaState::lua_Town_getHouse()
{
	Houses& houses = Houses::getInstance();

	uint32_t houseid = 0;
	std::string name;

	if(isNumber()){
		houseid = popInteger();
		std::ostringstream os;
		os << houseid;
		name = os.str();
	}
	else{
		name = popString();

		for(HouseMap::iterator it = houses.getHouseBegin(); it != houses.getHouseEnd(); ++it){
			if(it->second->getName() == name)
				houseid = it->second->getHouseId();
		}
		if(houseid == 0)
			throw Script::Error("Town.getHouse : No house by the name '" + name + "' exists.");
	}
	Town* town = popTown();

	House* house = houses.getHouse(houseid);

	if(!house)
		throw Script::Error("Town.getHouse : No house by the name '" + name + "' exists.");
	if(house->getTownId() != town->getTownID())
		throw Script::Error("Town.getHouse : No house by the name '" + name + "' exists belongs to the town " + town->getName());

	pushHouse(house);
	return 1;
}

int LuaState::lua_Town_getHouses()
{
	Town* town = popTown();

	Houses& houses = Houses::getInstance();

	newTable();
	int n = 1;
	for(HouseMap::iterator it = houses.getHouseBegin(); it != houses.getHouseEnd(); ++it){
		if(it->second->getTownId() == town->getTownID()){
			pushHouse(it->second);
			setField(-2, n++);
		}
	}
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// House

int LuaState::lua_House_cleanHouse()
{
	House* house = popHouse();
	house->cleanHouse();
	pushBoolean(true);
	return 1;
}

int LuaState::lua_House_getDoors()
{
	House* house = popHouse();

	newTable();
	int n = 1;
	for(HouseDoorList::iterator hit = house->getDoorBegin(), end = house->getDoorEnd();
		hit != end; ++hit)
	{
		pushThing(*hit);
		setField(-2, n++);
	}

	return 1;
}

int LuaState::lua_House_getExitPosition()
{
	House* house = popHouse();
	pushPosition(house->getEntryPosition());
	return 1;
}

int LuaState::lua_House_getID()
{
	House* house = popHouse();
	pushUnsignedInteger(house->getHouseId());
	return 1;
}

int LuaState::lua_House_getInvitedList()
{
	House* house = popHouse();
	std::string list;
	if(!house->getAccessList(GUEST_LIST, list))
		pushNil();
	// Function is overloaded in lua to return table
	pushString(list);
	return 1;
}

int LuaState::lua_House_getName()
{
	House* house = popHouse();
	pushString(house->getName());
	return 1;
}

int LuaState::lua_House_getPaidUntil()
{
	House* house = popHouse();
	pushUnsignedInteger(house->getPaidUntil());
	return 1;
}

int LuaState::lua_House_getRent()
{
	House* house = popHouse();
	pushInteger(house->getRent());
	return 1;
}

int LuaState::lua_House_getSubownerList()
{
	House* house = popHouse();
	std::string list;
	if(!house->getAccessList(SUBOWNER_LIST, list))
		pushNil();
	// Function is overloaded in lua to return table
	pushString(list);
	return 1;
}

int LuaState::lua_House_getTiles()
{
	House* house = popHouse();

	newTable();
	int n = 1;
	for(HouseTileList::iterator hit = house->getTileBegin(), end = house->getTileEnd();
		hit != end; ++hit)
	{
		pushTile(*hit);
		setField(-2, n++);
	}

	return 1;
}

int LuaState::lua_House_getTown()
{
	House* house = popHouse();

	pushTown(Towns::getInstance().getTown(house->getTownId()));
	return 1;
}
int LuaState::lua_House_kickPlayer()
{
	Player* who = popPlayer();
	House* house = popHouse();
	pushBoolean(house->kickPlayer(NULL, who->getName()));
	return 1;
}

int LuaState::lua_House_isInvited()
{
	Player* player = popPlayer();
	House* house = popHouse();
	pushBoolean(house->isInvited(player));
	return 1;
}

int LuaState::lua_House_setOwner()
{
	uint32_t guid = popUnsignedInteger();
	House* house = popHouse();

	house->setHouseOwner(guid);

	return 1;
}

int LuaState::lua_House_setInviteList()
{
	std::ostringstream list;

	// Flatten list
	pushNil();
	while(lua_next(state, -2) != 0) {
		list << popString() << "\n";
	}
	pop();

	House* house = popHouse();

	house->setAccessList(GUEST_LIST, list.str());

	return 1;
}

int LuaState::lua_House_setSubownerList()
{
	std::ostringstream list;

	// Flatten list
	pushNil();
	while(lua_next(state, -2) != 0) {
		list << popString() << "\n";
	}
	pop();

	House* house = popHouse();

	house->setAccessList(SUBOWNER_LIST, list.str());

	return 1;
}

int LuaState::lua_House_setPaidUntil()
{
	uint32_t until = popUnsignedInteger();
	House* house = popHouse();
	house->setPaidUntil(until);
	pushBoolean(true);
	return 1;
}


///////////////////////////////////////////////////////////////////////////////
// Channel

int LuaState::lua_Channel_getID()
{
	ChatChannel* channel = popChannel();
	push(channel->getId());
	return 1;
}

int LuaState::lua_Channel_getName()
{
	ChatChannel* channel = popChannel();
	push(channel->getName());
	return 1;
}

int LuaState::lua_Channel_getUsers()
{
	ChatChannel* channel = popChannel();
	newTable();
	int n = 1;
	for(UsersMap::const_iterator iter = channel->getUsers().begin(); iter != channel->getUsers().end(); ++iter)
	{
		pushThing(iter->second);
		setField(-2, n++);
	}
	return 1;
}

int LuaState::lua_Channel_addUser()
{
	Player* user = popPlayer();
	ChatChannel* channel = popChannel();
	g_game.playerOpenChannel(user->getID(), channel->getId());
	return 1;
}

int LuaState::lua_Channel_removeUser()
{
	Player* user = popPlayer();
	ChatChannel* channel = popChannel();
	g_game.playerCloseChannel(user->getID(), channel->getId());
	user->sendClosePrivate(channel->getId());
	return 1;
}

int LuaState::lua_Channel_talk()
{
	std::string text = popString();
	SpeakClass talkType = popEnum<SpeakClass>();
	/*Player* user = */popPlayer(ERROR_PASS);
	ChatChannel* channel = popChannel();
	channel->talk(NULL, talkType, text);
	pushBoolean(true);
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// (Class) Waypoint

int LuaState::lua_getWaypointByName()
{
	std::string name = popString();

	Waypoint_ptr wp = g_game.getMap()->waypoints.getWaypointByName(name);
	if(wp){
		pushWaypoint(wp);
	}
	else{
		pushNil();
	}
	return 1;
}

int LuaState::lua_Waypoint_getPosition()
{
	Waypoint_ptr wp = popWaypoint();

	pushPosition(wp->pos);
	return 1;
}

int LuaState::lua_Waypoint_getName()
{
	Waypoint_ptr wp = popWaypoint();

	pushString(wp->name);
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// (Class) Game

int LuaState::lua_sendMagicEffect()
{
	MagicEffect type = popEnum<MagicEffect>();
	Position pos = popPosition();

	g_game.addMagicEffect(pos, type);
	pushBoolean(true);
	return 1;
}

int LuaState::lua_sendDistanceEffect()
{
	ShootEffect type = popEnum<ShootEffect>();
	Position to = popPosition();
	Position from = popPosition();
	Creature* c = popCreature(ERROR_PASS);
	g_game.addDistanceEffect(c, from, to, type);
	pushBoolean(true);
	return 1;
}

int LuaState::lua_sendAnimatedText()
{
	std::string text = popString();
	uint32_t color = popUnsignedInteger();
	Position pos = popPosition();

	if(text.length() > 8)
		throw Error("Invalid value for parameter 'text': Animated text must be less than 8 characters long.");
	if(color > 255)
		throw Error("Invalid value for parameter 'color': Color is an unsigned integer between 0 and 255.");

	g_game.addAnimatedText(pos, color, text);
	pushBoolean(1);
	return 1;
}

int LuaState::lua_getTile()
{
	int32_t z = popInteger();
	int32_t y = popInteger();
	int32_t x = popInteger();
	pushTile(g_game.getParentTile(x, y, z));
	return 1;
}

int LuaState::lua_getTowns()
{
	Towns& towns = Towns::getInstance();

	newTable();
	int n = 1;
	for(TownMap::const_iterator i = towns.getTownBegin(); i != towns.getTownEnd(); ++i){
		pushTown(const_cast<Town*>((*i).second));
		setField(-2, n++);
	}
	return 1;
}

int LuaState::lua_sendMailTo()
{
	Town* town = NULL;
	if(getStackSize() > 2) {
		town = popTown();
	}

	uint32_t townId = 0;
	std::string name = popString();

	if(town == NULL){
		if(!IOPlayer::instance()->getDefaultTown(name, townId)){
			pushBoolean(false);
			return 1;
		}
	}
	else{
		townId = town->getTownID();
	}

	Item* item = popItem();
	if(item == NULL) {
		pushBoolean(false);
		return 1;
	}

	bool result = IOPlayer::instance()->sendMail(NULL, name, townId, item);
	pushBoolean(result);
	return 1;
}

int LuaState::lua_getGlobalValue()
{
	std::string key(popString());
	std::string v;
	if(g_game.getCustomValue(key, v))
		pushString(v);
	else
		pushNil();
	return 1;
}

int LuaState::lua_setGlobalValue()
{
	std::string value;
	if(isNil(-1)){
		pop();
		std::string key(popString());
		g_game.eraseCustomValue(key);
	}
	else{
		std::string value(popString());
		std::string key(popString());
		g_game.setCustomValue(key, value);
	}
	pushBoolean(true);
	return 1;
}

int LuaState::lua_getHouses()
{
	Houses& houses = Houses::getInstance();

	newTable();
	int n = 1;
	for(HouseMap::iterator it = houses.getHouseBegin(); it != houses.getHouseEnd(); ++it){
		pushHouse(it->second);
		setField(-2, n++);
	}

	return 1;
}

int LuaState::lua_getWorldType()
{
	pushEnum(g_game.getWorldType());
	return 1;
}

int LuaState::lua_getWorldTime()
{
	push(g_game.getLightHour());
	return 1;
}

int LuaState::lua_getWorldUpTime()
{
	push(Status::instance()->getUpTime());
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Bit definitions

int lua_BitNot(lua_State *L)
{
	int32_t number = (int32_t)lua_tonumber(L, -1);
	lua_pop(L, 1);
	lua_pushnumber(L, ~number);
	return 1;
}

int lua_BitUNot(lua_State *L)
{
	uint32_t number = (uint32_t)lua_tonumber(L, -1);
	lua_pop(L, 1);
	lua_pushnumber(L, ~number);
	return 1;
}

#define MULTIOP(type, name, op) \
	int lua_Bit##name(lua_State *L) { \
	int32_t i, n = lua_gettop(L); \
	type w = (type)lua_tonumber(L, -1); \
	lua_pop(L, 1); \
	for(i = 2; i <= n; ++i){ \
	w op (type)lua_tonumber(L, -1); \
	lua_pop(L, 1); \
	} \
	lua_pushnumber(L, w); \
	return 1; \
}

MULTIOP(int32_t, And, &=)
MULTIOP(int32_t, Or, |=)
MULTIOP(int32_t, Xor, ^=)
MULTIOP(uint32_t, UAnd, &=)
MULTIOP(uint32_t, UOr, |=)
MULTIOP(uint32_t, UXor, ^=)

#define SHIFTOP(type, name, op) \
	int lua_Bit##name(lua_State *L) { \
	type n2 = (type)lua_tonumber(L, -1), n1 = (type)lua_tonumber(L, -2); \
	lua_pop(L, 2); \
	lua_pushnumber(L, (n1 op n2)); \
	return 1; \
}

SHIFTOP(int32_t, LeftShift, <<)
SHIFTOP(int32_t, RightShift, >>)
SHIFTOP(uint32_t, ULeftShift, <<)
SHIFTOP(uint32_t, URightShift, >>)

