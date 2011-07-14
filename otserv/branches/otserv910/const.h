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

#ifndef __OTSERV_CONST_H__
#define __OTSERV_CONST_H__

#include "definitions.h"

#define NETWORKMESSAGE_MAXSIZE 15340

//Ranges for ID Creatures
#define PLAYER_ID_RANGE 0x10000000
#define MONSTER_ID_RANGE 0x40000000
#define NPC_ID_RANGE 0x80000000

enum MagicEffectClasses {
	NM_ME_DRAW_BLOOD       = 0x00,
	NM_ME_LOSE_ENERGY      = 0x01,
	NM_ME_PUFF             = 0x02,
	NM_ME_BLOCKHIT         = 0x03,
	NM_ME_EXPLOSION_AREA   = 0x04,
	NM_ME_EXPLOSION_DAMAGE = 0x05,
	NM_ME_FIRE_AREA        = 0x06,
	NM_ME_YELLOW_RINGS     = 0x07,
	NM_ME_POISON_RINGS     = 0x08,
	NM_ME_HIT_AREA         = 0x09,
	NM_ME_TELEPORT         = 0x0A, //10
	NM_ME_ENERGY_DAMAGE    = 0x0B, //11
	NM_ME_MAGIC_ENERGY     = 0x0C, //12
	NM_ME_MAGIC_BLOOD      = 0x0D, //13
	NM_ME_MAGIC_POISON     = 0x0E, //14
	NM_ME_HITBY_FIRE       = 0x0F, //15
	NM_ME_POISON           = 0x10, //16
	NM_ME_MORT_AREA        = 0x11, //17
	NM_ME_SOUND_GREEN      = 0x12, //18
	NM_ME_SOUND_RED        = 0x13, //19
	NM_ME_POISON_AREA      = 0x14, //20
	NM_ME_SOUND_YELLOW     = 0x15, //21
	NM_ME_SOUND_PURPLE     = 0x16, //22
	NM_ME_SOUND_BLUE       = 0x17, //23
	NM_ME_SOUND_WHITE      = 0x18, //24
	NM_ME_BUBBLES	       = 0x19, //25
	NM_ME_CRAPS            = 0x1A, //26
	NM_ME_GIFT_WRAPS       = 0x1B, //27
	NM_ME_FIREWORK_YELLOW  = 0x1C, //28
	NM_ME_FIREWORK_RED     = 0x1D, //29
	NM_ME_FIREWORK_BLUE    = 0x1E, //30
	NM_ME_STUN             = 0x1F, //31
	NM_ME_SLEEP            = 0x20, //32
	NM_ME_WATERCREATURE    = 0x21, //33
	NM_ME_GROUNDSHAKER     = 0x22, //34
	NM_ME_HEARTS           = 0x23, //35
	NM_ME_FIREATTACK       = 0x24, //36
	NM_ME_ENERGY_AREA      = 0x25, //37
	NM_ME_SMALLCLOUDS      = 0x26, //38
	NM_ME_HOLYDAMAGE       = 0x27, //39
	NM_ME_BIGCLOUDS        = 0x28, //40
	NM_ME_ICEAREA          = 0x29, //41
	NM_ME_ICETORNADO       = 0x2A, //42
	NM_ME_ICEATTACK        = 0x2B, //43
	NM_ME_STONES           = 0x2C, //44
	NM_ME_SMALLPLANTS      = 0x2D, //45
	NM_ME_CARNIPHILA       = 0x2E, //46
	NM_ME_PURPLEENERGY     = 0x2F, //47
	NM_ME_YELLOWENERGY     = 0x30, //48
	NM_ME_HOLYAREA         = 0x31, //49
	NM_ME_BIGPLANTS        = 0x32, //50
	NM_ME_CAKE             = 0x33, //51
	NM_ME_GIANTICE         = 0x34, //52
	NM_ME_WATERSPLASH      = 0x35, //53
	NM_ME_PLANTATTACK      = 0x36, //54
	NM_ME_TUTORIALARROW    = 0x37, //55
	NM_ME_TUTORIALSQUARE   = 0x38, //56
	NM_ME_MIRRORHORIZONTAL = 0x39, //57
	NM_ME_MIRRORVERTICAL   = 0x3A, //58
	NM_ME_SKULLHORIZONTAL  = 0x3B, //59
	NM_ME_SKULLVERTICAL    = 0x3C, //60
	NM_ME_ASSASSIN         = 0x3D, //61
	NM_ME_STEPSHORIZONTAL  = 0x3E, //62
	NM_ME_BLOODYSTEPS      = 0x3F, //63
	NM_ME_STEPSVERTICAL    = 0x40, //64
	NM_ME_YALAHARIGHOST    = 0x41, //65
	NM_ME_BATS             = 0x42, //66
	NM_ME_SMOKE            = 0x43, //67
	NM_ME_INSECTS          = 0x44, //68
	NM_ME_DRAGONHEAD	= 0x45, //69
	NM_ME_ORCSHAMAN		= 0x46, //70
	NM_ME_ORCSHAMAN_FIRE	= 0x47, //71
	NM_ME_LAST		= NM_ME_ORCSHAMAN_FIRE,

