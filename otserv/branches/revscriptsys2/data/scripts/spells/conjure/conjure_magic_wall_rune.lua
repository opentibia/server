local conjureMagicWallRune = Spell:new("Conjure Magic Wall Rune")

conjureMagicWallRune.words         = "adevo grav tera"
conjureMagicWallRune.vocation      = {"Sorcerer", "Master Sorcerer"}
conjureMagicWallRune.level         = 32
conjureMagicWallRune.mana          = 750
conjureMagicWallRune.soul          = 5

conjureMagicWallRune.reagent       = 2260
conjureMagicWallRune.product.id    = 2293
conjureMagicWallRune.product.count = 3

conjureMagicWallRune.effect        = MAGIC_EFFECT_MAGIC_ENERGY

conjureMagicWallRune:register()
