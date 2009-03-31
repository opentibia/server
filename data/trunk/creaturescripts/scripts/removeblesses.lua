function onDie(cid, corpse)
	if isPlayer(cid) == TRUE then
		-- Remove all blesses
		-- We can do this without problems, since onDie is called after removing players experience/skill tries/mana spent
		local i = 0
		while i < 5 do
			doPlayerRemoveBless(cid, i)
			i = i + 1
		end
	end
end