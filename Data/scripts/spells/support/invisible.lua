local light = Spell:new("Invisibility")

light.words       = "utana vid"
light.vocation    = {"Sorcerer", "Master Sorcerer", "Druid", "Elder Druid", "Paladin", "Royal Paladin"}
light.level       = 35
light.mana        = 440
light.effect      = MAGIC_EFFECT_BLUE_SHIMMER

light.condition  = {"invisible", duration = 200000,
		["script"] = {name = "invisible"}
	}


light:register()
