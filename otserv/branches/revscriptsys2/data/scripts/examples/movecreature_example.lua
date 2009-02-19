local movecreature_example = {}

local count_steps = {}

function movecreature_example.stepIn_callback(event)
	local c = count_steps[event.creature:getID()]
	if c and c.steps > 0 then
		c.steps = 0
		return
	end
	
	count_steps[event.creature:getID()] = 
		{
			steps = 0, 
			listener = registerOnMoveCreature(event.creature, movecreature_example.move_callback)
		}
end

movecreature_example.stepInCreature_listener = registerOnStepInCreature("itemid", 420, movecreature_example.stepIn_callback)

function movecreature_example.move_callback(event)
	local c = count_steps[event.creature:getID()]
	if c then
		c.steps = c.steps + 1
		if c.steps > 100 then
			stopListener(c.listener)
			count_steps[event.creature:getID()] = nil
			return
		end
	end
	
	wait(200)
	local field = createItem(1488)
	event.toTile:addItem(field)
	field:startDecaying()
end

