local conjureParalyzeRune = Spell:new("Conjure Paralyze Rune")

conjureParalyzeRune.words         = "adana ani"
conjureParalyzeRune.vocation      = {"Druid", "Elder Druid"}
conjureParalyzeRune.level         = 54
conjureParalyzeRune.mana          = 1400
conjureParalyzeRune.soul          = 3
conjureParalyzeRune.premium       = true

conjureParalyzeRune.reagent       = 2260
conjureParalyzeRune.product.id    = 2278
conjureParalyzeRune.product.count = 1

conjureParalyzeRune.effect        = MAGIC_EFFECT_RED_SHIMMER

conjureParalyzeRune:register()