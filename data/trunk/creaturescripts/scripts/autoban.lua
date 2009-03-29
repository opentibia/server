function onKill(cid, target)
	if(isPlayer(cid) == TRUE and isPlayer(target) == TRUE) then
		if(getPlayerFrags(cid) >= getConfigValue('frags_to_banishment')) then
			local playername = getPlayerName(cid)
			local accno = getAccountNumberByPlayerName(playername)
			addAccountBan(accno, getConfigValue('kill_ban_time'), 0, "Excessive player killing.")
			-- Kick all players that belong to the banished account
			local players = getPlayersByAccountNumber(accno)
			for _, pid in pairs(players) do
				addEvent(doRemoveCreature, 1000, pid)
			end
		end
	end	

	return 1
end