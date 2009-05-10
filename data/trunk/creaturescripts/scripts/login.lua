function onLogin(cid)
	--Register the kill/die event
	registerCreatureEvent(cid, "AutoBan")
	registerCreatureEvent(cid, "RemoveBlesses")

	--Remove blesses if necessary
	if getPlayerStorageValue(cid, STORAGE_REMOVE_BLESSES) == 1 then
		local i = 0
		while i < 5 do
			doPlayerRemoveBless(cid, i)
			i = i + 1
		end
		setPlayerStorageValue(cid, STORAGE_REMOVE_BLESSES, -1)
	end

	return TRUE
end