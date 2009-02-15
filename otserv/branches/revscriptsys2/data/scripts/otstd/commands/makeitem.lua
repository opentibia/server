
local MakeItem = Command:new("MakeItem")

MakeItem.words = "/n"
MakeItem.groups = {"GM"}

function MakeItem.handler(event)
	local name = event.param
	local count = name:match("(%d+)")
	
	if count then
		name = name:gsub("(%w+)(%w)" .. count .. "(%w)(%w+)", " ")
		name = name:gsub(count, "")
	else
		count = -1
	end
	
	name = name:strip_whitespace()
	
	local itemid = getItemIDByName(name)
	local item = nil
	
	--event.creature:sendNote(name[#name])
	if itemid then
		item = createItem(itemid, count)
	elseif name:sub(-1) == "s" then
		name = name:sub(1, #name - 1)
		itemid = getItemIDByName(name)
		if itemid then
			item = createItem(itemid, count)
		end
	end
	
	if item == nil then
		id = nil
		count = 1
		for w in event.param:gmatch("(%d+)") do
			if id then
				count = w
			else
				id = w
			end
		end
		if id then
			item = createItem(id, count)
		end
	end
	
	if item then
		event.creature:addItem(item)
		event.creature:sendNote(event.param:gsub(item:getName(), function(s) return "'" .. s .. "'" end) .. " created. (id:" .. item:getItemID() .. ")")
	else
		event.creature:sendNote("There is no item with that name!")
	end
	
	event.text = "" -- Don't display a message
end

MakeItem:register()
