area = {
{0, 0, 1, 1, 1, 0, 0},
{0, 1, 1, 1, 1, 1, 0},
{1, 1, 1, 1, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1},
{0, 1, 1, 1, 1, 1, 0},
{0, 0, 1, 1, 1, 0, 0}
}

needDirection = false
areaEffect = NM_ME_FIRE_AREA
animationEffect = NM_ANI_FIRE

damageEffect = NM_ME_FIRE_AREA
animationColor = RED
offensive = true
physical = false

GreatFireballObject = MagicDamageObject(animationEffect, damageEffect, animationColor, offensive, physical, 0, 0)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}
GreatFireballObject.minDmg = (level * 2 + maglv *3) * 0.35
GreatFireballObject.maxDmg = (level * 2 + maglv *3) * 0.65
return doAreaMagic(cid, centerpos, needDirection, areaEffect, area, GreatFireballObject:ordered())
end
