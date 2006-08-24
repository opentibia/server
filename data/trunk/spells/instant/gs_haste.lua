attackType = ATTACK_NONE
animationEffect = NM_ANI_NONE

hitEffect = NM_ME_NONE
damageEffect = NM_ME_MAGIC_POISEN
animationColor = GREEN
offensive = false
drawblood = false

GreatHasteObject = MagicDamageObject(attackType, animationEffect, hitEffect, damageEffect, animationColor, offensive, drawblood, 0, 0)

function onCast(cid, creaturePos, level, maglv, var)
	centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}
	ret = doTargetMagic(cid, centerpos, GreatHasteObject:ordered())
	
	changeSpeedMonster(cid, 345, 40)

	return ret
end  

