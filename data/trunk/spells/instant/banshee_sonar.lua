area = {
{0, 0, 1, 1, 1, 0, 0},
{0, 1, 1, 1, 1, 1, 0},
{1, 1, 1, 1, 1, 1, 1},
{1, 1, 1, 0, 1, 1, 1},
{1, 1, 1, 1, 1, 1, 1},
{0, 1, 1, 1, 1, 1, 0},
{0, 0, 1, 1, 1, 0, 0}
}

attackType = ATTACK_FIRE
needDirection = false
areaEffect = NM_ME_SOUND_RED
animationEffect = NM_ANI_NONE

hitEffect = NM_ME_EXPLOSION_DAMAGE
damageEffect = NM_ME_SOUND_RED
animationColor = RED
offensive = true
drawblood = true

SonarObject = MagicDamageObject(attackType, animationEffect, hitEffect, damageEffect, animationColor, offensive, drawblood, 0, 0)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}
n = tonumber(var)   -- try to convert it to a number
if n ~= nil then
	SonarObject.minDmg = 0
	SonarObject.maxDmg = 0 
else
	SonarObject.minDmg = (level * 2 + maglv * 2) * 1.3 - 30
	SonarObject.maxDmg = (level * 2 + maglv * 2) * 2.0 	
end 

return doAreaMagic(cid, centerpos, needDirection, areaEffect, area, SonarObject:ordered())
end  
