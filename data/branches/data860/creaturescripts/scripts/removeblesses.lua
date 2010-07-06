function onDie(cid, corpse)
	if isPlayer(cid) == TRUE then
		-- Remove all blesses
		-- We can not remove blesses now, since onDie is called before removing experience
		-- So we will set storage to remove in onLogin
		setPlayerStorageValue(cid, STORAGE_REMOVE_BLESSES, 1)
	end
end