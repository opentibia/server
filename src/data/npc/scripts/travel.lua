-- This is an example NPC script that can be used on Jiddo's NPC system
-- Coordinates are not real, same for the name of cities or other server-based things here

local keywordHandler = KeywordHandler:new()
local npcHandler = NpcHandler:new(keywordHandler)
NpcSystem.parseParameters(npcHandler)



-- OTServ event handling functions start
function onCreatureAppear(cid)              npcHandler:onCreatureAppear(cid) end
function onCreatureDisappear(cid)           npcHandler:onCreatureDisappear(cid) end
function onCreatureSay(cid, type, msg)      npcHandler:onCreatureSay(cid, type, msg) end
function onThink()                          npcHandler:onThink() end
-- OTServ event handling functions end

local travelNode = {}

-- Don't forget npcHandler = npcHandler in the parameters. It is required for all StdModule functions!
travelNode = keywordHandler:addKeyword({'derelin'}, StdModule.say, {npcHandler = npcHandler, onlyFocus = true, text = 'Do you wish to travel to Derelin for 119 gold coins?'})
	travelNode:addChildKeyword({'yes'}, StdModule.travel, {npcHandler = npcHandler, premium = false, level = 0, cost = 119, destination = {x=1367, y=403, z=7} })
	travelNode:addChildKeyword({'no'}, StdModule.say, {npcHandler = npcHandler, onlyFocus = true, reset = true, text = 'I wouldn\'t go there either.'})

travelNode = keywordHandler:addKeyword({'drunia'}, StdModule.say, {npcHandler = npcHandler, onlyFocus = true, text = 'Do you wish to travel to Drunia for 123 gold coins?'})
	travelNode:addChildKeyword({'yes'}, StdModule.travel, {npcHandler = npcHandler, premium = false, level = 0, cost = 123, destination = {x=967, y=247, z=7} })
	travelNode:addChildKeyword({'no'}, StdModule.say, {npcHandler = npcHandler, onlyFocus = true, reset = true, text = 'Too expensive, eh?'})

keywordHandler:addKeyword({'destination'}, StdModule.say, {npcHandler = npcHandler, onlyFocus = true, text = 'I can take you to Derelin or Drunia for just a small fee.'})

-- Makes sure the npc reacts when you say hi, bye etc.
npcHandler:addModule(FocusModule:new())