local energyWave = Spell:new("Energy Wave")

energyWave.words       = "exevo mort hur"
energyWave.vocation    = "sorcerer"
energyWave.level       = 38
energyWave.magic_level = 40
energyWave.mana        = 170
energyWave.aggressive  = false

energyWave.damagetype = COMBAT_ENERGYDAMAGE,
energyWave.effect     = COMBAT_ME_ENERGYAREA
energyWave.area       =
	{
		{1, 1, 1},
		{1, 1, 1},
		{1, 1, 1},
		{0, 1, 0},
		{0, 3, 0}
	}

energyWave.formula = ???

energyWave:register()
