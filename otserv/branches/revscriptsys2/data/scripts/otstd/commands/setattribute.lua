
SetItemAttribute = Command:new("SetItemAttribute")

SetItemAttribute.words = {"/setattribute", "/setitemattribute", "/sia"}
SetItemAttribute.groups = "All"

-- Handlers
function SetItemAttribute.handler(event)
	local item = event.creature:getInventoryItem(SLOT_HAND)
	local key, value = event.param:match("(%w+)=(.*)")
	
	if not key then
		event.creature:sendNote("Invalid syntax, use as \"/setitemattribute attribute=value\"")
	elseif not item then
		event.creature:sendNote("You are not carrying an item in your hand.")
	else
		key = key:strip_whitespace()
		value = value:strip_whitespace()
		
		item:setAttribute(key, value)
		event.creature:sendNote("Set attribute \"" .. key .. "\" to \"" .. (value or "") .. "\".")
		event.text = ""
	end
end

SetItemAttribute:register()
