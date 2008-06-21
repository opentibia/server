local focus = 0

local onBuy = function(cid, item, count, amount)
	selfSay("You can't buy this, noob!", cid)
end

local onSell = function(cid, item, count, amount)
	selfSay("You want to sell me this trash? No way!", cid)
end

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
		selfSay("Pretty nice, right?", cid)
		sendShopWindow(cid, {{id=2160, charges=1, buy=10000, sell=10000},
							{id=2152, charges=1, buy=100, sell=100},
							{id=2148, charges=1, buy=1, sell=1}}, onBuy, onSell)
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
