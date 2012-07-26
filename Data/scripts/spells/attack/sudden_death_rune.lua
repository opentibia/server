
local suddenDeathRune = Spell:new("Sudden Death Rune")

suddenDeathRune.rune        = 2268
suddenDeathRune.damageType  = COMBAT_DEATHDAMAGE
suddenDeathRune.aggressive  = true
suddenDeathRune.needTarget  = true
suddenDeathRune.level       = 45
suddenDeathRune.magicLevel  = 15
suddenDeathRune.area        = MAGIC_EFFECT_DEATH_AREA
suddenDeathRune.shootEffect = SHOOT_EFFECT_SUDDENDEATH
	
suddenDeathRune.formula = formulaLevelMagic(60, 4.0, 60, 7.0)

suddenDeathRune:register()
