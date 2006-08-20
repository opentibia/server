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

attackType = ATTACK_NONE
needDirection = false
areaEffect = NM_ME_MAGIC_ENERGY
animationEffect = NM_ANI_NONE

hitEffect = NM_ME_NONE
damageEffect = NM_ME_MAGIC_ENERGY
animationColor = GREEN
offensive = false
drawblood = false

MassHealingObject = MagicDamageObject(attackType, animationEffect, hitEffect, damageEffect, animationColor, offensive, drawblood, 0, 0)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}

n = tonumber(var)   -- try to convert it to a number

	if n ~= nil then
		-- healing testing exura gran mas res "number
		MassHealingObject.minDmg = var+0
		MassHealingObject.maxDmg = var+0

		-- MassHealingObject.minDmg = 0
		-- MassHealingObject.maxDmg = 0
	else
		MassHealingObject.minDmg = ((level+maglv)*3) * 1.0
		MassHealingObject.maxDmg = ((level+maglv)*3) * 2.2

	end

return doAreaMagic(cid, centerpos, needDirection, areaEffect, area, MassHealingObject:ordered())
end





