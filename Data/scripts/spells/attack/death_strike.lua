local deathStrike = Spell:new("Death Strike")

deathStrike.words       = "exori mort"
deathStrike.vocation    = {"Sorcerer", "Master Sorcerer", "Druid", "Elder Druid"}
deathStrike.damageType  = COMBAT_DEATHDAMAGE
deathStrike.level       = 16
deathStrike.mana        = 20
deathStrike.aggressive  = true
deathStrike.maybeTarget = true
deathStrike.areaEffect  = MAGIC_EFFECT_DEATH_AREA
deathStrike.shootEffect = SHOOT_EFFECT_DEATH

deathStrike.area        = 
	{
		{" ", "n", " "},
		{"w", " ", "e"},
		{" ", "s", " "},
	}
	
deathStrike.formula = formulaLevelMagic(10, 1.4, 20, 2.1)

deathStrike:register()
