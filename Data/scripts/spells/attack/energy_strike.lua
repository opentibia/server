local energyStrike = Spell:new("Energy Strike")

energyStrike.words       = "exori vis"
energyStrike.vocation    = {"Sorcerer", "Master Sorcerer"}
energyStrike.damageType  = COMBAT_ENERGYDAMAGE
energyStrike.level       = 12
energyStrike.mana        = 20
energyStrike.aggressive  = true
energyStrike.maybeTarget = true
energyStrike.areaEffect  = MAGIC_EFFECT_ENERGY_AREA
energyStrike.shootEffect = SHOOT_EFFECT_ENERGY

energyStrike.area        =
	{
		{" ", "n", " "},
		{"w", " ", "e"},
		{" ", "s", " "},
	}
	
energyStrike.formula = formulaLevelMagic(10, 1.4, 20, 2.1)

energyStrike:register()
