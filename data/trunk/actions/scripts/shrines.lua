local HOTA_WEAK = 2342
local HOTA_FULL = 2343

local SHRINES = {
	-- Fire Shrine
	[SMALL_RUBY]	= {7504, 7505, 7506, 7507},

	-- Ice Shrine
	[SMALL_SAPPHIRE]= {7508, 7509, 7510, 7511},

	-- Energy Shrine
	[SMALL_AMETHYST]= {7512, 7513, 7514, 7515},

	-- Earth Shrine
	[SMALL_EMERALD]	= {7516, 7517, 7518, 7519}
}

local ENCHANTED_GEMS = {
	[SMALL_SAPPHIRE]= ENCHANTED_SMALL_SAPPHIRE,
	[SMALL_RUBY]	= ENCHANTED_SMALL_RUBY,
	[SMALL_EMERALD]	= ENCHANTED_SMALL_EMERALD,
	[SMALL_AMETHYST]= ENCHANTED_SMALL_AMETHYST
}

function onUse(cid, item, frompos, item2, topos)
	if(item2.itemid == HOTA_WEAK and item.itemid == SMALL_RUBY) then
		doRemoveItem(item.uid, 1)
		doTransformItem(item2.uid, HOTA_FULL)
		if item2.actionid ~= 0 then
			doSetItemActionId(item2.uid, item2.actionid)
		end
		doSendMagicEffect(topos, CONST_ME_MAGIC_RED)
		return true
	end

	if (isInArray(SHRINES[item.itemid], item2.itemid) == false) then
		return false
	end

	local count = item.type ~= 0 and item.type or 1
	local manaCost = 300 * count
	local soulCost = 2 * count
	local requiredLevel = 30

	if (getPlayerLevel(cid) < requiredLevel) then
		doPlayerSendDefaultCancel(cid, RETURNVALUE_NOTENOUGHLEVEL)
		return true
	end

	if (isPremium(cid) == false) then
		doPlayerSendDefaultCancel(cid, RETURNVALUE_YOUNEEDPREMIUMACCOUNT)
		return true
	end

	if (getPlayerMana(cid) < manaCost) then
		doPlayerSendDefaultCancel(cid, RETURNVALUE_NOTENOUGHMANA)
		return true
	end

	if (getPlayerSoul(cid) < soulCost) then
		doPlayerSendDefaultCancel(cid, RETURNVALUE_NOTENOUGHSOUL)
		return true
	end

	doPlayerAddMana(cid, -manaCost)
	doPlayerAddSoul(cid, -soulCost)
	doTransformItem(item.uid, ENCHANTED_GEMS[item.itemid], count)

	return true
end
