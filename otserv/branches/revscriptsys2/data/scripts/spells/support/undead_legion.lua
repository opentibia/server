local undeadLegion = Spell:new("Undead Legion")

undeadLegion.words       = "exana mas mort"
undeadLegion.vocation    = "druid"
undeadLegion.level       = 30
undeadLegion.mana        = 500
undeadLegion.premium = true
undeadLegion.areaEffect  = MAGIC_EFFECT_MAGIC_BLOOD

undeadLegion.area        =
	{
		{" ", " ", "a", "a", "a", " ", " "},
		{" ", "a", "a", "a", "a", "a", " "},
		{"a", "a", "a", "a", "a", "a", "a"},
		{"a", "a", "a", " ", "a", "a", "a"},
		{"a", "a", "a", "a", "a", "a", "a"},
		{" ", "a", "a", "a", "a", "a", " "},
		{" ", " ", "a", "a", "a", " ", " "},
	}
	
undeadLegion.onHitTile = function(tile, event)
	local corpse = tile:getTopItem()
	if corpse:isCorpse() and corpse:isMoveable() then
		corpse:destroy()
		local skeleton = createMonster("skeleton", tile)
		if skeleton then
			event.caster:addSummon(skeleton)
		end
	end
end

undeadLegion:register()