	//for internal use, dont send to client
	NM_ME_NONE             = 0xFF,
	NM_ME_UNK              = 0xFFFF
};

enum ShootType_t {
	NM_SHOOT_SPEAR          = 0x00,
	NM_SHOOT_BOLT           = 0x01,
	NM_SHOOT_ARROW          = 0x02,
	NM_SHOOT_FIRE           = 0x03,
	NM_SHOOT_ENERGY         = 0x04,
	NM_SHOOT_POISONARROW    = 0x05,
	NM_SHOOT_BURSTARROW     = 0x06,
	NM_SHOOT_THROWINGSTAR   = 0x07,
	NM_SHOOT_THROWINGKNIFE  = 0x08,
	NM_SHOOT_SMALLSTONE     = 0x09,
	NM_SHOOT_DEATH          = 0x0A, //10
	NM_SHOOT_LARGEROCK      = 0x0B, //11
	NM_SHOOT_SNOWBALL       = 0x0C, //12
	NM_SHOOT_POWERBOLT      = 0x0D, //13
	NM_SHOOT_POISONFIELD    = 0x0E, //14
	NM_SHOOT_INFERNALBOLT   = 0x0F, //15
	NM_SHOOT_HUNTINGSPEAR	= 0x10, //16
	NM_SHOOT_ENCHANTEDSPEAR = 0x11, //17
	NM_SHOOT_REDSTAR        = 0x12, //18
	NM_SHOOT_GREENSTAR      = 0x13, //19
	NM_SHOOT_ROYALSPEAR 	= 0x14, //20
	NM_SHOOT_SNIPERARROW 	= 0x15, //21
	NM_SHOOT_ONYXARROW      = 0x16, //22
	NM_SHOOT_PIERCINGBOLT	= 0x17, //23
	NM_SHOOT_WHIRLWINDSWORD = 0x18, //24
	NM_SHOOT_WHIRLWINDAXE	= 0x19, //25
	NM_SHOOT_WHIRLWINDCLUB	= 0x1A, //26
	NM_SHOOT_ETHEREALSPEAR	= 0x1B, //27
	NM_SHOOT_ICE            = 0x1C, //28
	NM_SHOOT_EARTH          = 0x1D, //29
	NM_SHOOT_HOLY           = 0x1E, //30
	NM_SHOOT_SUDDENDEATH    = 0x1F, //31
	NM_SHOOT_FLASHARROW     = 0x20, //32
	NM_SHOOT_FLAMMINGARROW  = 0x21, //33
	NM_SHOOT_SHIVERARROW    = 0x22, //34
	NM_SHOOT_ENERGYBALL     = 0x23, //35
	NM_SHOOT_SMALLICE       = 0x24, //36
	NM_SHOOT_SMALLHOLY      = 0x25, //37
	NM_SHOOT_SMALLEARTH     = 0x26, //38
	NM_SHOOT_EARTHARROW     = 0x27, //39
	NM_SHOOT_EXPLOSION      = 0x28, //40
	NM_SHOOT_CAKE	        = 0x29, //41
	//for internal use, dont send to client
	NM_SHOOT_WEAPONTYPE     = 0xFE, //254
	NM_SHOOT_NONE           = 0xFF,
	NM_SHOOT_UNK            = 0xFFFF
};

enum SpellGroups_t {
	SPELLGROUP_NONE = 0x00,
	SPELLGROUP_MELEE = 0x01,
	SPELLGROUP_HEALING = 0x02,
	SPELLGROUP_SUPPORT = 0x03,
	SPELLGROUP_SPECIAL = 0x04
};

