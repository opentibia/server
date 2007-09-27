local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_BLUE)
setCombatParam(combat, COMBAT_PARAM_AGGRESSIVE, FALSE)
setCombatParam(combat, COMBAT_PARAM_DISPEL, CONDITION_INVISIBLE)

local area = createCombatArea(AREA_CIRCLE3X3)
setCombatArea(combat, area)

function onTargetCreature(cid, target)
	if(isPlayer(target) == TRUE) then
		if(getWorldType() == WORLD_TYPE_PVP_ENFORCED) then
			if(getPlayerItemCount(target, 2202) >= 1) then
				local rand = math.random(1,4) --25% chance to break it
				if(rand == 1) then
					doPlayerRemoveItem(target, 2202, 1)
					doSendMagicEffect(getPlayerPosition(target), CONST_ME_MAGIC_RED) --Not sure about the effect
				end
			end
		end
	end
	return TRUE
end
setCombatCallback(combat, CALLBACK_PARAM_TARGETCREATURE, "onTargetCreature")

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end