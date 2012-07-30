function registerWeapons(weapons)
	local weapon = nil
	for weaponID, info in pairs(weapons) do
		-- create the weapon
		weapon = Weapon:new(weaponID)
		-- set params
		for k, v in pairs(info) do
			weapon[k] = v
		end
		-- register weapon
		weapon:register()
	end
end

require("weapons/swords")
require("weapons/axes")
require("weapons/clubs")
require("weapons/distance")
require("weapons/wands")

