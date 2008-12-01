movecreature_example = {}

function movecreature_example.stepIn_callback(event)
	registerOnMoveCreature(event.creature, movecreature_example.move_callback)
end

movecreature_example.stepInCreature_listener = registerOnStepInCreature("itemid", 420, movecreature_example.stepIn_callback)

function movecreature_example.move_callback(event)
	local field = createItem(1488)
	event.toTile:addItem(field)
end

