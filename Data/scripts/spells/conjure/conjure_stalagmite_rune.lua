local conjureStalagmiteRune = Spell:new("Stalagmite")

conjureStalagmiteRune.words         = "adori tera"
conjureStalagmiteRune.vocation      = {"Druid", "Elder Druid", "Sorcerer", "Master Sorcerer"}
conjureStalagmiteRune.level         = 24
conjureStalagmiteRune.mana          = 350
conjureStalagmiteRune.soul          = 2
conjureStalagmiteRune.premium       = true

conjureStalagmiteRune.reagent       = 2260
conjureStalagmiteRune.product.id    = 2292
conjureStalagmiteRune.product.count = 10

conjureStalagmiteRune.effect        = MAGIC_EFFECT_RED_SHIMMER

conjureStalagmiteRune:register()