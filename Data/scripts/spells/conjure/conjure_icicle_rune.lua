local conjureIcicleRune = Spell:new("Conjure Icicle Rune")

conjureIcicleRune.words         = "adori frigo"
conjureIcicleRune.vocation      = {"Druid", "Elder Druid"}
conjureIcicleRune.level         = 28
conjureIcicleRune.mana          = 460
conjureIcicleRune.soul          = 3
conjureIcicleRune.premium       = true

conjureIcicleRune.reagent       = 2260
conjureIcicleRune.product.id    = 2271
conjureIcicleRune.product.count = 5

conjureIcicleRune.effect        = MAGIC_EFFECT_RED_SHIMMER

conjureIcicleRune:register()