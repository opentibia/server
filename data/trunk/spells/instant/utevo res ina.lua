attackType = ATTACK_NONE
animationEffect = NM_ANI_NONE

hitEffect = NM_ME_NONE
damageEffect = NM_ME_MAGIC_ENERGIE
animationColor = GREEN
offensive = false
drawblood = false

CreatureIllusionObject = MagicDamageObject(attackType, animationEffect, hitEffect, damageEffect, animationColor, offensive, drawblood, 0, 0)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}

if pcall(function () n = outfits[var] end) then
	if n~=nil then
		time = 20 --time in seconds
		ret = doTargetMagic(cid, centerpos, CreatureIllusionObject:ordered())

		if(ret) then
			changeOutfit(cid, time, n)
		end
		
		return ret
	end
end

return false
end
