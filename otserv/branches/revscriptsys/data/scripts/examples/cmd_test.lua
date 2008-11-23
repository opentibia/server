otstd.Commands.TestCommand = {}

function otstd.Commands.TestCommand.GoofHandler(event)
	event.text = string.gsub(event.text, "remere", "Remere The Great")
end

function otstd.Commands.TestCommand.StorageTestSetHandler(event)
	local param = event.text:sub(5)
	if #param > 0 then
		local eq = param:find("=")
		if eq then
			local key = param:sub(1, eq-1)
			local value = param:sub(eq+1)
			event.player:setStorageValue(key, value)
		end
	end
end

function otstd.Commands.TestCommand.StorageTestGetHandler(event)
	local key = event.text:sub(5)
	if #key > 0 then
		event.text = event.player:getStorageValue(key) or "nothing"
	end
end

goof_listener = ""

function otstd.Commands.TestCommand.TestSpecific(event)
	goof_listener = registerOnCreatureSay(event.player, "substring", false, "remere", otstd.Commands.TestCommand.GoofHandler)
end

function otstd.Commands.TestCommand.TestSpecificStop(event)
	stopListener(goof_listener);
end

--registerGenericOnSayListener("substring", false, "remere", otstd.Commands.TestCommand.GoofHandler)
registerOnSay("beginning", false, "/set", otstd.Commands.TestCommand.StorageTestSetHandler)
registerOnSay("beginning", false, "/get", otstd.Commands.TestCommand.StorageTestGetHandler)

registerOnSay("beginning", false, "/tie",   otstd.Commands.TestCommand.TestSpecific)
registerOnSay("beginning", false, "/untie", otstd.Commands.TestCommand.TestSpecificStop)