enum Spells_t {
	SPELL_NONE                    = 0x00, 	//0
	SPELL_LIGHT_HEALING           = 0x01,	//1
	SPELL_INTENSE_HEALING         = 0x02,	//2
	SPELL_ULTIMATE_HEALING        = 0x03,	//3
	SPELL_INTENSE_HEALING_RUNE    = 0x04,	//4
	SPELL_ULTIMATE_HEALING_RUNE   = 0x05,	//5
	SPELL_HASTE                   = 0x06,	//6
	SPELL_LIGHT_MAGIC_MISSILE     = 0x07,	//7
	SPELL_HEAVY_MAGIC_MISSILE     = 0x08,	//8
	SPELL_SUMMON_CREATURE         = 0x09,	//9
	SPELL_LIGHT                   = 0x0A,	//10
	SPELL_GREAT_LIGHT             = 0x0B,	//11
	SPELL_CONVINCE_CREATURE       = 0x0C,	//12
	SPELL_ENERGY_WAVE             = 0x0D,	//13
	SPELL_CHAMELEON               = 0x0E,	//14
	SPELL_FIREBALL                = 0x0F,	//15
	SPELL_GREAT_FIREBALL          = 0x10,	//16
	SPELL_FIREBOMB                = 0x11,	//17
	SPELL_EXPLOSION               = 0x12,	//18
	SPELL_FIRE_WAVE               = 0x13,	//19
	SPELL_FIND_PERSON             = 0x14,	//20
	SPELL_SUDDEN_DEATH            = 0x15,	//21
	SPELL_ENERGY_BEAM             = 0x16,	//22
	SPELL_GREAT_ENERGY_BEAM       = 0x17,	//23
	SPELL_HELLS_CORE              = 0x18,	//24
	SPELL_FIRE_FIELD              = 0x19,	//25
	SPELL_POISON_FIELD            = 0x1A,	//26
	SPELL_ENERGY_FIELD            = 0x1B,	//27
	SPELL_FIRE_WALL               = 0x1C,	//28
	SPELL_CURE_POISON             = 0x1D,	//29
	SPELL_DESTROY_FIELD           = 0x1E,	//30
	SPELL_ANTIDOTE_RUNE           = 0x1F,	//31
	SPELL_POISON_WALL             = 0x20,	//32
	SPELL_ENERGY_WALL             = 0x21,	//33
	SPELL_UNKNOWN_1               = 0x22,	//34
	SPELL_UNKNOWN_2               = 0x23,	//35
	SPELL_SALVATION               = 0x24,	//36
	SPELL_MOVE                    = 0x25,	//37
	SPELL_CREATURE_ILLUSION       = 0x26,	//38
	SPELL_STRONG_HASTE            = 0x27,	//39
	SPELL_UNKNOWN_3               = 0x28,	//40
	SPELL_UNKNOWN_4               = 0x29,	//41
	SPELL_FOOD                    = 0x2A,	//42
	SPELL_STRONG_ICE_WAVE         = 0x2B,	//43
	SPELL_MAGIC_SHIELD            = 0x2C,	//44
	SPELL_INVISIBLE               = 0x2D,	//45
	SPELL_UNKNOWN_5               = 0x2E,	//46
	SPELL_UNKNOWN_6               = 0x2F,	//47
	SPELL_POISONED_ARROW          = 0x30,	//48
	SPELL_EXPLOSIVE_ARROW         = 0x31,	//49
	SPELL_SOULFIRE                = 0x32,	//50
	SPELL_CONJURE_ARROW           = 0x33,	//51
	SPELL_RETRIEVE_FRIEND         = 0x34,	//52
	SPELL_UNKNOWN_7               = 0x35,	//53
	SPELL_PARALYZE                = 0x36,	//54
	SPELL_ENERGYBOMB              = 0x37,	//55
	SPELL_WRATH_OF_NATURE         = 0x38,	//56
	SPELL_STRONG_ETHEREAL_SPEAR   = 0x39,	//57
	SPELL_UNKNOWN_8               = 0x3A,	//58
	SPELL_FRONT_SWEEP             = 0x3B,	//59
	SPELL_UNKNOWN_9               = 0x3C,	//60
	SPELL_BRUTAL_STRIKE           = 0x3D,	//61
	SPELL_ANNIHILATION            = 0x3E,	//62
	SPELL_UNKNOWN_10              = 0x3F,	//63
	SPELL_UNKNOWN_11              = 0x40,	//64
	SPELL_UNKNOWN_12              = 0x41,	//65
	SPELL_UNKNOWN_13              = 0x42,	//66
	SPELL_UNKNOWN_14              = 0x43,	//67
	SPELL_UNKNOWN_15              = 0x44,	//68
	SPELL_UNKNOWN_16              = 0x45,	//69
	SPELL_UNKNOWN_17              = 0x46,	//70
	SPELL_INVITE_GUESTS           = 0x47,	//71
	SPELL_INVITE_SUBOWNERS        = 0x48,	//72
	SPELL_KICK_GUEST              = 0x49,	//73
	SPELL_EDIT_DOOR               = 0x4A,	//74
	SPELL_ULTIMATE_LIGHT          = 0x4B,	//75
	SPELL_MAGIC_ROPE              = 0x4C,	//76
	SPELL_STALAGMITE              = 0x4D,	//77
	SPELL_DESINTEGRATE            = 0x4E,	//78
	SPELL_CONJURE_BOLT            = 0x4F,	//79
	SPELL_BERSERK                 = 0x50,	//80
	SPELL_LEVITATE                = 0x51,	//81
	SPELL_MASS_HEALING            = 0x52,	//82
	SPELL_ANIMATE_DEAD            = 0x53,	//83
	SPELL_HEAL_FRIEND             = 0x54,	//84
	SPELL_UNDEAD_LEGION           = 0x55,	//85
	SPELL_MAGIC_WALL              = 0x56,	//86
	SPELL_DEATH_STRIKE            = 0x57,	//87
	SPELL_ENERGY_STRIKE           = 0x58,	//88
	SPELL_FLAME_STRIKE            = 0x59,	//89
	SPELL_CANCEL_INVISIBILITY     = 0x5A,	//90
	SPELL_POISONBOMB              = 0x5B,	//91
	SPELL_ENCHANT_STAFF           = 0x5C,	//92
	SPELL_CHALLENGE               = 0x5D,	//93
	SPELL_WILD_GROWTH             = 0x5E,	//94
	SPELL_POWER_BOLT              = 0x5F,	//95
	SPELL_UNKNOWN_18              = 0x60,	//96
	SPELL_UNKNOWN_19              = 0x61,	//97
	SPELL_UNKNOWN_20              = 0x62,	//98
	SPELL_UNKNOWN_21              = 0x63,	//99
	SPELL_UNKNOWN_22              = 0x64,	//100
	SPELL_UNKNOWN_23              = 0x65,	//101
	SPELL_UNKNOWN_24              = 0x66,	//102
	SPELL_UNKNOWN_25              = 0x67,	//103
	SPELL_UNKNOWN_26              = 0x68,	//104
	SPELL_FIERCE_BERSERK          = 0x69,	//105
	SPELL_GROUNDSHAKER            = 0x6A,	//106
	SPELL_WHIRLWIND_THROW         = 0x6B,	//107
	SPELL_SNIPER_ARROW            = 0x6C,	//108
	SPELL_PIERCING_BOLT           = 0x6D,	//109
	SPELL_ENCHANT_SPEAR           = 0x6E,	//110
	SPELL_ETHEREAL_SPEAR          = 0x6F,	//111
	SPELL_ICE_STRIKE              = 0x70,	//112
	SPELL_TERRA_STRIKE            = 0x71,	//113
	SPELL_ICICLE                  = 0x72,	//114
	SPELL_AVALANCHE               = 0x73,	//115
	SPELL_STONE_SHOWER            = 0x74,	//116
	SPELL_THUNDERSTORM            = 0x75,	//117
	SPELL_ETERNAL_WINTER          = 0x76,	//118
	SPELL_RAGE_OF_THE_SKIES       = 0x77,	//119
	SPELL_TERRA_WAVE              = 0x78,	//120
	SPELL_ICE_WAVE                = 0x79,	//121
	SPELL_DIVINE_MISSILE          = 0x7A,	//122
	SPELL_WOUND_CLEANSING         = 0x7B,	//123
	SPELL_DIVINE_CALDERA          = 0x7C,	//124
	SPELL_DIVINE_HEALING          = 0x7D,	//125
	SPELL_TRAIN_PARTY             = 0x7E,	//126
	SPELL_PROTECT_PARTY           = 0x7F,	//127
	SPELL_HEAL_PARTY              = 0x80,	//128
	SPELL_ENCHANT_PARTY           = 0x81,	//129
	SPELL_HOLY_MISSILE            = 0x82,	//130
	SPELL_CHARGE                  = 0x83,	//131
	SPELL_PROTECTOR               = 0x84,	//132
	SPELL_BLOOD_RAGE              = 0x85,	//133
	SPELL_SWIFT_FOOT              = 0x86,	//134
	SPELL_SHARPSHOOTER            = 0x87,	//135
	SPELL_UNKNOWN_27              = 0x88,	//136
	SPELL_UNKNOWN_28              = 0x89,	//137
	SPELL_IGNITE                  = 0x8A,	//138
	SPELL_CURSE                   = 0x8B,	//139
	SPELL_ELECTRIFY               = 0x8C,	//140
	SPELL_INFLICT_WOUND           = 0x8D,	//141
	SPELL_ENVENOM                 = 0x8E,	//142
	SPELL_HOLY_FLASH              = 0x8F,	//143
	SPELL_CURE_BLEEDING           = 0x90,	//144
	SPELL_CURE_BURNING            = 0x91,	//145
	SPELL_CURE_ELECTRIFICATION    = 0x92,	//146
	SPELL_CURE_CURSE              = 0x93,	//147
	SPELL_PHYSICAL_STRIKE         = 0x94,	//148
	SPELL_LIGHTNING               = 0x95,	//149
	SPELL_STRONG_FLAME_STRIKE     = 0x96,	//150
	SPELL_STRONG_ENERGY_STRIKE    = 0x97,	//151
	SPELL_STRONG_ICE_STRIKE       = 0x98,	//152
	SPELL_STRONG_TERRA_STRIKE     = 0x99,	//153
	SPELL_ULTIMATE_FLAME_STRIKE   = 0x9A,	//154
	SPELL_ULTIMATE_ENERGY_STRIKE  = 0x9B,	//155
	SPELL_ULTIMATE_ICE_STRIKE     = 0x9C,	//156
	SPELL_ULTIMATE_TERRA_STRIKE   = 0x9D,	//157
	SPELL_INTENSE_WOUND_CLEANSING = 0x9E,	//158
	SPELL_RECOVERY                = 0x9F,	//159
	SPELL_INTENSE_RECOVERY        = 0xA0	//160
};

