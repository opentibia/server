local conjureConvinceCreatureRune = Spell:new("Conjure Convince Creature Rune")

conjureConvinceCreatureRune.words         = "adeta sio"
conjureConvinceCreatureRune.vocation      = {"Druid", "Elder Druid"}
conjureConvinceCreatureRune.level         = 16
conjureConvinceCreatureRune.mana          = 200
conjureConvinceCreatureRune.soul          = 3

conjureConvinceCreatureRune.reagent       = 2260
conjureConvinceCreatureRune.product.id    = 2290
conjureConvinceCreatureRune.product.count = 1

conjureConvinceCreatureRune.effect        = MAGIC_EFFECT_RED_SHIMMER

conjureConvinceCreatureRune:register()