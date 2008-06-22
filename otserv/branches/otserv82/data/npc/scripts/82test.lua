local focuses = {}
local function isFocused(cid)
	for i, v in pairs(focuses) do
		if(v == cid) then
			return true
		end
	end
	return false
end

local function addFocus(cid)
	if(not isFocused(cid)) then
		table.insert(focuses, cid)
	end
end
local function removeFocus(cid)
	for i, v in pairs(focuses) do
		if(v == cid) then
			table.remove(focuses, i)
			break
		end
	end
end
local function lookAtFocus()
	for i, v in pairs(focuses) do
		if(isPlayer(v) == TRUE) then
			doNpcSetCreatureFocus(v)
			return
		end
	end
	doNpcSetCreatureFocus(0)
end

local itemWindow = {
	{id=2160, charges=1, buy=10000, sell=10000},
	{id=2152, charges=1, buy=100, sell=100},
	{id=2148, charges=1, buy=1, sell=1}
}

local items = {}
for _, item in ipairs(itemWindow) do
	items[item.id] = {buyPrice = item.buy, sellPrice = item.sell, charges = item.charges}
end

local onBuy = function(cid, item, charges, amount)
	if(items[item] == nil) then
		selfSay("Ehm.. sorry... this shouldn't be there, I'm not selling it.", cid)
		return
	end

	if(doPlayerRemoveMoney(cid, amount*items[item].buyPrice) == TRUE) then
		doPlayerAddItem(cid, item, amount)
		selfSay("Thanks for the money!", cid)
	else
		selfSay("Stfu noob, you don't have money.", cid)
	end
end

local onSell = function(cid, item, charges, amount)
	if(items[item] == nil) then
		selfSay("Ehm.. sorry... this shouldn't be there, I'm not buying it.", cid)
	end

	if(charges <= 1) then
		charges = -1
	end
	if(doPlayerRemoveItem(cid, item, amount, charges) == TRUE) then
		doPlayerAddMoney(cid, items[item].sellPrice*amount)
		selfSay("Here you are.", cid)
	else
		selfSay("No item, no deal.", cid)
	end
end

function onCreatureAppear(cid)
end

function onCreatureDisappear(cid)
	if(isFocused(cid)) then
		selfSay("Hmph!")
		focus = 0
		if(isPlayer(cid) == TRUE) then --Be sure he's online
			closeShopWindow(cid)
		end
	end
end

function onCreatureSay(cid, type, msg)
	if((msg == "hi") and not (isFocused(cid))) then
		selfSay("Welcome, ".. getCreatureName(cid) ..".", cid, TRUE)
		selfSay("Do you want to see my {wares}?", cid)
		addFocus(cid)
	elseif((isFocused(cid)) and (msg == "wares" or msg == "trade")) then
		selfSay("Pretty nice, right?", cid)
		sendShopWindow(cid, itemWindow, onBuy, onSell)
	elseif((isFocused(cid)) and (msg == "bye" or msg == "goodbye" or msg == "cya")) then
		selfSay("Goodbye!", cid, TRUE)
		closeShopWindow(cid)
		focus = 0
	end
end

function onPlayerCloseChannel(cid)
	if(isFocused(cid)) then
		selfSay("Hmph!")
		closeShopWindow(cid)
		focus = 0
	end
end

function onPlayerEndTrade(cid)
	selfSay("It was a pleasure doing business with you.", cid)
end

function onThink()
	for i, focus in pairs(focuses) do
		if(isCreature(focus) == FALSE) then
			removeFocus(focus)
		else
			local distance = getDistanceTo(focus) or -1
			if((distance > 4) or (distance == -1)) then
				selfSay("Hmph!")
				closeShopWindow(focus)
				removeFocus(focus)
			end
		end
	end
	lookAtFocus()
end
