local food =
	{
		2666, -- Meat
		2671, -- Ham
		2674, -- Apple
		2681, -- Grapes
		2689, -- Bread
		2690, -- Roll
		2696 -- Cheese
	}

local conjureFood = Spell:new("Conjure Food")

conjureFood.words         = "exevo pan"
conjureFood.vocation      = {"Druid", "Elder Druid", "Paladin", "Royal Paladin"}
conjureFood.level         = 14
conjureFood.mana          = 120
conjureFood.soul          = 1

conjureFood.effect        = MAGIC_EFFECT_GREEN_SHIMMER


function conjureFood.onFinishCast(event)
	local r1 = math.random(#food + 1)
	if r1 == (#food + 1) then -- 1/8 chance at having two items summoned, unknown what real chance is
		local r2 = math.random(#food)
		r1 = math.random(#food)
		if r1 == r2 then
			event.caster:addItem(createItem(food[r1], 2))
		else
			event.caster:addItem(createItem(food[r1], 1))
			event.caster:addItem(createItem(food[r2], 1))
		end
	else
		event.caster:addItem(createItem(food[r1], 1))
	end
	return true
end

conjureFood:register()
