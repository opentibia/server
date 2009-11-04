Spell = {}
Spell_mt = {__index=Spell}
otstd.registered_instants = {} -- By words
otstd.registered_runes    = {} -- By rune ID

function Spell:new(name)
	local spell = {
		-- Common for all spells
		name            = name,
		vocation        = "any",
		level           = 0,
		magicLevel      = 0,
		mana            = 0,
		health          = 0,
		soul            = 0,
		premium         = false,
		
		damageType      = COMBAT_NONE,
		needTarget      = false,
		aggressive      = false,
		blockedByArmor  = false,
		blockedByShield = false,
		area            = nil,
		effect          = MAGIC_EFFECT_NONE,
		failEffect      = MAGIC_EFFECT_POFF,
		areaEffect      = MAGIC_EFFECT_NONE,
		
		-- Overrideable functions
		onBeginCast     = nil,
		onCast          = nil,
		onHitCreature   = nil,
		onHitTile	    = nil,
		onFinishCast    = nil,
		
		-- Instant spells
		words           = nil,
		
		-- Weapon spells
		weapon          = 0,
		
		-- Rune spells
		rune            = 0,
		range           = 0,
		
		-- Conjure spells
		reagent         = 0,
		product         = {id = 0, count = 0, charges = 0}
	}
	setmetatable(spell, Spell_mt)
	return spell
end

function otstd.canCastSpellOnTile(spell, tile)
	if spell.aggressive and tile:isPz()  then
		return false
	end

	return true
end

function otstd.canCastSpellOnCreature(spell, creature)
	return true
end

-- Generic spell checks for both instant and rune spells
function otstd.onSpellCheck(event)
	local caster = event.caster
	local spell = event.spell
	local tile = map:getTile(caster:getPosition())
	
	if caster and typeof(caster, "Player") then
		if caster:canUseSpells() == false then
			caster:sendCancel("You cannot cast spells.")
		elseif caster:ignoreSpellCheck() then
			return true
		elseif tile:isPz() and spell.aggressive then
			caster:sendCancel("This action is not permitted in a protection zone.")
		elseif (spell.aggressive and caster:isCombatExhausted()) or (not spell.aggressive and caster:isHealExhausted()) then
			caster:sendCancel("You are exhausted.")
		elseif caster:getLevel() < spell.level then
			caster:sendCancel("You don not have enough level.")
		elseif caster:getMagicLevel() < spell.magic_level then
			caster:sendCancel("You don not have enough magic level.")
		elseif caster:getMana() < spell.mana and not caster:hasInfiniteMana() then
			caster:sendCancel("You do not have enough mana.")
		elseif caster:getHealth() < spell.health and not caster:isInvulnerable() then
			caster:sendCancel("You do not have enough health.")
		elseif caster:getSoulPoints() < spell.soul and not caster:hasInfiniteSoul() then
			caster:sendCancel("You do not have enough soul.")
		elseif caster:hasLearnedSpell(spell.name) == false then
			caster:sendCancel("You need to learn this spell first.")
		elseif spell.weapon ~= 0 and caster:isEquipped(spell.weapon) then
			caster:sendCancel("You need to equip a weapon to use this spell.")
		elseif spell.premium and caster:isPremium() then
			caster:sendCancel("You need a premium account to use this spell.")
		else
			return true
		end
		return false
	else
		-- NPCs can always cast spells
		return true
	end
end

