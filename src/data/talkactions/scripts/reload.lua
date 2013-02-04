function onSay(cid, words, param)
	if getPlayerFlagValue(cid, PLAYERFLAG_CANRELOADCONTENT) == false then
		return true
	end

	if param == "" then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "You need to type the parameter.")
		return false
	end

	if param == "actions" or param == "action" then
		doReloadInfo(RELOAD_TYPE_ACTIONS)
		param = "actions"
	elseif param == "monsters" or param == "monster" then
		doReloadInfo(RELOAD_TYPE_MONSTERS)
		param = "monsters"
	elseif param == "npcs" or param == "npc" then
		doReloadInfo(RELOAD_TYPE_NPCS)
		param = "npcs"
	elseif param == "config" then
		doReloadInfo(RELOAD_TYPE_CONFIG)
	elseif param == "talkactions" or param == "talkaction" or param == "ta" or param == "talk" then
		doReloadInfo(RELOAD_TYPE_TALKACTIONS)
		param = "talkactions"
	elseif param == "movements" or param == "movement" or param == "move" then
		doReloadInfo(RELOAD_TYPE_MOVEMENTS)
		param = "movements"
	elseif param == "spells" or param == "spell" then
		doReloadInfo(RELOAD_TYPE_SPELLS)
		param = "spells"
	elseif param == "raids" or param == "raid" then
		doReloadInfo(RELOAD_TYPE_RAIDS)
		param = "raids"
	elseif param == "creaturescripts" or param == "creaturescript" or param == "cs" then
		doReloadInfo(RELOAD_TYPE_CREATURESCRIPTS)
		param = "creaturescripts"
	elseif param == "outfits" or param == "outfit" then
		doReloadInfo(RELOAD_TYPE_OUTFITS)
		param = "outfits"
	elseif param == "mounts" or param == "mount" then
		doReloadInfo(RELOAD_TYPE_MOUNTS)
		param = "mounts"
	elseif param == "items" or param == "item" then
		doReloadInfo(RELOAD_TYPE_ITEMS)
		param = "items"
	elseif param == "globalevent" or param == "globalevents" then
		doReloadInfo(RELOAD_TYPE_GLOBALEVENTS)
		param = "globalevents"
	else
		param = ""
	end

	if param == "" then
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Invalid parameter.")
	else
		doPlayerSendTextMessage(cid, MESSAGE_STATUS_CONSOLE_BLUE, "Reloaded " .. param .. ".")
	end

	return false
end