enum SpeakClasses
{
	SPEAK_SAY		= 0x01,	//normal talk
	SPEAK_WHISPER		= 0x02,	//whispering - #w text
	SPEAK_YELL		= 0x03,	//yelling - #y text
	SPEAK_PRIVATE_FROM	= 0x04,	//Players speaking privately from players
	SPEAK_PRIVATE_TO	= 0x05,	//Players speaking privately to players
	SPEAK_CHANNEL_MANAGEMENT		= 0x06,	//?
	SPEAK_CHANNEL_Y		= 0x07,	//Yellow message in chat
	SPEAK_CHANNEL_O		= 0x08,	//Talk orange on text
	SPEAK_SPELL			= 0x09,	//?
	SPEAK_PRIVATE_PN	= 0x0B,	//Player-to-NPC speaking(NPCs channel)
	SPEAK_PRIVATE_NP	= 0x0A,	//NPC-to-Player speaking
	SPEAK_BROADCAST		= 0x0C,	//Broadcast a message - #b
	SPEAK_CHANNEL_R1        = 0x0D, //red - #c text
	SPEAK_PRIVATE_RED_FROM	= 0x0E, //@name@text
	SPEAK_PRIVATE_RED_TO	= 0x0F, //@name@text
	SPEAK_MONSTER_SAY	= 0x22,	//Talk orange
	SPEAK_MONSTER_YELL	= 0x23,	//Yell orange
	
