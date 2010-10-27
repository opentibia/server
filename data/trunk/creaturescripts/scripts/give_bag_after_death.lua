function onDie(cid, corpse)
	if isPlayer(cid)  then
		-- if a player dies, set storage value to add bag at the relogin
		setPlayerStorageValue(cid, STORAGE_DEATH_BAG, 1)
	end
end