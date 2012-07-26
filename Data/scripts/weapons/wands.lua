-- Wand damage formula callback
function formulaWand(minDamage, maxDamage)
	return function(player, target, weapon)
		return -math.random(minDamage, maxDamage)
	end
end

local wands = {
	-- Rods
	[2182] = {vocation = {"Druid", "Elder Druid"}, level = 7, mana = 2, combatType = COMBAT_EARTHDAMAGE, damageFormula = formulaWand(8, 18)}; -- Snakebite Rod
	[2186] = {vocation = {"Druid", "Elder Druid"}, level = 13, mana = 3, combatType = COMBAT_ICEDAMAGE, damageFormula = formulaWand(13, 25)}; -- Moonlight Rod
	[2185] = {vocation = {"Druid", "Elder Druid"}, level = 19, mana = 5, combatType = COMBAT_DEATHDAMAGE, damageFormula = formulaWand(27, 33)}; -- Necrotic Rod
	[8911] = {vocation = {"Druid", "Elder Druid"}, level = 22, mana = 5, combatType = COMBAT_ICEDAMAGE, damageFormula = formulaWand(27, 33)}; -- Northwind Rod
	[2181] = {vocation = {"Druid", "Elder Druid"}, level = 26, mana = 8, combatType = COMBAT_EARTHDAMAGE, damageFormula = formulaWand(42, 48)}; -- Terra Rod
	[2183] = {vocation = {"Druid", "Elder Druid"}, level = 33, mana = 13, combatType = COMBAT_ICEDAMAGE, damageFormula = formulaWand(55, 75)}; -- Hailstorm Rod
	[8912] = {vocation = {"Druid", "Elder Druid"}, level = 37, mana = 13, combatType = COMBAT_EARTHDAMAGE, damageFormula = formulaWand(55, 75)}; -- Springsprout Rod
	[8910] = {vocation = {"Druid", "Elder Druid"}, level = 42, mana = 13, combatType = COMBAT_DEATHDAMAGE, damageFormula = formulaWand(55, 75)}; -- Underworld Rod

	-- Wands
	[2190] = {vocation = {"Sorcerer", "Master Sorcerer"}, level = 7, mana = 2, combatType = COMBAT_ENERGYDAMAGE, damageFormula = formulaWand(8, 18)}; -- Wand of Vortex
	[2191] = {vocation = {"Sorcerer", "Master Sorcerer"}, level = 13, mana = 3, combatType = COMBAT_FIREDAMAGE, damageFormula = formulaWand(13, 25)}; -- Wand of Dragonbreath
	[2188] = {vocation = {"Sorcerer", "Master Sorcerer"}, level = 19, mana = 5, combatType = COMBAT_DEATHDAMAGE, damageFormula = formulaWand(27, 33)}; -- Wand of Decay
	[8921] = {vocation = {"Sorcerer", "Master Sorcerer"}, level = 22, mana = 5, combatType = COMBAT_FIREDAMAGE, damageFormula = formulaWand(27, 33)}; -- Wand of Draconia
	[2189] = {vocation = {"Sorcerer", "Master Sorcerer"}, level = 26, mana = 8, combatType = COMBAT_ENERGYDAMAGE, damageFormula = formulaWand(42, 48)}; -- Wand of Cosmic Energy
	[2187] = {vocation = {"Sorcerer", "Master Sorcerer"}, level = 33, mana = 13, combatType = COMBAT_FIREDAMAGE, damageFormula = formulaWand(55, 75)}; -- Wand of Inferno
	[8920] = {vocation = {"Sorcerer", "Master Sorcerer"}, level = 37, mana = 13, combatType = COMBAT_ENERGYDAMAGE, damageFormula = formulaWand(55, 75)}; -- Wand of Starstorm
	[8922] = {vocation = {"Sorcerer", "Master Sorcerer"}, level = 42, mana = 13, combatType = COMBAT_DEATHDAMAGE, damageFormula = formulaWand(55, 75)}; -- Wand of Voodoo
}

registerWeapons(wands)