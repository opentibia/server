
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
	
greatFireball.formula = formulaLevelMagic(40, 1.4, 70, 2.8)

greatFireball:register()
