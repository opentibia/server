area = {
{0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0},
{2, 2, 2, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 3, 3, 3},
{2, 2, 2, 2, 2, 0, 0, 0, 1, 0, 0, 0, 3, 3, 3, 3, 3},
{2, 2, 2, 2, 2, 2, 0, 0, 1, 0, 0, 3, 3, 3, 3, 3, 3},
{2, 2, 2, 2, 2, 2, 2, 2, 0, 3, 3, 3, 3, 3, 3, 3, 3},
{2, 2, 2, 2, 2, 2, 0, 0, 4, 0, 0, 3, 3, 3, 3, 3, 3},
{2, 2, 2, 2, 2, 0, 0, 0, 4, 0, 0, 0, 3, 3, 3, 3, 3},
{2, 2, 2, 0, 0, 0, 0, 4, 4, 4, 0, 0, 0, 0, 3, 3, 3},
{0, 0, 0, 0, 0, 0, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 4, 4, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 4, 4, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 4, 4, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0}
}

attackType = ATTACK_POISON
needDirection = true
areaEffect = NM_ME_POISONCLOUD
animationEffect = NM_ANI_NONE

hitEffect = NM_ME_POISONCLOUD
damageEffect = NM_ME_POISONCLOUD
animationColor = GREEN
offensive = true
drawblood = false

PoisonWaveObject = MagicDamageObject(attackType, animationEffect, hitEffect, damageEffect, animationColor, offensive, drawblood, 0, 0)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}
PoisonWaveObject.minDmg = 90
PoisonWaveObject.maxDmg = 140

return doAreaMagic(cid, centerpos, needDirection, areaEffect, area, PoisonWaveObject:ordered())
end  

