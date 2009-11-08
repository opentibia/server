otstd.enchant = {}

otstd.enchant.enchanted_gems = {
		[7759] = {},
		[7760] = {},
		[7761] = {},
		[7762] = {}
	}

local ENCHANT_ICE = 7759
local ENCHANT_FIRE = 7760
local ENCHANT_EARTH = 7761
local ENCHANT_ENERGY = 7762
	
otstd.enchant.shrines = {
		[7508] = {ENCHANT_ICE}, [7509] = {ENCHANT_ICE}, [7510] = {ENCHANT_ICE}, [7511] = {ENCHANT_ICE},		
		[7504] = {ENCHANT_FIRE}, [7505] = {ENCHANT_FIRE}, [7506] = {ENCHANT_FIRE}, [7507] = {ENCHANT_FIRE},		
		[7516] = {ENCHANT_EARTH}, [7517] = {ENCHANT_EARTH}, [7518] = {ENCHANT_EARTH}, [7519] = {ENCHANT_EARTH},
		[7512] = {ENCHANT_ENERGY}, [7513] = {ENCHANT_ENERGY}, [7514] = {ENCHANT_ENERGY}, [7515] = {ENCHANT_ENERGY}
	}
	

otstd.enchant.enchantable_gems = {
		[2146] = {newid = ENCHANT_ICE},
		[2147] = {newid = ENCHANT_FIRE},
		[2149] = {newid = ENCHANT_EARTH},
		[2150] = {newid = ENCHANT_ENERGY}
	}
	
otstd.enchant.disenchant_tools = {
		[5942] = {}
	}

otstd.enchant.disenchantable_items = {
		[2956] = {newid = 2957, dust = 5905, chance = 8},
		[2916] = {newid = 2917, dust = 5906, chance = 8}
	}
	
otstd.enchant.enchantable_weapons = {
		[2383] = {enchants = {[ENCHANT_ICE] = 7763, [ENCHANT_FIRE] = 7744, [ENCHANT_EARTH] = 7854, [ENCHANT_ENERGY] = 7869}, charges = 1000},
		[2391] = {enchants = {[ENCHANT_ICE] = 7777, [ENCHANT_FIRE] = 7758, [ENCHANT_EARTH] = 7868, [ENCHANT_ENERGY] = 7883}, charges = 1000},
		[2423] = {enchants = {[ENCHANT_ICE] = 7773, [ENCHANT_FIRE] = 7754, [ENCHANT_EARTH] = 7864, [ENCHANT_ENERGY] = 7879}, charges = 1000},
		[2429] = {enchants = {[ENCHANT_ICE] = 7768, [ENCHANT_FIRE] = 7749, [ENCHANT_EARTH] = 7859, [ENCHANT_ENERGY] = 7874}, charges = 1000},
		[2430] = {enchants = {[ENCHANT_ICE] = 7769, [ENCHANT_FIRE] = 7750, [ENCHANT_EARTH] = 7860, [ENCHANT_ENERGY] = 7875}, charges = 1000},
		[2445] = {enchants = {[ENCHANT_ICE] = 7774, [ENCHANT_FIRE] = 7755, [ENCHANT_EARTH] = 7865, [ENCHANT_ENERGY] = 7880}, charges = 1000},
		[2454] = {enchants = {[ENCHANT_ICE] = 7772, [ENCHANT_FIRE] = 7753, [ENCHANT_EARTH] = 7863, [ENCHANT_ENERGY] = 7878}, charges = 1000},
		[7383] = {enchants = {[ENCHANT_ICE] = 7764, [ENCHANT_FIRE] = 7745, [ENCHANT_EARTH] = 7855, [ENCHANT_ENERGY] = 7870}, charges = 1000},
		[7384] = {enchants = {[ENCHANT_ICE] = 7765, [ENCHANT_FIRE] = 7746, [ENCHANT_EARTH] = 7856, [ENCHANT_ENERGY] = 7871}, charges = 1000},
		[7380] = {enchants = {[ENCHANT_ICE] = 7771, [ENCHANT_FIRE] = 7752, [ENCHANT_EARTH] = 7862, [ENCHANT_ENERGY] = 7877}, charges = 1000},
		[7389] = {enchants = {[ENCHANT_ICE] = 7770, [ENCHANT_FIRE] = 7751, [ENCHANT_EARTH] = 7861, [ENCHANT_ENERGY] = 7876}, charges = 1000},
		[7392] = {enchants = {[ENCHANT_ICE] = 7776, [ENCHANT_FIRE] = 7757, [ENCHANT_EARTH] = 7867, [ENCHANT_ENERGY] = 7882}, charges = 1000},
		[7402] = {enchants = {[ENCHANT_ICE] = 7767, [ENCHANT_FIRE] = 7748, [ENCHANT_EARTH] = 7858, [ENCHANT_ENERGY] = 7873}, charges = 1000},
		[7406] = {enchants = {[ENCHANT_ICE] = 7766, [ENCHANT_FIRE] = 7747, [ENCHANT_EARTH] = 7857, [ENCHANT_ENERGY] = 7872}, charges = 1000},
		[7415] = {enchants = {[ENCHANT_ICE] = 7775, [ENCHANT_FIRE] = 7756, [ENCHANT_EARTH] = 7866, [ENCHANT_ENERGY] = 7881}, charges = 1000},
		[8905] = {enchants = {[ENCHANT_ICE] = 8907, [ENCHANT_FIRE] = 8906, [ENCHANT_EARTH] = 8909, [ENCHANT_ENERGY] = 8908}},

		[9934] = {enchants = {[ENCHANT_ICE] = nil, [ENCHANT_FIRE] = nil, [ENCHANT_EARTH] = 9933, [ENCHANT_ENERGY] = nil}},
		[9949] = {enchants = {[ENCHANT_ICE] = nil, [ENCHANT_FIRE] = 9933, [ENCHANT_EARTH] = nil, [ENCHANT_ENERGY] = nil}},
		[9954] = {enchants = {[ENCHANT_ICE] = nil, [ENCHANT_FIRE] = nil, [ENCHANT_EARTH] = 9953, [ENCHANT_ENERGY] = nil}},
		[10022] = {enchants = {[ENCHANT_ICE] = nil, [ENCHANT_FIRE] = 9933, [ENCHANT_EARTH] = nil, [ENCHANT_ENERGY] = nil}}
	}
	
