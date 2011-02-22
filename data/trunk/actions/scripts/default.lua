function isBreakingTool(item)
	local t = getItemWeaponTypeByUID(item.uid)
	return (t == WEAPON_SWORD) or (t == WEAPON_CLUB) or (t == WEAPON_AXE)
end

function onUse(cid, item, frompos, item2, topos)
	if BREAKABLE_BY_WEAPONS[item2.itemid] ~= nil and isBreakingTool(item) then
		local chance = BREAKABLE_BY_WEAPONS[item2.itemid].chance or 20
		if math.random(100) <= chance then
			doTransformItem(item2.uid, BREAKABLE_BY_WEAPONS[item2.itemid].remainings)
			doDecayItem(item2.uid)
			if item2.actionid ~= 0 then
				doSetItemActionId(item2.uid, item2.actionid)
			end
		end
		doSendMagicEffect(topos, CONST_ME_POFF)
		return true
	end

	return false
end
