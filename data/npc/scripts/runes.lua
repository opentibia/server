-- This is an example NPC script that can be used on Jiddo's NPC system

local keywordHandler = KeywordHandler:new()
local npcHandler = NpcHandler:new(keywordHandler)
NpcSystem.parseParameters(npcHandler)



-- OTServ event handling functions start
function onCreatureAppear(cid)              npcHandler:onCreatureAppear(cid) end
function onCreatureDisappear(cid)           npcHandler:onCreatureDisappear(cid) end
function onCreatureSay(cid, type, msg)      npcHandler:onCreatureSay(cid, type, msg) end
function onThink()                          npcHandler:onThink() end
-- OTServ event handling functions end

local shopModule = ShopModule:new()
npcHandler:addModule(shopModule)

shopModule:addBuyableItem({'light wand', 'lightwand'}, 		2163, 500, 		'magic light wand')
shopModule:addBuyableItem({'mana fluid', 'manafluid'}, 		11396, 100, 	7, 	'mana fluid')
shopModule:addBuyableItem({'life fluid', 'lifefluid'}, 		11396, 80, 	10,	'life fluid')
shopModule:addBuyableItem({'heavy magic missile', 'hmm'}, 	2311, 125, 	10,	'heavy magic missile rune')
shopModule:addBuyableItem({'great fireball', 'gfb'}, 			2304, 180, 	4, 	'great fireball rune')
shopModule:addBuyableItem({'explo', 'xpl'}, 					2313, 250, 	6, 	'explosion rune')
shopModule:addBuyableItem({'ultimate healing', 'uh'}, 		2273, 175, 	2, 	'ultimate healing rune')
shopModule:addBuyableItem({'sudden death', 'sd'}, 			2268, 325, 	2, 	'sudden death rune')
shopModule:addBuyableItem({'blank', 'rune'}, 					2260, 10, 		'blank rune')


npcHandler:addModule(FocusModule:new())