function registerWeapons(weapons)
	local weapon = nil
	for weaponID, info in pairs(weapons) do
		-- create the weapon
		weapon = Weapon:new(weaponID)

		-- set infos

		-- default params
		if info.vocation ~= nil then
			weapon.vocation = info.vocation
		end

		if info.level ~= nil then
			weapon.level = info.level
		end

		if info.magicLevel ~= nil then
			weapon.magicLevel = info.magicLevel
		end

		if info.mana ~= nil then
			weapon.mana = info.mana
		end

		if info.manaPercent ~= nil then
			weapon.manaPercent = info.manaPercent
		end

		if info.soul ~= nil then
			weapon.soul = info.soul
		end

		if info.exhaustion ~= nil then
			weapon.exhaustion = info.exhaustion
		end

		if info.premium ~= nil then
			weapon.premium = info.premium
		end

		if info.unproperly ~= nil then
			weapon.unproperly = info.unproperly
		end

		-- combat params
		if info.combatType ~= nil then
			weapon.combatType = info.combatType
		end

		if info.blockedByDefense ~= nil then
			weapon.blockedByDefense = info.blockedByDefense
		end

		if info.blockedByArmor ~= nil then
			weapon.blockedByArmor = info.blockedByArmor
		end

		-- damage formula
		if info.damageFormula ~= nil then
			weapon.damageFormula = info.damageFormula
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
