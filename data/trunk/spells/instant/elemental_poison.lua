area = {
 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
 {0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
 {0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0},
 {0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0},
 {0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0},
 {0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0},
 {0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0},
 {0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
 }
 
 attackType = ATTACK_POISON
 needDirection = false
 areaEffect = 20
 animationEffect = NM_ANI_NONE
 
 hitEffect = NM_ME_POISEN
 damageEffect = 20
 animationColor = GREEN
 offensive = true
 needDirection = false
 drawblood = false
 minDmg = 20
 maxDmg = 20
 
 PoisonStormObject = MagicDamageObject(attackType, animationEffect, hitEffect, damageEffect, animationColor, offensive, drawblood, minDmg, maxDmg)
 SubPoisonStormObject1 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 15, 15)
 SubPoisonStormObject2 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 10, 10)
 SubPoisonStormObject3 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 5, 5)
 
 function onCast(cid, creaturePos, level, maglv, var)
 	centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}

	PoisonStormObject.minDmg = (level * 2 + maglv * 3) * 1.5 - 30
 	PoisonStormObject.maxDmg = (level * 2 + maglv * 3) * 2
 
 	return doAreaExMagic(cid, centerpos, needDirection, areaEffect, area, PoisonStormObject:ordered(),
 		2000, 1, SubPoisonStormObject1:ordered(),
 		2000, 2, SubPoisonStormObject2:ordered(),
 		2000, 10, SubPoisonStormObject3:ordered(),
 		3)
 end
 
