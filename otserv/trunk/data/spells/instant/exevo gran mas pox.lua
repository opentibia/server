area = {
{0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
{0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0},
{0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0},
{0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
{0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
{1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1},
{0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
{0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
{0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0},
{0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0},
{0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0}
}
needDirection = false
areaEffect = NM_ME_POISEN_RINGS
animationEffect = NM_ANI_NONE

damageEffect = NM_ME_POISEN_RINGS
animationColor = GREEN
offensive = true
needDirection = false
physical = false
magicType = MAGIC_CONDITIONPOISON
minDmg = 20
maxDmg = 20
PoisonStormObject = MagicDamageObject(animationEffect, damageEffect, animationColor, offensive, physical, minDmg, maxDmg)
SubPoisonStormObject1 = MagicDamageObject(animationEffect, damageEffect, animationColor, offensive, physical, 15, 15)
SubPoisonStormObject2 = MagicDamageObject(animationEffect, damageEffect, animationColor, offensive, physical, 10, 10)
SubPoisonStormObject3 = MagicDamageObject(animationEffect, damageEffect, animationColor, offensive, physical, 5, 5)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}
return doAreaExMagic(cid, centerpos, needDirection, areaEffect, area, PoisonStormObject:ordered(), magicType,
2000, 1, SubPoisonStormObject1:ordered(),
2000, 2, SubPoisonStormObject2:ordered(),
2000, 10, SubPoisonStormObject3:ordered(),
3)
end