-- Called once a spell has been casted
function otstd.onCastSpell(event)
	local caster = event.caster
	local casterPos = caster and caster:getPosition()
	--local toPos = casterPos
	local target = event.targetCreature
	local spell = event.spell
	
	if caster and typeof(caster, "Player") then
		-- All spell checks have been done, remove mana etc.
		caster:spendMana(spell.mana)
		caster:removeHealth(spell.health)
		--caster:removeSoul(spell.soul)
		
		if spell.aggressive then
			--caster:addCombatExhaustion(config["fightexhausted"])
		else
			--caster:addHealExhaustion(config["exhausted"])
		end
	end
		
	if spell.onFinishCast then
		-- Normal cast function has been overridden
		return spell.onFinishCast(event)
	end
	
	if spell.internalFinishCast then
		return spell.internalFinishCast(event)
	end
	
	-- default spell handling
	local centerPos = event.targetPosition or casterPos
	
	local list = {}
	if spell.area then
		-- Collect the positions that the spell hit on					
		local areaWidth = #spell.area[1]
		local areaHeight = table.getn(spell.area)
		
		local centerX = (areaWidth - 1) / 2
		local centerY = (areaHeight - 1) / 2

		local dir = caster:getDirection()

		local dx = centerPos.x - casterPos.x
		local dy = centerPos.y - casterPos.y		
		if not (dx == 0 and dy == 0) then
			if dx < 0 and dy < 0 then
				dir = NORTHWEST
			elseif dx > 0 and dy < 0 then
				dir = NORTHEAST
			elseif dx < 0 and dy > 0 then
				dir = SOUTHWEST
			elseif dx > 0 and dy > 0 then
				dir = SOUTHEAST
			elseif dx < 0 then
				dir = WEST
			elseif dx > 0 then
				dir = EAST
			elseif dy < 0 then
				dir = NORTH
			else
				dir = SOUTH
			end
		end
				
		for rowIndex, rows in pairs(spell.area) do
			for colIndex, value in ipairs(rows) do
				
				if (value == "a" or value:find("%[a%]") ) or
				(dir == NORTHWEST and ( value:find("%[nw%]") or value:find("%[wn%]")) ) or
				(dir == NORTHEAST and ( value:find("%[ne%]") or value:find("%[en%]")) ) or
				(dir == SOUTHWEST and ( value:find("%[sw%]") or value:find("%[ws%]")) ) or
				(dir == SOUTHEAST and ( value:find("%[se%]") or value:find("%[es%]")) ) or
				(dir == NORTH     and ( value == "n" or string.find(value, "%[n%]")) ) or
				(dir == SOUTH     and ( value == "s" or string.find(value, "%[s%]")) ) or
				(dir == WEST      and ( value == "w" or string.find(value, "%[w%]")) ) or
				(dir == EAST      and ( value == "e" or value:find("%[e%]")) ) then
					local posx = centerPos.x + (centerX - (areaWidth - 1)) + colIndex - 1
					local posy = centerPos.y + (centerY - (areaHeight - 1)) + rowIndex - 1	
					
					local pos = {x = posx, y = posy, z = centerPos.z}
					local tile = map:getTile(pos)
					list[pos] = tile and tile:getCreatures()
				end
			end
		end
	else
		local tile = map:getTile(centerPos)
		list[centerPos] = tile and tile:getCreatures()
	end	
	
	for pos, creatures in pairs(list) do
		local position targetTile = map:getTile(pos)		
		if targetTile and otstd.canCastSpellOnTile(spell, targetTile) then 
		
			for __,target in ipairs(creatures) do
			
				if otstd.canCastSpellOnCreature(spell, target) then 
					if damageType ~= COMBAT_NONE then
						local amount = 0
						if spell.formula then
							amount = spell.formula(caster)
						end
						if internalCastSpell(spell.damageType, caster, target, amount, spell.blockedByShield, spell.blockedByArmor) then
							if spell.onHitCreature then
								spell.onHitCreature(target)
							end
						end
					elseif spell.onHitCreature then
						spell.onHitCreature(target)
					end
				end
			end

			if spell.onHitTile then
				spell.onHitTile(targetTile)
			end					
		end

		if spell.areaEffect then
			sendMagicEffect(pos, spell.areaEffect)
		end
	end
	
	if caster and spell.effect ~= MAGIC_EFFECT_NONE then
		sendMagicEffect(caster:getPosition(), spell.effect)
	end
end

-------------------------------------------------------------------------------
-- Instant spells

-- Handler
function otstd.onSaySpell(event)
	local caster = event.caster
	local spell = event.spell

	if otstd.onSpellCheck(event) and otstd.onInstantSpellCheck(event) then
		-- Check extra conditions
		if (not spell.internalBeginCast or spell.internalBeginCast(event)) and (not spell.onBeginCast or spell.onBeginCast(event)) then
			
			-- Cast the spell!
			if spell.onCast then
				-- Normal cast function has been overridden
				spell.onCast(event)
			else
				otstd.onCastSpell(event)
			end
			return true
		end
	end
	
	event.text = ""
	if caster and spell.failEffect ~= MAGIC_EFFECT_NONE then
		sendMagicEffect(caster:getPosition(), spell.failEffect)
	end
end

-- Spell checks
function otstd.onInstantSpellCheck(event)	
	local caster = event.caster
	
	if caster and typeof(caster, "Player") then
		--TODO: Additional checks like black skull, safe-mode
	end
	
	return true
end

-------------------------------------------------------------------------------
-- Rune spells

-- Handler
function otstd.onUseRuneSpell(event)
	local caster = event.creature
	local spell = event.spell
	
	if otstd.onSpellCheck(event) and otstd.onRuneSpellCheck(event) then
		-- Check extra conditions
		if (not spell.internalBeginCast or spell:internalBeginCast(event)) and (not spell.onBeginCast or spell:onBeginCast(event)) then
			-- Cast the spell!
			if spell.onCast then
				-- Normal cast function has been overridden
				spell.onCast(event)
			else
				otstd.onCastSpell(event)
			end
			return true
		end
	end
	
	if caster and spell.failEffect ~= MAGIC_EFFECT_NONE then
		sendMagicEffect(caster:getPosition(), spell.failEffect)
	end
