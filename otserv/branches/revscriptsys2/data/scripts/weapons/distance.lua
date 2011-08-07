local distanceWeapons = {
	-- Bows and Crossbows
	[8849] = {vocation = {"Paladin", "Royal Paladin"}, level = 45, unproperly = true}; -- Modified Crossbow
	[8850] = {vocation = {"Paladin", "Royal Paladin"}, level = 60, unproperly = true}; -- Chain Bolter
	[8853] = {vocation = {"Paladin", "Royal Paladin"}, level = 80, unproperly = true}; -- Ironworker
	[8852] = {vocation = {"Paladin", "Royal Paladin"}, level = 100, unproperly = true}; -- Devileye
	[8851] = {vocation = {"Paladin", "Royal Paladin"}, level = 130, unproperly = true}; -- Royal Crossbow
	[8857] = {vocation = {"Paladin", "Royal Paladin"}, level = 40, unproperly = true}; -- Silkweaver Bow
	[8855] = {vocation = {"Paladin", "Royal Paladin"}, level = 50, unproperly = true}; -- Composite Hornbow
	[8856] = {vocation = {"Paladin", "Royal Paladin"}, level = 60, unproperly = true}; -- Yol's Bow
	[8858] = {vocation = {"Paladin", "Royal Paladin"}, level = 70, unproperly = true}; -- Elethriel's Elemental Bow
	[8854] = {vocation = {"Paladin", "Royal Paladin"}, level = 80, unproperly = true}; -- Warsinger Bow

	[2455] = {}; -- Crossbow
	[2456] = {}; -- Bow
	[7438] = {}; -- Elvish Bow

	-- Ammunition
	[2545] = {damageFormula = function(player, target, weapon)
		--
	end}; -- Poison Arrow
	[2546] = {damageFormula = function(player, target, weapon)
		--
	end}; -- Explosive Arrow
	[7366] = {damageFormula = function(player, target, weapon)
		--
	end}; -- Viper Star

	[7838] = {combatType = COMBAT_ENERGYDAMAGE}; -- Flash Arrow
	[7839] = {combatType = COMBAT_ICEDAMAGE}; -- Shiver Arrow
	[7840] = {combatType = COMBAT_FIREDAMAGE}; -- Flaming Arrow
	[7850] = {combatType = COMBAT_EARTHDAMAGE}; -- Earth Arrow

	[3965] = {level = 20}; -- Hunting Spear
	[7378] = {level = 25}; -- Royal Spear
	[7367] = {level = 42}; -- Enchanted Spear
	[7368] = {level = 80}; -- Assassin Star
	[7364] = {level = 20}; -- Sniper Arrow
	[7365] = {level = 40}; -- Onyx Arrow
	[7363] = {level = 30}; -- Piercing Bolt
	[2547] = {level = 55}; -- Power Bolt
	[6529] = {level = 70}; -- Infernal Bolt

	[2543] = {}; -- Bolt
	[2544] = {}; -- Arrow
}

registerWeapons(distanceWeapons)