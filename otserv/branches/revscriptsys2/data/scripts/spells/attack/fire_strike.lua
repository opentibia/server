local flameStrike = Spell:new("Flame Strike")

flameStrike.words       = "exori flam"
flameStrike.vocation    = {"Sorcerer", "Master Sorcerer", "Druid", "Elder Druid"}
flameStrike.damageType  = COMBAT_FIREDAMAGE
flameStrike.level       = 14
flameStrike.mana        = 20
flameStrike.aggressive  = true
flameStrike.maybeTarget = true
flameStrike.areaEffect  = MAGIC_EFFECT_FIRE_AREA
flameStrike.shootEffect = SHOOT_EFFECT_FIRE

flameStrike.area        =
	{
		{" ", "n", " "},
		{"w", " ", "e"},
		{" ", "s", " "},
	}
	
flameStrike.formula = formulaLevelMagic(10, 1.4, 20, 2.1)

flameStrike:register()