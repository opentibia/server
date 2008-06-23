-- This file is part of Jiddo's advanced NpcSystem v3.0x. This npcsystem is free to use by anyone, for any purpuse. 
-- Initial release date: 2007-02-21
-- Credits: Jiddo, honux(I'm using a modified version of his Find function).
-- Please include full credits whereever you use this system, or parts of it.
-- For support, questions and updates, please consult the following thread:
-- http://otfans.net/showthread.php?t=67810

if(Modules == nil) then
	
	-- default words for greeting and ungreeting the npc. Should be a talbe containing all such words.
	FOCUS_GREETWORDS = {'hi', 'hello'}
	FOCUS_FAREWELLWORDS = {'bye', 'farewell', 'cya'}
	
	-- The word for accepting/declining an offer. CAN ONLY CONTAIN ONE FIELD! Should be a teble with a single string value.
	SHOP_YESWORD = {'yes'}
	SHOP_NOWORD = {'no'}
	
	-- Pattern used to get the amount of an item a player wants to buy/sell.
	PATTERN_COUNT = '%d+'
	
	
	-- Constants used to separate buying from selling.
	SHOPMODULE_SELL_ITEM 	= 1
	SHOPMODULE_BUY_ITEM 	= 2
	
	
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
			error('StdModule.say called without any npcHandler instance.')
		end
		if(cid ~= npcHandler.focus and (parameters.onlyFocus == nil or parameters.onlyFocus == true)) then
			return false
		end
		local parseInfo = {
				[TAG_PLAYERNAME] = getPlayerName(cid),
			}
		msgout = npcHandler:parseMessage(parameters.text or parameters.message, parseInfo)
		npcHandler:say(msgout)
		if(parameters.reset == true) then
			npcHandler:resetNpc()
		elseif(parameters.moveup ~= nil and type(parameters.moveup) == 'number') then
			npcHandler.keywordHandler:moveUp(parameters.moveup)
		end
		return true
	end
	
	
	--Usage:
		-- local node1 = keywordHandler:addKeyword({'promot'}, StdModule.say, {npcHandler = npcHandler, text = 'I can promote you for 20000 gold coins. Do you want me to promote you?'})
		-- 		node1:addChildKeyword({'yes'}, StdModule.promotePlayer, {npcHandler = npcHandler, promotions = {[1] = 5, [2] = 6, [3] = 7, [4] = 8}, cost = 20000, level = 20}, text = 'Congratulations! You are now promoted.')
		-- 		node1:addChildKeyword({'no'}, StdModule.say, {npcHandler = npcHandler, text = 'Allright then. Come back when you are ready.'}, reset = true)
	function StdModule.promotePlayer(cid, message, keywords, parameters, node)
		local npcHandler = parameters.npcHandler
		if(npcHandler == nil) then
			error('StdModule.promotePlayer called without any npcHandler instance.')
		end
		if(cid ~= npcHandler.focus) then
			return false
		end

		if(isPlayerPremiumCallback == nil or isPlayerPremiumCallback(cid) == true or parameters.premium == false) then
			local oldVoc = getPlayerVocation(cid)
			if(parameters.promotions[oldVoc] == oldVoc or parameters.promotions[oldVoc] == nil) then
				npcHandler:say('You are already promoted!')
			elseif(getPlayerLevel(cid) < parameters.level) then
				npcHandler:say('I am sorry, but I can only promote you once you have reached level ' .. parameters.level .. '.')
			elseif(doPlayerRemoveMoney(cid, parameters.cost) ~= TRUE) then
				npcHandler:say('You do not have enough money!')
			else
				doPlayerSetVocation(cid, parameters.promotions[oldVoc])
				npcHandler:say(parameters.text)
			end
		else
			npcHandler:say('Sorry, only premium characters are allowed to be promoted.')
		end
		
		npcHandler:resetNpc()
		return true
		
	end
	
	
	
	function StdModule.travel(cid, message, keywords, parameters, node)
		local npcHandler = parameters.npcHandler
		if(npcHandler == nil) then
			error('StdModule.travel called without any npcHandler instance.')
		end
		if(cid ~= npcHandler.focus) then
			return false
		end
		
		if(isPlayerPremiumCallback == nil or isPlayerPremiumCallback(cid) == true or parameters.premium == false) then
			if(parameters.level ~= nil and getPlayerLevel(cid) < parameters.level) then
				npcHandler:say('You must reach level ' .. parameters.level .. ' before I can let you go there.')
			elseif(doPlayerRemoveMoney(cid, parameters.cost) ~= TRUE) then
				npcHandler:say('You do not have enough money!')
			else
				doTeleportThing(cid, parameters.destination)
				doSendMagicEffect(parameters.destination, 10)
			end
		else
			npcHandler:say('I can only allow premium players to travel with me.')
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
		if(parameters.module.npcHandler.focus == cid) then
			parameters.module.npcHandler:onFarewell()
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
				i = i+1
			end
			
			if(i ~= 1) then
				local reply = NpcSystem.getParameter('keyword_reply' .. n)
				if(reply ~= nil) then
					self:addKeyword(keywords, reply)
				else
					print('[Warning] NpcSystem:', 'Parameter \'' .. 'keyword_reply' .. n .. '\' missing. Skipping...')
				end
			else
				print('[Warning] NpcSystem:', 'No keywords found for keyword set #' .. n .. '. Skipping...')
			end
			n = n+1
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
				else
					print('[Warning] NpcSystem:', 'Unknown parameter found in travel destination parameter.', temp, destination)
				end
				i = i+1
			end
			
			if(name ~= nil and x ~= nil and y ~= nil and z ~= nil and cost ~= nil) then
				self:addDestination(name, {x=x, y=y, z=z}, cost, premium)
			else
				print('[Warning] NpcSystem:', 'Parameter(s) missing for travel destination:', name, x, y, z, cost, premium)
			end
		end
	end
	
	function TravelModule:addDestination(name, position, price, premium)
		table.insert(self.destinations, name)
		
		local parameters = {
				cost = price,
				destination = position,
				premium = premium,
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
		if(cid ~= module.npcHandler.focus) then
			return false
		end
		
		local npcHandler = module.npcHandler
		
		
		local cost = parameters.cost
		local destination = parameters.destination
		local premium = parameters.premium
		
		module.npcHandler:say('Do you want to travel to ' .. keywords[1] .. ' for ' .. cost .. ' gold coins?')
		return true
		
	end
	
	function TravelModule.onConfirm(cid, message, keywords, parameters, node)
		local module = parameters.module
		if(cid ~= module.npcHandler.focus) then
			return false
		end
		
		local npcHandler = module.npcHandler
		
		
		local parentParameters = node:getParent():getParameters()
		local cost = parentParameters.cost
		local destination = parentParameters.destination
		local premium = parentParameters.premium
		
		if(isPlayerPremiumCallback == nil or isPlayerPremiumCallback(cid) == true or parameters.premium ~= true) then
			if(doPlayerRemoveMoney(cid, cost) ~= TRUE) then
				npcHandler:say('You do not have enough money!')
			else
				npcHandler:say('It was a pleasure doing business with you.', false)
				npcHandler:releaseFocus()
				doTeleportThing(cid, destination)
				doSendMagicEffect(destination, 10)
			end
		else
			npcHandler:say('I can only allow premium players to travel there.')
		end
		
		npcHandler:resetNpc()
		return true
	end
	
	-- onDecliune keyword callback function. Generally called when the player sais 'no' after wanting to buy an item. 
	function TravelModule.onDecline(cid, message, keywords, parameters, node)
		local module = parameters.module
		if(cid ~= module.npcHandler.focus) then
			return false
		end
		local parentParameters = node:getParent():getParameters()
		local parseInfo = {
				[TAG_PLAYERNAME] = getPlayerName(cid),
			}
		local msg = module.npcHandler:parseMessage(module.npcHandler:getMessage(MESSAGE_DECLINE), parseInfo)
		module.npcHandler:say(msg)
		module.npcHandler:resetNpc()
		return true
	end
	
	function TravelModule.bringMeTo(cid, message, keywords, parameters, node)
		local module = parameters.module
		if(cid == module.npcHandler.focus) then
			return false
		end
		
		local cost = parameters.cost
		local destination = parameters.destination
		local premium = parameters.premium
		
		if(isPlayerPremiumCallback == nil or isPlayerPremiumCallback(cid) == true or parameters.premium ~= true) then
			if(doPlayerRemoveMoney(cid, cost) == TRUE) then
				doTeleportThing(cid, destination)
				doSendMagicEffect(destination, 10)
			end
		end
		
		return true
	end
	
	function TravelModule.listDestinations(cid, message, keywords, parameters, node)
		local module = parameters.module
		if(cid ~= module.npcHandler.focus) then
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
		
		module.npcHandler:say(msg)
		module.npcHandler:resetNpc()
		return true
	end
	
	
	
	
	ShopModule = {
		yesNode = nil,
		noNode = nil,
		npcHandler = nil,
		noText = '',
		maxCount = 500,
		amount = 0
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
			
			local name = nil
			local itemid = nil
			local cost = nil
			local charges = nil
			
			for temp in string.gmatch(item, '[^,]+') do
				if(i == 1) then
					name = temp
				elseif(i == 2) then
					itemid = tonumber(temp)
				elseif(i == 3) then
					cost = tonumber(temp)
				elseif(i == 4) then
					charges = tonumber(temp)
				else
					print('[Warning] NpcSystem:', 'Unknown parameter found in buyable items parameter.', temp, item)
				end
				i = i+1
			end
			
			if(name ~= nil and itemid ~= nil and cost ~= nil) then
				if((isItemRune(itemid) == TRUE or isItemFluidContainer(itemid) == TRUE) and charges == nil) then
					print('[Warning] NpcSystem:', 'Charges missing for parameter item:' , item)
				else
					local names = {}
					table.insert(names, name)
					self:addBuyableItem(names, itemid, cost, charges)
				end
			else
				print('[Warning] NpcSystem:', 'Parameter(s) missing for item:', name, itemid, cost)
			end
		end
	end
	
	-- Parse a string contaning a set of sellable items.
	function ShopModule:parseSellable(data)
		for item in string.gmatch(data, '[^;]+') do
			local i = 1
			
			local name = nil
			local itemid = nil
			local cost = nil
			
			for temp in string.gmatch(item, '[^,]+') do
				if(i == 1) then
					name = temp
				elseif(i == 2) then
					itemid = tonumber(temp)
				elseif(i == 3) then
					cost = tonumber(temp)
				else
					print('[Warning] NpcSystem:', 'Unknown parameter found in sellable items parameter.', temp, item)
				end
				i = i+1
			end
			
			if(name ~= nil and itemid ~= nil and cost ~= nil) then
				local names = {}
				table.insert(names, name)
				self:addSellableItem(names, itemid, cost)
			else
				print('[Warning] NpcSystem:', 'Parameter(s) missing for item:', name, itemid, cost)
			end
		end
	end
	
	-- Initializes the module and associates handler to it.
	function ShopModule:init(handler)
		self.npcHandler = handler
		self.yesNode = KeywordNode:new(SHOP_YESWORD, ShopModule.onConfirm, {module = self})
		self.noNode = KeywordNode:new(SHOP_NOWORD, ShopModule.onDecline, {module = self})
		self.noText = handler:getMessage(MESSAGE_DECLINE)
		
		return true
	end
	
	-- Resets the module-specific variables.
	function ShopModule:reset()
		self.amount = 0
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
	--	names = A table containing one or more strings of alternative names to this item.
	--	itemid = the itemid of the buyable item
	--	cost = the price of one single item with item id itemid ^^
	--	charges - The charges of each rune or fluidcontainer item. Can be left out if it is not a rune/fluidcontainer and no realname is needed. Default value is nil.
	--	realname - The real, full name for the item. Will be used as ITEMNAME in MESSAGE_ONBUY and MESSAGE_ONSELL if defined. Default value is nil (keywords[1]/names  will be used)
	function ShopModule:addBuyableItem(names, itemid, cost, charges, realname)
		for i, name in pairs(names) do
			local parameters = {
					itemid = itemid,
					cost = cost,
					eventType = SHOPMODULE_BUY_ITEM,
					module = self
				}
			if(realname ~= nil) then
				parameters.realname = realname
			end
			if(isItemRune(itemid) == TRUE or isItemFluidContainer(itemid) == TRUE) then
				parameters.charges = charges
			end
			keywords = {}
			table.insert(keywords, name)
			local node = self.npcHandler.keywordHandler:addKeyword(keywords, ShopModule.tradeItem, parameters)
			node:addChildKeywordNode(self.yesNode)
			node:addChildKeywordNode(self.noNode)
		end
	end
	
	-- Adds a new sellable item. 
	--	names = A table containing one or more strings of alternative names to this item.
	--	itemid = the itemid of the buyable item
	--	cost = the price of one single item with item id itemid ^^
	--	realname - The real, full name for the item. Will be used as ITEMNAME in MESSAGE_ONBUY and MESSAGE_ONSELL if defined. Default value is nil (keywords[2]/names will be used)
	function ShopModule:addSellableItem(names, itemid, cost, realname)
		for i, name in pairs(names) do
			local parameters = {
					itemid = itemid,
					cost = cost,
					eventType = SHOPMODULE_SELL_ITEM,
					module = self
				}
			if(realname ~= nil) then
				parameters.realname = realname
			end
			keywords = {}
			table.insert(keywords, 'sell')
			table.insert(keywords, name)
			local node = self.npcHandler.keywordHandler:addKeyword(keywords, ShopModule.tradeItem, parameters)
			node:addChildKeywordNode(self.yesNode)
			node:addChildKeywordNode(self.noNode)
		end
	end
	
	
	-- onModuleReset callback function. Calls ShopModule:reset()
	function ShopModule:callbackOnModuleReset()
		self:reset()
		
		return true
	end
	
	
	-- tradeItem callback function. Makes the npc say the message defined by MESSAGE_BUY or MESSAGE_SELL
	function ShopModule.tradeItem(cid, message, keywords, parameters, node)
		local module = parameters.module
		if(cid ~= module.npcHandler.focus) then
			return false
		end
		local count = module:getCount(message)
		module.amount = count
		local tmpName = nil
		if(parameters.eventType == SHOPMODULE_SELL_ITEM) then
			tmpName = node:getKeywords()[2]
		elseif(parameters.eventType == SHOPMODULE_BUY_ITEM) then
			tmpName = node:getKeywords()[1]
		end
		local parseInfo = {
				[TAG_PLAYERNAME] = getPlayerName(cid),
				[TAG_ITEMCOUNT] = module.amount,
				[TAG_TOTALCOST] = parameters.cost*module.amount,
				[TAG_ITEMNAME] = parameters.realname or tmpName
			}
		
		if(parameters.eventType == SHOPMODULE_SELL_ITEM) then
			local msg = module.npcHandler:getMessage(MESSAGE_SELL)
			msg = module.npcHandler:parseMessage(msg, parseInfo)
			module.npcHandler:say(msg)
		elseif(parameters.eventType == SHOPMODULE_BUY_ITEM) then
			local msg = module.npcHandler:getMessage(MESSAGE_BUY)
			msg = module.npcHandler:parseMessage(msg, parseInfo)
			module.npcHandler:say(msg)
		end
		
		return true
		
	end
	
	
	-- onConfirm keyword callback function. Sells/buys the actual item.
	function ShopModule.onConfirm(cid, message, keywords, parameters, node)
		local module = parameters.module
		if(cid ~= module.npcHandler.focus) then
			return false
		end
		local parentParameters = node:getParent():getParameters()
		local parseInfo = {
				[TAG_PLAYERNAME] = getPlayerName(cid),
				[TAG_ITEMCOUNT] = module.amount,
				[TAG_TOTALCOST] = parentParameters.cost*module.amount,
				[TAG_ITEMNAME] = parentParameters.realname or node:getParent():getKeywords()[1]
			}
		
		if(parentParameters.eventType == SHOPMODULE_SELL_ITEM) then
			local ret = doPlayerSellItem(cid, parentParameters.itemid, module.amount, parentParameters.cost*module.amount)
			if(ret == LUA_NO_ERROR) then
				local msg = module.npcHandler:getMessage(MESSAGE_ONSELL)
				msg = module.npcHandler:parseMessage(msg, parseInfo)
				module.npcHandler:say(msg)
			else
				local msg = module.npcHandler:getMessage(MESSAGE_NOTHAVEITEM)
				msg = module.npcHandler:parseMessage(msg, parseInfo)
				module.npcHandler:say(msg)
			end
		elseif(parentParameters.eventType == SHOPMODULE_BUY_ITEM) then
			local ret = doPlayerBuyItem(cid, parentParameters.itemid, module.amount, parentParameters.cost*module.amount, parentParameters.charges)
			if(ret == LUA_NO_ERROR) then
				local msg = module.npcHandler:getMessage(MESSAGE_ONBUY)
				msg = module.npcHandler:parseMessage(msg, parseInfo)
				module.npcHandler:say(msg)
			else
				local msg = module.npcHandler:getMessage(MESSAGE_NEEDMOREMONEY)
				msg = module.npcHandler:parseMessage(msg, parseInfo)
				module.npcHandler:say(msg)
			end
		end
		
		module.npcHandler:resetNpc()
		return true
	end
	
	-- onDecliune keyword callback function. Generally called when the player sais 'no' after wanting to buy an item. 
	function ShopModule.onDecline(cid, message, keywords, parameters, node)
		local module = parameters.module
		if(cid ~= module.npcHandler.focus) then
			return false
		end
		local parentParameters = node:getParent():getParameters()
		local parseInfo = {
				[TAG_PLAYERNAME] = getPlayerName(cid),
				[TAG_ITEMCOUNT] = module.amount,
				[TAG_TOTALCOST] = parentParameters.cost*module.amount,
				[TAG_ITEMNAME] = parentParameters.realname or node:getParent():getKeywords()[1]
			}
		local msg = module.npcHandler:parseMessage(module.noText, parseInfo)
		module.npcHandler:say(msg)
		module.npcHandler:resetNpc()
		return true
	end
end