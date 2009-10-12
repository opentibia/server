local UltimateHealingRune = Spell:new("Ultimate Healing Rune")

UltimateHealingRune.words       = "adura vita"
UltimateHealingRune.vocation    = Player.isDruid
UltimateHealingRune.level       = 22
UltimateHealingRune.mana        = 400
UltimateHealingRune.health      = 0

UltimateHealingRune.effect = MAGIC_EFFECT_MAGIC_BLUE

UltimateHealingRune.reagent       = 2260
UltimateHealingRune.product.id    = 2273
UltimateHealingRune.product.count = 1

UltimateHealingRune:register()
