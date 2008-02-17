--[[
Damage Class 0.1
Nostradamus
------------------------------------------------------------------------------------------------------------

OBS: 
In spells damage, minB and maxB are used in the damage formula only for a better usage, but Tibia 8.0- don't use it since they have more accurate formula.
This script works interpreting this values as nil

Since 2007 summer update, this formula is inaccurate - now level is also counted in some way.

LEVEL, MAGIC and MIN, MAX are arrays with more then one values that can be:
If 2 values:
	It will count between the given values (second must be higher then the first).
If 4+ (pairs) values:
	It will count between two values as pairs, so if we have 1, 5, 10, 20 it will count 1 to 5, after 10 to 20.

In melee formulas, the damage you will do will be between 0 and the calculated number, please note that shielding and armor from the target will reduce player's damage.
Use the constant values refering to attack types as factors.

------------------------------------------------------------------------------------------------------------
--]]

Damage = {}
Damage.__index = Damage
	
	-- Global consts
	LEVEL = {8, 100}
	MAGIC = {0, 60}
	MIN, MAX = {1, 2}, {1, 2}
	FULL_ATTACK  = 10
	BALANCED 	 = 7
	FULL_DEFENSE = 5

	-- Returns the minA, maxA and averange values for spells formulas
	function Damage:getSpellFormula(level, magLevel, min, max)                                                                   
		minA = max / ((level / 3) + (magLevel / 2)) 
		maxA = min / ((level / 3) + (magLevel / 2))
		avg = math.floor(minA + maxA / 2) 
		return minA, maxA, avg
	end

	-- Returns all the possible combinations between the desired level, magic level, min and max damages
	function Damage:getSpellCombinations(level, magic, min, max)
		for _,x in ipairs(LEVEL) do
			for _,y in ipairs(MAGIC) do
			    for _,z in ipairs(MIN) do
			        for _,w in ipairs(MAX) do
				        minA, maxA, avg = self:getSpellFormula(x, y, z, w)
				        print('Level: ' .. x .. ' | Magic: ' .. y .. ' | Min: ' .. z .. ' | Max: ' .. w .. ' | minA: ' .. minA .. ' | maxA: ' .. maxA .. ' | avg: ' .. avg)
			        end
		        end    
			end
		end
	end
	
	-- Returns the maximum melee damage giving the factor (full attack, balanced or full defense), player level, player skill and the attack of the weapon
	function Damage:getMelee(factor, playerLevel, playerSkill, weaponAttack)
		return (weaponAttack / 20 * playerSkill*2 + weaponAttack + playerLevel / 10) / 10 * factor
	end
		