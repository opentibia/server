local conjureAvalancheRune = Spell:new("Conjure Avalanche Rune")

conjureAvalancheRune.words         = "adori mas frigo"
conjureAvalancheRune.vocation      = {"Druid", "Elder Druid"}
conjureAvalancheRune.level         = 30
conjureAvalancheRune.mana          = 530
conjureAvalancheRune.soul          = 3

conjureAvalancheRune.reagent       = 2260
conjureAvalancheRune.product.id    = 2274
conjureAvalancheRune.product.count = 4

conjureAvalancheRune.effect        = MAGIC_EFFECT_RED_SHIMMER

conjureAvalancheRune:register()