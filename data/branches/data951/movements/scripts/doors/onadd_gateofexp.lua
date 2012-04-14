function onAddItem(item, tile, pos)
	local isLevelDoor = (item.actionid >= 1001 and item.actionid <= 1999)
	local isVocationDoor = (item.actionid >= 2001 and item.actionid <= 2008)

	if not(isLevelDoor or isVocationDoor) then
		-- It's a normal door, we don't need special descriptions.
		return true
	end

	if(isLevelDoor) then
		-- Level Door. Level is actionid - 1000
		local level = item.actionid - 1000
		doSetItemSpecialDescription(item.uid, "It is a gate for level " .. level .. ".\n Only the worthy may pass.")
	else
		-- Set a description for each voc
		local vocDescriptions = {
			[1] = "sorcerers", [2] = "druids", [3] = "paladins", [4] = "knights",
			[5] = "master sorcerers", [6] = "elder druids", [7] = "royal paladins", [8] = "elite knights"
		}
		local voc = vocDescriptions[item.actionid-2000]
		doSetItemSpecialDescription(item.uid, "It is a gate for " .. voc .. ".\n Only the worthy may pass.")
	end
	return true
end