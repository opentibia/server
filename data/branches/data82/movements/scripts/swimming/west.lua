dofile(getDataDir() .. "movements/scripts/swimming/swimlib.lua")

function onStepIn(cid, item, pos)
	return checkSwim(cid, WEST, EAST)
end