	//removed from game
	SPEAK_RVR_CHANNEL       = 0xFF + 1, //Reporting rule violation - Ctrl+R
	SPEAK_RVR_ANSWER        = 0xFF + 2, //Answering report
	SPEAK_RVR_CONTINUE	    = 0xFF + 3, //Answering the answer of the report
	SPEAK_CHANNEL_R2        = 0xFF + 4,	//Talk red anonymously on chat - #d
	SPEAK_CHANNEL_W			= 0xFF + 5	// White message in chat
};

enum MessageClasses
{
	MSG_STATUS_CONSOLE_BLUE		= 0x04, /*FIXME Blue message in the console*/
	MSG_STATUS_CONSOLE_RED		= 0x0C, /*Red message in the console*/
	MSG_STATUS_DEFAULT		= 0x10, /*White message at the bottom of the game window and in the console*/
	MSG_STATUS_WARNING		= 0x11, /*Red message in game window and in the console*/
	MSG_EVENT_ADVANCE		= 0x12, /*White message in game window and in the console*/
	MSG_STATUS_SMALL		= 0x13, /*White message at the bottom of the game window"*/
	MSG_INFO_DESCR			= 0x14, /*Green message in game window and in the console*/
	MSG_DAMAGE_DEALT		= 0x15,	// ?? == to fullfill I don't know exactly those 
	MSG_DAMAGE_RECEIVED		= 0x16,	//?
	MSG_HEALED			= 0x17,	//?
	MSG_EXPERIENCE			= 0x18,	//?
	MSG_DAMAGE_OTHERS		= 0x19,	//?
	MSG_HEALED_OTHERS		= 0x1A,	//?
	MSG_EXPERIENCE_OTHERS		= 0x1B,	//?
	MSG_EVENT_DEFAULT		= 0x1C, /*White message at the bottom of the game window and in the console*/
	MSG_LOOT			= 0x1D,	//?
	MSG_EVENT_ORANGE		= 0x22, /*Orange message in the console*/
	MSG_STATUS_CONSOLE_ORANGE	= 0x23,  /*Orange message in the console*/
	

