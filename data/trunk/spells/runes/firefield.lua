area = {
{0, 0, 0},
{0, 1, 0},
{0, 0, 0}
}

attackType = ATTACK_FIRE
needDirection = false
areaEffect = NM_ME_FIRE_AREA
animationEffect = NM_ANI_FIRE

hitEffect = NM_ME_FIRE_AREA
damageEffect = NM_ME_HITBY_FIRE
animationColor = RED
offensive = true
drawblood = false
minDmg = 20
maxDmg = 20

FireBombObject = MagicDamageObject(attackType, animationEffect, hitEffect, damageEffect, animationColor, offensive, drawblood, 0, 0)
SubFireBombObject1 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, minDmg, maxDmg)
SubFireBombObject2 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 10, 10)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}

return doAreaGroundMagic(cid, centerpos, needDirection, areaEffect, area, FireBombObject:ordered(),
	0, 1, SubFireBombObject1:ordered(),
	5000, 1, SubFireBombObject2:ordered(),
	2, 60000, 1492,
	5000, 6, SubFireBombObject2:ordered(),
	1, 60000, 1493,
	0, 25000, 1494, 3)
end
