--[[
Tibia Calculator Class 0.1.1
Nostradamus

OBS: This is just a helpful library. If you feel that you need these functions, add the following line to global.lua:
dofile(getDataDir() .. "game_classes/calculator.lua")
--]]

-- Global consts
MAGES = 1
PALADINS = 2
KNIGHTS = 3
ROOKERS = 4

Calculator = {}
Calculator.__index = Calculator

	-- Returns the experience of the desired level
	function Calculator:getLevelExp(level)
		return (50 / 3) * level ^ 3 - (100 * level ^ 2) + ((850 / 3) * level) - 200
	end

	-- Returns the experience left between a current level and a desired level
	function Calculator:getExpLeft(currentLevel, desiredLevel)
		return self:getLevelExp(desiredLevel) - self:getLevelExp(currentLevel)
	end
	
	-- Returns the exp gain percent into the exp left to up a level
	function Calculator:getExpGainPercent(creatureExp, currentLevel)
		return (100 * creatureExp) / (self:getExpLeft(currentLevel, currentLevel + 1))
	end

	-- Returns how much creatures the player have to kill to get the desired level beeing in the current level
	function Calculator:getCreatureLeft(currentLevel, desiredLevel, creatureExp)
		return (self:getExpLeft(currentLevel, desiredLevel) / getConfigInfo(rateExp)) * (1 / creatureExp)
	end
	
	-- Returns how much the creature's exp represents in percent in the current level exp of a player
	function Calculator:getCreatureExpPercent(level, creatureExp)
		local levelExp = self:getLevelExp(level)
		local totalExp = levelExp + creatureExp
		return ((totalExp - levelExp) / totalExp) * 100
	end
	
	-- Returns how much mana points the player have to spend to advance to the next magic level
	function Calculator:getMPLeft(currentML, vocation)
		local multipliers = {1.1, 1.4, 3}
		return 1600 * multipliers[vocation] ^ currentML
	end
	
	-- Returns the cap of a player
	function Calculator:getCap(level, vocation)
		local capGain = {10, 20, 25, 10}
		return (level - 8) * capGain[vocation] + 470
	end
	
	-- Returns the free cap percent of a player
	function Calculator:getCapStats(cid, level, vocation)
		return (getPlayerFreeCap(cid) / self:getCap(level, vocation)) * 100
	end

	-- Returns the experience gained by the killer in enforced servers
	function Calculator:getEnforcedExpGain(killerLevel, victimLevel)
		return (1 - (math.floor(killerLevel * 0.9) / victimLevel)) * 0.05 * self:getLevelExp(victimLevel)
	end

	-- Returns how much worms a player can buy with a desired amount of money
	function Calculator:getWormsAmount(money)
		return money * 1.2
	end
