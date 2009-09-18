local conjure_arrows = Spell:new("Conjure Arrows")

conjure_arrows.words       = "exevo con"
conjure_arrows.vocation    = Player.isPaladin
conjure_arrows.level       = 8
conjure_arrows.mana        = 30
conjure_arrows.health      = 3

conjure_arrows.effect = CONST_ME_MAGIC_BLUE

conjure_arrows.product.id    = 2544
conjure_arrows.product.count = 15

conjure_arrows:register()
