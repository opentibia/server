local magic_rope = Spell:new("Magic Rope")

magic_rope.words       = "exani tera"
magic_rope.vocation    = "any"
magic_rope.level       = 9
magic_rope.mana        = 20

magic_rope.effect = MAGIC_EFFECT_TELEPORT

function magic_rope:onBeginCast(event)
	local caster = event.caster
	local tile = map:getTile(caster:getPosition())
	
	for spotid, _ in pairs(otstd.ropespots) do
		print(spotid .. " == " .. tile:getGround():getItemID())
		if tile:getGround():getItemID() == spotid then
			return true
		end
	end
	caster:sendCancel("Sorry not possible.")
	return false
end

function magic_rope:onFinishCast(event)
	local caster = event.caster
	local npos = caster:getPosition()
	npos.y = npos.y + 1
	npos.z = npos.z - 1
	caster:moveTo(npos)
end

magic_rope:register()
