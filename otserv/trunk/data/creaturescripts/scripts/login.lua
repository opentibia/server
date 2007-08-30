

function onLogin(cid)

	doPlayerSay(cid, "Hi all!", 3)
	registerCreatureEvent(cid, "KILL_MESSAGES")
	return 1

end