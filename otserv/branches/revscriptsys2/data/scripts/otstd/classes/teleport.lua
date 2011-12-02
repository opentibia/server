
function Teleport:type()
	return "Teleport"
end


function createTeleport()
	local t = createItem(1387)
	assert(t:isTeleport(), "Item 1387 is not a teleport (magic forcefield), OTB error!")
	return t
end

