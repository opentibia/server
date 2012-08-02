local summonCreature = Spell:new("Summon Creature")

summonCreature.words       = "utevo res"
summonCreature.vocation    = {DRUID, SORCERER}
summonCreature.level       = 30
summonCreature.areaEffect  = MAGIC_EFFECT_RED_SHIMMER

summonCreature.onBeginCast = function(spell, event)
	local caster = event.caster
	
	if #caster:getSummons() >= 2 then
		caster:sendCancel("You can only have two summons at the same time.")
		return false
	end
	
	local m = createMonster(event.param, {x=0, y=0, z=0})
	
	if not m or not m:isSummonable() then
		if m then m:destroy() end
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

summonCreature.onFinishCast = function(spell, event)
	local caster = event.caster
	
	caster:spendMana(event.summon:getManaCost())
	event.summon:teleportTo(event.caster:getPosition())
	event.caster:addSummon(event.summon)
end

summonCreature:register()
