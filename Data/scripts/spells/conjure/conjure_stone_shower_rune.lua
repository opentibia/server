local conjureStoneShowerRune = Spell:new("Conjure Stone Shower Rune")

conjureStoneShowerRune.words         = "adori mas tera"
conjureStoneShowerRune.vocation      = {"Druid", "Elder Druid"}
conjureStoneShowerRune.level         = 28
conjureStoneShowerRune.mana          = 430
conjureStoneShowerRune.soul          = 3
conjureStoneShowerRune.premium       = true

conjureStoneShowerRune.reagent       = 2260
conjureStoneShowerRune.product.id    = 2288
conjureStoneShowerRune.product.count = 4

conjureStoneShowerRune.effect        = MAGIC_EFFECT_RED_SHIMMER

conjureStoneShowerRune:register()