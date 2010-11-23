-- Wand Formula
local function formulaWand(minDamage, maxDamage)
	return function(player, target, weapon)
		return -math.random(minDamage, maxDamage)
	end
end

-- Weapon list
local defaultWeapons = {
	[2400] = {level = 80, unproperly = true}; -- Magic Sword
	[6529] = {level = 70}; -- Infernal Bolt
}

function registerDefaultWeapons()
	for weaponID, info in pairs(defaultWeapons) do
		-- create the weapon
		local weapon = Weapon:new(weaponID)

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
		if info.damageType ~= nil then
			weapon.damageType = info.damageType
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

registerDefaultWeapons()
