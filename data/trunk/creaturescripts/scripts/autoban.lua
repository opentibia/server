function onKill(cid, target)
	local frags_to_banishment = getConfigValue('frags_to_banishment')
	if(isPlayer(cid) == TRUE and isPlayer(target) == TRUE) then
		if(getPlayerFrags(cid) >= frags_to_banishment and frags_to_banishment > 0) then
			local playername = getPlayerName(cid)
			local accno = getAccountNumberByPlayerName(playername)
			addAccountBan(accno, getConfigValue('kill_ban_time'), 0, "Excessive player killing.")
			-- Kick all players that belong to the banished account
			local players = getPlayersByAccountNumber(accno)
			for _, pid in pairs(players) do
				doSendMagicEffect(getPlayerPosition(pid), CONST_ME_MAGIC_GREEN)
				addEvent(doRemoveCreature, 1000, pid)
			end
		end
	end	

	return 1
end