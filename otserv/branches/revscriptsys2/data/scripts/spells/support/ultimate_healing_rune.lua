
local ulimateHealingRune = Spell:new("Ultimate Healing Rune")

ulimateHealingRune.rune        = 2273
ulimateHealingRune.damageType  = COMBAT_HEALING
ulimateHealingRune.needTarget  = true
ulimateHealingRune.level       = 24
ulimateHealingRune.magicLevel  = 4
ulimateHealingRune.area        = MAGIC_EFFECT_BLUE_SHIMMER
	
ulimateHealingRune.formula = formulaLevelMagic(0, 10.0, 0, 12.0)

ulimateHealingRune:register()
