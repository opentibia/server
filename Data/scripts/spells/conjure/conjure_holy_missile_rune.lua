local conjureHolyMissileRune = Spell:new("Conjure Holy Missile Rune")

conjureHolyMissileRune.words         = "adori san"
conjureHolyMissileRune.vocation      = {"Paladin", "Roayl Paladin"}
conjureHolyMissileRune.level         = 27
conjureHolyMissileRune.mana          = 350
conjureHolyMissileRune.soul          = 3
conjureHolyMissileRune.premium       = true

conjureHolyMissileRune.reagent       = 2260
conjureHolyMissileRune.product.id    = 2295
conjureHolyMissileRune.product.count = 5

conjureHolyMissileRune.effect        = MAGIC_EFFECT_RED_SHIMMER

conjureHolyMissileRune:register()