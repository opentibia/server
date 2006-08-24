area = {
{1, 1, 1},
{1, 0, 1},
{1, 1, 1},
}

attackType = ATTACK_ENERGY
needDirection = false
areaEffect = NM_ME_ENERGY_DAMAGE
animationEffect = NM_ANI_NONE

hitEffect = NM_ME_NONE
damageEffect = NM_ME_ENERGY_DAMAGE
animationColor = RED
offensive = true
drawblood = true

DjinnExoriObject = MagicDamageObject(attackType, animationEffect, hitEffect, damageEffect, animationColor, offensive, drawblood, 0, 0)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}
n = tonumber(var)   -- try to convert it to a number
if n ~= nil then
	DjinnExoriObject.minDmg = 0
	DjinnExoriObject.maxDmg = 0 
else
	DjinnExoriObject.minDmg = 25
	DjinnExoriObject.maxDmg = 80
end 

return doAreaMagic(cid, centerpos, needDirection, areaEffect, area, DjinnExoriObject:ordered())
end  
