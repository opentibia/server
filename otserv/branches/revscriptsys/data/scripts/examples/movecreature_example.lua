movecreature_example = {}

function movecreature_example.stepIn_callback(event)
	event.creature:say("Stepped into tile")
end

movecreature_example.stepInCreature_listener = registerOnStepInCreature("itemid", 420, movecreature_example.stepIn_callback)

