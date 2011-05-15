-- Based on: http://otfans.net/showthread.php?t=148083

local PANDA_TEDDY = 5080
local MYSTERIOUS_VOODOO_SKULL =  5669
local ENIGMATED_VOODOO_SKULL = 5670
local STUFFED_DRAGON = 5791
local SANTA_DOLL = 6512
local ORACLE_FIGURINE = 8974
local TIBIACITY_ENCICLOPEDIA = 8977
local GOLDEN_NEWSPAPPER = 8981
local NORSEMAN_DOLL = 8982
local TOY_SPIDER = 9006
local CHRISTMAS_CARD = 6388

local USED_DOLLS = {
	[PANDA_TEDDY] = 6568,
	[MYSTERIOUS_VOODOO_SKULL] = 5670,
	[STUFFED_DRAGON] = 6566,
	[SANTA_DOLL] = 6567,
	[ORACLE_FIGURINE] = 8975,
	[TIBIACITY_ENCICLOPEDIA] = 9002,
	[NORSEMAN_DOLL] = 8985,
	[TOY_SPIDER] = 9007
}

function onUse(cid, item, frompos, item2, topos)

	if DOLLS[item.itemid] == nil then
		return false
	end

	local random = math.random(1, #DOLLS[item.itemid])
	local sound = DOLLS[item.itemid][random]

	if item.itemid == STUFFED_DRAGON then
		if random == 3 then
			doSendMagicEffect(getPlayerPosition(cid), CONST_ME_POFF)
		elseif random == 4 then
			doSendMagicEffect(getPlayerPosition(cid), CONST_ME_FIREAREA)
		elseif(random == 5) then
			doTargetCombatHealth(0, cid, COMBAT_PHYSICALDAMAGE, -1, -1, CONST_ME_EXPLOSIONHIT)
		end
	elseif item.itemid == MYSTERIOUS_VOODOO_SKULL then
		doSendMagicEffect(frompos, CONST_ME_MAGIC_RED)
	elseif item.itemid == CHRISTMAS_CARD then
		doSendMagicEffect(topos, CONST_ME_SOUND_YELLOW)
		sound = sound .. getPlayerName(cid) .. "."
		doPlayerSay(cid, sound, TALKTYPE_ORANGE_1)
		return true
	end

	doPlayerSay(cid, sound, TALKTYPE_ORANGE_1)
	doTransformItem(item.uid, USED_DOLLS[item.itemid])
	if item.actionid ~= 0 then
		doSetItemActionId(item.uid, item.actionid)
	end

	doDecayItem(item.uid)

	return true
end
