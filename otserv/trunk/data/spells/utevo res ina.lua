
function onCast(cid, creaturePos, level, maglv, var)
	if pcall(function () n = outfits[var] end) then
		if n~=nil then
		time = 10 --time in seconds
		changeOutfit(cid, time, n)
		end
	end
end



