local light = Spell:new("Light")

light.words       = "utevo lux"
light.vocation    = "any"
light.level       = 8
light.mana        = 20
light.effect      = MAGIC_EFFECT_BLUE_SHIMMER

light.condition  = {"light", duration = 370000,
		["light"] = {level = 6, color = 215}
	}


light:register()
