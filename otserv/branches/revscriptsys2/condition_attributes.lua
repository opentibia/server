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

-- These are the default ids to make sure you only have one of each of these conditions types
-- You can however use your own ids

enum ("ConditionId",
	-- special conditions
	{"CONDITION_NONE", "none", "unknown"},
	{"CONDITION_POISONED", "poisoned"},
	{"CONDITION_BURNING", "burning"},
	{"CONDITION_ELECTRIFIED", "electrified"},
	{"CONDITION_DROWNING", "drowning"},
	{"CONDITION_FREEZING", "freezing"},
	{"CONDITION_DAZZLED", "dazzled"},
	{"CONDITION_CURSED", "cursed"},	
	{"CONDITION_PARALYZED", "paralyzed"},
	{"CONDITION_INVISIBLE", "invisible"},
	{"CONDITION_DRUNK", "drunk"},
	{"CONDITION_HASTE", "haste"},	
	{"CONDITION_INFIGHT", "infight"},
	{"CONDITION_SHAPESHIFT", "shapeshift"},
	{"CONDITION_MANASHIELD", "manashield"},

	-- used internal, not actually sent to the client
	{"CONDITION_PACIFIED", "pacified"},
	{"CONDITION_DISARMED", "disarmed"},
	{"CONDITION_SILENCED", "silenced"},
	{"CONDITION_LIGHT", "light"},
	{"CONDITION_REGENERATION", "regeneration"},
	{"CONDITION_REGENSOUL", "regensoul"},
	{"CONDITION_MUTED_CHAT", "muted_chat"},
	{"CONDITION_MUTED_CHAT_TRADE", "muted_chat_trade"},
	{"CONDITION_EXHAUST_DAMAGE", "exhaust_damage"},
	{"CONDITION_EXHAUST_HEAL", "exhaust_heal"},
	{"CONDITION_EXHAUST_YELL", "exhaust_yell"},
	{"CONDITION_HUNTING", "hunting"}
)

enum ("ConditionAttribute", 
	"CONDITIONATTRIBUTE_MECHANIC = 1",
	"CONDITIONATTRIBUTE_COMBAT = 2",
	"CONDITIONATTRIBUTE_SOURCE = 3",
	"CONDITIONATTRIBUTE_TICKS = 4",
	"CONDITIONATTRIBUTE_NAME = 5",
	"CONDITIONATTRIBUTE_FLAGS = 6",
	"CONDITIONATTRIBUTE_EFFECT = 7",

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
