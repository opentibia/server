local conjurePoisonedArrow = Spell:new("Conjure Poisoned Arrow")

conjurePoisonedArrow.words         = "exevo con pox"
conjurePoisonedArrow.vocation      = {"Paladin", "Royal Paladin"}
conjurePoisonedArrow.level         = 16
conjurePoisonedArrow.mana          = 130
conjurePoisonedArrow.soul          = 2

conjurePoisonedArrow.product.id    = 2545
conjurePoisonedArrow.product.count = 7

conjurePoisonedArrow.effect        = MAGIC_EFFECT_RED_SHIMMER

conjurePoisonedArrow:register()
