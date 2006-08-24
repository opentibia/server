area = {
{1, 1, 1},
{1, 1, 1},
{1, 1, 1},
}

attackType = ATTACK_FIRE
needDirection = false
areaEffect = NM_ME_FIRE_AREA
animationEffect = NM_ANI_BURSTARROW

hitEffect = NM_ME_FIRE_AREA
damageEffect = NM_ME_HITBY_FIRE
animationColor = RED
offensive = true
drawblood = false

BurstObject = MagicDamageObject(attackType, animationEffect, hitEffect, damageEffect, animationColor, offensive, drawblood, 0, 0)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}

BurstObject.minDmg = 90
BurstObject.maxDmg = 180

return doAreaMagic(cid, centerpos, needDirection, areaEffect, area, BurstObject:ordered())
end
