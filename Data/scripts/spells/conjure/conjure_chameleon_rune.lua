local conjureChameleonRune = Spell:new("Conjure Chameleon Rune")

conjureChameleonRune.words         = "adevo ina"
conjureChameleonRune.vocation      = {"Druid", "Elder Druid"}
conjureChameleonRune.level         = 27
conjureChameleonRune.mana          = 600
conjureChameleonRune.soul          = 2

conjureChameleonRune.reagent       = 2260
conjureChameleonRune.product.id    = 2291
conjureChameleonRune.product.count = 1

conjureChameleonRune.effect        = MAGIC_EFFECT_RED_SHIMMER

conjureChameleonRune:register()