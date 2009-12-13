local magic_field = {}

function magic_field.callback(event)
	local target = event.creature
	
	local condition = {"burning", 0, COMBAT_FIREDAMAGE,
		["damage"] = {
				10000,
				COMBAT_FIREDAMAGE,
				rounds = 7,
				min = -10,
				max = -10,
				icon = ICON_BURN
			}
	}
	
	if internalCastSpell(COMBAT_FIREDAMAGE, nil, target, -20) then
		target:addCondition(condition)
	end
end

function magic_field.registerHandler()
	if magic_field.listener then
		stopListener(magic_field.listener)
	end
	magic_field.listener =
		registerOnAnyCreatureMoveIn("itemid", 1492, magic_field.callback)
end

magic_field.registerHandler()