	MSG_CHANNEL_GUILD			= 0x1F, /*SPEAK_CHANNEL_W(?) guild messages. not in use just const?*/	
	MSG_TRADE_NPC				= 0x1E,	// not in use just const?
	MSG_PARTY_MANAGEMENT		= 0x20,	// not in use just const?
	MSG_PARTY					= 0x21,	// not in use just const?
	MSG_REPORT 					= 0x24,	// not in use just const?
	MSG_HOTKEY_USE				= 0x25,	// not in use just const?
	MSG_TUTORIAL_HINT			= 0x26	// not in use just const?
};

enum FluidColors_t {
	FLUID_EMPTY   = 0x00,
	FLUID_BLUE    = 0x01,
	FLUID_RED     = 0x02,
	FLUID_BROWN   = 0x03,
	FLUID_GREEN   = 0x04,
	FLUID_YELLOW  = 0x05,
	FLUID_WHITE   = 0x06,
	FLUID_PURPLE  = 0x07
};

enum FluidTypes_t {
	FLUID_WATER	      = FLUID_BLUE,
	FLUID_BLOOD       = FLUID_RED,
	FLUID_BEER        = FLUID_BROWN,
	FLUID_SLIME       = FLUID_GREEN,
	FLUID_LEMONADE    = FLUID_YELLOW,
	FLUID_MILK        = FLUID_WHITE,
	FLUID_MANA        = FLUID_PURPLE,

	FLUID_LIFE        = FLUID_RED + 8,
	FLUID_OIL         = FLUID_BROWN + 8,
	FLUID_URINE       = FLUID_YELLOW + 8,
	FLUID_COCONUTMILK = FLUID_WHITE + 8,
	FLUID_WINE        = FLUID_PURPLE + 8,

	FLUID_MUD         = FLUID_BROWN + 16,
	FLUID_FRUITJUICE  = FLUID_YELLOW + 16,

	FLUID_LAVA        = FLUID_RED + 24,
	FLUID_RUM         = FLUID_BROWN + 24,
	FLUID_SWAMP       = FLUID_GREEN + 24
};

const uint8_t reverseFluidMap[] = {
	FLUID_EMPTY,
	FLUID_WATER,
	FLUID_MANA,
	FLUID_BEER,
	FLUID_EMPTY,
	FLUID_BLOOD,
	FLUID_SLIME,
	FLUID_EMPTY,
	FLUID_LEMONADE,
	FLUID_MILK
};

enum ClientFluidTypes_t {
	CLIENTFLUID_EMPTY   = 0x00,
	CLIENTFLUID_BLUE    = 0x01,
	CLIENTFLUID_PURPLE  = 0x02,
	CLIENTFLUID_BROWN_1 = 0x03,
	CLIENTFLUID_BROWN_2 = 0x04,
	CLIENTFLUID_RED     = 0x05,
	CLIENTFLUID_GREEN   = 0x06,
	CLIENTFLUID_BROWN   = 0x07,
	CLIENTFLUID_YELLOW  = 0x08,
	CLIENTFLUID_WHITE   = 0x09
};

const uint8_t fluidMap[] = {
	CLIENTFLUID_EMPTY,
	CLIENTFLUID_BLUE,
	CLIENTFLUID_RED,
	CLIENTFLUID_BROWN_1,
	CLIENTFLUID_GREEN,
	CLIENTFLUID_YELLOW,
	CLIENTFLUID_WHITE,
	CLIENTFLUID_PURPLE
};

enum SquareColor_t {
	SQ_COLOR_NONE   = 256,
	SQ_COLOR_BLACK  = 0
};

enum TextColor_t {
	TEXTCOLOR_BLUE        = 5,
	TEXTCOLOR_GREEN       = 18,
	TEXTCOLOR_LIGHTBLUE   = 35,
	TEXTCOLOR_LIGHTGREEN  = 30,
	TEXTCOLOR_PURPLE      = 83,
	TEXTCOLOR_LIGHTGREY   = 129,
	TEXTCOLOR_DARKRED     = 144,
	TEXTCOLOR_RED         = 180,
	TEXTCOLOR_ORANGE      = 198,
	TEXTCOLOR_YELLOW      = 210,
	TEXTCOLOR_WHITE_EXP   = 215,
	TEXTCOLOR_NONE        = 255,
	TEXTCOLOR_UNK         = 256
};

enum Icons_t{
	ICON_NONE       = 0,
	ICON_POISON     = 1,
	ICON_BURN       = 2,
	ICON_ENERGY     = 4,
	ICON_DRUNK      = 8,
	ICON_MANASHIELD = 16,
	ICON_PARALYZE   = 32,
	ICON_HASTE      = 64,
	ICON_SWORDS     = 128,
	ICON_DROWNING   = 256,
	ICON_FREEZING   = 512,
	ICON_DAZZLED    = 1024,
	ICON_CURSED     = 2048,
	ICON_PARTY_BUFF = 4096,
	ICON_PZBLOCK    = 8192,
	ICON_PZ         = 16384,
	ICON_BLEEDING   = 32768,
	ICON_HUNGRY 	= 65536
};

