local conjureIntenseHealingRune = Spell:new("Conjure Intense Healing Rune")

conjureIntenseHealingRune.words         = "adura gran"
conjureIntenseHealingRune.vocation      = {"Druid", "Elder Druid"}
conjureIntenseHealingRune.level         = 15
conjureIntenseHealingRune.mana          = 120
conjureIntenseHealingRune.soul          = 2

conjureIntenseHealingRune.reagent       = 2260
conjureIntenseHealingRune.product.id    = 2265
conjureIntenseHealingRune.product.count = 1

conjureIntenseHealingRune.effect        = MAGIC_EFFECT_RED_SHIMMER

conjureIntenseHealingRune:register()