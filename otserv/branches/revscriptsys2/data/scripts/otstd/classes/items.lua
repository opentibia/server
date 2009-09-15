
Items = {}
setmetatable(Items, {
	__index = 
		function(o, itemid)
			local t = getItemType(itemid)
			if t then
				o[t] = t
			end
			return t
		end;
})

function getItemNameByID(itemid, count, alternate_name)
	local item_type = Items[itemid]
	
	s = ""
	
	if not count or count == 1 then
		if item_type then
			s = s .. item_type.article .. " " .. (alternate_name or item_type.name)
		else
			s = s .. "an item of type " .. itemid
		end
	else
		s = s .. count .. " " 
		if item_type then
			if alternate_name then
				s = s .. alternate_name .. "s"
			else
				if #item_type.pluralName > 0 then
					s = s .. item_type.pluralName
				else
					s = s .. item_type.name .. "s"
				end
			end
		else
			s = s .. count .. " items of type " .. itemid
		end
	end
	
	return s
end
