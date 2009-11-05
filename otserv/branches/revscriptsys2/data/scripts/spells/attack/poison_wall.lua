local poisonWall = Spell:new("Poison Wall")

poisonWall.rune        = 2289
poisonWall.damageType  = COMBAT_EARTHDAMAGE
poisonWall.aggressive  = true
poisonWall.level       = 14
poisonWall.magicLevel  = 0
poisonWall.areaEffect  = MAGIC_EFFECT_POISON_RINGS
poisonWall.shootEffect = SHOOT_EFFECT_POISONFIELD

poisonWall.field       = 1496

poisonWall.area        = {
		{"[ne][sw]", "        ", "[e][w]", "        ", "[nw][se]"},
		{"        ", "[ne][sw]", "[e][w]", "[nw][se]", "        "},
		{"[s][n]  ", "[s][n]  ","  [a]  ", "[s][n]  ", "[s][n]  "},
		{"        ", "[nw][se]", "[e][w]", "[ne][sw]", "        "},
		{"[nw][se]", "        ", "[e][w]", "        ", "[ne][sw]"}
    }
    

poisonWall:register()