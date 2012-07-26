walkTo_example = {}

function walkTo_example.handler(event)
	if event.targetPosition then
		local item = with(map(targetPosition), Tile.getTopItem)
		if item and item:canFish() then
			return
		end
		
		if event.player:walkTo(event.targetPosition) then
			sendAnimatedText(event.player:getPosition(), 214, "Here!")
		else
			sendAnimatedText(event.player:getPosition(), 214, "Abort!")
		end
	end
end

walkTo_example.listener = registerOnUseItem("actionid", 23888, walkTo_example.handler)