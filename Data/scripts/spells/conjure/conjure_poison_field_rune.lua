local conjurePoisonFieldRune = Spell:new("Conjure Poison Field Rune")

conjurePoisonFieldRune.words         = "adevo grav pox"
conjurePoisonFieldRune.vocation      = {"Druid", "Elder Druid", "Sorcerer", "Master Sorcerer"}
conjurePoisonFieldRune.level         = 14
conjurePoisonFieldRune.mana          = 200
conjurePoisonFieldRune.soul          = 1

conjurePoisonFieldRune.reagent       = 2260
conjurePoisonFieldRune.product.id    = 2285
conjurePoisonFieldRune.product.count = 3

conjurePoisonFieldRune.effect        = MAGIC_EFFECT_RED_SHIMMER

conjurePoisonFieldRune:register()