local conjureEnergyFieldRune = Spell:new("Conjure Energy Field Rune")

conjureEnergyFieldRune.words         = "adevo grav vis"
conjureEnergyFieldRune.vocation      = {"Druid", "Elder Druid", "Sorcerer", "Master Sorcerer"}
conjureEnergyFieldRune.level         = 18
conjureEnergyFieldRune.mana          = 320
conjureEnergyFieldRune.soul          = 2

conjureEnergyFieldRune.reagent       = 2260
conjureEnergyFieldRune.product.id    = 2277
conjureEnergyFieldRune.product.count = 3

conjureEnergyFieldRune.effect        = MAGIC_EFFECT_RED_SHIMMER

conjureEnergyFieldRune:register()