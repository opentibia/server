local conjureExplosionRune = Spell:new("Conjure Explosion Rune")

conjureExplosionRune.words         = "adevo mas hur"
conjureExplosionRune.vocation      = {"Sorcerer", "Master Sorcerer", "Druid", "Elder Druid"}
conjureExplosionRune.level         = 31
conjureExplosionRune.mana          = 570
conjureExplosionRune.soul          = 4

conjureExplosionRune.reagent       = 2260
conjureExplosionRune.product.id    = 2313
conjureExplosionRune.product.count = 6

conjureExplosionRune.effect        = MAGIC_EFFECT_RED_SHIMMER

conjureExplosionRune:register()
