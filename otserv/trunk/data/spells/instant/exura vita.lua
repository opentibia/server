animationEffect = NM_ANI_NONE
damageEffect = NM_ME_MAGIC_ENERGIE
animationColor = GREEN
offensive = false
physical = false

UltimateHealingObject = MagicDamageObject(animationEffect, damageEffect, animationColor, offensive, physical, 0, 0)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}
n = tonumber(var)   -- try to convert it to a number
	if n ~= nil then
		-- bugged
		-- UltimateHealingObject.minDmg = var+0
		-- UltimateHealingObject.maxDmg = var+0
		UltimateHealingObject.minDmg = 0
		UltimateHealingObject.maxDmg = 0
	else
		UltimateHealingObject.minDmg = (level * 2 + maglv * 3) * 2
		UltimateHealingObject.maxDmg = (level * 2 + maglv * 3) * 2.8
	end
return doTargetMagic(cid, centerpos, UltimateHealingObject:ordered())
end
