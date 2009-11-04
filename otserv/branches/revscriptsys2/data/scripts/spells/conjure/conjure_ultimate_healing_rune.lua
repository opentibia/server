local conjureUltimateHealingRune = Spell:new("Conjure Ultimate Healing Rune")

conjureUltimateHealingRune.words         = "adura vita"
conjureUltimateHealingRune.vocation      = {"Druid", "Elder Druid"}
conjureUltimateHealingRune.level         = 24
conjureUltimateHealingRune.mana          = 400
conjureUltimateHealingRune.soul          = 3

conjureUltimateHealingRune.reagent       = 2260
conjureUltimateHealingRune.product.id    = 2273
conjureUltimateHealingRune.product.count = 1

conjureUltimateHealingRune.effect        = MAGIC_EFFECT_MAGIC_ENERGY

conjureUltimateHealingRune:register()
