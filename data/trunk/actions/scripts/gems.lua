--[[
This file will, in future, have the script for the holy shrines
Meanwhile, it only has the HoTA code.
]]
local HOTA_WEAK = 2342
local HOTA_FULL = 2343
--local SMALL_RUBY = 2147

function onUse(cid, item, frompos, item2, topos)
	--if(item.itemid == SMALL_RUBY) then
		if(item2.itemid == HOTA_WEAK) then
			doRemoveItem(item.uid, 1)
			doTransformItem(item2.uid, HOTA_FULL)
			doSendMagicEffect(topos, CONST_ME_MAGIC_RED)
			return TRUE
		end
	--end

	--[[ Holy Shrines code, in the future ]]

	return FALSE
end