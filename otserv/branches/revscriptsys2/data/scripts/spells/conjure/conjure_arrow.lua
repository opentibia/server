local conjureArrow = Spell:new("Conjure Arrow")

conjureArrow.words         = "exevo con"
conjureArrow.vocation      = {"Paladin", "Royal Paladin"}
conjureArrow.level         = 13
conjureArrow.mana          = 100
conjureArrow.soul          = 3

conjureArrow.product.id    = 2544
conjureArrow.product.count = 10

conjureArrow.effect        = MAGIC_EFFECT_RED_SHIMMER

conjureArrow:register()
