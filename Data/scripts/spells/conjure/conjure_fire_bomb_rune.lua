local conjureFireBombRune = Spell:new("Conjure Fire Bomb Rune")

conjureFireBombRune.words         = "adevo mas flam"
conjureFireBombRune.vocation      = {"Druid", "Elder Druid", "Sorcerer", "Master Sorcerer"}
conjureFireBombRune.level         = 27
conjureFireBombRune.mana          = 600
conjureFireBombRune.soul          = 4

conjureFireBombRune.reagent       = 2260
conjureFireBombRune.product.id    = 2305
conjureFireBombRune.product.count = 2

conjureFireBombRune.effect        = MAGIC_EFFECT_RED_SHIMMER

conjureFireBombRune:register()