
otstd.Commands.MakeItem = Command:new()

otstd.Commands.MakeItem.words = "/n"
otstd.Commands.MakeItem.groups = {"GM"}

function otstd.Commands.MakeItem.handler(event)
	if event.speaker:type() ~= "Player" then
		return
	end
	
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
	
	--event.speaker:sendNote(name[#name])
	if itemid then
		item = createItem(itemid, count)
		event.speaker:addItem(item)
	elseif name:sub(-1) == "s" then
		name = name:sub(1, #name - 1)
		itemid = getItemIDByName(name)
		if itemid then
			item = createItem(itemid, count)
			event.speaker:addItem(item)
		end
	end
	
	if item then
		event.speaker:sendNote(event.param:gsub(item:getName(), function(s) return "'" .. s .. "'" end) .. " created. (id:" .. item:getItemID() .. ")")
	else
		event.speaker:sendNote("There is no item with that name!")
	end
	
	event.text = "" -- Don't display a message
end

otstd.Commands.MakeItem:register()
