local conjurePoisonWallRune = Spell:new("Conjure Poison Wall Rune")

conjurePoisonWallRune.words         = "adevo mas grav pox"
conjurePoisonWallRune.vocation      = {"Druid", "Elder Druid"}
conjurePoisonWallRune.level         = 29
conjurePoisonWallRune.mana          = 640
conjurePoisonWallRune.soul          = 3

conjurePoisonWallRune.reagent       = 2260
conjurePoisonWallRune.product.id    = 2289
conjurePoisonWallRune.product.count = 4

conjurePoisonWallRune.effect        = MAGIC_EFFECT_RED_SHIMMER

conjurePoisonWallRune:register()