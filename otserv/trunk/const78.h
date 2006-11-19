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

#ifndef __OTSERV_CONST78_H__
#define __OTSERV_CONST78_H__

#define NETWORKMESSAGE_MAXSIZE 16768

enum MagicEffectClasses {
	NM_ME_DRAW_BLOOD  	= 0x00,
	NM_ME_LOSE_ENERGY	= 0x01,
	NM_ME_PUFF			= 0x02,
	NM_ME_BLOCKHIT		= 0x03,
	NM_ME_EXPLOSION_AREA   = 0x04,
	NM_ME_EXPLOSION_DAMAGE = 0x05,
	NM_ME_FIRE_AREA        = 0x06,
	NM_ME_YELLOW_RINGS     = 0x07,
	NM_ME_POISON_RINGS     = 0x08,
	NM_ME_HIT_AREA         = 0x09,
	NM_ME_ENERGY_AREA      = 0x0A, //10
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

	//for internal use, dont send to client
	NM_ME_NONE             = 0xFF
};

#define NM_ANI_BOLT              1
#define NM_ANI_ARROW             2
#define NM_ANI_FIRE              3
#define NM_ANI_ENERGY            4
#define NM_ANI_POISONARROW       5
#define NM_ANI_BURSTARROW        6
#define NM_ANI_THROWINGSTAR      7
#define NM_ANI_THROWINGKNIFE     8
#define NM_ANI_SMALLSTONE        9
#define NM_ANI_SUDDENDEATH       10
#define NM_ANI_LARGEROCK         11
#define NM_ANI_SNOWBALL          12
#define NM_ANI_SPEAR             0
#define NM_ANI_POWERBOLT         13
#define NM_ANI_FLYPOISONFIELD    14

//for internal use, dont send to client
#define NM_ANI_NONE             255

enum SpeakClasses {
	SPEAK_SAY	  		= 0x01, 
	SPEAK_WHISPER 		= 0x02, 
	SPEAK_YELL	  		= 0x03, 
	//SPEAK_			= 0x0D,	
		
	SPEAK_BROADCAST		= 0x09, 
		
	SPEAK_PRIVATE 		= 0x04, 
	SPEAK_PRIVATE_RED	= 0x0B,	//@name@text
	//SPEAK_			= 0x0F, 
		
	//SPEAK_CHANNEL?? 	= 0x06,
		
	//SPEAK_CHANNEL?? 	= 0x07,
	//SPEAK_CHANNEL?? 	= 0x08,
	
	SPEAK_CHANNEL_Y		= 0x05,	//yellow
	SPEAK_CHANNEL_R1	= 0x0A,	//red - #c text -- gamemaster command
	SPEAK_CHANNEL_R2	= 0x0E,	//red - #d text -- counsellor command(?)
	SPEAK_CHANNEL_O		= 0x0C,	//orange
		
	SPEAK_MONSTER1 		= 0x10,
	SPEAK_MONSTER2 		= 0x11,
};

enum MessageClasses {
	MSG_STATUS_WARNING      = 0x12, /*Red message in game window and in the console*/
	MSG_EVENT_ADVANCE       = 0x13, /*White message in game window and in the console*/
	MSG_EVENT_DEFAULT       = 0x14, /*White message at the bottom of the game window and in the console*/
	MSG_STATUS_DEFAULT      = 0x15, /*White message at the bottom of the game window and in the console*/
	MSG_INFO_DESCR          = 0x16, /*Green message in game window and in the console*/
	MSG_STATUS_SMALL        = 0x17, /*White message at the bottom of the game window"*/
	MSG_STATUS_CONSOLE_BLUE = 0x18, /*Blue message in the console*/
	MSG_STATUS_CONSOLE_RED  = 0x19, /*Red message in the console*/
};

enum FluidColors {
	FLUID_EMPTY   = 0x00,
	FLUID_BLUE	  = 0x01,
	FLUID_RED	  = 0x02,
	FLUID_BROWN   = 0x03,
	FLUID_GREEN   = 0x04,
	FLUID_YELLOW  = 0x05,
	FLUID_WHITE   = 0x06,
	FLUID_PURPLE  = 0x07
};

enum FluidClasses {
	FLUID_EMPTY_1 = 0x00,
	FLUID_BLUE_1 = 0x01,
	FLUID_PURPLE_1 = 0x02,
	FLUID_BROWN_1 = 0x03,
	FLUID_BROWN_2 = 0x04,
	FLUID_RED_1 = 0x05,
	FLUID_GREEN_1 = 0x06,
	FLUID_BROWN_3 = 0x07,
	FLUID_YELLOW_1= 0x08,
	FLUID_WHITE_1 = 0x09,
	FLUID_PURPLE_2 = 0x0a,
	FLUID_RED_2 = 0x0b,
	FLUID_YELLOW_2= 0x0c,
	FLUID_BROWN_4= 0x0d,
	FLUID_YELLOW_3= 0x0e,
	FLUID_WHITE_2 = 0x0f,
	FLUID_BLUE_2 = 0x10,
};

