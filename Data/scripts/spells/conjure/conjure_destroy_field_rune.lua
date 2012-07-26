local conjureDestroyFieldRune = Spell:new("Conjure Destroy Field Rune")

conjureDestroyFieldRune.words         = "adito grav"
conjureDestroyFieldRune.vocation      = {"Druid", "Elder Druid", "Sorcerer", "Master Sorcerer", "Paladin", "Royal Paladin"}
conjureDestroyFieldRune.level         = 17
conjureDestroyFieldRune.mana          = 120
conjureDestroyFieldRune.soul          = 2

conjureDestroyFieldRune.reagent       = 2260
conjureDestroyFieldRune.product.id    = 2261
conjureDestroyFieldRune.product.count = 3

conjureDestroyFieldRune.effect        = MAGIC_EFFECT_RED_SHIMMER

conjureDestroyFieldRune:register()