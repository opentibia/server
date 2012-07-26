local iceStrike = Spell:new("Ice Strike")

iceStrike.words       = "exori frigo"
iceStrike.vocation    = {"Sorcerer", "Master Sorcerer", "Druid", "Elder Druid"}
iceStrike.damageType  = COMBAT_ICEDAMAGE
iceStrike.level       = 15
iceStrike.mana        = 20
iceStrike.aggressive  = true
iceStrike.maybeTarget = true
iceStrike.areaEffect  = MAGIC_EFFECT_ICE_AREA
iceStrike.shootEffect = SHOOT_EFFECT_ICE

iceStrike.area        =
	{
		{" ", "n", " "},
		{"w", " ", "e"},
		{" ", "s", " "},
	}
	
iceStrike.formula = formulaLevelMagic(10, 1.4, 20, 2.1)

iceStrike:register()
