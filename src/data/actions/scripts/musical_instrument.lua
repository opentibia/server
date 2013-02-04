local BIRD_CAGE = 2095
local WOODEN_WHISTLE = 5786
local DIDGERIDOO = 3952
local CORNUCOPIA = 2369
local PARTY_TRUMPET = 6572
local USED_PARTY_TRUMPET = 6573

function onUse(cid, item, frompos, item2, topos)
	local random = math.random(1, 5)

	if (isInArray(MUSICAL_INSTRUMENTS, item2.itemid) ) then
		doSendMagicEffect(frompos, CONST_ME_SOUND_BLUE)
	elseif (item.itemid == BIRD_CAGE) then
		doSendMagicEffect(frompos, CONST_ME_SOUND_YELLOW)
	elseif (item.itemid == DIDGERIDOO) then
		if (random == 1) then
			doSendMagicEffect(frompos, CONST_ME_SOUND_BLUE)
		end
	elseif (item.itemid == PARTY_TRUMPET) then
		doTransformItem(item.uid, USED_PARTY_TRUMPET)
		doCreatureSay(cid, "TOOOOOOT!", TALKTYPE_ORANGE_1)
		doSendMagicEffect(frompos, CONST_ME_SOUND_BLUE)
		doDecayItem(item.uid)
		if item.actionid ~= 0 then
			doSetItemActionId(item.uid, item.actionid)
		end
	elseif (item.itemid == CORNUCOPIA) then
		for i = 1, 11 do
			doPlayerAddItem(cid, 2681)
		end
		doRemoveItem(item.uid, 1)
		doSendMagicEffect(frompos, CONST_ME_SOUND_YELLOW)
	elseif (item.itemid == WOODEN_WHISTLE) then
		if (random == 2) then
			doSendMagicEffect(frompos, CONST_ME_SOUND_RED)
			doRemoveItem(item.uid, 1)
		else
			local pos = getPlayerPosition(cid)
			pos = {x=pos.x+1, y=pos.y, z=pos.z}
			doSendMagicEffect(frompos, CONST_ME_SOUND_PURPLE)
			doSummonCreature("Wolf", pos)
		end
	end
	return true
end

