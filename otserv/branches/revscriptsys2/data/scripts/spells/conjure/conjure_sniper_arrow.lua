local conjureSniperArrow = Spell:new("Conjure Sniper Arrow")

conjureSniperArrow.words         = "exevo con hur"
conjureSniperArrow.vocation      = {"Paladin", "Royal Paladin"}
conjureSniperArrow.level         = 24
conjureSniperArrow.mana          = 160
conjureSniperArrow.soul          = 3
conjureSniperArrow.premium       = true

conjureSniperArrow.product.id    = 7364
conjureSniperArrow.product.count = 5

conjureSniperArrow.effect        = MAGIC_EFFECT_RED_SHIMMER

conjureSniperArrow:register()
