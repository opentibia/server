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

function TestCommand.registerHandlers()
	if TestCommand.set_listener then
		stopListener(TestCommand.set_listener)
	end
	TestCommand.set_listener =
		registerOnSay("beginning", false, "/set", TestCommand.StorageTestSetHandler)

	if TestCommand.get_listener then
		stopListener(TestCommand.get_listener)
	end
	TestCommand.get_listener =
		registerOnSay("beginning", false, "/get", TestCommand.StorageTestGetHandler)

	if TestCommand.tie_listener then
		stopListener(TestCommand.tie_listener)
	end
	TestCommand.tie_listener =
		registerOnSay("beginning", false, "/tie",   TestCommand.TestSpecific)

	if TestCommand.untie_listener then
		stopListener(TestCommand.untie_listener)
	end
	TestCommand.untie_listener =
		registerOnSay("beginning", false, "/untie", TestCommand.TestSpecificStop)
end

TestCommand.registerHandlers()
