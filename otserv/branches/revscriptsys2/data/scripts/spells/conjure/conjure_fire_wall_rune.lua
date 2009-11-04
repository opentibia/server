local conjureFireWallRune = Spell:new("Conjure Fire Wall Rune")

conjureFireWallRune.words         = "adevo mas grav flam"
conjureFireWallRune.vocation      = {"Sorcerer", "Master Sorcerer", "Druid", "Elder Druid"}
conjureFireWallRune.level         = 33
conjureFireWallRune.mana          = 780
conjureFireWallRune.soul          = 4

conjureFireWallRune.reagent       = 2260
conjureFireWallRune.product.id    = 2303
conjureFireWallRune.product.count = 4

conjureFireWallRune.effect        = MAGIC_EFFECT_MAGIC_ENERGY

conjureFireWallRune:register()
