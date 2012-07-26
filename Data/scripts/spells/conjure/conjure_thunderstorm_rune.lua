local conjureThunderstormRune = Spell:new("Conjure Thunderstorm Rune")

conjureThunderstormRune.words         = "adori mas vis"
conjureThunderstormRune.vocation      = {"Sorcerer", "Master Sorcerer"}
conjureThunderstormRune.level         = 28
conjureThunderstormRune.mana          = 430
conjureThunderstormRune.soul          = 3
conjureThunderstormRune.premium       = true

conjureThunderstormRune.reagent        = 2260
conjureThunderstormRune.product.id    = 2315
conjureThunderstormRune.product.count = 4

conjureThunderstormRune.effect        = MAGIC_EFFECT_RED_SHIMMER

conjureThunderstormRune:register()