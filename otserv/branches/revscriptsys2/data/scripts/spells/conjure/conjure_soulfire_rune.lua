local conjureSoulfireRune = Spell:new("Conjure Soulfire Rune")

conjureSoulfireRune.words         = "adevo res flam"
conjureSoulfireRune.vocation      = {"Druid", "Elder Druid", "Sorcerer", "Master Sorcerer"}
conjureSoulfireRune.level         = 27
conjureSoulfireRune.mana          = 420
conjureSoulfireRune.soul          = 3
conjureSoulfireRune.premium       = true

conjureSoulfireRune.reagent       = 2260
conjureSoulfireRune.product.id    = 2308
conjureSoulfireRune.product.count = 3

conjureSoulfireRune.effect        = MAGIC_EFFECT_RED_SHIMMER

conjureSoulfireRune:register()