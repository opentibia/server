
otstd.GM_XRay_Vision = {}

otstd.GM_XRay_Vision.groups = {"Gamemaster", "Senior Gamemaster", "Community Manager", "Server Administrator"}

function otstd.GM_XRay_Vision.look_handler(evt)
	local desc = evt.description
	local obj = evt.object
	local pos = obj:getPosition()
	
	desc = desc .. "\n"
	desc = desc .. "x:" .. pos.x .. " y:" .. pos.y .. " z:" .. pos.z
	if typeof(obj, "Creature") then
		desc = desc .. "\nHealth:" .. obj:getHealth() .. "/" .. obj:getHealthMax()
	elseif typeof(obj, "Item") then
		desc = desc .. " ItemID:" .. obj:getItemID()
		if obj:getActionID() ~= 0 then
			desc = desc .. " ActionID:" .. obj:getActionID()
		end
		if obj:getUniqueID() ~= 0 then
			desc = desc .. " UniqueID:" .. obj:getUniqueID()
		end
	end
	evt.description = desc
end

function otstd.GM_XRay_Vision.login_handler(evt)
	local player = evt.player
	if (type(otstd.GM_XRay_Vision.groups) == "string" and
		otstd.GM_XRay_Vision.groups == "All") or
		table.find(otstd.GM_XRay_Vision.groups, player:getAccessGroup())
	then
		registerOnPlayerLookAt(player, otstd.GM_XRay_Vision.look_handler)
	end
end

function otstd.GM_XRay_Vision.load_handler(evt)
	if evt.reload then
		for _, player in ipairs(getOnlinePlayers()) do
			if (type(otstd.GM_XRay_Vision.groups) == "string" and
				otstd.GM_XRay_Vision.groups == "All") or
				table.find(otstd.GM_XRay_Vision.groups, player:getAccessGroup())
			then
				registerOnPlayerLookAt(player, otstd.GM_XRay_Vision.look_handler)
			end
		end
	end
end

function otstd.GM_XRay_Vision:register()
	if self.login_listener then
		stopListener(self.login_listener)
	end
	self.login_listener = 
		registerOnLogin(self.login_handler)
	
	registerOnServerLoad(otstd.GM_XRay_Vision.load_handler)
end

otstd.GM_XRay_Vision:register()
