otstd.food = {}

otstd.food_list = {
		[2328] = {amount = 84, text = "Gulp."},
		[2362] = {amount = 48, text = "Yum."},
		[2666] = {amount = 180, text = "Munch."},
		[2667] = {amount = 144, text = "Munch."},
		[2668] = {amount = 120, text = "Mmmm."},
		[2669] = {amount = 204, text = "Munch."},
		[2670] = {amount = 48, text = "Gulp."},
		[2671] = {amount = 360, text = "Chomp."},
		[2672] = {amount = 720, text = "Chomp."},
		[2673] = {amount = 60, text = "Yum."},
		[2674] = {amount = 72, text = "Yum."},
		[2675] = {amount = 156, text = "Yum."},
		[2676] = {amount = 96, text = "Yum."},
		[2677] = {amount = 12, text = "Yum."},
		[2678] = {amount = 216, text = "Slurp."},
		[2679] = {amount = 12, text = "Yum."},
		[2680] = {amount = 24, text = "Yum."},
		[2681] = {amount = 108, text = "Yum."},
		[2682] = {amount = 240, text = "Yum."},
		[2683] = {amount = 204, text = "Munch."},
		[2684] = {amount = 60, text = "Crunch."},
		[2685] = {amount = 72, text = "Munch."},
		[2686] = {amount = 108, text = "Crunch."},
		[2687] = {amount = 24, text = "Crunch."},
		[2688] = {amount = 24, text = "Mmmm."},
		[2689] = {amount = 120, text = "Crunch."},
		[2690] = {amount = 72, text = "Crunch."},
		[2691] = {amount = 96, text = "Crunch."},
		[2695] = {amount = 72, text = "Gulp."},
		[2696] = {amount = 108, text = "Smack."},
		[2769] = {amount = 60, text = "Crunch."},
		[2787] = {amount = 108, text = "Crunch."},
		[2788] = {amount = 48, text = "Crunch."},
		[2789] = {amount = 264, text = "Crunch."},
		[2790] = {amount = 360, text = "Crunch."},
		[2791] = {amount = 108, text = "Crunch."},
		[2792] = {amount = 72, text = "Crunch."},
		[2793] = {amount = 144, text = "Crunch."},
		[2794] = {amount = 36, text = "Crunch."},
		[2795] = {amount = 432, text = "Crunch."},
		[2796] = {amount = 300, text = "Crunch."},
		[5097] = {amount = 48, text = "Yum."},
		[5678] = {amount = 96, text = "Gulp."},
		[6125] = {amount = 96, text = "Mmmm."},
		[6278] = {amount = 120, text = "Mmmm."},
		[6279] = {amount = 180, text = "Mmmm."},
		[6393] = {amount = 144, text = "Mmmm."},
		[6394] = {amount = 180, text = "Mmmm."},
		[6501] = {amount = 240, text = "Mmmm."},
		[6541] = {amount = 72, text = "Gulp."},
		[6542] = {amount = 72, text = "Gulp."},
		[6543] = {amount = 72, text = "Gulp."},
		[6544] = {amount = 72, text = "Gulp."},
		[6545] = {amount = 72, text = "Gulp."},
		[6569] = {amount = 12, text = "Mmmm."},
		[6574] = {amount = 60, text = "Mmmm."},
		[7158] = {amount = 300, text = "Munch."},
		[7159] = {amount = 180, text = "Munch."},
		[7372] = {amount = 0, text = "Yummy."},
		[7373] = {amount = 0, text = "Yummy."},
		[7374] = {amount = 0, text = "Yummy."},
		[7375] = {amount = 0, text = "Yummy."},
		[7376] = {amount = 0, text = "Yummy."},
		[7377] = {amount = 0, text = "Yummy."},
		[7963] = {amount = 720, text = "Munch."},
		[8838] = {amount = 120, text = "Gulp."},
		[8839] = {amount = 60, text = "Yum."},
		[8840] = {amount = 12, text = "Yum."},
		[8841] = {amount = 12, text = "Urgh."},
		[8842] = {amount = 84, text = "Munch."},
		[8843] = {amount = 60, text = "Crunch."},
		[8844] = {amount = 12, text = "Gulp."},
		[8868] = {amount = 108, text = "Smack."},
		[9005] = {amount = 88, text = "Slurp."},
		[9996] = {amount = 0, text = "Slurp."},
		[10454] = {amount = 0, text = "Your head begins to feel better."}

		--[[
		--special food
		TODO:
		[9992] = {text = "Gulp."},
		[9993] = {text = "Chomp."},
		[9994] = {text = "Chomp."},
		[9995] = {text = "Chomp."},
		[9997] = {text = "Yum."},
		[9998] = {text = "Munch."},
		[9999] = {text = "Chomp."},
		[10000] = {text = "Mmmm."},
		[10001] = {text = "Smack."}
		]]--
	}

function otstd.food.handler(event)
	local player = event.player
	local item = event.item
	
	local amount = event.food.amount or 0
	if player:getFood() + amount > 1200 then
		player:sendInfo("You are full.")
		return
	end
	
	--player:addFood(v.amount)
	sendAnimatedText(player:getPosition(), TEXTCOLOR_ORANGE:value(), event.food.text)
	if item:getCount() > 1 then
		item:setCount(item:getCount() - 1)
	else
		item:destroy()
	end
	
	event:skip()
end

function otstd.food.registerHandlers()
	for id, data in pairs(otstd.food_list) do
		if data.listener then
			stopListener(data.listener)
		end
		
		function lamba_callback(event)
			event.food = data
			otstd.food.handler(event)
		end
		data.listener =
			registerOnUseItem("itemid", id, lamba_callback)
	end
end

otstd.food.registerHandlers()
