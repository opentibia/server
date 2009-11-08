local conjureSuddenDeathRune = Spell:new("Conjure Sudden Death Rune")

conjureSuddenDeathRune.words         = "adori gran mort"
conjureSuddenDeathRune.vocation      = {"Sorcerer", "Master Sorcerer"}
conjureSuddenDeathRune.level         = 45
conjureSuddenDeathRune.mana          = 985
conjureSuddenDeathRune.soul          = 5

conjureSuddenDeathRune.reagent       = 2260
conjureSuddenDeathRune.product.id    = 2268
conjureSuddenDeathRune.product.count = 3

conjureSuddenDeathRune.effect        = MAGIC_EFFECT_RED_SHIMMER

conjureSuddenDeathRune:register()