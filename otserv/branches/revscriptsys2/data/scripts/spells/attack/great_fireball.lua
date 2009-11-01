
local greatFireball = Spell:new("Great Fireball")

greatFireball.rune        = 2304
greatFireball.damageType  = COMBAT_FIREDAMAGE
greatFireball.aggressive  = true
greatFireball.level       = 30
greatFireball.magicLevel  = 4
greatFireball.areaEffect  = MAGIC_EFFECT_FIRE_AREA

greatFireball.area        = {
		{" ", " ", "a", "a", "a", " ", " "},
		{" ", "a", "a", "a", "a", "a", " "},
		{"a", "a", "a", "a", "a", "a", "a"},
		{"a", "a", "a", "a", "a", "a", "a"},
		{"a", "a", "a", "a", "a", "a", "a"},
		{" ", "a", "a", "a", "a", "a", " "},
		{" ", " ", "a", "a", "a", " ", " "}
	}
	
greatFireball.formula = 
	function(player)
		return -math.random( math.min(40, player:getLevel()/5 + player:getMagicLevel() * 1.4),
							math.min(70, player:getLevel()/5 + player:getMagicLevel() * 2.8) )
	end

greatFireball:register()
