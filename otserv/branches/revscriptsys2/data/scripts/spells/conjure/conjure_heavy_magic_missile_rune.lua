local conjureHeavyMagicMissileRune = Spell:new("Conjure Heavy Magic Missile Rune")

conjureHeavyMagicMissileRune.words         = "adori vis"
conjureHeavyMagicMissileRune.vocation      = {"Druid", "Elder Druid", "Sorcerer", "Master Sorcerer"}
conjureHeavyMagicMissileRune.level         = 25
conjureHeavyMagicMissileRune.mana          = 350
conjureHeavyMagicMissileRune.soul          = 2

conjureHeavyMagicMissileRune.reagent       = 2260
conjureHeavyMagicMissileRune.product.id    = 2311
conjureHeavyMagicMissileRune.product.count = 10

conjureHeavyMagicMissileRune.effect        = MAGIC_EFFECT_RED_SHIMMER

conjureHeavyMagicMissileRune:register()