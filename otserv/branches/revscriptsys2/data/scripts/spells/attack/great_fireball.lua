
local greatFireball = Spell:new("Great Fireball")

greatFireball.rune        = 2304
greatFireball.damageType  = COMBAT_FIREDAMAGE
greatFireball.aggressive  = true
greatFireball.level       = 30
greatFireball.magicLevel  = 4
greatFireball.areaEffect  = MAGIC_EFFECT_FIRE_AREA
greatFireball.shootEffect = SHOOT_EFFECT_FIRE

greatFireball.area        = {
		{" ", " ", "a", "a", "a", " ", " "},
		{" ", "a", "a", "a", "a", "a", " "},
		{"a", "a", "a", "a", "a", "a", "a"},
		{"a", "a", "a", "a", "a", "a", "a"},
		{"a", "a", "a", "a", "a", "a", "a"},
		{" ", "a", "a", "a", "a", "a", " "},
		{" ", " ", "a", "a", "a", " ", " "}
	}
	
greatFireball.formula = formulaLevelMagic(0, 1.4, 40, 0, 2.8, 70)

greatFireball:register()
