--gear storage
local STORAGE_VALUE_LOCATION = 12345

function onUse(cid, item, frompos, item2, topos)
	--Jammed
	local tim = getPlayerStorageValue(cid, STORAGE_VALUE_LOCATION)
	if tim ~= -1 and os.time() < tim then
		doPlayerSay(cid, "The tool jammed. Please wait " .. -(os.time()-tim) .. " seconds before using it again.", TALKTYPE_ORANGE)
		return true
	end
	if math.random(1, 100) == 1 then
		setPlayerStorageValue(cid, STORAGE_VALUE_LOCATION, os.time()+60)
		doPlayerSay(cid, "The tool jammed. Please wait 60 seconds before using it again.", TALKTYPE_ORANGE)
		return true
	end

	--Rope
	if useRope(cid, item, frompos, item2, topos) then
		return true
	end
	
	
	--Shovel
	if useShovel(cid, item, frompos, item2, topos) then
		return true
	end
	

	--Pick
	if usePick(cid, item, frompos, item2, topos) then
		return true
	end
	
	--Machete
	
	if useMachete(cid, item, frompos, item2, topos) then
		return true
	end
	
	--Scythe
	if useScythe(cid, item, frompos, item2, topos) then
		return true
	end	
	
	return false
end

