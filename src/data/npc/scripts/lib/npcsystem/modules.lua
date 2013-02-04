-- This file is part of Jiddo's advanced NpcSystem v3.0x. This npcsystem is free to use by anyone, for any purpuse.
-- Initial release date: 2007-02-21
-- Credits: Jiddo, honux(I'm using a modified version of his Find function).
-- Please include full credits whereever you use this system, or parts of it.
-- For support, questions and updates, please consult the following thread:
-- http://otfans.net/showthread.php?t=67810

if(Modules == nil) then

	--TODO: modifiers for these values
	-- default words for greeting and ungreeting the npc. Should be a talbe containing all such words.
	FOCUS_GREETWORDS = {'hi', 'hello'}
	FOCUS_FAREWELLWORDS = {'bye', 'farewell', 'cya'}

	-- The words for requesting trade window.
	SHOP_TRADEREQUEST = {'offer', 'trade', 'wares'}

	-- The word for accepting/declining an offer. CAN ONLY CONTAIN ONE FIELD! Should be a teble with a single string value.
	-- Mainly used for travel module
	SHOP_YESWORD = {'yes'}
	SHOP_NOWORD = {'no'}

	-- Pattern used to get the amount of an item a player wants to buy/sell.
	PATTERN_COUNT = '%d+'


	Modules = {
			parseableModules = {}
		}






	StdModule = {

		}

	-- These callback function must be called with parameters.npcHandler = npcHandler in the parameters table or they will not work correctly.
	-- Notice: The members of StdModule have not yet been tested. If you find any bugs, please report them to me.

	-- Usage:
		-- keywordHandler:addKeyword({'offer'}, StdModule.say, {npcHandler = npcHandler, text = 'I sell many powerful melee weapons.'})
	function StdModule.say(cid, message, keywords, parameters, node)
		local npcHandler = parameters.npcHandler
		if(npcHandler == nil) then
			print('[Warning - ' .. getCreatureName(getNpcCid()) .. '] StdModule.say called without any npcHandler instance.')
			return false
		end
		local onlyFocus = (parameters.onlyFocus == nil or parameters.onlyFocus == true)
		if(not npcHandler:isFocused(cid) and onlyFocus) then
			return false
		end
		local parseInfo = {
				[TAG_PLAYERNAME] = getPlayerName(cid),
			}
		msgout = npcHandler:parseMessage(parameters.text or parameters.message, parseInfo)
		npcHandler:say(msgout, cid, parameters.publicize and true)
		if(parameters.reset == true) then
			npcHandler:resetNpc()
		elseif(parameters.moveup ~= nil and type(parameters.moveup) == 'number') then
			npcHandler.keywordHandler:moveUp(parameters.moveup)
		end
		return true
	end



	--Usage:
		-- local node1 = keywordHandler:addKeyword({'promot'}, StdModule.say, {npcHandler = npcHandler, text = 'I can promote you for 20000 gold coins. Do you want me to promote you?'})
		--         node1:addChildKeyword({'yes'}, StdModule.promotePlayer, {npcHandler = npcHandler, cost = 20000, level = 20}, text = 'Congratulations! You are now promoted.')
		--         node1:addChildKeyword({'no'}, StdModule.say, {npcHandler = npcHandler, text = 'Allright then. Come back when you are ready.'}, reset = true)
		
local function getPromotedVocation(voc)
		if(voc >= 1 and voc <= 4) then
				return voc + 4
		else
				return voc
		end
end
		
	function StdModule.promotePlayer(cid, message, keywords, parameters, node)
		local npcHandler = parameters.npcHandler
		if(npcHandler == nil) then
			print('[Warning - ' .. getCreatureName(getNpcCid()) .. '] StdModule.promotePlayer called without any npcHandler instance.')
			return false
		end
		if(not npcHandler:isFocused(cid)) then
			return false
		end

		if(isPlayerPremiumCallback == nil or isPlayerPremiumCallback(cid) == true or parameters.premium == false) then
			local currentVoc = getPlayerVocation(cid)
			local promotedVoc = getPromotedVocation(currentVoc)
			
			if(currentVoc == promotedVoc) then
				npcHandler:say('You are already promoted!', cid)
			elseif(getPlayerLevel(cid) < parameters.level) then
				npcHandler:say('I am sorry, but I can only promote you once you have reached level ' .. parameters.level .. '.', cid)
			elseif(not doPlayerRemoveMoney(cid, parameters.cost)) then
				npcHandler:say('You do not have enough money!', cid)
			else
				doPlayerSetVocation(cid, promotedVoc)
				doPlayerRemoveSkillLossPercent(cid, 30)
				setPlayerStorageValue(cid, STORAGE_PROMOTION, -1)
				npcHandler:say(parameters.text, cid)
			end
		else
			npcHandler:say("You need a premium account in order to get promoted", cid)
		end
		npcHandler:resetNpc()
		return true
	end
	
	--Usage:
		--local node1 = keywordHandler:addKeyword({'wisdom of solitude'}, StdModule.say, {npcHandler = npcHandler, onlyFocus = true, text = 'Do you want to buy wisdom of solitude for 2000 (plus level depending amount) gold?'})
		--node1:addChildKeyword({'yes'}, StdModule.bless, {npcHandler = npcHandler, number = 0, premium = false, baseCost = 2000, levelCost = 200, startLevel = 30, endLevel = 120})
		--node1:addChildKeyword({'no'}, StdModule.say, {npcHandler = npcHandler, onlyFocus = true, reset = true, text = 'Too expensive, eh?'})
	
	function StdModule.bless(cid, message, keywords, parameters, node)
		local npcHandler = parameters.npcHandler
		if(npcHandler == nil) then
			print('[Warning - ' .. getCreatureName(getNpcCid()) .. '] StdModule.bless called without any npcHandler instance.')
			return false
		end

		if(not npcHandler:isFocused(cid)) then
			return false
		end

		if(isPlayerPremiumCallback(cid) or not(parameters.premium)) then
			local price = parameters.baseCost
			if(getPlayerLevel(cid) > parameters.startLevel) then
				price = (price + ((math.min(parameters.endLevel, getPlayerLevel(cid)) - parameters.startLevel) * parameters.levelCost))
			end

			if(getPlayerBless(cid, parameters.number))  then
				npcHandler:say("Gods have already blessed you with this blessing!", cid)
			elseif doPlayerRemoveMoney(cid, price) == false then
				npcHandler:say("You don't have enough money for blessing.", cid)
			else
				npcHandler:say("You have been blessed by one of the five gods!", cid)
				doPlayerAddBless(cid, parameters.number)
			end
		else
			npcHandler:say('You need a premium account in order to be blessed.', cid)
		end

		npcHandler:resetNpc()
		return true
	end
	

	function StdModule.travel(cid, message, keywords, parameters, node)
		local npcHandler = parameters.npcHandler
		if(npcHandler == nil) then
			print('[Warning - ' .. getCreatureName(getNpcCid()) .. '] StdModule.travel called without any npcHandler instance.')
			return false
		end
		if(not npcHandler:isFocused(cid)) then
			return false
		end

		if(isPlayerPremiumCallback == nil or isPlayerPremiumCallback(cid) or parameters.premium == false) then
			if(parameters.level ~= nil and getPlayerLevel(cid) < parameters.level) then
				npcHandler:say('You must reach level ' .. parameters.level .. ' before I can let you go there.', cid)
			elseif(not doPlayerRemoveMoney(cid, parameters.cost)) then
				npcHandler:say('You do not have enough money!', cid)
			elseif(isPzLocked(cid) ) then
				npcHandler:say('Get out of there with this blood!', cid)
			else
				doTeleportThing(cid, parameters.destination)
				doSendMagicEffect(parameters.destination, 10)
			end
		else
			npcHandler:say('I can only allow premium players to travel with me.', cid)
		end

		npcHandler:resetNpc()
		return true
	end



	FocusModule = {
			npcHandler = nil
		}

	-- Creates a new instance of FocusModule without an associated NpcHandler.
	function FocusModule:new()
		local obj = {}
		setmetatable(obj, self)
		self.__index = self
		return obj
	end

	-- Inits the module and associates handler to it.
	function FocusModule:init(handler)
		self.npcHandler = handler
		for i, word in pairs(FOCUS_GREETWORDS) do
			local obj = {}
			table.insert(obj, word)
			obj.callback = FOCUS_GREETWORDS.callback or FocusModule.messageMatcher
			handler.keywordHandler:addKeyword(obj, FocusModule.onGreet, {module = self})
		end

		for i, word in pairs(FOCUS_FAREWELLWORDS) do
			local obj = {}
			table.insert(obj, word)
			obj.callback = FOCUS_FAREWELLWORDS.callback or FocusModule.messageMatcher
			handler.keywordHandler:addKeyword(obj, FocusModule.onFarewell, {module = self})
		end

		return true
	end


	-- Greeting callback function.
	function FocusModule.onGreet(cid, message, keywords, parameters)
		parameters.module.npcHandler:onGreet(cid)
		return true
	end

	-- UnGreeting callback function.
	function FocusModule.onFarewell(cid, message, keywords, parameters)
		if(parameters.module.npcHandler:isFocused(cid)) then
			parameters.module.npcHandler:onFarewell(cid)
			return true
		else
			return false
		end
	end

	-- Custom message matching callback function for greeting messages.
	function FocusModule.messageMatcher(keywords, message)
		for i, word in pairs(keywords) do
			if(type(word) == 'string') then
				if string.find(message, word) and not string.find(message, '[%w+]' .. word) and not string.find(message, word .. '[%w+]') then
					return true
				end
			end
		end
		return false
	end



	KeywordModule = {
		npcHandler = nil
	}
	-- Add it to the parseable module list.
	Modules.parseableModules['module_keywords'] = KeywordModule

	function KeywordModule:new()
		local obj = {}
		setmetatable(obj, self)
		self.__index = self
		return obj
	end

	function KeywordModule:init(handler)
		self.npcHandler = handler
		return true
	end

	-- Parses all known parameters.
	function KeywordModule:parseParameters()
		local ret = NpcSystem.getParameter('keywords')
		if(ret ~= nil) then
			self:parseKeywords(ret)
		end
	end

	function KeywordModule:parseKeywords(data)
		local n = 1
		for keys in string.gmatch(data, '[^;]+') do
			local i = 1

			local keywords = {}

			for temp in string.gmatch(keys, '[^,]+') do
				table.insert(keywords, temp)
				i = i + 1
			end

			if(i ~= 1) then
				local reply = NpcSystem.getParameter('keyword_reply' .. n)
				if(reply ~= nil) then
					self:addKeyword(keywords, reply)
				else
					print('[Warning - ' .. getCreatureName(getNpcCid()) .. '] NpcSystem:', 'Parameter \'' .. 'keyword_reply' .. n .. '\' missing. Skipping...')
				end
			else
				print('[Warning - ' .. getCreatureName(getNpcCid()) .. '] NpcSystem:', 'No keywords found for keyword set #' .. n .. '. Skipping...')
			end
			n = n + 1
		end
	end

	function KeywordModule:addKeyword(keywords, reply)
		self.npcHandler.keywordHandler:addKeyword(keywords, StdModule.say, {npcHandler = self.npcHandler, onlyFocus = true, text = reply, reset = true})
	end



	TravelModule = {
		npcHandler = nil,
		destinations = nil,
		yesNode = nil,
		noNode = nil,
	}
	-- Add it to the parseable module list.
	Modules.parseableModules['module_travel'] = TravelModule

	function TravelModule:new()
		local obj = {}
		setmetatable(obj, self)
		self.__index = self
		return obj
	end

	function TravelModule:init(handler)
		self.npcHandler = handler
		self.yesNode = KeywordNode:new(SHOP_YESWORD, TravelModule.onConfirm, {module = self})
		self.noNode = KeywordNode:new(SHOP_NOWORD, TravelModule.onDecline, {module = self})
		self.destinations = {}
		return true
	end

	-- Parses all known parameters.
	function TravelModule:parseParameters()
		local ret = NpcSystem.getParameter('travel_destinations')
		if(ret ~= nil) then
			self:parseDestinations(ret)

			self.npcHandler.keywordHandler:addKeyword({'destination'}, TravelModule.listDestinations, {module = self})
			self.npcHandler.keywordHandler:addKeyword({'where'}, TravelModule.listDestinations, {module = self})
			self.npcHandler.keywordHandler:addKeyword({'travel'}, TravelModule.listDestinations, {module = self})

		end
	end

	function TravelModule:parseDestinations(data)
		for destination in string.gmatch(data, '[^;]+') do
			local i = 1

			local name = nil
			local x = nil
			local y = nil
			local z = nil
			local cost = nil
			local premium = false
			local checkPzBlock = true

			for temp in string.gmatch(destination, '[^,]+') do
				if(i == 1) then
					name = temp
				elseif(i == 2) then
					x = tonumber(temp)
				elseif(i == 3) then
					y = tonumber(temp)
				elseif(i == 4) then
					z = tonumber(temp)
				elseif(i == 5) then
					cost = tonumber(temp)
				elseif(i == 6) then
					premium = temp == 'true'
				elseif(i == 7) then
					checkPzBlock = temp == 'true'
				else
					print('[Warning - ' .. getCreatureName(getNpcCid()) .. '] NpcSystem:', 'Unknown parameter found in travel destination parameter.', temp, destination)
				end
				i = i+1
			end

			if(name ~= nil and x ~= nil and y ~= nil and z ~= nil and cost ~= nil) then
				self:addDestination(name, {x=x, y=y, z=z}, cost, premium, checkPzBlock)
			else
				print('[Warning - ' .. getCreatureName(getNpcCid()) .. '] NpcSystem:', 'Parameter(s) missing for travel destination:', name, x, y, z, cost, premium)
			end
		end
	end

	function TravelModule:addDestination(name, position, price, premium, checkPzBlock)
		table.insert(self.destinations, name)

		local parameters = {
				cost = price,
				destination = position,
				premium = premium,
				checkPzBlock = checkPzBlock,
				module = self
			}
		local keywords = {}
		table.insert(keywords, name)

		local keywords2 = {}
		table.insert(keywords2, 'bring me to ' .. name)
		local node = self.npcHandler.keywordHandler:addKeyword(keywords, TravelModule.travel, parameters)
		self.npcHandler.keywordHandler:addKeyword(keywords2, TravelModule.bringMeTo, parameters)
		node:addChildKeywordNode(self.yesNode)
		node:addChildKeywordNode(self.noNode)
	end

	function TravelModule.travel(cid, message, keywords, parameters, node)
		local module = parameters.module
		if(not module.npcHandler:isFocused(cid)) then
			return false
		end

		local npcHandler = module.npcHandler


		local cost = parameters.cost
		local destination = parameters.destination
		local premium = parameters.premium

		module.npcHandler:say('Do you want to travel to ' .. keywords[1] .. ' for ' .. cost .. ' gold coins?', cid)
		return true

	end

	function TravelModule.onConfirm(cid, message, keywords, parameters, node)
		local module = parameters.module
		if(not module.npcHandler:isFocused(cid)) then
			return false
		end

		local npcHandler = module.npcHandler


		local parentParameters = node:getParent():getParameters()
		local cost = parentParameters.cost
		local destination = parentParameters.destination
		local premium = parentParameters.premium

		if(not isPzLocked(cid) or not parameters.checkPzBlock) then
			if(isPlayerPremiumCallback == nil or isPlayerPremiumCallback(cid) or not parameters.premium) then
				if not doPlayerRemoveMoney(cid, cost) then
					npcHandler:say('You do not have enough money!', cid)
				else
					npcHandler:say('It was a pleasure doing business with you.', cid, true, false)
					npcHandler:releaseFocus(cid)
					doTeleportThing(cid, destination)
					doSendMagicEffect(destination, 10)
				end
			else
				npcHandler:say('I can only allow premium players to travel there.', cid)
			end
		else
			npcHandler:say('I will not take you there with this blood!', cid)
		end

		npcHandler:resetNpc()
		return true
	end

	-- onDecliune keyword callback function. Generally called when the player sais 'no' after wanting to buy an item.
	function TravelModule.onDecline(cid, message, keywords, parameters, node)
		local module = parameters.module
		if(not module.npcHandler:isFocused(cid)) then
			return false
		end
		local parentParameters = node:getParent():getParameters()
		local parseInfo = {
				[TAG_PLAYERNAME] = getPlayerName(cid),
			}
		local msg = module.npcHandler:parseMessage(module.npcHandler:getMessage(MESSAGE_DECLINE), parseInfo)
		module.npcHandler:say(msg, cid)
		module.npcHandler:resetNpc()
		return true
	end

	function TravelModule.bringMeTo(cid, message, keywords, parameters, node)
		local module = parameters.module
		if(not module.npcHandler:isFocused(cid)) then
			return false
		end

		local cost = parameters.cost
		local destination = parameters.destination
		local premium = parameters.premium

		if(isPlayerPremiumCallback == nil or isPlayerPremiumCallback(cid) or not parameters.premium) then
			if(doPlayerRemoveMoney(cid, cost) ) then
				doTeleportThing(cid, destination)
				doSendMagicEffect(destination, 10)
			end
		end

		return true
	end

	function TravelModule.listDestinations(cid, message, keywords, parameters, node)
		local module = parameters.module
		if(not module.npcHandler:isFocused(cid)) then
			return false
		end

		local msg = 'I can bring you to '
		--local i = 1
		local maxn = table.maxn(module.destinations)
		for i,destination in pairs(module.destinations) do
			msg = msg .. destination
			if(i == maxn-1) then
				msg = msg .. ' and '
			elseif(i == maxn) then
				msg = msg .. '.'
			else
				msg = msg .. ', '
			end
			i = i+1
		end

		module.npcHandler:say(msg, cid)
		module.npcHandler:resetNpc()
		return true
	end




	ShopModule = {
		npcHandler = nil
	}
	-- Add it to the parseable module list.
	Modules.parseableModules['module_shop'] = ShopModule

	-- Creates a new instance of ShopModule
	function ShopModule:new()
		local obj = {}
		setmetatable(obj, self)
		self.__index = self
		return obj
	end

	-- Parses all known parameters.
	function ShopModule:parseParameters()

		local ret = NpcSystem.getParameter('shop_sellable')
		if(ret ~= nil) then
			self:parseSellable(ret)
		end

		local ret = NpcSystem.getParameter('shop_buyable')
		if(ret ~= nil) then
			self:parseBuyable(ret)
		end

	end

	-- Parse a string contaning a set of buyable items.
	function ShopModule:parseBuyable(data)
		for item in string.gmatch(data, '[^;]+') do
			local i = 1

			local itemid = nil
			local cost = nil
			local charges = nil

			for temp in string.gmatch(item, '[^,]+') do
				if(i == 1) then
					--name = temp
				elseif(i == 2) then
					itemid = tonumber(temp)
				elseif(i == 3) then
					cost = tonumber(temp)
				elseif(i == 4) then
					charges = tonumber(temp)
				else
					print('[Warning - ' .. getCreatureName(getNpcCid()) .. '] NpcSystem:', 'Unknown parameter found in buyable items parameter.', temp, item)
				end
				i = i+1
			end

			if(itemid ~= nil and cost ~= nil) then
				if((isItemRune(itemid) or isItemFluidContainer(itemid) ) and charges == nil) then
					print('[Warning - ' .. getCreatureName(getNpcCid()) .. '] NpcSystem:', 'Charges missing for parameter item:' , item)
				else
					self:addBuyableItem(nil, itemid, cost, charges)
				end
			else
				print('[Warning - ' .. getCreatureName(getNpcCid()) .. '] NpcSystem:', 'Parameter(s) missing for item:', name, itemid, cost)
			end
		end
	end

	-- Parse a string contaning a set of sellable items.
	function ShopModule:parseSellable(data)
		for item in string.gmatch(data, '[^;]+') do
			local i = 1

			local itemid = nil
			local cost = nil

			for temp in string.gmatch(item, '[^,]+') do
				if(i == 1) then
					--name = temp
				elseif(i == 2) then
					itemid = tonumber(temp)
				elseif(i == 3) then
					cost = tonumber(temp)
				else
					print('[Warning - ' .. getCreatureName(getNpcCid()) .. '] NpcSystem:', 'Unknown parameter found in sellable items parameter.', temp, item)
				end
				i = i+1
			end

			if(itemid ~= nil and cost ~= nil) then
				self:addSellableItem(nil, itemid, cost)
			else
				print('[Warning - ' .. getCreatureName(getNpcCid()) .. '] NpcSystem:', 'Parameter(s) missing for item:', itemid, cost)
			end
		end
	end

	-- Initializes the module and associates handler to it.
	function ShopModule:init(handler)
		self.npcHandler = handler

		for i, word in pairs(SHOP_TRADEREQUEST) do
			local obj = {}
			table.insert(obj, word)
			obj.callback = SHOP_TRADEREQUEST.callback or ShopModule.messageMatcher
			handler.keywordHandler:addKeyword(obj, ShopModule.requestTrade, {module = self})
		end

		return true
	end

	-- Custom message matching callback function for requesting trade messages.
	function ShopModule.messageMatcher(keywords, message)
		for i, word in pairs(keywords) do
			if(type(word) == 'string') then
				if string.find(message, word) and not string.find(message, '[%w+]' .. word) and not string.find(message, word .. '[%w+]') then
					return true
				end
			end
		end
		return false
	end

	-- Resets the module-specific variables.
	function ShopModule:reset()
		--
	end

	-- Function used to match a number value from a string.
	function ShopModule:getCount(message)
		local ret = 1
		local b, e = string.find(message, PATTERN_COUNT)
		if b ~= nil and e ~= nil then
			ret = tonumber(string.sub(message, b, e))
		end
		if(ret <= 0) then
			ret = 1
		elseif(ret > self.maxCount) then
			ret = self.maxCount
		end

		return ret
	end

	-- Adds a new buyable item.
	--	names = Deprecated, used only for compatibility
	--	itemid = the itemid of the buyable item
	--	cost = the price of one single item
	--	subType - The subType of each rune or fluidcontainer item. Can be left out if it is not a rune/fluidcontainer. Default value is 0.
	--	realName - The real, full name for the item. Will be used as ITEMNAME in MESSAGE_ONBUY and MESSAGE_ONSELL if defined. Default value is item's real name.
	function ShopModule:addBuyableItem(names, itemid, cost, subType, realName)
		if(self.npcHandler.shopItems[itemid] == nil) then
			self.npcHandler.shopItems[itemid] = {buyPrice = 0, sellPrice = 0, subType = 0, realName = realName or getItemName(itemid)}
		end
		self.npcHandler.shopItems[itemid].buyPrice = cost
		self.npcHandler.shopItems[itemid].subType = subType or 0
	end

	-- Adds a new sellable item.
	--	names = Deprecated, used only for compatibility
	--	itemid = the itemid of the sellable item
	--	cost = the price of one single item
	--	realName - The real, full name for the item. Will be used as ITEMNAME in MESSAGE_ONBUY and MESSAGE_ONSELL if defined. Default value is nil (keywords[2]/names will be used)
	function ShopModule:addSellableItem(names, itemid, cost, realName)
		if(self.npcHandler.shopItems[itemid] == nil) then
			self.npcHandler.shopItems[itemid] = {buyPrice = 0, sellPrice = 0, subType = 0, realName = realName or getItemName(itemid)}
		end
		self.npcHandler.shopItems[itemid].sellPrice = cost
	end


	-- onModuleReset callback function. Calls ShopModule:reset()
	function ShopModule:callbackOnModuleReset()
		self:reset()
		return true
	end


	-- doPlayerAddItem function variation. Used specifically for NPCs.
	ShopModule.doPlayerAddItem = function(cid, itemid, subType, amount, ignoreCapacity, buyWithBackpacks, backpackId)
		local amount = amount or 1
		local subType = subType or 0
		local ignoreCapacity = ignoreCapacity and true or false

		if(isItemStackable(itemid) ) then
			if(buyWithBackpacks) then
				local backpack = doCreateItemEx(backpackId, 1)
				doAddContainerItem(backpack, itemid, amount)
				if(doPlayerAddItemEx(cid, backpack, ignoreCapacity) ~= RETURNVALUE_NOERROR) then
					return {}, 0
				end
				return backpack, amount
			end

			local item = doCreateItemEx(itemid, amount)
			if(doPlayerAddItemEx(cid, item, ignoreCapacity) ~= RETURNVALUE_NOERROR) then
				return {}, 0
			end
			return item, amount
		end

		if(buyWithBackpacks) then
			local backpackCapacity = getContainerCapById(backpackId)
			local backpackCount = math.ceil(amount / backpackCapacity)
			local itemCount = amount

			local items = {}
			for i = 1, backpackCount do
				items[i] = doCreateItemEx(backpackId, 1)
				local k = itemCount
				if itemCount > backpackCapacity then
					k = backpackCapacity
				end
				for j = 1, k do
					doAddContainerItem(items[i], itemid, subType)
					itemCount = itemCount - 1
				end
				if(doPlayerAddItemEx(cid, items[i], ignoreCapacity) ~= RETURNVALUE_NOERROR) then
					itemCount = itemCount + k
					doRemoveItem(items[i])
					table.remove(items)
					break
				end
			end
			return items, (amount - itemCount)
		end

		local items = {}
		local ret = 0
		local a = 0
		for i = 1, amount do
			items[i] = doCreateItemEx(itemid, subType)
			ret = doPlayerAddItemEx(cid, items[i], ignoreCapacity)
			if(ret ~= RETURNVALUE_NOERROR) then
				break
			end
			a = a + 1
		end

		return items, a
	end

	-- Callback onBuy() function. If you wish, you can change certain Npc to use your onBuy().
	function ShopModule:callbackOnBuy(cid, itemid, subType, amount, ignoreCapacity, buyWithBackpacks)
		if(self.npcHandler.shopItems[itemid] == nil) then
			print("[Warning - ' .. getCreatureName(getNpcCid()) .. '] [ShopModule.onBuy]", "items[itemid] == nil")
			return false
		end

		local parseInfo = {
			[TAG_PLAYERNAME] = getPlayerName(cid),
			[TAG_ITEMCOUNT] = amount,
			[TAG_TOTALCOST] = amount*self.npcHandler.shopItems[itemid].buyPrice,
			[TAG_ITEMNAME] = self.npcHandler.shopItems[itemid].realName
		}

		if amount <= 0 then
			return false
		end

		local cost = amount*self.npcHandler.shopItems[itemid].buyPrice
		local backpackId = 1988

		if(buyWithBackpacks) then
			cost = cost + (math.ceil(amount / getContainerCapById(backpackId)) * 20)
		end

		if(getPlayerMoney(cid) < cost) then
			local msg = self.npcHandler:getMessage(MESSAGE_NEEDMOREMONEY)
			msg = self.npcHandler:parseMessage(msg, parseInfo)
			self.npcHandler:say(msg, cid)
			return false
		end

		local boughtItems, i = ShopModule.doPlayerAddItem(cid, itemid, subType, amount, ignoreCapacity, buyWithBackpacks, backpackId)
		if(i < amount) then
			local msgId = MESSAGE_ONBUYNEEDSPACE
			if(i == 0) then
				msgId = MESSAGE_NEEDMORESPACE
			end

			local msg = self.npcHandler:getMessage(msgId)
			parseInfo[TAG_ITEMCOUNT] = i
			msg = self.npcHandler:parseMessage(msg, parseInfo)
			self.npcHandler:say(msg, cid)
			if(NPCHANDLER_CONVBEHAVIOR ~= CONVERSATION_DEFAULT) then
				self.npcHandler.talkStart[cid] = os.time()
			else
				self.npcHandler.talkStart = os.time()
			end
			if(i > 0) then
				cost = i * self.npcHandler.shopItems[itemid].buyPrice
				if(buyWithBackpacks) then cost = cost + table.maxn(boughtItems) * 20 end
				doPlayerRemoveMoney(cid, cost)
				doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "Bought " .. i .. "x " .. self.npcHandler.shopItems[itemid].realName .. " for " .. cost .. " gold.")
				return true
			end
			return false
		else
			local msg = self.npcHandler:getMessage(MESSAGE_ONBUY)
			msg = self.npcHandler:parseMessage(msg, parseInfo)
			self.npcHandler:say(msg, cid)
			doPlayerRemoveMoney(cid, cost)
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "Bought " .. amount .. "x " .. self.npcHandler.shopItems[itemid].realName .. " for " .. cost .. " gold.")
			if(NPCHANDLER_CONVBEHAVIOR ~= CONVERSATION_DEFAULT) then
				self.npcHandler.talkStart[cid] = os.time()
			else
				self.npcHandler.talkStart = os.time()
			end
			return true
		end
	end

	-- Callback onSell() function. If you wish, you can change certain Npc to use your onSell().
	function ShopModule:callbackOnSell(cid, itemid, subType, amount, ignoreEquipped, dummy)
		if(self.npcHandler.shopItems[itemid] == nil) then
			print("[Warning - ' .. getCreatureName(getNpcCid()) .. '] [ShopModule.onSell]", "items[itemid] == nil")
			return false
		end

		if(not isItemStackable(itemid) and amount > MAX_NONESTACKABLE_SELL_AMOUNT) then
			amount = MAX_NONESTACKABLE_SELL_AMOUNT
		end

		local parseInfo = {
			[TAG_PLAYERNAME] = getPlayerName(cid),
			[TAG_ITEMCOUNT] = amount,
			[TAG_TOTALCOST] = amount*self.npcHandler.shopItems[itemid].buyPrice,
			[TAG_ITEMNAME] = self.npcHandler.shopItems[itemid].realName
		}

		if(subType < 1) then
			subType = -1
		end
		if(doPlayerRemoveItem(cid, itemid, amount, subType, ignoreEquipped)) then
			local msg = self.npcHandler:getMessage(MESSAGE_ONSELL)
			msg = self.npcHandler:parseMessage(msg, parseInfo)
			self.npcHandler:say(msg, cid)
			doPlayerAddMoney(cid, amount*self.npcHandler.shopItems[itemid].sellPrice)
			doPlayerSendTextMessage(cid, MESSAGE_INFO_DESCR, "Sold " .. amount .. "x " .. self.npcHandler.shopItems[itemid].realName .. " for " .. amount*self.npcHandler.shopItems[itemid].sellPrice .. " gold.")
			if(NPCHANDLER_CONVBEHAVIOR ~= CONVERSATION_DEFAULT) then
				self.npcHandler.talkStart[cid] = os.time()
			else
				self.npcHandler.talkStart = os.time()
			end
			return true
		else
			local msg = self.npcHandler:getMessage(MESSAGE_NOTHAVEITEM)
			msg = self.npcHandler:parseMessage(msg, parseInfo)
			self.npcHandler:say(msg, cid)
			if(NPCHANDLER_CONVBEHAVIOR ~= CONVERSATION_DEFAULT) then
				self.npcHandler.talkStart[cid] = os.time()
			else
				self.npcHandler.talkStart = os.time()
			end
			return false
		end
	end

	-- Callback for requesting a trade window with the NPC.
	function ShopModule.requestTrade(cid, message, keywords, parameters, node)
		local module = parameters.module
		if(not module.npcHandler:isFocused(cid)) then
			return false
		end

		local itemWindow = {}
		for itemid, attr in pairs(module.npcHandler.shopItems) do
			local item = {id = itemid, buy = attr.buyPrice, sell = attr.sellPrice, subtype = attr.subType}
			table.insert(itemWindow, item)
		end

		if(itemWindow[1] == nil) then
			local parseInfo = { [TAG_PLAYERNAME] = getPlayerName(cid) }
			local msg = module.npcHandler:parseMessage(module.npcHandler:getMessage(MESSAGE_NOSHOP), parseInfo)
			module.npcHandler:say(msg, cid)
			return true
		end

		local parseInfo = { [TAG_PLAYERNAME] = getPlayerName(cid) }
		local msg = module.npcHandler:parseMessage(module.npcHandler:getMessage(MESSAGE_SENDTRADE), parseInfo)
		openShopWindow(cid, itemWindow,
						function(cid, itemid, subType, amount, ignoreCapacity, buyWithBackpacks) module.npcHandler:onBuy(cid, itemid, subType, amount, ignoreCapacity, buyWithBackpacks) end,
						function(cid, itemid, subType, amount) module.npcHandler:onSell(cid, itemid, subType, amount) end)
		module.npcHandler:say(msg, cid)
		return true
	end

end
