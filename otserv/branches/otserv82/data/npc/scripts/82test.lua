local focus = 0

function onCreatureAppear(cid)
end

function onCreatureDisappear(cid)
end

function onCreatureSay(cid, type, msg)
	if((msg == "hi") and (focus == 0))then
		selfSay("Welcome, ".. getCreatureName(cid) ..".")
		selfSay("Do you want to see my {wares}?", cid)
		focus = cid
	elseif((focus == cid) and (msg == "wares" or msg == "trade"))then
		selfSay("I don't have anything to trade, sorry.", cid)
	elseif((focus == cid) and (msg == "bye" or msg == "goodbye" or msg == "cya"))then
		selfSay("Goodbye!", cid, TRUE)
		focus = 0
	else
		selfSay("I don't understand you.", cid)
	end
	
	return 1
end

function onThink()
	if(focus ~= 0)then
		local distance = getDistanceTo(focus)
		if((distance > 4) or (distance == -1))then
			selfSay("Hmph!")
			focus = 0
		end
	end
end
