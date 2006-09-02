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
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    }
 
 attackType = ATTACK_POISON
 needDirection = false
 areaEffect = NM_ME_POISEN_RINGS
 animationEffect = NM_ANI_NONE
 
 hitEffect = NM_ME_POISEN
 damageEffect = NM_ME_POISEN_RINGS
 animationColor = GREEN
 offensive = true
 needDirection = false
 drawblood = false
 minDmg = 20
 maxDmg = 20
 
 PoisonStormObject = MagicDamageObject(attackType, animationEffect, hitEffect, damageEffect, animationColor, offensive, drawblood, minDmg, maxDmg)
 SubPoisonStormObject1 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 15, 15)
 SubPoisonStormObject2 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 14, 14)
 SubPoisonStormObject3 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 13, 13)
 SubPoisonStormObject4 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 12, 12)
 SubPoisonStormObject5 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 11, 11)
 SubPoisonStormObject6 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 10, 10)
 SubPoisonStormObject7 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 9, 9)
 SubPoisonStormObject8 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 8, 8)
 SubPoisonStormObject9 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 7, 7)
 SubPoisonStormObject10 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 6, 6)
 SubPoisonStormObject11 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 5, 5)
 SubPoisonStormObject12 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 4, 4)
 SubPoisonStormObject13 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 3, 3)
 SubPoisonStormObject14 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 2, 2)
 SubPoisonStormObject15 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 1, 1)

 function onCast(cid, creaturePos, level, maglv, var)
 	centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}

	PoisonStormObject.minDmg = (level * 1 + maglv * 2) * 2.3 - 30
 	PoisonStormObject.maxDmg = (level * 1 + maglv * 2) * 3.0 
 
 	return doAreaExMagic(cid, centerpos, needDirection, areaEffect, area, PoisonStormObject:ordered(),
 		3000, 2, SubPoisonStormObject1:ordered(),
 		3000, 2, SubPoisonStormObject2:ordered(),
 		3000, 3, SubPoisonStormObject3:ordered(),
 		3000, 3, SubPoisonStormObject4:ordered(),
 		3000, 3, SubPoisonStormObject5:ordered(),
 		3000, 4, SubPoisonStormObject6:ordered(),
 		3000, 4, SubPoisonStormObject7:ordered(),
 		3000, 4, SubPoisonStormObject8:ordered(),
 		3000, 4, SubPoisonStormObject9:ordered(),
 		3000, 6, SubPoisonStormObject10:ordered(),
 		3000, 6, SubPoisonStormObject11:ordered(),
 		3000, 6, SubPoisonStormObject12:ordered(),
 		3000, 6, SubPoisonStormObject13:ordered(),
 		3000, 7, SubPoisonStormObject14:ordered(),
 		3000, 20, SubPoisonStormObject15:ordered(),
 		15)
 end