enum WeaponType_t {
	WEAPON_NONE     = 0,
	WEAPON_SWORD    = 1,
	WEAPON_CLUB     = 2,
	WEAPON_AXE      = 3,
	WEAPON_SHIELD   = 4,
	WEAPON_DIST     = 5,
	WEAPON_WAND     = 6,
	WEAPON_AMMO     = 7
};

enum Ammo_t {
	AMMO_NONE           = 0,
	AMMO_BOLT           = 1,
	AMMO_ARROW          = 2,
	AMMO_SPEAR          = 3,
	AMMO_THROWINGSTAR   = 4,
	AMMO_THROWINGKNIFE  = 5,
	AMMO_STONE          = 6,
	AMMO_SNOWBALL       = 7
};

enum AmmoAction_t{
	AMMOACTION_NONE,
	AMMOACTION_REMOVECOUNT,
	AMMOACTION_REMOVECHARGE,
	AMMOACTION_MOVE,
	AMMOACTION_MOVEBACK
};

enum WieldInfo_t{
	WIELDINFO_LEVEL     = 1,
	WIELDINFO_MAGLV     = 2,
	WIELDINFO_VOCREQ    = 4,
	WIELDINFO_PREMIUM   = 8
};

enum Skulls_t{
	SKULL_NONE      = 0,
	SKULL_YELLOW    = 1,
	SKULL_GREEN     = 2,
	SKULL_WHITE     = 3,
	SKULL_RED       = 4,
	SKULL_BLACK     = 5,
	SKULL_ORANGE    = 6,
	SKULL_LAST
};

enum PartyShields_t{
	SHIELD_NONE = 0,
	SHIELD_WHITEYELLOW = 1,
	SHIELD_WHITEBLUE = 2,
	SHIELD_BLUE = 3,
	SHIELD_YELLOW = 4,
	SHIELD_BLUE_SHAREDEXP = 5,
	SHIELD_YELLOW_SHAREDEXP = 6,
	SHIELD_BLUE_NOSHAREDEXP_BLINK = 7,
	SHIELD_YELLOW_NOSHAREDEXP_BLINK = 8,
	SHIELD_BLUE_NOSHAREDEXP = 9,
	SHIELD_YELLOW_NOSHAREDEXP = 10
};

enum GuildEmblems_t{
	EMBLEM_NONE = 0,
	EMBLEM_GREEN = 1,
	EMBLEM_RED = 2,
	EMBLEM_BLUE = 3
};

enum item_t {
	ITEM_FIREFIELD        = 1492,
	ITEM_FIREFIELD_SAFE   = 1500,
	ITEM_FIREFIELD_NOT_DECAYING = 1487,

	ITEM_POISONFIELD      = 1496,
	ITEM_POISONFIELD_SAFE = 1503,
	ITEM_POISONFIELD_NOT_DECAYING = 1490,

	ITEM_ENERGYFIELD      = 1495,
	ITEM_ENERGYFIELD_SAFE = 1504,
	ITEM_ENERGYFIELD_NOT_DECAYING = 1491,

	ITEM_MAGICWALL        = 1497,
	ITEM_MAGICWALL_SAFE   = 11098,

	ITEM_WILDGROWTH       = 1499,
	ITEM_WILDGROWTH_SAFE  = 11099,

	ITEM_COINS_GOLD       = 2148,
	ITEM_COINS_PLATINUM   = 2152,
	ITEM_COINS_CRYSTAL    = 2160,

