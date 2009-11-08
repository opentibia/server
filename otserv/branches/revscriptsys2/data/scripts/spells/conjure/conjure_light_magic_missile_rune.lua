local conjureLightMagicMissileRune = Spell:new("Conjure Light Magic Missile Rune")

conjureLightMagicMissileRune.words         = "adori min vis"
conjureLightMagicMissileRune.vocation      = {"Druid", "Elder Druid", "Sorcerer", "Master Sorcerer"}
conjureLightMagicMissileRune.level         = 15
conjureLightMagicMissileRune.mana          = 120
conjureLightMagicMissileRune.soul          = 1

conjureLightMagicMissileRune.reagent       = 2260
conjureLightMagicMissileRune.product.id    = 2287
conjureLightMagicMissileRune.product.count = 10

conjureLightMagicMissileRune.effect        = MAGIC_EFFECT_RED_SHIMMER

conjureLightMagicMissileRune:register()