--[[
This file will, in future, have the script for the holy shrines
Meanwhile, it only has the HoTA code.
(I created enchanting.lua that is more powerful than this script :) )
]]
local HOTA_WEAK = 2342
local HOTA_FULL = 2343

function onUse(cid, item, frompos, item2, topos)
		if(item2.itemid == HOTA_WEAK) then
			doRemoveItem(item.uid, 1)
			doTransformItem(item2.uid, HOTA_FULL)
			doSendMagicEffect(topos, CONST_ME_MAGIC_RED)
			return TRUE
		end
	return FALSE
end