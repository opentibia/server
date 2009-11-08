local conjureEnergyWallRune = Spell:new("Conjure Energy Wall Rune")

conjureEnergyWallRune.words         = "adevo mas grav vis"
conjureEnergyWallRune.vocation      = {"Druid", "Elder Druid", "Sorcerer", "Master Sorcerer"}
conjureEnergyWallRune.level         = 41
conjureEnergyWallRune.mana          = 1000
conjureEnergyWallRune.soul          = 5

conjureEnergyWallRune.reagent       = 2260
conjureEnergyWallRune.product.id    = 2279
conjureEnergyWallRune.product.count = 4

conjureEnergyWallRune.effect        = MAGIC_EFFECT_RED_SHIMMER

conjureEnergyWallRune:register()