enum e_fluids {	
	FLUID_WATER	= FLUID_BLUE,
	FLUID_BLOOD	= FLUID_RED,
	FLUID_BEER = FLUID_BROWN,
	FLUID_SLIME = FLUID_GREEN,
	FLUID_LEMONADE = FLUID_YELLOW,
	FLUID_MILK = FLUID_WHITE,
	FLUID_MANAFLUID = FLUID_PURPLE,
	FLUID_LIFEFLUID = FLUID_RED,
	FLUID_OIL = FLUID_BROWN,
	FLUID_WINE = FLUID_PURPLE,
};

enum SquareColor_t {
	SQ_COLOR_NONE = 256,
	SQ_COLOR_BLACK = 0,
};

enum TextColor_t {
	TEXTCOLOR_BLUE        = 5,
	TEXTCOLOR_LIGHTBLUE   = 35,
	TEXTCOLOR_LIGHTGREEN  = 30,
	TEXTCOLOR_LIGHTGREY   = 172,
	TEXTCOLOR_RED         = 180,
	TEXTCOLOR_ORANGE      = 198,
	TEXTCOLOR_WHITE_EXP   = 215,
	TEXTCOLOR_NONE        = 255
};

enum Icons_t{
	ICON_POISON = 1,
	ICON_BURN = 2, 
	ICON_ENERGY =  4, 
	ICON_DRUNK = 8, 
	ICON_MANASHIELD = 16, 
	ICON_PARALYZE = 32, 
	ICON_HASTE = 64, 
	ICON_SWORDS = 128,
	ICON_DROWNING = 256
};

enum WeaponType_t {
	WEAPON_NONE = 0,
	WEAPON_SWORD = 1,
	WEAPON_CLUB = 2,
	WEAPON_AXE = 3,
	WEAPON_SHIELD = 4,
	WEAPON_DIST = 5,
	WEAPON_WAND = 6,
	WEAPON_AMMO = 7
};

enum Ammo_t {
	AMMO_NONE = 0,
	AMMO_BOLT = 1,
	AMMO_ARROW = 2
};

enum ShootType_t {
	SHOOT_NONE = 0,
	SHOOT_BOLT = NM_ANI_BOLT,
	SHOOT_ARROW = NM_ANI_ARROW, 
	SHOOT_FIRE = NM_ANI_FIRE,
	SHOOT_ENERGY = NM_ANI_ENERGY,
	SHOOT_POISONARROW = NM_ANI_POISONARROW,
	SHOOT_BURSTARROW = NM_ANI_BURSTARROW,
	SHOOT_THROWINGSTAR = NM_ANI_THROWINGSTAR,
	SHOOT_THROWINGKNIFE = NM_ANI_THROWINGKNIFE,
	SHOOT_SMALLSTONE = NM_ANI_SMALLSTONE,
	SHOOT_SUDDENDEATH = NM_ANI_SUDDENDEATH,
	SHOOT_LARGEROCK = NM_ANI_LARGEROCK,
	SHOOT_SNOWBALL = NM_ANI_SNOWBALL,
	SHOOT_POWERBOLT = NM_ANI_POWERBOLT,
	SHOOT_SPEAR = NM_ANI_SPEAR,
	SHOOT_POISONFIELD = NM_ANI_FLYPOISONFIELD
};

enum Skulls_t{
	SKULL_NONE = 0,
	SKULL_YELLOW = 1,
	SKULL_GREEN = 2,
	SKULL_WHITE = 3,
	SKULL_RED = 4,
};

enum item_t {
	ITEM_FISHING_ROD      = 2580,
	ITEM_SHOVEL           = 2554,
	ITEM_ROPE             = 2120,
	ITEM_MACHETE          = 2420,
	ITEM_SCYTHE           = 2550,
	ITEM_COINS_GOLD       = 2148,
	ITEM_COINS_PLATINUM   = 2152,
	ITEM_COINS_CRYSTAL    = 2160,
	ITEM_DEPOT            = 2594,
	ITEM_RUNE_BLANK       = 2260,
	ITEM_MALE_CORPSE      = 3058,
	ITEM_FEMALE_CORPSE    = 3065,
	ITEM_MEAT             = 2666,
	ITEM_HAM              = 2671,
	ITEM_GRAPE            = 2681,
	ITEM_APPLE            = 2674,
	ITEM_BREAD            = 2689,
	ITEM_ROLL             = 2690,
	ITEM_CHEESE           = 2696,

	ITEM_BOLT             = 2543,
	ITEM_ARROW            = 2544,

	ITEM_FULLSPLASH       = 2016,
	ITEM_SMALLSPLASH      = 2019,
	ITEM_LOCKER1          = 2589,
	ITEM_LOCKER2          = 2590,
	ITEM_LOCKER3          = 2591,
	ITEM_LOCKER4          = 2592,
	ITEM_DUSTBIN          = 1777,
	
	ITEM_PARCEL           = 2595,
	ITEM_PARCEL_STAMPED   = 2596,
	ITEM_LETTER           = 2597,
	ITEM_LETTER_STAMPED   = 2598,
	ITEM_LABEL            = 2599,
	ITEM_MAILBOX1         = 2334,
	ITEM_MAILBOX2         = 2593,
	ITEM_MAILBOX3         = 3981,
	
	ITEM_DOCUMENT_RO      = 1968, //read-only
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
