local TestCommand = {}

function TestCommand.GoofHandler(event)
	event.text = string.gsub(event.text, "remere", "Remere The Great")
end

function TestCommand.StorageTestSetHandler(event)
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

function TestCommand.StorageTestGetHandler(event)
	local key = event.text:sub(5)
	if #key > 0 then
		event.text = event.player:getStorageValue(key) or "nothing"
	end
end

goof_listener = ""

function TestCommand.TestSpecific(event)
	goof_listener = registerOnCreatureSay(event.player, "substring", false, "remere", TestCommand.GoofHandler)
end

function TestCommand.TestSpecificStop(event)
	stopListener(goof_listener);
end

registerOnSay("beginning", false, "/set", TestCommand.StorageTestSetHandler)
registerOnSay("beginning", false, "/get", TestCommand.StorageTestGetHandler)

registerOnSay("beginning", false, "/tie",   TestCommand.TestSpecific)
registerOnSay("beginning", false, "/untie", TestCommand.TestSpecificStop)
