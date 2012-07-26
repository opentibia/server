local enchantSpear = Spell:new("Enchant Spear")

enchantSpear.words         = "exeta con"
enchantSpear.vocation      = {"Paladin", "Royal Paladin"}
enchantSpear.level         = 45
enchantSpear.mana          = 350
enchantSpear.soul          = 3
enchantSpear.premium       = true

enchantSpear.reagent       = 2389
enchantSpear.product.id    = 7367
enchantSpear.product.count = 1

enchantSpear.effect        = MAGIC_EFFECT_RED_SHIMMER

enchantSpear:register()
