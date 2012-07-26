local conjureAntidoteRune = Spell:new("Conjure Antidote Rune")

conjureAntidoteRune.words         = "adana pox"
conjureAntidoteRune.vocation      = {"Druid", "Elder Druid"}
conjureAntidoteRune.level         = 15
conjureAntidoteRune.mana          = 200
conjureAntidoteRune.soul          = 1

conjureAntidoteRune.reagent       = 2260
conjureAntidoteRune.product.id    = 2266
conjureAntidoteRune.product.count = 1

conjureAntidoteRune.effect        = MAGIC_EFFECT_RED_SHIMMER

conjureAntidoteRune:register()