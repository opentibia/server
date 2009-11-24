local terraStrike = Spell:new("Terra Strike")

terraStrike.words       = "exori tera"
terraStrike.vocation    = {"Sorcerer", "Master Sorcerer", "Druid", "Elder Druid"}
terraStrike.damageType  = COMBAT_EARTHDAMAGE
terraStrike.level       = 13
terraStrike.mana        = 20
terraStrike.aggressive  = true
terraStrike.maybeTarget = true
terraStrike.areaEffect  = MAGIC_EFFECT_EARTH_AREA
terraStrike.shootEffect = SHOOT_EFFECT_EARTH

terraStrike.area        =
	{
		{" ", "n", " "},
		{"w", " ", "e"},
		{" ", "s", " "},
	}
	
terraStrike.formula = formulaLevelMagic(10, 1.4, 20, 2.1)

terraStrike:register()
