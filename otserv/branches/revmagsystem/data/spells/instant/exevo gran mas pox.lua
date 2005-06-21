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

attackType = ATTACK_POISON
needDirection = false
areaEffect = NM_ME_POISEN_RINGS
distanceEffect = NM_ANI_NONE

hitEffect = NM_ME_POISEN
damageEffect = NM_ME_POISEN_RINGS
animationColor = GREEN
needDirection = false

function onLoad()
  --createAttackSpell(hitEffect, damageEffect, areaEffect, area, needDirection)
  --setParam(DISTANCE_SHOOT, distanceEffect)

  addCondition(CONDITION_POISON, 2000, 15)
  addCondition(CONDITION_POISON, 2000, 10)
  addCondition(CONDITION_POISON, 2000, 5)
  addCondition(CONDITION_POISON, 2000, 5)
end  

function onCast(cid, var)
return true
end  

function onUse(cid, tid)
return math.random(100, 200)
end
