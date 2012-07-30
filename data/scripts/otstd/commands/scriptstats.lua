
local ScriptStatistics = Command:new("ScriptStatistics")

ScriptStatistics.words = "/stats"
ScriptStatistics.groups = {"Server Administrator"}

function ScriptStatistics.handler(event)
	local stats = scriptStatistics()
	local t = "Script Statistics:\n"
	
	for k, v in pairs(stats) do
		t = t .. k .. ": " .. v .. "\n"
	end
	
	event.player:sendNote(t)
end

ScriptStatistics:register()
