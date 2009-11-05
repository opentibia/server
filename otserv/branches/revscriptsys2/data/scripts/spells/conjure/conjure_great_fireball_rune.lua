local conjureGreatFireballRune = Spell:new("Conjure Great Fireball Rune")

conjureGreatFireballRune.words         = "adori mas flam"
conjureGreatFireballRune.vocation      = {"Sorcerer", "Master Sorcerer"}
conjureGreatFireballRune.level         = 30
conjureGreatFireballRune.mana          = 530
conjureGreatFireballRune.soul          = 3

conjureGreatFireballRune.reagent       = 2260
conjureGreatFireballRune.product.id    = 2304
conjureGreatFireballRune.product.count = 4

conjureGreatFireballRune.effect        = MAGIC_EFFECT_MAGIC_ENERGY


conjureGreatFireballRune:register()
