
otstd.GM_XRay_Vision = {}

otstd.GM_XRay_Vision.groups = {"GM"}

function otstd.GM_XRay_Vision.look_handler(evt)
	local desc = evt.description
	local obj = evt.object
	local pos = obj:getPosition()
	
	desc = desc .. " ("
	desc = desc .. "x:" .. pos.x .. " y:" .. pos.y .. " z:" .. pos.z
	if typeof(obj, "Creature") then
		desc = desc .. " health:" .. obj:getHealth() .. "/" .. obj:getHealthMax()
	elseif typeof(obj, "Item") then
		desc = desc .. " itemid:" .. obj:getItemID()
		if obj:getActionID() ~= 0 then
			desc = desc .. " actionid:" .. obj:getActionID()
		end
		if obj:getUniqueID() ~= 0 then
			desc = desc .. " uniqueid:" .. obj:getUniqueID()
		end
	end
	desc = desc .. ")"
	evt.description = desc
end

function otstd.GM_XRay_Vision.login_handler(evt)
	local player = evt.who
	if (type(otstd.GM_XRay_Vision.groups) == "string" and
			otstd.GM_XRay_Vision.groups == "All") or
			table.contains(otstd.GM_XRay_Vision.groups, player:getAccessGroup())
			then
		registerCreatureOnLookAt(player, otstd.GM_XRay_Vision.look_handler)
	end
end

function otstd.GM_XRay_Vision:register()
	if self.login_listener then
		stopListener(self.login_listener)
	end
	self.login_listener = 
		registerOnLogin(self.login_handler)
end

otstd.GM_XRay_Vision:register()