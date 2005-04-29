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

#ifndef __CONST74_H__
#define __CONST74_H__


#define NETWORKMESSAGE_MAXSIZE 16384

#define NM_ME_DRAW_BLOOD          0
#define NM_ME_LOOSE_ENERGY        1
#define NM_ME_PUFF                2
#define NM_ME_BLOCKHIT            3
#define NM_ME_EXPLOSION_AREA      4
#define NM_ME_EXPLOSION_DAMAGE    5
#define NM_ME_FIRE_AREA           6
#define NM_ME_YELLOW_RINGS        7
#define NM_ME_POISEN_RINGS        8
#define NM_ME_HIT_AREA            9
#define NM_ME_ENERGY_AREA        10
#define NM_ME_ENERGY_DAMAGE      11

#define NM_ME_MAGIC_ENERGIE      12
#define NM_ME_MAGIC_BLOOD        13
#define NM_ME_MAGIC_POISEN       14

#define NM_ME_HITBY_FIRE         15
#define NM_ME_POISEN             16
#define NM_ME_MORT_AREA          17
#define NM_ME_SOUND              18

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

enum SpeakClasses {
	SPEAK_MONSTER = 0x11
};

enum MessageClasses {
      MSG_SMALLINFO = 0x17,
		  MSG_INFO      = 0x16,
		  MSG_EVENT     = 0x14,
		  MSG_ADVANCE   = 0x13,
};

enum Icons {
 ICON_POISON = 1,
 ICON_BURN = 2, 
 ICON_ENERGY =  4, 
 ICON_DRUNK = 8, 
 ICON_MANASHIELD = 16, 
 ICON_PARALYZE = 32, 
 ICON_HASTE = 64, 
 ICON_SWORDS = 128
 };

enum WeaponType 
{
  NONE, SWORD, CLUB, AXE, DIST, MAGIC, AMO, SHIELD
};

enum amu_t{
	AMU_NONE,
	AMU_BOLT,
	AMU_ARROW
};


enum subfight_t {
	DIST_NONE = 0,
	DIST_BOLT = NM_ANI_BOLT,
  DIST_ARROW = NM_ANI_ARROW, 
  DIST_FIRE = NM_ANI_FIRE,
  DIST_ENERGY = NM_ANI_ENERGY,
  DIST_POISONARROW = NM_ANI_POISONARROW,
  DIST_BURSTARROW = NM_ANI_BURSTARROW,
  DIST_THROWINGSTAR = NM_ANI_THROWINGSTAR,
  DIST_THROWINGKNIFE = NM_ANI_THROWINGKNIFE,
  DIST_SMALLSTONE = NM_ANI_SMALLSTONE,
  DIST_SUDDENDEATH = NM_ANI_SUDDENDEATH,
  DIST_LARGEROCK = NM_ANI_LARGEROCK,
  DIST_SNOWBALL = NM_ANI_SNOWBALL,
  DIST_POWERBOLT = NM_ANI_POWERBOLT,
  DIST_SPEAR = NM_ANI_SPEAR,
  DIST_POISONFIELD = NM_ANI_FLYPOISONFIELD
};




#endif
