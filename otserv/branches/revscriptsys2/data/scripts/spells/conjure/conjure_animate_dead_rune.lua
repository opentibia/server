local conjureAnimateDeadRune = Spell:new("Conjure Animate Dead Rune")

conjureAnimateDeadRune.words         = "adana mort"
conjureAnimateDeadRune.vocation      = {"Druid", "Elder Druid", "Sorcerer", "Master Sorcerer"}
conjureAnimateDeadRune.level         = 27
conjureAnimateDeadRune.mana          = 600
conjureAnimateDeadRune.soul          = 5
conjureAnimateDeadRune.premium       = true

conjureAnimateDeadRune.reagent       = 2260
conjureAnimateDeadRune.product.id    = 2316
conjureAnimateDeadRune.product.count = 1

conjureAnimateDeadRune.effect        = MAGIC_EFFECT_RED_SHIMMER

conjureAnimateDeadRune:register()