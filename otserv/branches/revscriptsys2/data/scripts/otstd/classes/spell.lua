Spell = {}
Spell_mt = {__index=Spell}
otstd.registered_instants = {} -- By words
otstd.registered_runes    = {} -- By rune ID

function Spell:new()
	local spell = {
		-- Common for all spells
		vocation    = "any",
		words       = nil,
		level       = 0,
		magic_level = 0,
		mana        = 0,
		health      = 0,
		soul        = 0,
		aggressive  = false,
		premium     = false,
		effect      = MAGIC_EFFECT_NONE,
		failEffect  = MAGIC_EFFECT_POFF,
		
		-- Instant spells
		area        = nil,
		
		-- Weapon spells
		weapon      = 0,
		
		-- Rune spells
		rune           = 0,
		targetcreature = true,
		
		-- Conjure spells
		reagent        = 0,
		product        = {id=0, count=0, charges=0}
	}
	setmetatable(spell, Spell_mt)
	return spell
end

-- Handler for instant spells
function otstd.onSpellCheck(event)
	local caster = event.caster
	local spell = event.spell
	local tile = map:getTile(caster:getPosition())
	
	if typeof(caster, "Player") then
		if caster:canUseSpells() == false then
			caster:sendCancel("You cannot cast spells.")
		elseif caster:ignoreSpellCheck() then
			return true
		elseif tile:isPZ() and spell.aggressive then
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

function otstd.onCastSpell(event)
	local spell = event.spell
	local caster = event.caster
	-- All spell checks have been done, remove mana etc.
	
	caster:spendMana(spell.mana)
	caster:removeHealth(spell.health)
	--caster:removeSoul(spell.soul)
	
	if spell.aggressive then
		--caster:addCombatExhaustion(config["fightexhausted"])
	else
		--caster:addHealExhaustion(config["exhausted"])
	end
	
	if spell.internalFinishCast then
		spell:internalFinishCast(event)
	end
	
	if spell.onFinishCast then
		spell:onFinishCast(event)
	end
	
	sendMagicEffect(caster:getPosition(), spell.effect)
end

function otstd.onSaySpell(event)
	event.caster = event.creature
	local spell = event.spell
	
	if otstd.onSpellCheck(event) then
		-- Check extra conditions
		if (not spell.internalBeginCast or spell:internalBeginCast(event)) and (not spell.onBeginCast or spell:onBeginCast(event)) then
			-- Cast the spell!
			if spell.onCast then
				-- Normal cast function has been overridden
				spell:onCast(event)
			else
				otstd.onCastSpell(event)
			end
			return true
		end
	end
	
	event.text = ""
	if spell.failEffect then
		sendMagicEffect(event.creature:getPosition(), spell.failEffect)
	end
end

-------------------------------------------------------------------------------
-- Conjure spells

function otstd.onBeginCastConjureSpell(spell, event)
	local caster = event.caster
	
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

function otstd.onCastConjureSpell(spell, event)
	local caster = event.caster
	
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
			if param:len() > 0 and param:sub(1, 1) ~= "\"" then
				return
			end
			
			event.spell = self
			event.param = param
			
			if self.onSay then
				self:onSay(event)
			else
				otstd.onSaySpell(event)
			end
		end
		
		-- Register the onSay event
		self.onSayHandler = registerOnSay("beginning", false, self.words, spellSayHandler)
	elseif spell.rune ~= 0 and spell.rune then
		-- Rune spell
	else
		error("Unknown spell type, spell must be either instant (words) or rune type")
	end
end

function Spell:unregister()
	if self.onSayHandler then
		stopListener(self.onSayHandler)
		self.onSayHandler = nil
	end
end

function Player:hasLearnedSpell(spellname)
	return true
end
