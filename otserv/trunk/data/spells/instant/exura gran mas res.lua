-- sven, the bewitched bunny
-- it's a sample script, i dont know lua well enough to
-- make some fancy code
-- the good thing is, that this scripts can easily be developed
-- seperately from the main programm
-- perhaps we should write some docu

-- the id of the creature we are attacking, following, etc.

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
areaEffect = NM_ME_MAGIC_ENERGIE
animationEffect = NM_ANI_NONE

damageEffect = NM_ME_MAGIC_ENERGIE
animationColor = GREEN
offensive = false
physical = false

MassHealingObject = MagicDamageObject(animationEffect, damageEffect, animationColor, offensive, physical, 0, 0)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}

n = tonumber(var)   -- try to convert it to a number

	if n ~= nil then
		MassHealingObject.minDmg = var+0
		MassHealingObject.maxDmg = var+0

	else
		MassHealingObject.minDmg = ((level+maglv)*3) * 1.0
		MassHealingObject.maxDmg = ((level+maglv)*3) * 2.2

	end

return doAreaMagic(cid, centerpos, needDirection, areaEffect, area, MassHealingObject:ordered())
end





