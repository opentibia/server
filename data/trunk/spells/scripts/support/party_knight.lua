local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_RED)
setCombatParam(combat, COMBAT_PARAM_AGGRESSIVE, 0)

local condition = createConditionObject(CONDITION_ATTRIBUTES)
setConditionParam(condition, CONDITION_PARAM_BUFF_SPELL, 1)
setConditionParam(condition, CONDITION_PARAM_SUBID, 1)
setConditionParam(condition, CONDITION_PARAM_TICKS, 2 * 60 * 1000)
setConditionParam(condition, CONDITION_PARAM_SKILL_MELEE, 3)

local area = createCombatArea(AREA_CROSS5X5)
setCombatArea(combat, area)

function onCastSpell(cid, var)
	local memberlist = getPartyMembers(cid)
	if memberlist == nil then
		doPlayerSendCancel(cid, "You have to be in a party to cast this spell.")
		return 0
	end

	local manaCost = table.getn(memberlist) * 50
	local playerMana = getPlayerMana(cid)
	local playerPos = getCreaturePosition(cid)
	if(playerMana < manaCost) then
		doPlayerSendDefaultCancel(RETURNVALUE_NOTENOUGHMANA)
		return 0
	end
	
	doPlayerAddMana(cid, -manaCost)
	doPlayerAddManaSpent(cid, manaCost)
	doCombat(cid, combat, var)

	for index, member in pairs(memberlist) do
		local memberPos = getCreaturePosition(member)
		if(getDistanceBetween(memberPos, playerPos)) <= 36 then
			doAddCondition(member, condition)
		end
	end

	return 1	
end
