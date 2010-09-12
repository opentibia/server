-- This file is part of Jiddo's advanced NpcSystem v3.0x. This npcsystem is free to use by anyone, for any purpuse.
-- Initial release date: 2007-02-21
-- Credits: Jiddo, honux(I'm using a modified version of his Find function).
-- Please include full credits whereever you use this system, or parts of it.
-- For support, questions and updates, please consult the following thread:
-- http://otfans.net/showthread.php?t=67810

if(NpcSystem == nil) then

	-- Loads the underlying classes of the npcsystem.
	dofile(getDataDir() .. 'npc/scripts/lib/npcsystem/keywordhandler.lua')
	dofile(getDataDir() .. 'npc/scripts/lib/npcsystem/queue.lua')
	dofile(getDataDir() .. 'npc/scripts/lib/npcsystem/npchandler.lua')
	dofile(getDataDir() .. 'npc/scripts/lib/npcsystem/modules.lua')


	-- Global npc constants:

	-- Keyword nestling behavior. For more information look at the top of keywordhandler.lua
	KEYWORD_BEHAVIOR = BEHAVIOR_NORMAL_EXTENDED

	-- Gerrting and unGreeting keywords. For more information look at the top of modules.lua
	FOCUS_GREETWORDS = {'hi', 'hello'}
	FOCUS_FAREWELLWORDS = {'bye', 'farewell', 'cya'}

	-- The word for requesting trade window. For more information look at the top of modules.lua
	SHOP_TRADEREQUEST = {'offer', 'trade', 'wares'}

	-- The word for accepting/declining an offer. CAN ONLY CONTAIN ONE FIELD! For more information look at the top of modules.lua
	SHOP_YESWORD = {'yes'}
	SHOP_NOWORD = {'no'}

	-- Pattern used to get the amount of an item a player wants to buy/sell.
	PATTERN_COUNT = '%d+'

	-- Talkdelay behavior. For more information, look at the top of npchandler.lua.
	NPCHANDLER_TALKDELAY = TALKDELAY_ONTHINK

	-- Conversation behavior. For more information, look at the top of npchandler.lua.
	NPCHANDLER_CONVBEHAVIOR = CONVERSATION_PRIVATE

	-- Constant strings defining the keywords to replace in the default messages.
	--	For more information, look at the top of npchandler.lua...
	TAG_PLAYERNAME = '|PLAYERNAME|'
	TAG_ITEMCOUNT = '|ITEMCOUNT|'
	TAG_TOTALCOST = '|TOTALCOST|'
	TAG_ITEMNAME = '|ITEMNAME|'
	TAG_QUEUESIZE = '|QUEUESIZE|'





	NpcSystem = {
			--
		}

	-- Gets an npcparameter with the specified key. Returns nil if no such parameter is found.
	function NpcSystem.getParameter(key)
		local ret = getNpcParameter(tostring(key))
		if((type(ret) == 'number' and ret == 0) or ret == nil) then
			return nil
		else
			return ret
		end
	end

	-- Parses all known parameters for the npc. Also parses parseable modules.
	function NpcSystem.parseParameters(npcHandler)

		local ret = NpcSystem.getParameter('idletime')
		if(ret ~= nil) then
			npcHandler.idleTime = tonumber(ret)
		end

		local ret = NpcSystem.getParameter('talkradius')
		if(ret ~= nil) then
			npcHandler.talkRadius = tonumber(ret)
		end

		local ret = NpcSystem.getParameter('talkdelaytime')
		if(ret ~= nil) then
			npcHandler.talkDelayTime = tonumber(ret)
		end

		local ret = NpcSystem.getParameter('message_greet')
		if(ret ~= nil) then
			npcHandler:setMessage(MESSAGE_GREET, ret)
		end
		local ret = NpcSystem.getParameter('message_farewell')
		if(ret ~= nil) then
			npcHandler:setMessage(MESSAGE_FAREWELL, ret)
		end
		local ret = NpcSystem.getParameter('message_onbuy')
		if(ret ~= nil) then
			npcHandler:setMessage(MESSAGE_ONBUY, ret)
		end
		local ret = NpcSystem.getParameter('message_onsell')
		if(ret ~= nil) then
			npcHandler:setMessage(MESSAGE_ONSELL, ret)
		end
		local ret = NpcSystem.getParameter('message_needmoremoney')
		if(ret ~= nil) then
			npcHandler:setMessage(MESSAGE_NEEDMOREMONEY, ret)
		end
		local ret = NpcSystem.getParameter('message_nothaveitem')
		if(ret ~= nil) then
			npcHandler:setMessage(MESSAGE_NOTHAVEITEM, ret)
		end
		local ret = NpcSystem.getParameter('message_idletimeout')
		if(ret ~= nil) then
			npcHandler:setMessage(MESSAGE_IDLETIMEOUT, ret)
		end
		local ret = NpcSystem.getParameter('message_walkaway')
		if(ret ~= nil) then
			npcHandler:setMessage(MESSAGE_WALKAWAY, ret)
		end
		local ret = NpcSystem.getParameter('message_decline')
		if(ret ~= nil) then
			npcHandler:setMessage(MESSAGE_DECLINE, ret)
		end
		local ret = NpcSystem.getParameter('message_needmorespace')
		if(ret ~= nil) then
			npcHandler:setMessage(MESSAGE_NEEDMORESPACE, ret)
		end
		local ret = NpcSystem.getParameter('message_onbuy_needspace')
		if(ret ~= nil) then
			npcHandler:setMessage(MESSAGE_ONBUYNEEDSPACE, ret)
		end
		local ret = NpcSystem.getParameter('message_sendtrade')
		if(ret ~= nil) then
			npcHandler:setMessage(MESSAGE_SENDTRADE, ret)
		end
		local ret = NpcSystem.getParameter('message_noshop')
		if(ret ~= nil) then
			npcHandler:setMessage(MESSAGE_NOSHOP, ret)
		end
		local ret = NpcSystem.getParameter('message_oncloseshop')
		if(ret ~= nil) then
			npcHandler:setMessage(MESSAGE_ONCLOSESHOP, ret)
		end
		local ret = NpcSystem.getParameter('message_alreadyfocused')
		if(ret ~= nil) then
			npcHandler:setMessage(MESSAGE_ALREADYFOCUSED, ret)
		end
		local ret = NpcSystem.getParameter('message_placedinqueue')
		if(ret ~= nil) then
			npcHandler:setMessage(MESSAGE_PLACEDINQUEUE, ret)
		end
		local ret = NpcSystem.getParameter('message_buy')
		if(ret ~= nil) then
			npcHandler:setMessage(MESSAGE_BUY, ret)
		end
		local ret = NpcSystem.getParameter('message_sell')
		if(ret ~= nil) then
			npcHandler:setMessage(MESSAGE_SELL, ret)
		end

		-- Parse modules.
		for parameter, module in pairs(Modules.parseableModules) do
			local ret = NpcSystem.getParameter(parameter)
			if(ret ~= nil) then
				local number = tonumber(ret)
				if(number ~= 0 and module.parseParameters ~= nil) then
					local instance = module:new()
					npcHandler:addModule(instance)
					instance:parseParameters()
				end
			end
		end

	end
end
