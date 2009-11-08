local conjureWildGrowthRune = Spell:new("Conjure Wild Growth Rune")

conjureWildGrowthRune.words         = "adevo grav vita"
conjureWildGrowthRune.vocation      = {"Elder Druid"}
conjureWildGrowthRune.level         = 27
conjureWildGrowthRune.mana          = 600
conjureWildGrowthRune.soul          = 5
conjureWildGrowthRune.premium       = true

conjureWildGrowthRune.reagent       = 2260
conjureWildGrowthRune.product.id    = 2269
conjureWildGrowthRune.product.count = 2

conjureWildGrowthRune.effect        = MAGIC_EFFECT_RED_SHIMMER

conjureWildGrowthRune:register()