function otstd.enchant.weapon_callback(event)
	local player = event.player
	local enchantedItem = event.item
	local toPos = event.targetPosition
	local tile = toPos and map:getTile(toPos)
	
	local weaponItem = event.targetInventoryItem or tile:getTopThing()
	if(weaponItem) then
		local weapondata = otstd.enchant.enchantable_weapons[weaponItem:getItemID()];
		if weapondata then
			if weapondata.handler then
				event.weapon = weaponItem
				weapondata.handler(event)
			else
				local charges = weapon.charges or -1
				local newid = weapondata.enchants[enchantedItem:getItemID()]
				if not newid then
					return
				end

				weaponItem:setItemID(newid, charges)
				enchantedItem:destroy()

				event.retcode = RETURNVALUE_NOERROR
				event:skip()
			end
		end

	end
end

function Player:enchantGem(gem, enchantItemId)
	local count = gem:getCount()
	local manaCost = 300 * count
	local soulCost = 2 * count
	local requiredLevel = 30

	if self:getLevel() < requiredLevel then
		self:SendCancel(RET_NOTENOUGHLEVEL)
		return false
	end

	if not self:isPremium() then
		self:sendCancel(RET_YOUNEEDPREMIUMACCOUNT)
		return false
	end

	if self:getMana() < manaCost then
		self:SendCancel(RET_NOTENOUGHMANA)
		return false
	end

	if self:getSoulPoints() < soulCost then
		self:SendCancel(RET_NOTENOUGHSOUL)
		return false
	end
	
	self:setMana(self:getMana() - manaCost)
	self:setSoulPoints(self:getSoulPoints() - soulCost)

	gem:setItemID(enchantItemId)
	return true
end

function otstd.enchant.gem_handler(event)
	local player = event.player
	local item = event.item
	
	local shrineItem = tile:getTopThing()
	if(shrineItem) then			
		local shrinedata = otstd.enchant.shrines[shrineItem:getItemID()];
		if shrinedata then
			if shrinedata.handler then
				event.shrine = shrineItem
				shrinedata.handler(event)
			else
				player:enchantGem(item, event.newid)
				event.retcode = RETURNVALUE_NOERROR
				event:skip()
			end
		end

	end
end

function otstd.enchant.disenchant_handler(event)
	local player = event.player
	local disenchant_tool = event.item
	local toPos = event.targetPosition
	local tile = toPos and map:getTile(toPos)
	
	local disenchantItem = event.targetInventoryItem or tile:getTopThing()
	if(disenchantItem) then			
		local tooldata = otstd.enchant.disenchantable_items[disenchantItem:getItemID()];
		if(tooldata) then
			if tooldata.handler then
				event.newid = tooldata.newid
				tooldata.handler(event)
			else
				local chance = tooldata.chance
				local newid = tooldata.newid
				
				if chance >= math.random(1, 100) then
					local dustid = tooldata.dust
					local dust = createItem(dustid)
					player:addItem(dust)
					sendMagicEffect(disenchantItem:getPosition(), MAGIC_EFFECT_STUN)
				else
					sendMagicEffect(disenchantItem:getPosition(), MAGIC_EFFECT_YELLOW_SPARK)
				end

				disenchantItem:setItemID(newid)
					
				event.retcode = RETURNVALUE_NOERROR
				event:skip()
			end
		end

	end
end

function otstd.enchant.registerHandlers()
	for id, data in pairs(otstd.enchant.enchantable_gems) do
		if data.listener ~= nil then
			stopListener(data.listener)
		end

		local function lamba_callback(event)
			event.newid = data.newid
			otstd.enchant.gem_handler(event)
		end
		data.listener =
			registerOnUseItemNearby("itemid", id, lamba_callback)
	end

	for id, data in pairs(otstd.enchant.enchanted_gems) do
		if data.listener ~= nil then
			stopListener(data.listener)
		end

		data.listener =
			registerOnUseItemNearby("itemid", id, otstd.enchant.weapon_callback)
	end

	for id, data in pairs(otstd.enchant.disenchant_tools) do
		if data.listener ~= nil then
			stopListener(data.listener)
		end

		local function lamba_callback(event)
			event.newid = data.newid
			otstd.enchant.disenchant_handler(event)
		end
		data.listener =
			registerOnUseItemNearby("itemid", id, lamba_callback)
	end
end

otstd.enchant.registerHandlers()
