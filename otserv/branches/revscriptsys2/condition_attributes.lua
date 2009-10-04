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

enum ("ConditionType",
	"CONDITION_NONE",
	
	"CONDITION_PHYSICAL",
	"CONDITION_ENERGY",
	"CONDITION_EARTH",
	"CONDITION_FIRE",
	"CONDITION_LIFEDRAIN",
	"CONDITION_MANADRAIN",
	"CONDITION_DROWN",
	"CONDITION_ICE",
	"CONDITION_HOLY",
	"CONDITION_DEATH",
	
	"CONDITION_INFIGHT",
	"CONDITION_INVISIBLE",
	"CONDITION_MANASHIELD",
	"CONDITION_PARALYZE",
	"CONDITION_SHAPESHIFT",
	"CONDITION_HASTE",
	"CONDITION_DRUNK",
	"CONDITION_LIGHT",
	"CONDITION_REGENERATION",
	"CONDITION_SOULREGEN",
	"CONDITION_EXHAUST_DAMAGE",
	"CONDITION_EXHAUST_HEAL",
	"CONDITION_EXHAUST_YELL",
	"CONDITION_DISARMED",
	"CONDITION_PACIFIED",
	"CONDITION_SILENCED",
	"CONDITION_MUTED_CHAT",
	"CONDITION_MUTED_TRADECHAT",
	"CONDITION_HUNTING"
)

enum ("ConditionAttribute", 
	"CONDITIONATTRIBUTE_MECHANIC = 1",
	"CONDITIONATTRIBUTE_COMBAT = 2",
	"CONDITIONATTRIBUTE_SOURCE = 3",
	"CONDITIONATTRIBUTE_TICKS = 4",
	"CONDITIONATTRIBUTE_ID = 5",
	"CONDITIONATTRIBUTE_FLAGS = 6",
	"CONDITIONATTRIBUTE_EFFECT_TYPE = 7",
	"CONDITIONATTRIBUTE_EFFECT_MODTYPE = 8",
	"CONDITIONATTRIBUTE_EFFECT_MODVALUE = 9",
	"CONDITIONATTRIBUTE_EFFECT_MODTOTAL = 10",
	"CONDITIONATTRIBUTE_EFFECT_MODPERCENT = 11",
	"CONDITIONATTRIBUTE_EFFECT_MODTICKS = 12",
	"CONDITIONATTRIBUTE_EFFECT_MODPOD = 13",

	--reserved for serialization
	"CONDITIONATTR_END = 254"
)

enum ({name="MechanicType", bitmask=true},
	{"MECHANIC_NONE", null=true},
	"MECHANIC_SHAPESHIFT",
	"MECHANIC_PACIFIED",
	"MECHANIC_DISARMED",
	"MECHANIC_SHIELDED",
	"MECHANIC_SILENCED",
	"MECHANIC_PARALYZED",
	"MECHANIC_DRUNK",
	"MECHANIC_INVISIBLE"
)

enum ("ConditionEnd",
	"CONDITIONEND_DURATION",
	"CONDITIONEND_DEATH",
	"CONDITIONEND_REMOVED",
	"CONDITIONEND_UPDATE",
	"CONDITIONEND_CLEANUP"
)

EndEnumFile()
