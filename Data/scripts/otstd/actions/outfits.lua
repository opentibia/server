otstd.outfit = {}
otstd.Player.outfits = {} -- The player outfit maps are stored here, for faster access
otstd.outfits = {}

function registerOutfit(outfit)
	local function checkName(name)
		if tonumber(name) ~= nil then
			error("Outfit name cannot be a number ('" .. name .. "')")
		end
		if otstd.outfits[name] ~= nil then
			error("Cannot replace already existing outfit or reserved keyword '" .. name .. "'")
		end
		
		return name
	end
	
	local name = checkName(outfit.name) or
			checkName(outfit.femalename) or
			checkName(outfit.malename)
	
	if not name then
		error("Outfit must have a name to be valid!")
	end
	
	table.append(otstd.outfits, outfit)
	if outfit.name then
		otstd.outfits[outfit.name] = outfit
	else
		outfit.name = outfit.femalename or outfit.malename
	end
	if outfit.femalename then
		otstd.outfits[outfit.femalename] = outfit
	end
	if outfit.malename then
		otstd.outfits[outfit.malename] = outfit
	end
end

-- Add standardaddons = X to make addons default for an outfit
registerOutfit{name="Citizen",      female=136, male=128, addons=3, standard=true}
registerOutfit{name="Hunter",       female=137, male=129, addons=3, standard=true}
registerOutfit{name="Mage",         female=138, male=130, addons=3, standard=true}
registerOutfit{name="Knight",       female=139, male=131, addons=3, standard=true}
registerOutfit{femalename="Noblewoman", malename="Nobleman", female=137, male=132, addons=3, premium=true, standard=true}
registerOutfit{name="Summoner",     female=141, male=133, addons=3, premium=true, standard=true}
registerOutfit{name="Warrior",      female=142, male=134, addons=3, premium=true, standard=true}
registerOutfit{name="Barbarian",    female=147, male=143, addons=3, premium=true, standard=true}
registerOutfit{name="Druid",        female=148, male=144, addons=3, premium=true, standard=true}
registerOutfit{name="Wizard",       female=149, male=145, addons=3, premium=true, standard=true}
registerOutfit{name="Oriental",     female=150, male=146, addons=3, premium=true, standard=true}
registerOutfit{name="Pirate",       female=155, male=151, addons=3, premium=true}

function Player:canWearOutfit(outfit)
	if outfit.premium and not self:isPremium() then
		return false
	end
	
	if outfit.condition and not outfit.condition(self) then
		return false
	end
	
	-- Do we have this outfit in our standard repetaure?
	local o = otstd.Player.outfits[#self][outfit.name]
	
	if not outfit.standard and o then
		return true
	end
	
	return outfit.standard
end

function Player:getWearableAddons(outfit)
	local o = otstd.Player.outfits[#self][outfit.name]
	if o then
		return bit.band(bit.bor(o.addons, outfit.standardaddons), outfit.addons)
	end
	return outfit.standardaddons
end

function Player:loadOutfits()
	-- Load from storage map
	otstd.Player.outfits[#self] = self:getStorageValue("__outfits") or {}
end

function Player:saveOutfits()
	-- Save to storage map
	otstd.Player.outfits[#self] = self:setStorageValue("__outfits", self.outfits)
end


function otstd.outfit.onChangeCallback(event)
	local player = event.player
	
	for _, outfit in pairs(otstd.outfits) do
		if player:canWearOutfit(outfit) then
			local o = {}
			
			if not player:isPremium() and outfit.premium then
				break
			end
			
			if player:isFemale() then
				o.name = outfit.femalename or outfit.name
				o.type = outfit.female
			else
				o.name = outfit.malename or outfit.name
				o.type = outfit.male
			end
			o.addons = player:getWearableAddons(outfit)
			
			table.append(event.outfits, o)
		end
	end
end

function otstd.outfit.registerHandlers()
	with(otstd.outfit.onChangeListener, stopListener)
	
	otstd.outfit.onChangeListener =
		registerOnChangeOutfit(otstd.outfit.onChangeCallback)
end

otstd.outfit.registerHandlers()

