local conjureFireballRune = Spell:new("Conjure Fireball Rune")

conjureFireballRune.words         = "adori flam"
conjureFireballRune.vocation      = {"Sorcerer", "Master Sorcerer"}
conjureFireballRune.level         = 27
conjureFireballRune.mana          = 460
conjureFireballRune.soul          = 3
conjureFireballRune.premium       = true

conjureFireballRune.reagent       = 2260
conjureFireballRune.product.id    = 2302
conjureFireballRune.product.count = 5

conjureFireballRune.effect        = MAGIC_EFFECT_RED_SHIMMER

conjureFireballRune:register()