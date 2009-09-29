
otstd.mailbox = {}

function otstd.mailbox.handler(event)
	event.creature:sendNote("Attempting to send")
	local mail = event.item
	local label = nil
	
	if mail:getItemID() == 2595 then
		label = filter((function(i) return i:getItemID() == 2599 end), mail:getItems())
		if #label == 0 then
			event.creature:sendNote("No label found")
			return
		end
		event.creature:sendNote("Found a label " .. #label)
		label = label[1]
	end
	
	if mail:getItemID() == 2597 then
		label = mail
	end
	
	if not label then
		-- Not a mail item
		event.creature:sendNote("No label")
		return
	end
	event.creature:sendNote("Found label")
	
	local text = label:getText()
	
	local lines = text:explode()
	if #lines < 2 then
		local s = "label too short:"
		for k, v in pairs(lines) do
			s = s .. k .. '=' .. v .. '  '
		end
		event.creature:sendNote(s)
		return
	end
	local name = lines[1]:strip_whitespace()
	local town = map:getTown(lines[2]:strip_whitespace())
	
	if not town then
		event.creature:sendNote("No town specified")
		return
	end
	
	event.creature:sendNote("Sending mail")
	sendMail(mail, name, town)
end

otstd.mailbox.listener = registerOnMoveItemToItem("itemid", 2593, otstd.mailbox.handler)
