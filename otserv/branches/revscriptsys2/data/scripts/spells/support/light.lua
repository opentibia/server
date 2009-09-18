--[[
local light = Spell:new("Light")

light.words       = "utevo lux"
light.vocation    = "any"
light.level       = 8
light.mana        = 20

light.effect     = COMBAT_ME_MAGIC_BLUE

light.condition  = Condition:new{
		CONDITION_LIGHT,
		level=6,
		color=215,
		duration = 3*60 + 10
	}


light:register()
]]--