local energyWave = Spell:new("Energy Wave")

energyWave.words       = "exevo mort hur"
energyWave.vocation    = "sorcerer"
energyWave.damageType  = COMBAT_ENERGYDAMAGE
energyWave.level       = 38
energyWave.magicLevel  = 40
energyWave.mana        = 170
energyWave.aggressive  = true
energyWave.areaEffect  = MAGIC_EFFECT_ENERGY_AREA

energyWave.area        =
	{
		{" ", " ", " ", " ", "n", "n", "n", " ", " ", " ", " "},
		{" ", " ", " ", " ", "n", "n", "n", " ", " ", " ", " "},
		{" ", " ", " ", " ", "n", "n", "n", " ", " ", " ", " "},
		{" ", " ", " ", " ", " ", "n", " ", " ", " ", " ", " "},
		{"w", "w", "w", " ", " ", "n", " ", " ", "e", "e", "e"},
		{"w", "w", "w", "w", "w", " ", "e", "e", "e", "e", "e"},
		{"w", "w", "w", " ", " ", "s", " ", " ", "e", "e", "e"},
		{" ", " ", " ", " ", " ", "s", " ", " ", " ", " ", " "},
		{" ", " ", " ", " ", "s", "s", "s", " ", " ", " ", " "},
		{" ", " ", " ", " ", "s", "s", "s", " ", " ", " ", " "},
		{" ", " ", " ", " ", "s", "s", "s", " ", " ", " ", " "},
	}
	
energyWave.formula = formulaLevelMagic(0, 4.5, 0, 9.0)

energyWave:register()
