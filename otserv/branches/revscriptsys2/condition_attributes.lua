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

BeginEnumFile("condition_attributes")

enum({name="ConditionType", bitmask=true}, 
	{"CONDITION_NONE", null=true},
	"CONDITION_POISON",
	"CONDITION_FIRE",
	"CONDITION_ENERGY",
	"CONDITION_LIFEDRAIN",
	"CONDITION_HASTE",
	"CONDITION_PARALYZE",
	"CONDITION_OUTFIT",
	"CONDITION_INVISIBLE",
	"CONDITION_LIGHT",
	"CONDITION_MANASHIELD",
	"CONDITION_INFIGHT",
	"CONDITION_DRUNK",
	"CONDITION_EXHAUSTED",
	"CONDITION_REGENERATION",
	"CONDITION_SOUL",
	"CONDITION_DROWN",
	"CONDITION_MUTED",
	"CONDITION_ATTRIBUTES",
	"CONDITION_FREEZING",
	"CONDITION_DAZZLED",
	"CONDITION_CURSED",
	"CONDITION_EXHAUST_COMBAT",
	"CONDITION_EXHAUST_HEAL",
	"CONDITION_PACIFIED",
	"CONDITION_HUNTING",
	"CONDITION_TRADE_MUTED"
)

enum ("ConditionEnd",
	"CONDITIONEND_CLEANUP",
	"CONDITIONEND_DIE",
	"ConditionEndICKS",
	"CONDITIONEND_ABORT"
)

enum ("ConditionAttribute", 
	"CONDITIONATTRIBUTE_TYPE = 1",
	"CONDITIONATTR_ID = 2",
	"CONDITIONATTRIBUTE_TICKS = 3",
	"CONDITIONATTR_HEALTHTICKS = 4",
	"CONDITIONATTR_HEALTHGAIN = 5",
	"CONDITIONATTR_MANATICKS = 6",
	"CONDITIONATTR_MANAGAIN = 7",
	"CONDITIONATTR_DELAYED = 8",
	"CONDITIONATTR_OWNER = 9",
	"CONDITIONATTR_INTERVALDATA = 10",
	"CONDITIONATTR_SPEEDDELTA = 11",
	"CONDITIONATTR_FORMULA_MINA = 12",
	"CONDITIONATTR_FORMULA_MINB = 13",
	"CONDITIONATTR_FORMULA_MAXA = 14",
	"CONDITIONATTR_FORMULA_MAXB = 15",
	"CONDITIONATTR_LIGHTCOLOR = 16",
	"CONDITIONATTR_LIGHTLEVEL = 17",
	"CONDITIONATTR_LIGHTTICKS = 18",
	"CONDITIONATTR_LIGHTINTERVAL = 19",
	"CONDITIONATTR_SOULTICKS = 20",
	"CONDITIONATTR_SOULGAIN = 21",
	"CONDITIONATTR_SKILLS = 22",
	"CONDITIONATTR_STATS = 23",
	"CONDITIONATTR_OUTFIT = 24",
	"CONDITIONATTR_PERIODDAMAGE = 25",
	"CONDITIONATTR_SKILLSPERCENT = 26",
	"CONDITIONATTR_ISBUFF = 27",
	"CONDITIONATTR_SUBID = 28",

	--reserved for serialization
	"CONDITIONATTR_END = 254"
)

EndEnumFile()
