area = {
{1, 1, 1},
{1, 1, 1},
{1, 1, 1}}

needDirection = false
areaEffect = NM_ME_NONE
animationEffect = NM_ANI_ENERGY

damageEffect = NM_ME_ENERGY_DAMAGE
animationColor = RED
offensive = true
physical = false
minDmg = 30
maxDmg = 30
magicType = MAGIC_CONDITIONENERGY

EnergyBombObject = MagicDamageObject(animationEffect, damageEffect, animationColor, offensive, physical, 0, 0)
SubEnergyBombObject1 = MagicDamageObject(NM_ANI_NONE, damageEffect, animationColor, offensive, physical, minDmg, maxDmg)
SubEnergyBombObject2 = MagicDamageObject(NM_ANI_NONE, damageEffect, animationColor, offensive, physical, 25, 25)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}

return doAreaGroundMagic(cid, centerpos, needDirection, areaEffect, area, EnergyBombObject:ordered(), magicType,
0, 1, SubEnergyBombObject1:ordered(),
5000, 1, SubEnergyBombObject2:ordered(),
2, 105000, 1495, 1)

end
