
local DebugMemory = Command:new("DebugMemory")

DebugMemory.words = "/mem"
DebugMemory.groups = {"Server Administrator"}

local track = {}
setmetatable(track, {__mode = "v"})

function DebugMemory.handler(event)
	table.insert(track, coroutine.running())
	table.insert(track, event)
	table.insert(track, {})
	
	s = ''
	for k, v in pairs(track) do
		s = s .. k .. '=' .. tostring(v)
		if type(v) == 'thread' then
			s = s .. "(" .. coroutine.status(v) .. ")"
		end
		s = s .. "\n"
	end
	collectgarbage("collect")
	event.creature:sendNote("Thread is " .. tostring(coroutine.running()) .. "\nMemory use is " .. collectgarbage("count") .. ".\n" .. s)
end

DebugMemory:register()
