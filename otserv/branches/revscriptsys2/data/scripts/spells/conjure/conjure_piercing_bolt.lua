local conjurePiercingBolt = Spell:new("Conjure Piercing Bolt")

conjurePiercingBolt.words         = "exevo con grav"
conjurePiercingBolt.vocation      = {"Paladin", "Royal Paladin"}
conjurePiercingBolt.level         = 33
conjurePiercingBolt.mana          = 180
conjurePiercingBolt.soul          = 3
conjurePiercingBolt.premium       = true

conjurePiercingBolt.product.id    = 7363
conjurePiercingBolt.product.count = 5

conjurePiercingBolt.effect        = MAGIC_EFFECT_RED_SHIMMER

conjurePiercingBolt:register()
