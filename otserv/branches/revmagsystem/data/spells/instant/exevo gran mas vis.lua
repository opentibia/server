
function onLoad()

attackType = ATTACK_PHYSICAL

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
areaEffect = NM_ME_EXPLOSION_AREA

damageEffect = NM_ME_DRAW_BLOOD
hitEffect = NM_ME_EXPLOSION_DAMAGE

createAreaAttackSpell(attackType, NM_ANI_NONE, area, needDirection, areaEffect, hitEffect, damageEffect)
end  

function onCast(cid, var)
end

-- called one time for each creature that is inside the 'spell area'
-- cid: spellcaster
-- tid: target creature
function onUse(cid, tid, var)

level = getPlayerLevel(cid)
maglv = getPlayerMagLevel(cid)

minDmg = (level * 2 + maglv * 3) * 2.3 - 30
maxDmg = (level * 2 + maglv * 3) * 3.0

return math.random(minDmg, maxDmg)
end  
