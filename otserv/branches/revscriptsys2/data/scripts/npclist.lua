
local names = {
	"Michael",
	}

	
	
	
	
-- Load the files
for _, n in ipairs(names) do
	require("npcs/" .. n)
end

