area = {
{1, 1, 1},
{1, 1, 1},
{1, 1, 1}
}

needDirection = false
areaEffect = NM_ME_FIRE_AREA
animationEffect = NM_ANI_FIRE

damageEffect = NM_ME_HITBY_FIRE
animationColor = RED
offensive = true
physical = false
minDmg = 20
maxDmg = 20
magicType = MAGIC_CONDITIONFIRE

FireBombObject = MagicDamageObject(animationEffect, damageEffect, animationColor, offensive, physical, 0, 0)
SubFireBombObject1 = MagicDamageObject(NM_ANI_NONE, damageEffect, animationColor, offensive, physical, minDmg, maxDmg)
SubFireBombObject2 = MagicDamageObject(NM_ANI_NONE, damageEffect, animationColor, offensive, physical, 10, 10)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}
return doAreaGroundMagic(cid, centerpos, needDirection, areaEffect, area, FireBombObject:ordered(), magicType,
0, 1, SubFireBombObject1:ordered(),
9000, 1, SubFireBombObject1:ordered(),
2, 60000, 1185,
9000, 6, SubFireBombObject2:ordered(),
1, 60000, 1186,
0, 25000, 1187, 3)
end
