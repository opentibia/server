function onLoad()
attackType = ATTACK_PHYSICAL
animationEffect = NM_ANI_SUDDENDEATH

hitEffect = NM_ME_MORT_AREA
damageEffect = NM_ME_DRAW_BLOOD

createTargetSpell(attackType, animationEffect, hitEffect, damageEffect)
end

function onCast(cid, var)
end

-- called one time for each creature that is inside the 'spell area'
-- cid: spellcaster
-- tid: target creature
function onUse(cid, tid, var)

level = getPlayerLevel(cid)
maglv = getPlayerMagLevel(cid)

minDmg = (level * 2 + maglv *3) * 1.3 - 30
maxDmg = (level * 2 + maglv *3) * 1.7

return math.random(minDmg, maxDmg)
end
