area = {
{0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
{2, 2, 2, 2, 2, 2, 0, 3, 3, 3, 3, 3, 3},
{0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0}
}

attackType = ATTACK_ENERGY
needDirection = true
areaEffect = NM_ME_ENERGY_DAMAGE
animationEffect = NM_ANI_NONE

hitEffect = NM_ME_EXPLOSION_DAMAGE
damageEffect = NM_ME_ENERGY_DAMAGE
animationColor = RED
offensive = true
drawblood = false

GreatEnergyBeamObject = MagicDamageObject(attackType, animationEffect, hitEffect, damageEffect, animationColor, offensive, drawblood, 0, 0)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}
GreatEnergyBeamObject.minDmg = (level * 2 + maglv *3) * 0.8
GreatEnergyBeamObject.maxDmg = (level * 2 + maglv *3)

return doAreaMagic(cid, centerpos, needDirection, areaEffect, area, GreatEnergyBeamObject:ordered())
end  

