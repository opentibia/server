attackType = ATTACK_NONE
animationEffect = NM_ANI_NONE

hitEffect = NM_ME_NONE
damageEffect = NM_ME_MAGIC_ENERGIE
animationColor = GREEN
offensive = false
drawblood = false

LightHealingObject = MagicDamageObject(attackType, animationEffect, hitEffect, damageEffect, animationColor, offensive, drawblood, 0, 0)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}
n = tonumber(var)   -- try to convert it to a number
	if n ~= nil then
		-- bugged
		-- LightHealingObject.minDmg = var+0
		-- LightHealingObject.maxDmg = var+0
		LightHealingObject.minDmg = 0
		LightHealingObject.maxDmg = 0
	else
		LightHealingObject.minDmg = (level * 2 + maglv * 3) * 0.08
		LightHealingObject.maxDmg = (level * 2 + maglv * 3) * 0.33
	end
return doTargetMagic(cid, centerpos, LightHealingObject:ordered())
end