area = {
{0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
{0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
{0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
{2, 2, 2, 0, 0, 1, 0, 0, 3, 3, 3},
{2, 2, 2, 2, 2, 0, 3, 3, 3, 3, 3},
{2, 2, 2, 0, 0, 4, 0, 0, 3, 3, 3},
{0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 4, 4, 4, 0, 0, 0, 0},
{0, 0, 0, 0, 4, 4, 4, 0, 0, 0, 0},
{0, 0, 0, 0, 4, 4, 4, 0, 0, 0, 0}
}
needDirection = true
areaEffect = NM_ME_ENERGY_AREA
animationEffect = NM_ANI_NONE

damageEffect = NM_ME_ENERGY_DAMAGE
animationColor = RED
offensive = true
physical = true

EnergyWaveObject = MagicDamageObject(animationEffect, damageEffect, animationColor, offensive, physical, 0, 0)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}
		EnergyWaveObject.minDmg = (level * 2 + maglv * 3) * 1.3
		EnergyWaveObject.maxDmg = (level * 2 + maglv * 3) * 1.7
return doAreaMagic(cid, centerpos, needDirection, areaEffect, area, EnergyWaveObject:ordered())
end



