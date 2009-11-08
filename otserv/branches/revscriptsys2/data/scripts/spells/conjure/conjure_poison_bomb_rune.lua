local conjurePoisonBombRune = Spell:new("Conjure Poison Bomb Rune")

conjurePoisonBombRune.words         = "adevo mas pox"
conjurePoisonBombRune.vocation      = {"Druid", "Elder Druid"}
conjurePoisonBombRune.level         = 25
conjurePoisonBombRune.mana          = 520
conjurePoisonBombRune.soul          = 2
conjurePoisonBombRune.premium       = true

conjurePoisonBombRune.reagent       = 2260
conjurePoisonBombRune.product.id    = 2286
conjurePoisonBombRune.product.count = 2

conjurePoisonBombRune.effect        = MAGIC_EFFECT_RED_SHIMMER

conjurePoisonBombRune:register()