local conjureArrows = Spell:new("Conjure Arrows")

conjureArrows.words         = "exevo con"
conjureArrows.vocation      = {"Paladin", "Royal Paladin"}
conjureArrows.level         = 13
conjureArrows.mana          = 100
conjureArrows.soul          = 3

conjureArrows.product.id    = 2544
conjureArrows.product.count = 10

conjureArrows.effect        = MAGIC_EFFECT_MAGIC_ENERGY

conjureArrows:register()
