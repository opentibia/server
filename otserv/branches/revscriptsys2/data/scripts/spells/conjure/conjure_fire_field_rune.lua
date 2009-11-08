local conjureFireFieldRune = Spell:new("Conjure Fire Field Rune")

conjureFireFieldRune.words         = "adevo grav flam"
conjureFireFieldRune.vocation      = {"Druid", "Elder Druid", "Sorcerer", "Master Sorcerer"}
conjureFireFieldRune.level         = 15
conjureFireFieldRune.mana          = 240
conjureFireFieldRune.soul          = 1

conjureFireFieldRune.reagent       = 2260
conjureFireFieldRune.product.id    = 2301
conjureFireFieldRune.product.count = 3

conjureFireFieldRune.effect        = MAGIC_EFFECT_RED_SHIMMER

conjureFireFieldRune:register()