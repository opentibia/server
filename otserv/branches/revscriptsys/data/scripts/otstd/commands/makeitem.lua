
otstd.Commands.MakeItem = Command:new()

otstd.Commands.MakeItem.words = "/n"
otstd.Commands.MakeItem.groups = {"GM"}

otstd.Commands.MakeItem.handler = function(event)
	if event.speaker:type() ~= "Player" then
		return
	end
	
	local name = event.text:sub(3)
	name = name:strip_whitespace()
	
	local itemid = getItemIDByName(name)
	
	if itemid then
		event.speaker:addItem(createItem(itemid))
	end
	
	event.text = "" -- Don't display a message
end

otstd.Commands.MakeItem:register()
