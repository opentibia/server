local convinceCreature = Spell:new("Convince Creature")

convinceCreature.rune        = 2290
convinceCreature.magicLevel  = 5
convinceCreature.needTarget  = true
convinceCreature.areaEffect  = MAGIC_EFFECT_RED_SHIMMER

convinceCreature.onBeginCast = function(spell, event)
	local caster = event.caster
	local m = event.targetCreature
	
	if #caster:getSummons() >= 2 then
		caster:sendCancel("You can only have two summons at the same time.")
		return false
	end
	
	if not m or not m:isConvinceable() then
		caster:sendCancel(RET_NOTPOSSIBLE)
		return false
	end
	
	if caster:getMana() < m:getManaCost() and not caster:hasInfiniteMana() then
		m:destroy()
		caster:sendCancel(RET_NOTENOUGHMANA)
		return false
	end
	
	event.summon = m
	return true
end

convinceCreature.onFinishCast = function(spell, event)
	local caster = event.caster
	
	caster:spendMana(event.summon:getManaCost())
	event.caster:addSummon(event.summon)
end

convinceCreature:register()
