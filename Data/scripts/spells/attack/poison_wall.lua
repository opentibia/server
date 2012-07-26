local poisonWall = Spell:new("Poison Wall")

poisonWall.rune        = 2289
poisonWall.damageType  = COMBAT_EARTHDAMAGE
poisonWall.aggressive  = true
poisonWall.level       = 14
poisonWall.magicLevel  = 0
poisonWall.areaEffect  = MAGIC_EFFECT_GREEN_RING
poisonWall.shootEffect = SHOOT_EFFECT_POISONFIELD

poisonWall.field       = 1496

poisonWall.area        = {
		{"[sw][ne]", "                      ", "[e][w]", "                      ", "[se][nw]"},
		{"[sw][ne]", "      [sw][ne]        ", "[e][w]", "              [se][nw]", "[se][nw]"},
		{"[s][n]  ", "[s][n][sw][ne][se][nw]", "a     ", "[s][n][sw][ne][se][nw]", "[s][n]  "},
		{"[se][nw]", "              [se][nw]", "[e][w]", "      [sw][ne]        ", "[sw][ne]"},
		{"[se][nw]", "                      ", "[e][w]", "                      ", "[sw][ne]"}
    }

poisonWall:register()