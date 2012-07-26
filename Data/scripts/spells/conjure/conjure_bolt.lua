local conjureBolt = Spell:new("Conjure Bolt")

conjureBolt.words         = "exevo con mort"
conjureBolt.vocation      = {"Paladin", "Royal Paladin"}
conjureBolt.level         = 17
conjureBolt.mana          = 140
conjureBolt.soul          = 2
conjureBolt.premium       = true

conjureBolt.product.id    = 2543
conjureBolt.product.count = 5

conjureBolt.effect        = MAGIC_EFFECT_RED_SHIMMER

conjureBolt:register()
