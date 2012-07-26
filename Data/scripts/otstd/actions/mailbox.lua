
otstd.mailbox = {}

otstd.mailboxes = {
		[2593] = {},
		[3981] = {}
	}

function otstd.mailbox.handler(event)
	local mail = event.item
	local label = nil
	
	if mail:getItemID() == 2595 then
		label = filter((function(i) return i:getItemID() == 2599 end), mail:getItems())
		if #label == 0 then
			return
		end
		label = label[1]
	end
	
	if mail:getItemID() == 2597 then
		label = mail
	end
	
	if not label then
		-- Not a mail item
		return
	end
	
	local text = label:getText()
	
	local lines = text:explode()
	if #lines < 2 then
		for k, v in pairs(lines) do
			s = s .. k .. '=' .. v .. '  '
		end
		event.creature:sendNote(s)
		return
	end
	local name = lines[1]:strip_whitespace()
	local town = map:getTown(lines[2]:strip_whitespace())
	
	if not town then
		return
	end
	
	sendMail(mail, name, town)
end

function otstd.mailbox.registerHandlers()
	for id, data in pairs(otstd.mailboxes) do
		if data.listener ~= nil then
			stopListener(data.listener)
		end
		
		data.listener =
			registerOnMoveItemToItem("after", "itemid", id, otstd.mailbox.handler)
	end
end

otstd.mailbox.registerHandlers()