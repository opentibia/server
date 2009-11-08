local listeners = {}
local manaShield = Spell:new("Mana Shield")

manaShield.words       = "utamo vita"
manaShield.vocation    = {"druid", "elder druid", "sorcerer", "master sorcerer"}
manaShield.level       = 14
manaShield.mana        = 50
manaShield.condition   = {
	id = manaShield.name, duration=20000,
	["script"] = {name="magicshield_effect"}
}

function manaShield.onDamage(event)
	local caster = event.creature
	if event.value > 0 then
		return false
	end
	
	local damage = math.abs(event.value)
	local mana = caster:getMana()
	local manaLoss = math.min(mana, damage);

	if manaLoss > 0 then
		caster:removeMana(manaLoss)
		local casterPos = caster:getPosition()
		sendMagicEffect(casterPos, MAGIC_EFFECT_BLUE_RING);
		sendAnimatedText(casterPos, TEXTCOLOR_BLUE, manaLoss );
	end

	event.value = math.max(0, damage - manaLoss);
	if event.value == 0 then
		event:skip()
	end
		
	return true
end

function manaShield.onBegin(event)
	local caster = event.creature
	print(caster:getName() .. " gains " .. manaShield.name )

	local t = listeners[caster:getID()]
	if t then
		stopListener(t.listener)
	end
	
	listeners[caster:getID()] = {
			listener = registerOnCreatureDamage(caster, "all", manaShield.onDamage)
		}
end

function manaShield.onEnd(event)
	local caster = event.creature
	print(manaShield.name .. " fades from " .. caster:getName() )

	local t = listeners[caster:getID()]
	if t then
		stopListener(t.listener)
	end
end

function manaShield.registerHandlers()
	if manaShield.listener_added then
		stopListener(manaShield.listener_added)
	end	
	manaShield.listener_added = registerOnConditionEffect("magicshield_effect", "begin", manaShield.onBegin)

	if manaShield.listener_removed then
		stopListener(manaShield.listener_removed)
	end
	manaShield.listener_removed = registerOnConditionEffect("magicshield_effect", "end", manaShield.onEnd)
	
	manaShield:register()
end

manaShield.registerHandlers()
