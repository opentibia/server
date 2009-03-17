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

#ifndef __OTSERV_CONDITION_ATTRIBUTES_H__
#define __OTSERV_CONDITION_ATTRIBUTES_H__

enum ConditionType_t {
	CONDITION_NONE           = 0,
	CONDITION_POISON         = 1,
	CONDITION_FIRE           = 2,
	CONDITION_ENERGY         = 4,
	CONDITION_LIFEDRAIN      = 8,
	CONDITION_HASTE          = 16,
	CONDITION_PARALYZE	     = 32,
	CONDITION_OUTFIT         = 64,
	CONDITION_INVISIBLE      = 128,
	CONDITION_LIGHT          = 256,
	CONDITION_MANASHIELD     = 512,
	CONDITION_INFIGHT        = 1024,
	CONDITION_DRUNK          = 2048,
	CONDITION_EXHAUSTED      = 4096,
	CONDITION_REGENERATION   = 8192,
	CONDITION_SOUL           = 16384,
	CONDITION_DROWN          = 32768,
	CONDITION_MUTED          = 65536,
	CONDITION_ATTRIBUTES     = 131072,
	CONDITION_FREEZING       = 262144,
	CONDITION_DAZZLED        = 524288,
	CONDITION_CURSED         = 1048576,
	CONDITION_EXHAUST_COMBAT = 2097152,
	CONDITION_EXHAUST_HEAL   = 4194304,
	CONDITION_PACIFIED       = 8388608
};

enum ConditionEnd_t{
	CONDITIONEND_CLEANUP,
	CONDITIONEND_DIE,
	CONDITIONEND_TICKS,
	CONDITIONEND_ABORT
};

enum ConditionAttr_t{
	CONDITIONATTR_TYPE = 1,
	CONDITIONATTR_ID = 2,
	CONDITIONATTR_TICKS = 3,
	CONDITIONATTR_HEALTHTICKS = 4,
	CONDITIONATTR_HEALTHGAIN = 5,
	CONDITIONATTR_MANATICKS = 6,
	CONDITIONATTR_MANAGAIN = 7,
	CONDITIONATTR_DELAYED = 8,
	CONDITIONATTR_OWNER = 9,
	CONDITIONATTR_INTERVALDATA = 10,
	CONDITIONATTR_SPEEDDELTA = 11,
	CONDITIONATTR_FORMULA_MINA = 12,
	CONDITIONATTR_FORMULA_MINB = 13,
	CONDITIONATTR_FORMULA_MAXA = 14,
	CONDITIONATTR_FORMULA_MAXB = 15,
	CONDITIONATTR_LIGHTCOLOR = 16,
	CONDITIONATTR_LIGHTLEVEL = 17,
	CONDITIONATTR_LIGHTTICKS = 18,
	CONDITIONATTR_LIGHTINTERVAL = 19,
	CONDITIONATTR_SOULTICKS = 20,
	CONDITIONATTR_SOULGAIN = 21,
	CONDITIONATTR_SKILLS = 22,
	CONDITIONATTR_STATS = 23,
	CONDITIONATTR_OUTFIT = 24,
	CONDITIONATTR_PERIODDAMAGE = 25,
	CONDITIONATTR_SKILLSPERCENT = 26,
	CONDITIONATTR_ISBUFF = 27,
	CONDITIONATTR_SUBID = 28,

	//reserved for serialization
	CONDITIONATTR_END      = 254
};

#endif