	ITEM_DEPOT            = 2594,
	ITEM_LOCKER1          = 2589,
	ITEM_GLOWING_SWITCH   = 11063,

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

enum PlayerFlags{
	//Add the flag's numbers to get the groupFlags number you need
	PlayerFlag_CannotUseCombat = 0,         //2^0 = 1
	PlayerFlag_CannotAttackPlayer,          //2^1 = 2
	PlayerFlag_CannotAttackMonster,         //2^2 = 4
	PlayerFlag_CannotBeAttacked,            //2^3 = 8
	PlayerFlag_CanConvinceAll,              //2^4 = 16
	PlayerFlag_CanSummonAll,                //2^5 = 32
	PlayerFlag_CanIllusionAll,              //2^6 = 64
	PlayerFlag_CanSenseInvisibility,        //2^7 = 128
	PlayerFlag_IgnoredByMonsters,           //2^8 = 256
	PlayerFlag_NotGainInFight,              //2^9 = 512
	PlayerFlag_HasInfiniteMana,             //2^10 = 1024
	PlayerFlag_HasInfiniteSoul,             //2^11 = 2048
	PlayerFlag_HasNoExhaustion,             //2^12 = 4096
	PlayerFlag_CannotUseSpells,             //2^13 = 8192
	PlayerFlag_CannotPickupItem,            //2^14 = 16384
	PlayerFlag_CanAlwaysLogin,              //2^15 = 32768
	PlayerFlag_CanBroadcast,                //2^16 = 65536
	PlayerFlag_CanEditHouses,               //2^17 = 131072
	PlayerFlag_CannotBeBanned,              //2^18 = 262144
	PlayerFlag_CannotBePushed,              //2^19 = 524288
	PlayerFlag_HasInfiniteCapacity,         //2^20 = 1048576
	PlayerFlag_CanPushAllCreatures,         //2^21 = 2097152
	PlayerFlag_CanTalkRedPrivate,           //2^22 = 4194304
	PlayerFlag_CanTalkRedChannel,           //2^23 = 8388608
	PlayerFlag_TalkOrangeHelpChannel,       //2^24 = 16777216
	PlayerFlag_NotGainExperience,           //2^25 = 33554432
	PlayerFlag_NotGainMana,                 //2^26 = 67108864
	PlayerFlag_NotGainHealth,               //2^27 = 134217728
	PlayerFlag_NotGainSkill,                //2^28 = 268435456
	PlayerFlag_SetMaxSpeed,                 //2^29 = 536870912
	PlayerFlag_SpecialVIP,                  //2^30 = 1073741824
	PlayerFlag_NotGenerateLoot,             //2^31 = 2147483648
	PlayerFlag_CanTalkRedChannelAnonymous,  //2^32 = 4294967296
	PlayerFlag_IgnoreProtectionZone,        //2^33 = 8589934592
	PlayerFlag_IgnoreSpellCheck,            //2^34 = 17179869184
	PlayerFlag_IgnoreWeaponCheck,           //2^35 = 34359738368
	PlayerFlag_CannotBeMuted,               //2^36 = 68719476736
	PlayerFlag_IsAlwaysPremium,             //2^37 = 137438953472
	PlayerFlag_CanAnswerRuleViolations,     //2^38 = 274877906944
	PlayerFlag_CanReloadContent,            //2^39 = 549755813888
	PlayerFlag_ShowGroupInsteadOfVocation,  //2^40 = 1099511627776
	PlayerFlag_HasInfiniteStamina,          //2^41 = 2199023255552
	PlayerFlag_CannotMoveItems,             //2^42 = 4398046511104
	PlayerFlag_CannotMoveCreatures,         //2^43 = 8796093022208
	PlayerFlag_CanReportBugs,               //2^44 = 17592186044416
	PlayerFlag_CanSeeSpecialDescription,    //2^45 = 35184372088832
	PlayerFlag_CannotBeSeen,                //2^46 = 70368744177664
	PlayerFlag_HideHealth,                  //2^47 = 140737488355328
	PlayerFlag_CanPassThroughAllCreatures,  //2^48 = 281474976710656
	//add new flags here
	PlayerFlag_LastFlag
};

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

const int maxViolationLevel = 3;

const int32_t violationReasons[maxViolationLevel + 1] =
{
	0,	//ignore this
	3,	//all name reasons
	9,	//all name & statement reasons
	18,	//all name, statement & cheating reasons
};

const int32_t violationNames[maxViolationLevel + 1] =
{
	Action_None,
	Action_Namelock,
	Action_Namelock,
	Action_Namelock | Action_NamelockBan,
};

const int32_t violationStatements[maxViolationLevel + 1] =
{
	Action_None,
	Action_None,
	Action_StatementReport,
	Action_StatementReport | Action_Notation | Action_Banishment,
};

//Reserved player storage key ranges
//[10000000 - 20000000]
#define PSTRG_RESERVED_RANGE_START  10000000
#define PSTRG_RESERVED_RANGE_SIZE   10000000
//[1000 - 1500]
#define PSTRG_OUTFITS_RANGE_START   (PSTRG_RESERVED_RANGE_START + 1000)
#define PSTRG_OUTFITS_RANGE_SIZE    500

//[1500 - 2000]
#define PSTRG_OUTFITSID_RANGE_START   (PSTRG_RESERVED_RANGE_START + 1500)
#define PSTRG_OUTFITSID_RANGE_SIZE    500

//[2000 - 2500]
#define PSTRG_MOUNTS_RANGE_START   (PSTRG_RESERVED_RANGE_START + 2000)
#define PSTRG_MOUNTS_RANGE_SIZE    500

#define IS_IN_KEYRANGE(key, range) (key >= PSTRG_##range##_START && ((key - PSTRG_##range##_START) < PSTRG_##range##_SIZE))

#endif

