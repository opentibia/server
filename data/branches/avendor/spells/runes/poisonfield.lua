area = {
{0, 0, 0},
{0, 1, 0},
{0, 0, 0}
}

attackType = ATTACK_POISON
needDirection = false
areaEffect = NM_ME_NONE
animationEffect = 14

hitEffect = NM_ME_POISEN_RINGS
damageEffect = NM_ME_POISEN_RINGS
animationColor = GREEN
offensive = true
drawblood = false

PosionObject = MagicDamageObject(attackType, animationEffect, hitEffect, damageEffect, animationColor, offensive, drawblood, 1, 1)
SubPoisonObject1 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 10, 10)
SubPoisonObject2 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 5, 5)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}

return doAreaGroundMagic(cid, centerpos, needDirection, areaEffect, area, PosionObject:ordered(),
	1000, 10, SubPoisonObject1:ordered(),
	2000, 18, SubPoisonObject2:ordered(),
	2, 240000, 1490, 1)
end
