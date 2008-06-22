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

#ifndef __OTSERV_CONST80_H__
#define __OTSERV_CONST80_H__

#include "definitions.h"

#define NETWORKMESSAGE_MAXSIZE 15360

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

enum SpeakClasses {
	SPEAK_SAY				= 0x01,	//normal talk
	SPEAK_WHISPER			= 0x02,	//whispering - #w text
	SPEAK_YELL				= 0x03,	//yelling - #y text
	SPEAK_PRIVATE_PN		= 0x04, //Player-to-NPC speaking(NPCs channel)
	SPEAK_PRIVATE_NP		= 0x05, //NPC-to-Player speaking
	SPEAK_PRIVATE			= 0x06, //Players speaking privately to players
	SPEAK_CHANNEL_Y     	= 0x07,	//Yellow message on chat
	//SPEAK_RVR_CHANNEL		= 0x08, //Reporting rule violation - Ctrl+R
	//SPEAK_RVR_ANSWER		= 0x09, //Answering report
	//SPEAK_RVR_CONTINUE	= 0x0A  //Answering the answer of the report
	SPEAK_BROADCAST     	= 0x0B,	//Broadcast a message - #b
	SPEAK_CHANNEL_R1    	= 0x0C,	//Talk red on chat - #c
	SPEAK_PRIVATE_RED   	= 0x0D,	//Red private - @name@ text
	SPEAK_CHANNEL_O     	= 0x0E,	//Talk orange on text
	//SPEAK_                = 0x0F, //?
	SPEAK_CHANNEL_R2    	= 0x10,	//Talk red anonymously on chat - #d
	//SPEAK_                = 0x11, //?
	SPEAK_MONSTER_SAY   	= 0x12,	//Talk orange
	SPEAK_MONSTER_YELL  	= 0x13,	//Yell orange
};

enum MessageClasses {
	MSG_STATUS_CONSOLE_RED		= 0x11, //Red message in the console
	MSG_STATUS_CONSOLE_ORANGE	= 0x13, //Orange message in the console
	MSG_STATUS_WARNING			= 0x14, //Red message in game window and in the console
	MSG_EVENT_ADVANCE			= 0x15, //White message in game window and in the console
	MSG_EVENT_DEFAULT			= 0x16, //White message at the bottom of the game window and in the console
	MSG_STATUS_DEFAULT			= 0x17, //White message at the bottom of the game window and in the console
	MSG_INFO_DESCR				= 0x18, //Green message in game window and in the console
	MSG_STATUS_SMALL			= 0x19, //White message at the bottom of the game window"
	MSG_STATUS_CONSOLE_BLUE		= 0x1A, //Blue message in the console
};

enum FluidColors {
	FLUID_EMPTY   = 0x00,
	FLUID_BLUE    = 0x01,
	FLUID_RED     = 0x02,
	FLUID_BROWN   = 0x03,
	FLUID_GREEN   = 0x04,
	FLUID_YELLOW  = 0x05,
	FLUID_WHITE   = 0x06,
	FLUID_PURPLE  = 0x07
};

enum FluidClasses {
	FLUID_EMPTY_1   = 0x00,
	FLUID_BLUE_1    = 0x01,
	FLUID_PURPLE_1  = 0x02,
	FLUID_BROWN_1   = 0x03,
	FLUID_BROWN_2   = 0x04,
	FLUID_RED_1     = 0x05,
	FLUID_GREEN_1   = 0x06,
	FLUID_BROWN_3   = 0x07,
	FLUID_YELLOW_1  = 0x08,
	FLUID_WHITE_1   = 0x09,
	FLUID_PURPLE_2  = 0x0A,
	FLUID_RED_2     = 0x0B,
	FLUID_YELLOW_2  = 0x0C,
	FLUID_BROWN_4   = 0x0D,
	FLUID_YELLOW_3  = 0x0E,
	FLUID_WHITE_2   = 0x0F,
	FLUID_BLUE_2    = 0x10,
};

enum e_fluids {
	FLUID_WATER	    = FLUID_BLUE,
	FLUID_BLOOD     = FLUID_RED,
	FLUID_BEER      = FLUID_BROWN,
	FLUID_SLIME     = FLUID_GREEN,
	FLUID_LEMONADE  = FLUID_YELLOW,
	FLUID_MILK      = FLUID_WHITE,
	FLUID_MANAFLUID = FLUID_PURPLE,
	FLUID_LIFEFLUID = FLUID_RED,
	FLUID_OIL       = FLUID_BROWN,
	FLUID_WINE      = FLUID_PURPLE,
};

const uint32_t reverseFluidMap[] = {
	FLUID_EMPTY_1,
	FLUID_WATER,
	FLUID_MANAFLUID,
	FLUID_BEER,
	FLUID_EMPTY_1,
	FLUID_BLOOD,
	FLUID_SLIME,
	FLUID_EMPTY_1,
	FLUID_LEMONADE,
	FLUID_MILK
};

const uint32_t fluidMap[] = {
	FLUID_EMPTY_1,
	FLUID_BLUE_1,
	FLUID_RED_1,
	FLUID_BROWN_1,
	FLUID_GREEN_1,
	FLUID_YELLOW_1,
	FLUID_WHITE_1,
	FLUID_PURPLE_1
};

enum SquareColor_t {
	SQ_COLOR_NONE   = 256,
	SQ_COLOR_BLACK  = 0,
};

enum TextColor_t {
	TEXTCOLOR_BLUE        = 5,
	TEXTCOLOR_LIGHTBLUE   = 35,
	TEXTCOLOR_LIGHTGREEN  = 30,
	TEXTCOLOR_PURPLE      = 83,
	TEXTCOLOR_LIGHTGREY   = 129,
	TEXTCOLOR_DARKRED     = 144,
	TEXTCOLOR_RED         = 180,
	TEXTCOLOR_ORANGE      = 198,
	TEXTCOLOR_YELLOW      = 210,
	TEXTCOLOR_WHITE_EXP   = 215,
	TEXTCOLOR_NONE        = 255
};

enum Icons_t{
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
	ICON_CURSED     = 2048
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
};

enum item_t {
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

	ITEM_MALE_CORPSE      = 3058,
	ITEM_FEMALE_CORPSE    = 3065,

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

	ITEM_DOCUMENT_RO      = 1968, //read-only
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
	//PlayerFlag_CanAnswerRuleViolations,     //2^38 = 274877906944
	//add new flags here
	PlayerFlag_LastFlag
};

//Reserved player storage key ranges
//[10000000 - 20000000]
#define PSTRG_RESERVED_RANGE_START  10000000
#define PSTRG_RESERVED_RANGE_SIZE   10000000
//[1000 - 1500]
#define PSTRG_OUTFITS_RANGE_START   (PSTRG_RESERVED_RANGE_START + 1000)
#define PSTRG_OUTFITS_RANGE_SIZE    500

#define IS_IN_KEYRANGE(key, range) (key >= PSTRG_##range##_START && ((key - PSTRG_##range##_START) < PSTRG_##range##_SIZE))

#endif
