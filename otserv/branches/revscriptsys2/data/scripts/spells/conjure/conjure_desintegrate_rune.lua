local conjureDesintegrateRune = Spell:new("Conjure Desintegrate Rune")

conjureDesintegrateRune.words         = "adito tera"
conjureDesintegrateRune.vocation      = {"Druid", "Elder Druid", "Sorcerer", "Master Sorcerer", "Paladin", "Royal Paladin"}
conjureDesintegrateRune.level         = 21
conjureDesintegrateRune.mana          = 200
conjureDesintegrateRune.soul          = 3
conjureDesintegrateRune.premium       = true

conjureDesintegrateRune.reagent       = 2260
conjureDesintegrateRune.product.id    = 2310
conjureDesintegrateRune.product.count = 3

conjureDesintegrateRune.effect        = MAGIC_EFFECT_RED_SHIMMER

conjureDesintegrateRune:register()