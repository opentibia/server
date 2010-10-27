-- This is an example NPC script that can be used on Jiddo's NPC system
-- Coordinates are not real, same for the name of cities or other server-based things here

local LEVEL = 8

local keywordHandler = KeywordHandler:new()
local npcHandler = NpcHandler:new(keywordHandler)
NpcSystem.parseParameters(npcHandler)



-- OTServ event handling functions start
function onCreatureAppear(cid)              npcHandler:onCreatureAppear(cid) end
function onCreatureDisappear(cid)           npcHandler:onCreatureDisappear(cid) end
function onCreatureSay(cid, type, msg)      npcHandler:onCreatureSay(cid, type, msg) end
function onThink()                          npcHandler:onThink() end
-- OTServ event handling functions end


function oracle(cid, message, keywords, parameters, node)
	if(cid ~= npcHandler.focus) then
		return false
	end
	
	local cityNode = node:getParent():getParent()
	local vocNode = node:getParent()
	
	local destination = cityNode:getParameters().destination
	local townid = cityNode:getParameters().townid
	local voc = vocNode:getParameters().voc
	
	if(destination ~= nil and voc ~= nil and townid ~= nil) then
		if(getPlayerLevel(cid) < parameters.level) then
			npcHandler:say('You must first reach level ' .. parameters.level .. '!')
		else
			doPlayerSetVocation(cid,voc)
			doPlayerSetTown(cid,townid)
			doTeleportThing(cid,destination)
		end
	else
		error('Destination:', destination, 'Vocation:', vocation, 'Townid:', townid)
	end
	npcHandler:resetNpc()
	return true
end


function greetCallback(cid)
	if(getPlayerLevel(cid) < LEVEL) then
		npcHandler:say('CHILD! COME BACK WHEN YOU HAVE GROWN UP!')
		return false
	else
		return true
	end
end

-- Set the greeting callback function
npcHandler:setCallback(CALLBACK_GREET, greetCallback)

-- Set the greeting message.
npcHandler:setMessage(MESSAGE_GREET, 'Hello |PLAYERNAME|. Are you prepared to face your destiny?')

-- Pre-create the yes/no nodes.
local yesNode = KeywordNode:new({'yes'}, oracle, {level = LEVEL})
local noNode = KeywordNode:new({'no'}, StdModule.say, {npcHandler = npcHandler, onlyFocus = true, moveup = 1, text = 'Then what vocation do you want to become?'})

-- Create the actual keyword structure...
local node1 = keywordHandler:addKeyword({'yes'}, StdModule.say, {npcHandler = npcHandler, onlyFocus = true, text = 'What city do you wish to live in? Derelin or Drunia?'})
	local node2 = node1:addChildKeyword({'derelin'}, StdModule.say, {npcHandler = npcHandler, onlyFocus = true, townid = 3, destination = {x=1330, y=368, z=7}, text = 'The desert city of Derelin, eh? So what vocation do you wish to become? Sorcerer, druid, paladin or knight?'})
		local node3 = node2:addChildKeyword({'sorcerer'}, StdModule.say, {npcHandler = npcHandler, voc = 1, onlyFocus = true, text = 'So, you wish to be a powerful magician? Are you sure about that? This decision is irreversible!'})
			node3:addChildKeywordNode(yesNode)
			node3:addChildKeywordNode(noNode)
		local node3 = node2:addChildKeyword({'druid'}, StdModule.say, {npcHandler = npcHandler, voc = 2, onlyFocus = true, text = 'Are you sure that a druid is what you wish to become? This decision is irreversible!'})
			node3:addChildKeywordNode(yesNode)
			node3:addChildKeywordNode(noNode)
		local node3 = node2:addChildKeyword({'paladin'}, StdModule.say, {npcHandler = npcHandler, voc = 3, onlyFocus = true, text = 'A ranged marksman. Are you sure? This decision is irreversible!'})
			node3:addChildKeywordNode(yesNode)
			node3:addChildKeywordNode(noNode)
		local node3 = node2:addChildKeyword({'knight'}, StdModule.say, {npcHandler = npcHandler, voc = 4, onlyFocus = true, text = 'A mighty warrior. Is that your final decision? This decision is irreversible!'})
			node3:addChildKeywordNode(yesNode)
			node3:addChildKeywordNode(noNode)

	local node2 = node1:addChildKeyword({'drunia'}, StdModule.say, {npcHandler = npcHandler, onlyFocus = true, townid = 1, destination = {x=939, y=263, z=7}, text = 'The town of Drunia, eh? So what vocation do you wish to become? Sorcerer, druid, paladin or knight?'})
		local node3 = node2:addChildKeyword({'sorcerer'}, StdModule.say, {npcHandler = npcHandler, voc = 1, onlyFocus = true, text = 'So, you wish to be a powerful magician? Are you sure about that? This decision is irreversible!'})
			node3:addChildKeywordNode(yesNode)
			node3:addChildKeywordNode(noNode)
		local node3 = node2:addChildKeyword({'druid'}, StdModule.say, {npcHandler = npcHandler, voc = 2, onlyFocus = true, text = 'Are you sure that a druid is what you wish to become? This decision is irreversible!'})
			node3:addChildKeywordNode(yesNode)
			node3:addChildKeywordNode(noNode)
		local node3 = node2:addChildKeyword({'paladin'}, StdModule.say, {npcHandler = npcHandler, voc = 3, onlyFocus = true, text = 'A ranged marksman. Are you sure? This decision is irreversible!'})
			node3:addChildKeywordNode(yesNode)
			node3:addChildKeywordNode(noNode)
		local node3 = node2:addChildKeyword({'knight'}, StdModule.say, {npcHandler = npcHandler, voc = 4, onlyFocus = true, text = 'A mighty warrior. Is that your final decision? This decision is irreversible!'})
			node3:addChildKeywordNode(yesNode)
			node3:addChildKeywordNode(noNode)

keywordHandler:addKeyword({'no'}, StdModule.say, {npcHandler = npcHandler, onlyFocus = true, text = 'Then come back when you are ready.'})

-- Make it react to hi/bye etc.
npcHandler:addModule(FocusModule:new())