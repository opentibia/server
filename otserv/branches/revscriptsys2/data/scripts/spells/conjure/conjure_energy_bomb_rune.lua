local conjureEnergyBombRune = Spell:new("Conjure Energy Bomb Rune")

conjureEnergyBombRune.words          = "adevo mas vis"
conjureEnergyBombRune.vocation       = {"Sorcerer", "Master Sorcerer"}
conjureEnergyBombRune.level          = 37
conjureEnergyBombRune.mana           = 600
conjureEnergyBombRune.soul           = 5
conjureEnergyBombRune.premium        = true

conjureEnergyBombRune.reagent        = 2260
conjureEnergyBombRune.product.id     = 2262
conjureEnergyBombRune.product.count  = 2

conjureEnergyBombRune.effect         = MAGIC_EFFECT_MAGIC_ENERGY

conjureEnergyBombRune:register()