end

-- Spell checks
function otstd.onRuneSpellCheck(event)
	local caster = event.player
	local playerPos = caster:getPosition()
	local toPos = event.targetPosition
	local tile = toPos and map:getTile(toPos)
	local spell = event.spell
	
	if caster and typeof(caster, "Player") then
		if tile then
			if playerPos.z > toPos.z then
				player:sendCancel(RET_FIRSTGOUPSTAIRS)
				return false
			elseif playerPos.z < toPos.z then
				player:sendCancel(RET_FIRSTGODOWNSTAIRS)
				return false
			else
				if spell.range > 0 then
					--if not Map:canThrowObjectTo(playerPos, toPos, true, range, range) then
					--	player:sendCancel(RET_DESTINATIONOUTOFREACH)
					--	return false;
					--end
				end

				--TODO: Additional checks like black skull, safe-mode
			end
		end
	end
	
	return true
end

-------------------------------------------------------------------------------
-- Conjure spells

function otstd.onBeginCastConjureSpell(event)
	local caster = event.caster
	local spell = event.spell
	
	if spell.reagent ~= 0 then -- Reagents! => Rune spell
		local reagents = {}
		for _, item in ipairs(caster:getHandContents()) do
			if type(spell.reagent) == "table" then
				if table.find(spell.reagent, item:getItemID()) then
					table.append(reagents, item)
				end
			else
				-- Only one reagent
				if spell.reagent == item:getItemID() then
					table.append(reagents, item)
				end
			end
		end
		if #reagents == 0 then
			caster:sendCancel("You need a magic item to cast this spell.")
			return false
		end
		event.reagents = reagents
	end
	return true
end

function otstd.onCastConjureSpell(event)
	local caster = event.caster
	local spell = event.spell
	
	if event.reagents then -- Reagents! => Rune spell
		for _, item in ipairs(event.reagents) do
			item:setItemID(spell.product.id, spell.product.count)
			break -- Only make one rune
		end
	else -- Conjure item simply
		local count = spell.product.count
		repeat
			caster:addItem(createItem(spell.product.id, math.min(100, count)))
			count = count - math.min(100, count)
		until count <= 0
	end

	if caster and spell.effect ~= MAGIC_EFFECT_NONE then
		sendMagicEffect(caster:getPosition(), spell.effect)
	end
end

-------------------------------------------------------------------------------

function Spell:register()
	self:unregister()
	
	if self.words and self.words ~= "" then
		-- Instant spell
		if table.find(otstd.registered_instants, self.words) ~= nil then
			error("Duplicate spell \"" .. self.words .. "\": two instant spells can't have the same words.")
		end
		
		--  Figure out spell type
		if self.product.id ~= 0 then
			-- Conjure spell
			if self.product.charges ~= 0 then
				-- We alias count/charges
				self.product.count = self.product.charges
			elseif self.product.count ~= 0 then
				self.product.charges = self.product.count
			else
				self.product.count = 1
			end
			
			self.internalBeginCast  = otstd.onBeginCastConjureSpell
			self.internalFinishCast = otstd.onCastConjureSpell
		end
		
		-- Lamba callback to include what spell is being cast
		function spellSayHandler(event)
			event:propagate()
			
			local param = string.strip_whitespace(string.sub(event.text, self.words:len()+1) or "")
			event.spell = self
			event.caster = event.creature
			if needTarget then
				event.targetCreature = getPlayerByName(param)
			end
			
			if self.onSay then
				self:onSay(event)
			else
				otstd.onSaySpell(event)
			end
		end
		
		-- Register the onSay event
		self.onSayHandler = registerOnSay("beginning", false, self.words, spellSayHandler)
		
	elseif self.rune ~= 0 and self.rune then

		-- Lamba callback to include what spell is being used
		function spellUseRuneHandler(event)
			event:skip()

			event.caster = event.player
			event.spell = self			

			if self.onUseRune then
				self:onUseRune(event)
			else
				otstd.onUseRuneSpell(event)
			end
		end

		self.onUseRuneHandler = registerOnUseItem("itemid", self.rune, spellUseRuneHandler)
	else
		error("Unknown spell type, spell must be either instant (words) or rune type")
	end
end

function Spell:unregister()
	if self.onSayHandler then
		stopListener(self.onSayHandler)
		self.onSayHandler = nil
	end

	if self.onUseRuneHandler then
		stopListener(self.onUseRuneHandler)
		self.onUseRuneHandler = nil
	end
end

function Player:hasLearnedSpell(spellname)
	return true
end
