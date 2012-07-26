local conjureExplosiveArrow = Spell:new("Conjure Explosive Arrow")

conjureExplosiveArrow.words         = "exevo con flam"
conjureExplosiveArrow.vocation      = {"Paladin", "Royal Paladin"}
conjureExplosiveArrow.level         = 25
conjureExplosiveArrow.mana          = 290
conjureExplosiveArrow.soul          = 3

conjureExplosiveArrow.product.id    = 2546
conjureExplosiveArrow.product.count = 8

conjureExplosiveArrow.effect        = MAGIC_EFFECT_RED_SHIMMER

conjureExplosiveArrow:register()
