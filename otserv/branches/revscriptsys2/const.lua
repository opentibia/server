-------------------------------------------------------------------
-- OpenTibia - an opensource roleplaying game
----------------------------------------------------------------------
-- This program is free software; you can redistribute it and-or
-- modify it under the terms of the GNU General Public License
-- as published by the Free Software Foundation; either version 2
-- of the License, or (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this program; if not, write to the Free Software Foundation,
-- Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
----------------------------------------------------------------------

BeginEnumFile("const")

enum ("MagicEffect", 
	{"MAGIC_EFFECT_BLOOD            = 0x00", "redspark", "blood"},
	{"MAGIC_EFFECT_LOSE_ENERGY      = 0x01", "bluebubble"},
	{"MAGIC_EFFECT_PUFF             = 0x02", "puff", "poff"},
	{"MAGIC_EFFECT_BLOCKHIT         = 0x03", "yellowspark"},
	{"MAGIC_EFFECT_EXPLOSION_AREA   = 0x04", "explosionarea"},
	{"MAGIC_EFFECT_EXPLOSION_DAMAGE = 0x05", "explosion"},
	{"MAGIC_EFFECT_FIRE_AREA        = 0x06", "firearea"},
	{"MAGIC_EFFECT_YELLOW_RINGS     = 0x07", "yellowbubble"},
	{"MAGIC_EFFECT_POISON_RINGS     = 0x08", "greenbubble"},
	{"MAGIC_EFFECT_HIT_AREA         = 0x09", "blackspark"},
	{"MAGIC_EFFECT_TELEPORT         = 0x0A", "teleport"},
	{"MAGIC_EFFECT_ENERGY_DAMAGE    = 0x0B", "energy"},
	{"MAGIC_EFFECT_MAGIC_ENERGY     = 0x0C", "blueshimmer"},
	{"MAGIC_EFFECT_MAGIC_BLOOD      = 0x0D", "redshimmer"},
	{"MAGIC_EFFECT_MAGIC_POISON     = 0x0E", "greenshimmer"},
	{"MAGIC_EFFECT_HITBY_FIRE       = 0x0F", "fire"},
	{"MAGIC_EFFECT_POISON           = 0x10", "greenspark"},
	{"MAGIC_EFFECT_MORT_AREA        = 0x11", "mortarea"},
	{"MAGIC_EFFECT_SOUND_GREEN      = 0x12", "greennote"},
	{"MAGIC_EFFECT_SOUND_RED        = 0x13", "rednote"},
	{"MAGIC_EFFECT_POISON_AREA      = 0x14", "poison"},
	{"MAGIC_EFFECT_SOUND_YELLOW     = 0x15", "yellownote"},
	{"MAGIC_EFFECT_SOUND_PURPLE     = 0x16", "purplenote"},
	{"MAGIC_EFFECT_SOUND_BLUE       = 0x17", "bluenote"},
	{"MAGIC_EFFECT_SOUND_WHITE      = 0x18", "whitenote"},
	{"MAGIC_EFFECT_BUBBLES          = 0x19", "bubbles"},
	{"MAGIC_EFFECT_CRAPS            = 0x1A", "dice", "craps"},
	{"MAGIC_EFFECT_GIFT_WRAPS       = 0x1B", "giftwraps"},
	{"MAGIC_EFFECT_FIREWORK_YELLOW  = 0x1C", "yellowfirework"},
	{"MAGIC_EFFECT_FIREWORK_RED     = 0x1D", "redfirework"},
	{"MAGIC_EFFECT_FIREWORK_BLUE    = 0x1E", "bluefirework"},
	{"MAGIC_EFFECT_STUN             = 0x1F", "stun"},
	{"MAGIC_EFFECT_SLEEP            = 0x20", "sleep"},
	{"MAGIC_EFFECT_WATERCREATURE    = 0x21", "watercreature"},
	{"MAGIC_EFFECT_GROUNDSHAKER     = 0x22", "groundshaker"},
	{"MAGIC_EFFECT_HEARTS           = 0x23", "hearts"},
	{"MAGIC_EFFECT_FIREATTACK       = 0x24", "fireattack"},
	{"MAGIC_EFFECT_ENERGY_AREA      = 0x25", "energyarea"},
	{"MAGIC_EFFECT_SMALLCLOUDS      = 0x26", "smallclouds"},
	{"MAGIC_EFFECT_HOLYDAMAGE       = 0x27", "holydamage", "holy"},
	{"MAGIC_EFFECT_BIGCLOUDS        = 0x28", "bigclouds"},
	{"MAGIC_EFFECT_ICEAREA          = 0x29", "icearea"},
	{"MAGIC_EFFECT_ICETORNADO       = 0x2A", "icetornado"},
	{"MAGIC_EFFECT_ICEATTACK        = 0x2B", "iceattack"},
	{"MAGIC_EFFECT_STONES           = 0x2C", "stones"},
	{"MAGIC_EFFECT_SMALLPLANTS      = 0x2D", "smallplants"},
	{"MAGIC_EFFECT_CARNIPHILA       = 0x2E", "carniphila"},
	{"MAGIC_EFFECT_PURPLEENERGY     = 0x2F", "purpleenergy"},
	{"MAGIC_EFFECT_YELLOWENERGY     = 0x30", "yellowenergy"},
	{"MAGIC_EFFECT_HOLYAREA         = 0x31", "holyarea"},
	{"MAGIC_EFFECT_BIGPLANTS        = 0x32", "bigplants"},
	{"MAGIC_EFFECT_CAKE             = 0x33", "cake"},
	{"MAGIC_EFFECT_GIANTICE         = 0x34", "giantice"},
	{"MAGIC_EFFECT_WATERSPLASH      = 0x35", "watersplash"},
	{"MAGIC_EFFECT_PLANTATTACK      = 0x36", "plantattack"},
	{"MAGIC_EFFECT_TUTORIALARROW    = 0x37", "tutorialarrow"},
	{"MAGIC_EFFECT_TUTORIALSQUARE   = 0x38", "tutorialsquare"},
	{"MAGIC_EFFECT_MIRRORHORIZONTAL = 0x39", "mirrorhorizontal"},
	{"MAGIC_EFFECT_MIRRORVERTICAL   = 0x3A", "mirrorvertical"},
	{"MAGIC_EFFECT_SKULLHORIZONTAL  = 0x3B", "skullhorizontal"},
	{"MAGIC_EFFECT_SKULLVERTICAL    = 0x3C", "skullvertical"},
	{"MAGIC_EFFECT_ASSASSIN         = 0x3D", "assassin"},
	{"MAGIC_EFFECT_STEPSHORIZONTAL  = 0x3E", "stepshorizontal"},
	{"MAGIC_EFFECT_BLOODYSTEPS      = 0x3F", "bloodysteps"},
	{"MAGIC_EFFECT_STEPSVERTICAL    = 0x40", "stepsvertical"},
	{"MAGIC_EFFECT_YALAHARIGHOST    = 0x41", "yalaharighost"},
	{"MAGIC_EFFECT_BATS             = 0x42", "bats"},
	{"MAGIC_EFFECT_SMOKE            = 0x43", "smoke"},
	"MAGIC_EFFECT_NONE             = 0xFF",
	"MAGIC_EFFECT_UNK              = 0xFFFF"
)

enum("ShootEffect",
	{"SHOOT_EFFECT_SPEAR          = 0x00", "spear"},
	{"SHOOT_EFFECT_BOLT           = 0x01", "bolt"},
	{"SHOOT_EFFECT_ARROW          = 0x02", "arrow"},
	{"SHOOT_EFFECT_FIRE           = 0x03", "fire"},
	{"SHOOT_EFFECT_ENERGY         = 0x04", "energy"},
	{"SHOOT_EFFECT_POISONARROW    = 0x05", "poisonarrow"},
	{"SHOOT_EFFECT_BURSTARROW     = 0x06", "burstarrow"},
	{"SHOOT_EFFECT_THROWINGSTAR   = 0x07", "throwingstar"},
	{"SHOOT_EFFECT_THROWINGKNIFE  = 0x08", "throwingknife"},
	{"SHOOT_EFFECT_SMALLSTONE     = 0x09", "smallstone"},
	{"SHOOT_EFFECT_DEATH          = 0x0A", "death"},
	{"SHOOT_EFFECT_LARGEROCK      = 0x0B", "largerock"},
	{"SHOOT_EFFECT_SNOWBALL       = 0x0C", "snowball"},
	{"SHOOT_EFFECT_POWERBOLT      = 0x0D", "powerbolt"},
	{"SHOOT_EFFECT_POISONFIELD    = 0x0E", "poison"},
	{"SHOOT_EFFECT_INFERNALBOLT   = 0x0F", "infernalbolt"},
	{"SHOOT_EFFECT_HUNTINGSPEAR   = 0x10", "huntingspear"},
	{"SHOOT_EFFECT_ENCHANTEDSPEAR = 0x11", "enchantedspear"},
	{"SHOOT_EFFECT_REDSTAR        = 0x12", "redstar"},
	{"SHOOT_EFFECT_GREENSTAR      = 0x13", "greenstar"},
	{"SHOOT_EFFECT_ROYALSPEAR     = 0x14", "royalspear"},
	{"SHOOT_EFFECT_SNIPERARROW    = 0x15", "sniperarrow"},
	{"SHOOT_EFFECT_ONYXARROW      = 0x16", "onyxarrow"},
	{"SHOOT_EFFECT_PIERCINGBOLT   = 0x17", "piercingbolt"},
	{"SHOOT_EFFECT_WHIRLWINDSWORD = 0x18", "whirlwindsword"},
	{"SHOOT_EFFECT_WHIRLWINDAXE   = 0x19", "whirlwindaxe"},
	{"SHOOT_EFFECT_WHIRLWINDCLUB  = 0x1A", "whirlwindclub"},
	{"SHOOT_EFFECT_ETHEREALSPEAR  = 0x1B", "etherealspear"},
	{"SHOOT_EFFECT_ICE            = 0x1C", "ice"},
	{"SHOOT_EFFECT_EARTH          = 0x1D", "earth"},
	{"SHOOT_EFFECT_HOLY           = 0x1E", "holy"},
	{"SHOOT_EFFECT_SUDDENDEATH    = 0x1F", "suddendeath"},
	{"SHOOT_EFFECT_FLASHARROW     = 0x20", "flasharrow"},
	{"SHOOT_EFFECT_FLAMINGARROW   = 0x21", "flamingarrow"},
	{"SHOOT_EFFECT_SHIVERARROW    = 0x22", "shiverarrow"},
	{"SHOOT_EFFECT_ENERGYBALL     = 0x23", "energyball"},
	{"SHOOT_EFFECT_SMALLICE       = 0x24", "smallice"},
	{"SHOOT_EFFECT_SMALLHOLY      = 0x25", "smallholy"},
	{"SHOOT_EFFECT_SMALLEARTH     = 0x26", "smallearth"},
	{"SHOOT_EFFECT_EARTHARROW     = 0x27", "eartharrow"},
	{"SHOOT_EFFECT_EXPLOSION      = 0x28", "explosion"},
	{"SHOOT_EFFECT_CAKE           = 0x29", "cake"},
	{"SHOOT_EFFECT_WEAPONTYPE     = 0xFE"},
	{"SHOOT_EFFECT_NONE           = 0xFF"},
	{"SHOOT_EFFECT_UNK            = 0xFFFF"}
)

enum("SpeakClass",
	"SPEAK_SAY               = 0x01",	--normal talk
	"SPEAK_WHISPER           = 0x02",	--whispering - #w text
	"SPEAK_YELL              = 0x03",	--yelling - #y text
	"SPEAK_PRIVATE_PN        = 0x04",	--Player-to-NPC speaking(NPCs channel)
	"SPEAK_PRIVATE_NP        = 0x05",	--NPC-to-Player speaking
	"SPEAK_PRIVATE           = 0x06",	--Players speaking privately to players
	"SPEAK_CHANNEL_Y         = 0x07",	--Yellow message in chat
	"SPEAK_CHANNEL_W         = 0x08",	--White message in chat
	"SPEAK_RVR_CHANNEL       = 0x09",	--Reporting rule violation - Ctrl+R
	"SPEAK_RVR_ANSWER        = 0x0A",	--Answering report
	"SPEAK_RVR_CONTINUE      = 0x0B",	--Answering the answer of the report
	"SPEAK_BROADCAST         = 0x0C",	--Broadcast a message - #b
	"SPEAK_CHANNEL_R1        = 0x0D",	--Talk red on chat - #c
	"SPEAK_PRIVATE_RED       = 0x0E",	--Red private - @name@ text
	"SPEAK_CHANNEL_O         = 0x0F",	--Talk orange on text
	--"SPEAK_                 = 0x10",	--?
	"SPEAK_CHANNEL_R2        = 0x11",	--Talk red anonymously on chat - #d
	--"SPEAK_                 = 0x12",	--?
	"SPEAK_MONSTER_SAY       = 0x13",	--Talk orange
	"SPEAK_MONSTER_YELL      = 0x14"	--Yell orange
)

enum("MessageClass",
	"MSG_CLASS_FIRST             = 0x12",
	"MSG_STATUS_CONSOLE_RED      = MSG_CLASS_FIRST", --Red message in the console
	"MSG_STATUS_CONSOLE_ORANGE2  = 0x13", --Orange message in the console
	"MSG_STATUS_CONSOLE_ORANGE   = 0x14", --Orange message in the console
	"MSG_STATUS_WARNING          = 0x15", --Red message in game window and in the console
	"MSG_EVENT_ADVANCE           = 0x16", --White message in game window and in the console
	"MSG_EVENT_DEFAULT           = 0x17", --White message at the bottom of the game window and in the console
	"MSG_STATUS_DEFAULT          = 0x18", --White message at the bottom of the game window and in the console
	"MSG_INFO_DESCR              = 0x19", --Green message in game window and in the console
	"MSG_STATUS_SMALL            = 0x1A", --White message at the bottom of the game window"
	"MSG_STATUS_CONSOLE_BLUE     = 0x1B", --Blue message in the console
	"MSG_CLASS_LAST              = MSG_STATUS_CONSOLE_BLUE"
)

enum("FluidColor",
	"FLUID_EMPTY   = 0x00",
	"FLUID_BLUE    = 0x01",
	"FLUID_RED     = 0x02",
	"FLUID_BROWN   = 0x03",
	"FLUID_GREEN   = 0x04",
	"FLUID_YELLOW  = 0x05",
	"FLUID_WHITE   = 0x06",
	"FLUID_PURPLE  = 0x07"
)

enum("FluidType",
	{"FLUID_NONE = FLUID_EMPTY", "none"},
	{"FLUID_WATER       = FLUID_BLUE", "water"},
	{"FLUID_BLOOD       = FLUID_RED", "blood"},
	{"FLUID_BEER        = FLUID_BROWN", "beer"},
	{"FLUID_SLIME       = FLUID_GREEN", "slime"},
	{"FLUID_LEMONADE    = FLUID_YELLOW", "lemonade"},
	{"FLUID_MILK        = FLUID_WHITE", "milk"},
	{"FLUID_MANA        = FLUID_PURPLE", "mana"},

	{"FLUID_LIFE        = FLUID_RED + 8", "life"},
	{"FLUID_OIL         = FLUID_BROWN + 8", "oil"},
	{"FLUID_URINE       = FLUID_YELLOW + 8", "urine"},
	{"FLUID_COCONUTMILK = FLUID_WHITE + 8", "coconut", "coconutmilk"},
	{"FLUID_WINE        = FLUID_PURPLE + 8", "wine"},

	{"FLUID_MUD         = FLUID_BROWN + 16", "mud"},
	{"FLUID_FRUITJUICE  = FLUID_YELLOW + 16", "fruitjuice"},

	{"FLUID_LAVA        = FLUID_RED + 24", "lava"},
	{"FLUID_RUM         = FLUID_BROWN + 24", "rum"},
	{"FLUID_SWAMP       = FLUID_GREEN + 24", "swamp"}
)

definition([[
const FluidType reverseFluidMap[] = {
	FluidType(enums::FLUID_EMPTY),
	FluidType(enums::FLUID_WATER),
	FluidType(enums::FLUID_MANA),
	FluidType(enums::FLUID_BEER),
	FluidType(enums::FLUID_EMPTY),
	FluidType(enums::FLUID_BLOOD),
	FluidType(enums::FLUID_SLIME),
	FluidType(enums::FLUID_EMPTY),
	FluidType(enums::FLUID_LEMONADE),
	FluidType(enums::FLUID_MILK)
};
]])

enum("ClientFluidType",
	"CLIENTFLUID_EMPTY   = 0x00",
	"CLIENTFLUID_BLUE    = 0x01",
	"CLIENTFLUID_PURPLE  = 0x02",
	"CLIENTFLUID_BROWN_1 = 0x03",
	"CLIENTFLUID_BROWN_2 = 0x04",
	"CLIENTFLUID_RED     = 0x05",
	"CLIENTFLUID_GREEN   = 0x06",
	"CLIENTFLUID_BROWN   = 0x07",
	"CLIENTFLUID_YELLOW  = 0x08",
	"CLIENTFLUID_WHITE   = 0x09"
)

definition([[
const ClientFluidType fluidMap[] = {
	enums::CLIENTFLUID_EMPTY,
	enums::CLIENTFLUID_BLUE,
	enums::CLIENTFLUID_RED,
	enums::CLIENTFLUID_BROWN_1,
	enums::CLIENTFLUID_GREEN,
	enums::CLIENTFLUID_YELLOW,
	enums::CLIENTFLUID_WHITE,
	enums::CLIENTFLUID_PURPLE
};
]])

enum("SquareColor",
	"SQ_COLOR_NONE   = 256",
	"SQ_COLOR_BLACK  = 0"
)

enum("TextColor",
	"TEXTCOLOR_BLUE        = 5",
	"TEXTCOLOR_LIGHTBLUE   = 35",
	"TEXTCOLOR_LIGHTGREEN  = 30",
	"TEXTCOLOR_PURPLE      = 83",
	"TEXTCOLOR_LIGHTGREY   = 129",
	"TEXTCOLOR_DARKRED     = 144",
	"TEXTCOLOR_RED         = 180",
	"TEXTCOLOR_ORANGE      = 198",
	"TEXTCOLOR_YELLOW      = 210",
	"TEXTCOLOR_WHITE_EXP   = 215",
	"TEXTCOLOR_NONE        = 255",
	"TEXTCOLOR_UNK         = 256"
)

enum({name="IconType", bitmask=true},
	{"ICON_NONE", null=true},
	"ICON_POISON",
	"ICON_BURN",
	"ICON_ENERGY",
	"ICON_DRUNK",
	"ICON_MANASHIELD",
	"ICON_PARALYZE",
	"ICON_HASTE",
	"ICON_SWORDS",
	"ICON_DROWNING",
	"ICON_FREEZING",
	"ICON_DAZZLED",
	"ICON_CURSED",
	"ICON_PARTY_BUFF",
	"ICON_PZBLOCK",
	"ICON_PZ"
)

enum("WeaponType",
	"WEAPON_NONE     = 0",
	"WEAPON_SWORD    = 1",
	"WEAPON_CLUB     = 2",
	"WEAPON_AXE      = 3",
	"WEAPON_SHIELD   = 4",
	"WEAPON_DIST     = 5",
	"WEAPON_WAND     = 6",
	"WEAPON_AMMO     = 7"
)

enum("AmmunitionType",
	{"AMMO_NONE           = 0", "none"},
	{"AMMO_BOLT           = 1", "bolt", "piercingbolt", "powerbolt", "infernalbolt"},
	{"AMMO_ARROW          = 2", "arrow", "poisonarrow", "burstarrow", "sniperarrow", "onyxarrow", "flasharrow", "flamingarrow", "shiverarrow", "eartharrow"},
	{"AMMO_SPEAR          = 3", "spear", "huntingspear", "enchantedspear", "etherealspear", "royalspear"},
	{"AMMO_THROWINGSTAR   = 4", "throwingstar"},
	{"AMMO_THROWINGKNIFE  = 5", "throwingknife"},
	{"AMMO_STONE          = 6", "smallstone", "largerock"},
	{"AMMO_SNOWBALL       = 7", "snowball"}
)

enum("AmmunitionAction",
	{"AMMOACTION_NONE", "none"},
	{"AMMOACTION_REMOVECOUNT", "removecount"},
	{"AMMOACTION_REMOVECHARGE", "removecharge"},
	{"AMMOACTION_MOVE", "move"},
	{"AMMOACTION_MOVEBACK", "moveback"}
)

enum({name="WieldInformation", bitmask=true},
	{"WIELDINFO_NONE", null=true},
	"WIELDINFO_LEVEL",
	"WIELDINFO_MAGLV",
	"WIELDINFO_VOCREQ",
	"WIELDINFO_PREMIUM "
)

enum("SkullType",
	"SKULL_NONE      = 0",
	"SKULL_YELLOW    = 1",
	"SKULL_GREEN     = 2",
	"SKULL_WHITE     = 3",
	"SKULL_RED       = 4",
	"SKULL_BLACK     = 5"
)

enum("PartyShieldType",
	"SHIELD_NONE = 0",
	"SHIELD_WHITEYELLOW = 1",
	"SHIELD_WHITEBLUE = 2",
	"SHIELD_BLUE = 3",
	"SHIELD_YELLOW = 4",
	"SHIELD_BLUE_SHAREDEXP = 5",
	"SHIELD_YELLOW_SHAREDEXP = 6",
	"SHIELD_BLUE_NOSHAREDEXP_BLINK = 7",
	"SHIELD_YELLOW_NOSHAREDEXP_BLINK = 8",
	"SHIELD_BLUE_NOSHAREDEXP = 9",
	"SHIELD_YELLOW_NOSHAREDEXP = 10"
)

definition([[
enum ConstantItemID {
	ITEM_FIREFIELD_PVP    = 1492,
	ITEM_FIREFIELD_NOPVP  = 1500,

	ITEM_POISONFIELD_PVP    = 1496,
	ITEM_POISONFIELD_NOPVP  = 1503,

	ITEM_ENERGYFIELD_PVP    = 1495,
	ITEM_ENERGYFIELD_NOPVP  = 1504,

	ITEM_COINS_GOLD       = 2148,
	ITEM_COINS_PLATINUM   = 2152,
	ITEM_COINS_CRYSTAL    = 2160,

	ITEM_DEPOT            = 2594,
	ITEM_LOCKER1          = 2589,

	ITEM_MALE_CORPSE      = 6080,
	ITEM_FEMALE_CORPSE    = 6081,

	ITEM_MEAT             = 2666,
	ITEM_HAM              = 2671,
	ITEM_GRAPE            = 2681,
	ITEM_APPLE            = 2674,
	ITEM_BREAD            = 2689,
	ITEM_ROLL             = 2690,
	ITEM_CHEESE           = 2696,

	ITEM_FULLSPLASH       = 2016,
	ITEM_SMALLSPLASH      = 2019,

	ITEM_PARCEL           = 2595,
	ITEM_PARCEL_STAMPED   = 2596,
	ITEM_LETTER           = 2597,
	ITEM_LETTER_STAMPED   = 2598,
	ITEM_LABEL            = 2599,

	ITEM_DOCUMENT_RO      = 1968 //read-only
};
]])

enum("PlayerFlag",
	--Add the flag's numbers to get the groupFlags number you need
	"PlayerFlag_CannotUseCombat = 0",         --2^0 = 1
	"PlayerFlag_CannotAttackPlayer",          --2^1 = 2
	"PlayerFlag_CannotAttackMonster",         --2^2 = 4
	"PlayerFlag_CannotBeAttacked",            --2^3 = 8
	"PlayerFlag_CanConvinceAll",              --2^4 = 16
	"PlayerFlag_CanSummonAll",                --2^5 = 32
	"PlayerFlag_CanIllusionAll",              --2^6 = 64
	"PlayerFlag_CanSenseInvisibility",        --2^7 = 128
	"PlayerFlag_IgnoredByMonsters",           --2^8 = 256
	"PlayerFlag_NotGainInFight",              --2^9 = 512
	"PlayerFlag_HasInfiniteMana",             --2^10 = 1024
	"PlayerFlag_HasInfiniteSoul",             --2^11 = 2048
	"PlayerFlag_HasNoExhaustion",             --2^12 = 4096
	"PlayerFlag_CannotUseSpells",             --2^13 = 8192
	"PlayerFlag_CannotPickupItem",            --2^14 = 16384
	"PlayerFlag_CanAlwaysLogin",              --2^15 = 32768
	"PlayerFlag_CanBroadcast",                --2^16 = 65536
	"PlayerFlag_CanEditHouses",               --2^17 = 131072
	"PlayerFlag_CannotBeBanned",              --2^18 = 262144
	"PlayerFlag_CannotBePushed",              --2^19 = 524288
	"PlayerFlag_HasInfiniteCapacity",         --2^20 = 1048576
	"PlayerFlag_CanPushAllCreatures",         --2^21 = 2097152
	"PlayerFlag_CanTalkRedPrivate",           --2^22 = 4194304
	"PlayerFlag_CanTalkRedChannel",           --2^23 = 8388608
	"PlayerFlag_TalkOrangeHelpChannel",       --2^24 = 16777216
	"PlayerFlag_NotGainExperience",           --2^25 = 33554432
	"PlayerFlag_NotGainMana",                 --2^26 = 67108864
	"PlayerFlag_NotGainHealth",               --2^27 = 134217728
	"PlayerFlag_NotGainSkill",                --2^28 = 268435456
	"PlayerFlag_SetMaxSpeed",                 --2^29 = 536870912
	"PlayerFlag_SpecialVIP",                  --2^30 = 1073741824
	"PlayerFlag_NotGenerateLoot",             --2^31 = 2147483648
	"PlayerFlag_CanTalkRedChannelAnonymous",  --2^32 = 4294967296
	"PlayerFlag_IgnoreProtectionZone",        --2^33 = 8589934592
	"PlayerFlag_IgnoreSpellCheck",            --2^34 = 17179869184
	"PlayerFlag_IgnoreWeaponCheck",           --2^35 = 34359738368
	"PlayerFlag_CannotBeMuted",               --2^36 = 68719476736
	"PlayerFlag_IsAlwaysPremium",             --2^37 = 137438953472
	"PlayerFlag_CanAnswerRuleViolations",     --2^38 = 274877906944
	"PlayerFlag_CanReloadContent",            --2^39 = 549755813888
	"PlayerFlag_ShowGroupInsteadOfVocation",  --2^40 = 1099511627776
	"PlayerFlag_HasInfiniteStamina",          --2^41 = 2199023255552
	"PlayerFlag_CannotMoveItems",             --2^42 = 4398046511104
	"PlayerFlag_CannotMoveCreatures",         --2^43 = 8796093022208
	"PlayerFlag_CanReportBugs",               --2^44 = 17592186044416
	"PlayerFlag_CanSeeSpecialDescription",    --2^45 = 35184372088832
	"PlayerFlag_CannotBeSeen",                --2^46 = 70368744177664
	--add new flags here
	"PlayerFlag_LastFlag"
)

definition("typedef PlayerFlag PlayerFlags;")

-- Maybe this should be moved to another file
definition([[
enum ViolationActions_t
{
	Action_None                     = 0,
	Action_Notation                 = 1 << 0,
	Action_Namelock                 = 1 << 1,
	Action_Banishment               = 1 << 2,
	Action_NamelockBan              = 1 << 3,
	Action_BanFinalWarning          = 1 << 4,
	Action_NamelockBanFinalWarning  = 1 << 5,
	Action_StatementReport          = 1 << 6,
	Action_IpBan                    = 1 << 7
};

const int32_t violationReasons[5] =
{
	0,	//ignore this
	3,	//all name reasons
	9,	//all name & statement reasons
	18,	//all name, statement & cheating reasons
	19	//all name, statement & cheating reasons & ip-banishment
};

const int32_t violationNames[5] =
{
	Action_None,
	Action_Namelock,
	Action_Namelock,
	Action_Namelock | Action_NamelockBan,
	Action_Namelock | Action_NamelockBan | Action_NamelockBanFinalWarning | Action_IpBan
};

const int32_t violationStatements[5] =
{
	Action_None,
	Action_None,
	Action_StatementReport,
	Action_StatementReport | Action_Notation | Action_Banishment,
	Action_StatementReport | Action_Notation | Action_Banishment | Action_BanFinalWarning | Action_IpBan
};
]])

EndEnumFile()
