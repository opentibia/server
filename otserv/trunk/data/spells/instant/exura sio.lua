animationEffect = NM_ANI_NONE
damageEffect = NM_ME_MAGIC_ENERGIE
animationColor = GREEN
offensive = false
physical = false

HealFriendObject = MagicDamageObject(animationEffect, damageEffect, animationColor, offensive, physical, 0, 0)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}
targetpos = getPosition(var)
HealFriendObject.minDmg = (level * 2 + maglv * 3) * 3
HealFriendObject.maxDmg = (level * 2 + maglv * 3) * 3 + 40
if targetpos.x ~= nil and targetpos.z ~= nil and targetpos.y ~= nil then
	if math.abs(targetpos.x - centerpos.x) < 18 and math.abs(targetpos.y - centerpos.y) < 14 and targetpos.z == centerpos.z then
		return doTargetMagic(cid, targetpos, HealFriendObject:ordered())
	end
end

return false
end



