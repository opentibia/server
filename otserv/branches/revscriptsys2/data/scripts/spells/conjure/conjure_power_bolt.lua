local conjurePowerBolt = Spell:new("Conjure Power Bolt")

conjurePowerBolt.words         = "exevo con vis"
conjurePowerBolt.vocation      = {"Royal Paladin"}
conjurePowerBolt.level         = 59
conjurePowerBolt.mana          = 700
conjurePowerBolt.soul          = 4
conjurePowerBolt.premium       = true

conjurePowerBolt.product.id    = 2547
conjurePowerBolt.product.count = 10

conjurePowerBolt.effect        = MAGIC_EFFECT_RED_SHIMMER

conjurePowerBolt:register()
