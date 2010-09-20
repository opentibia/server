Spell = {}
Spell_mt = {__index=Spell}
otstd.spells = {} -- By name (all spells must have an unique name)

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
		maybeTarget     = false,
		needTarget      = false,
		condition       = nil,

		-- Area spells use this
		area            = nil,
		field           = 0,

		-- Damaging spells
		damageType      = COMBAT_NONE,
		aggressive      = false,
		blockedByArmor  = false,
		blockedByShield = false,
		effect          = MAGIC_EFFECT_NONE,
		failEffect      = MAGIC_EFFECT_POFF,
		areaEffect      = MAGIC_EFFECT_NONE,
		shootEffect     = SHOOT_EFFECT_NONE,

		-- Overrideable functions
		onBeginCast     = nil,
		onCast          = nil,
		onHitCreature   = nil,
		onHitTile	    = nil,
		onFinishCast    = nil,
		-- Very low-level, probably won't override these
		onSay           = nil,

		-- Instant spells
		words           = nil,

		-- Weapon spells
		weapon          = 0,

		-- Rune spells
		rune            = 0,
		range           = nil,

		-- Conjure spells
		reagent         = 0,
		product         = {id = 0, count = 0, charges = 0}
	}
	setmetatable(spell, Spell_mt)
	return spell
end

-- Some standard checks for availiability of targets
function otstd.canCastSpellOnTile(spell, caster, tile)
	if not tile then
		return true, RET_NOERROR
	end

	if tile:blockProjectile() or tile:floorChange() or tile:positionChange() then
		return false, RET_NOTENOUGHROOM
	end

	if typeof(caster, "Player") then
		local playerPos = caster:getPosition()
		local toPos = tile:getPosition()
		if playerPos.z > toPos.z then
			return false, RET_FIRSTGOUPSTAIRS
		elseif playerPos.z < toPos.z then
			return false, RET_FIRSTGODOWNSTAIRS
		end

		if caster:ignoreProtectionZone() then
			return true, RET_NOERROR
		end
	end

	if spell.aggressive and tile:isPz() then
		return false, RET_ACTIONNOTPERMITTEDINPROTECTIONZONE
	end

	return true, RET_NOERROR
end

function otstd.canCastSpellOnCreature(spell, caster, target)
	if caster and spell.aggressive then
		if target:getID() == caster:getID() then
			return false, RET_YOUMAYNOTATTACKTHISPERSON
		end

		caster = (typeof(caster, "Player") and caster) or caster:getMaster()
		if typeof(caster, "Player") then
			if typeof(target, "Player") then

				if caster:cannotAttackPlayer() then
					return false, RET_YOUMAYNOTATTACKTHISPERSON
				end

				if target:isInvulnerable() then
					return false, RET_YOUMAYNOTATTACKTHISPERSON
				elseif not caster:ignoreProtectionZone() and target:getParentTile():isPz() then
					return false, RET_ACTIONNOTPERMITTEDINANONPVPZONE
				end

				if caster:getSkull() == SKULL_BLACK then
					if target:getSkull() == SKULL_NONE and not target:hasAttacked(caster) then
						return false, RET_YOUMAYNOTATTACKTHISPERSON
					end
				end

				if getWorldType() == WORLD_TYPE_NOPVP and not otstd.isInPvpZone(caster, target) then
					return false, RET_YOUMAYNOTATTACKTHISPERSON
				end
			elseif typeof(target, "Actor") then
				if target:isPlayerSummon() then
					if getWorldType() == WORLD_TYPE_NOPVP and not otstd.isInPvpZone(caster, target) then
						return false, RET_YOUMAYNOTATTACKTHISCREATURE
					end
				else
					if caster:cannotAttackMonster() then
						return false, RET_YOUMAYNOTATTACKTHISCREATURE
					end
				end
			end
		end
	end

	return true, RET_NOERROR
end

function otstd.isInPvpZone(caster, creature)
	if caster and caster:getZone() ~= ZONE_PVP then
		return false
	end

	if creature:getZone() ~= ZONE_PVP then
		return false
	end

	return true
end

--------------------------------------------------------------------------------------------------------------------------------------------------------------
-- The logic behind spell casting is somewhat convuluted, this tries to explain it
--
-- When a spell is registered, a lambda callback is created to attach additional information to the event, see Spell.register
-- The lambda callback then calls the function otstd.onSpell, which is the same for all types of spells, only the information attached
--   to the event itself (or the spell) affects what this function does.
-- First it calls otstd.onSpellCheck, which checks all 'normal' conditions for spellcasting, such as the player having enough mana etc.
-- If these checks all pass, the internalBeginCast function is called on the spell, this is different depending on the type of the spell
--   and checks certain pre-conditions. Some types of spells don't use this at all.
-- After that the custom onBeginCast function is called on the spell, you can override this to add additional conditions for casting the spell
-- If one of the above calls returned false, the 'failEffect' is created on the tile, else the actual casting begins.
-- If the spell has defined a custom cast function spell.onCast, that function is called and after that casting finishes. Otherwise the standard
--    function otstd.onCastSpell is called, onCastSpell then removes mana, soul points, charges from the rune etc. for the spell, then it calculates
--    the area of the spell to be hit, and loops through it calling onHitCreature, onHitTile and creates magic effects & fields as applicable.
-- After the default handling has been done, any custom spell.onFinishCast function is called, if none is defined, the internal spell.internalFinishCast
--    function is called, and then casting is finished.

--------------------------------------------------------------------------------------------------------------------------------------------------------------

-- Handler for all spells, this is called first of everything
function otstd.onSpell(event)
	local caster = event.caster
	local spell = event.spell

	if otstd.onSpellCheck(event) then
		-- Check extra conditions
		if (not spell.internalBeginCast or spell.internalBeginCast(event)) and (not spell.onBeginCast or spell.onBeginCast(event)) then

			-- Cast the spell!
			if spell.onCast then
				-- Normal (low-level) cast function has been overridden
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

-- Checks that are common for all spells that are cast
function otstd.onSpellCheck(event)
	local caster = event.caster
	local spell = event.spell
	local tile = map:getTile(caster:getPosition())

	if typeof(caster, "Player") then
		if caster:canUseSpells() == false then
			caster:sendCancel("You cannot cast spells.")
		elseif caster:ignoreSpellCheck() then
			return true
		elseif tile:isPz() and spell.aggressive then
			caster:sendCancel(RET_ACTIONNOTPERMITTEDINPROTECTIONZONE)
		elseif (spell.aggressive and caster:isCombatExhausted()) or (not spell.aggressive and caster:isHealExhausted()) then
			caster:sendCancel(RET_YOUAREEXHAUSTED)
		elseif caster:getLevel() < spell.level then
			caster:sendCancel(RET_NOTENOUGHLEVEL)
		elseif caster:getMagicLevel() < spell.magicLevel then
			caster:sendCancel(RET_NOTENOUGHMAGICLEVEL)
		elseif caster:getMana() < spell.mana and not caster:hasInfiniteMana() then
			caster:sendCancel(RET_NOTENOUGHMANA)
		elseif caster:getHealth() < spell.health and not caster:isInvulnerable() then
			caster:sendCancel("You do not have enough health.")
		elseif caster:getSoulPoints() < spell.soul and not caster:hasInfiniteSoul() then
			caster:sendCancel(RET_NOTENOUGHSOUL)
		elseif caster:hasLearnedSpell(spell.name) == false then
			caster:sendCancel(RET_YOUNEEDTOLEARNTHISSPELL)
		elseif spell.weapon ~= 0 and not caster:isEquipped(spell.weapon) then
			caster:sendCancel(RET_YOUNEEDAWEAPONTOUSETHISSPELL)
		elseif spell.premium and not caster:isPremium() then
			caster:sendCancel(RET_YOUNEEDPREMIUMACCOUNT)
		elseif not checkVocation(caster:getVocationName(), spell.vocation) then
			caster:sendCancel(RET_YOURVOCATIONCANNOTUSETHISSPELL)
		else
			return true
		end
		return false
	else
		-- NPCs can always cast spells
		return true
	end
end

-- Called once a spell casting has been decided (ie. the cast did not abort)
function otstd.onCastSpell(event)
	local caster = event.caster
	local casterPos = caster and caster:getPosition()
	local target = event.targetCreature
	local targetPos = event.targetPosition or (target and target:getPosition())
	local spell = event.spell

	if typeof(caster, "Player") then
		-- All spell checks have been done, remove mana etc.
		if not caster:hasInfiniteMana() then
			caster:spendMana(spell.mana)
		end

		if not caster:hasInfiniteSoul() then
			caster:removeSoul(spell.soul)
		end

		if not caster:isInvulnerable() then
			caster:removeHealth(spell.health)
		end

		if not caster:canGetExhausted() then
			if spell.aggressive then
				caster:addCombatExhaustion(config["fight_exhausted"])
			else
				caster:addHealExhaustion(config["heal_exhausted"])
			end
		end

		if spell.aggressive and not caster:cannotGainInFight() then
			caster:addInFight(config["in_fight_duration"])
		end
	end

	-- default spell handling
	local centerPos = (target and target:getPosition()) or targetPos or casterPos
	local hitTiles = {}

	if target and (spell.maybeTarget or spell.needTarget) then
		local targetTile = map:getTile(targetPos)
		hitTiles[targetPos] = {[1] = target}
	else
		hitTiles = spell:getAffectedArea(centerPos, caster)
	end

	-- We got a list of all tiles, loop through and apply spell effect
	for pos, creatures in pairs(hitTiles) do
		local targetTile = map:getTile(pos)
		local canCast = not targetTile or otstd.canCastSpellOnTile(spell, caster, targetTile)

		if targetTile and canCast then
			for _, target in ipairs(creatures) do
				if otstd.canCastSpellOnCreature(spell, caster, target) then
					if spell.damageType ~= COMBAT_NONE then
						local amount = 0
						if spell.formula then
							amount = spell.formula(caster)
							if spell.aggressive then
								amount = -amount
							end
						end
						if caster ~= target then
							if amount == 0 or internalCastSpell(spell.damageType, caster, target, amount, spell.blockedByShield, spell.blockedByArmor) then
								if spell.condition then
									target:addCondition(spell.condition)
								end

								if spell.onHitCreature then
									spell.onHitCreature(target, event)
								end
							end
						end
					else
						if spell.condition then
							target:addCondition(spell.condition)
						end

						if spell.onHitCreature then
							spell.onHitCreature(target, event)
						end
					end

					if spell.aggressive and typeof(target, "Player") then
						target:addInFight(config["in_fight_duration"])
					end
				else
					-- If we can't attack the creature, just send the magic effect and finish cast
					local magicEffect = MAGIC_EFFECT_NONE
					if spell.damageType == COMBAT_ENERGYDAMAGE then
						magicEffect = MAGIC_EFFECT_ENERGY_HIT
					elseif spell.damageType == COMBAT_EARTHDAMAGE then
						magicEffect = MAGIC_EFFECT_GREEN_RING
					elseif spell.damageType == COMBAT_DROWNDAMAGE then
						magicEffect = MAGIC_EFFECT_BLUE_RING
					elseif spell.damageType == COMBAT_FIREDAMAGE then
						magicEffect = MAGIC_EFFECT_FIRE_HIT
					elseif spell.damageType == COMBAT_ICEDAMAGE then
						magicEffect = MAGIC_EFFECT_ICE_HIT
					elseif spell.damageType == COMBAT_HOLYDAMAGE then
						magicEffect = MAGIC_EFFECT_HOLY_HIT
					elseif spell.damageType == COMBAT_DEATHDAMAGE then
						magicEffect = MAGIC_EFFECT_SMALLCLOUDS
					elseif spell.damageType == COMBAT_LIFEDRAIN then
						magicEffect = MAGIC_EFFECT_RED_SHIMMER
					end

					sendMagicEffect(caster:getPosition(), magicEffect)
				end
			end

			-- Spawn a simple field for field spells
			if spell.field and spell.field ~= 0 then
				if not targetTile:isBlocking() then
					local field = createItem(spell.field)
					targetTile:addItem(field)
					field:startDecaying()
					event.field = field
				end
			end

			-- Call the tile hit callback
			if spell.onHitTile then
				spell.onHitTile(targetTile, event)
			end
		end

		-- Area effects should be displayed even if the tile did not exist, but not if it's PZ (and the spell is aggressive)
		if canCast and spell.areaEffect ~= MAGIC_EFFECT_NONE then
			sendMagicEffect(pos, spell.areaEffect)
		end
	end

	if caster and spell.effect ~= MAGIC_EFFECT_NONE then
		sendMagicEffect(caster:getPosition(), spell.effect)
	end
	if caster and spell.shootEffect ~= SHOOT_EFFECT_NONE then
		--caster:sendNote("Shoot from " .. table.serialize(caster:getPosition()) .. " to " .. table.serialize(centerPos) .. ".")
		sendDistanceEffect(caster:getPosition(), centerPos, spell.shootEffect)
	end

	-- finish cast
	if spell.onFinishCast then
		return spell.onFinishCast(event)
	end

	if spell.internalFinishCast then
		return spell.internalFinishCast(event)
	end
end

--------------------------------------------------------------------------------------------------------------------------------------------------------------
-- Instant spells default handler begin/finish handlers

function otstd.onFinishCastInstantSpell(event)
	local caster = event.caster
	local text = event.text

	if typeof(caster, "Player") then
		if config["orange_spell_text"] then
			caster:say(text, SPEAK_MONSTER_SAY)
		else
			caster:say(text, SPEAK_SAY)
		end
	end
end

--------------------------------------------------------------------------------------------------------------------------------------------------------------
-- Rune spells default handler begin/finish handlers

-- Extra conditions for casting rune spells
function otstd.onBeginCastRuneSpell(event)
	local caster = event.caster
	local creature = event.targetCreature
	local toPos = event.targetPosition
	local tile = map:getTile(toPos)
	local spell = event.spell

	if typeof(caster, "Player") then
		if tile then
			local playerPos = caster:getPosition()
			if playerPos.z > toPos.z then
				player:sendCancel(RET_FIRSTGOUPSTAIRS)
				return false
			elseif playerPos.z < toPos.z then
				player:sendCancel(RET_FIRSTGODOWNSTAIRS)
				return false
			else
				if not map:canThrowObjectTo(playerPos, toPos, true) then
					caster:sendCancel(RET_CANNOTTHROW)
					return false
				end

				if spell.needTarget and not event.targetCreature then
					caster:sendCancel(RET_CANONLYUSETHISRUNEONCREATURES)
					return false
				end

				if spell.range and spell.range ~= 0 then
					if not map:canThrowObjectTo(playerPos, toPos, true, spell.range, spell.range) then
						caster:sendCancel(RET_DESTINATIONOUTOFREACH)
						return false
					end
				end

				local result, retval = otstd.canCastSpellOnTile(spell, caster, tile)
				if not result then
					caster:sendCancel(retval)
					return false
				end

				--[[
				if(blockingCreature and tile->getTopCreature(caster) != NULL){
					caster:sendCancel(RET_NOTENOUGHROOM)
					return false
				}
				elseif(blockingSolid and tile->isBlocking() then
					caster:sendCancel(RET_NOTENOUGHROOM)
					return false
				}
				--]]

				if spell.aggressive and typeof(target, "Player") then
					if caster:getSkull() == SKULL_BLACK then
						if not spell.area then
							if target ~= caster and target:getSkull() == SKULL_NONE and not target:hasAttacked(caster) then
								caster:sendCancelMessage(RET_YOUMAYNOTATTACKTHISPERSON)
								return false
							end
						else
							caster:sendCancelMessage(RET_NOTPOSSIBLE)
							return false
						end
					end

					if caster:hasSafeMode() and spell.needTarget then
						caster:sendCancel(RET_TURNSECUREMODETOATTACKUNMARKEDPLAYERS)
						return false
					end
				end
			end
		end
	end

	return true
end

function otstd.onFinishCastRuneSpell(event)
	local caster = event.caster
	local rune = event.item

	if typeof(caster, "Player") and
		typeof(rune, "Item") then

		if config["remove_rune_charges"] then
			local newcount = rune:getCount() - 1
			if newcount <= 0 then
				rune:destroy()
			else
				rune:setCount(newcount)
			end
		end
	end
end

-------------------------------------------------------------------------------
--  Conjuration Spells default begin/finish handlers

-- Checks conditions for casting conjuration spells
function otstd.onBeginCastConjureSpell(event)
	local caster = event.caster
	local spell = event.spell

	if spell.reagent and spell.reagent ~= 0 then -- Reagents! => Rune spell
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
	else
		-- No extra checks for non-rune spells
		-- You can always conjure arrows
	end
	return true
end

-- Cast handler for conjuration spells
-- Converts the items, if runes, or creates the items if simple conjuration
function otstd.onCastConjureSpell(event)
	local caster = event.caster
	local spell = event.spell

	if event.reagents then -- Reagents! => Rune spell
		for _, item in ipairs(event.reagents) do
			item:setItemID(spell.product.id, spell.product.count)
			break -- Only make one rune
			-- TODO: Allow creating two runes at once
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

--------------------------------------------------------------------------------------------------------------------------------------------------------------
-- Common formulas

-- minFloor and maxFloor are optional
function formulaLevelMagic(minAbsolute, minDelta, minFloor, maxAbsolute, maxDelta, maxFloor)
	if maxDelta == nil and maxFloor == nil then
		-- None are set
		-- Move arguments down and set defaults
		maxDelta = maxAbsolute
		maxAbsolute = minFloor
		minFloor = 0
		maxFloor = 0
	elseif maxDelta == nil then
		-- 6th argument is set by 5th is not
		error("formulaLevelMagic takes 4 or 6 paramaters")
	else
		-- Both 5th and 6th are set
	end

	return function(player)
		return math.random(
				math.max(minFloor, minAbsolute + (player:getLevel() / 5 + player:getMagicLevel() * minDelta)),
				math.max(maxFloor, maxAbsolute + (player:getLevel() / 5 + player:getMagicLevel() * maxDelta)))
	end
end

function formulaOldLevelMagic(minAbsolute, minDelta, minFloor, maxAbsolute, maxDelta, maxFloor)
	if maxDelta == nil and maxFloor == nil then
		-- None are set
		-- Move arguments down and set defaults
		maxDelta = maxAbsolute
		maxAbsolute = minFloor
		minFloor = 0
		maxFloor = 0
	elseif maxDelta == nil then
		-- 6th argument is set by 5th is not
		error("formulaLevelMagic takes 4 or 6 paramaters")
	else
		-- Both 5th and 6th are set
	end

	return function(player)
		return math.random(
				math.max(minFloor, minAbsolute + (player:getLevel() / 3 + player:getMagicLevel() * 2) * minDelta),
				math.max(maxFloor, maxAbsolute + (player:getLevel() / 3 + player:getMagicLevel() * 2) * maxDelta))
	end
end

function formulaLevel(minAbsolute, minDelta, minFloor, maxAbsolute, maxDelta, maxFloor)
	if maxDelta == nil and maxFloor == nil then
		-- None are set
		-- Move arguments down and set defaults
		maxDelta = maxAbsolute
		maxAbsolute = minFloor
		minFloor = 0
		maxFloor = 0
	elseif maxDelta == nil then
		-- 6th argument is set by 5th is not
		error("formulaLevelMagic takes 4 or 6 paramaters")
	else
		-- Both 5th and 6th are set
	end

	return function(player)
		return math.random(
				math.max(minFloor, minAbsolute + player:getLevel() * minDelta),
				math.max(maxFloor, maxAbsolute + player:getLevel() * maxDelta))
	end
end

function formulaMagic(minAbsolute, minDelta, minFloor, maxAbsolute, maxDelta, maxFloor)
	if maxDelta == nil and maxFloor == nil then
		-- None are set
		-- Move arguments down and set defaults
		maxDelta = maxAbsolute
		maxAbsolute = minFloor
		minFloor = 0
		maxFloor = 0
	elseif maxDelta == nil then
		-- 6th argument is set by 5th is not
		error("formulaLevelMagic takes 4 or 6 paramaters")
	else
		-- Both 5th and 6th are set
	end

	return function(player)
		return math.random(
				math.max(minFloor, minAbsolute + player:getMagicLevel() * minDelta),
				math.max(maxFloor, maxAbsolute + player:getMagicLevel() * maxDelta))
	end
end

function formulaStatic(minFloor, maxFloor)
	if minFloor > maxFloor then
		local v = maxFloor
		maxFloor = minFloor
		minFloor = v
	end

	return function(player) return math.random(minFloor, maxFloor) end
end

--------------------------------------------------------------------------------------------------------------------------------------------------------------
-- Interface for spells,

-- Cast a spell, with or without a caster
-- Params is target for instant spells with a target (exura sio) or target creature / tile for rune spells
-- If there is no caster, the target tile can be any tile
function Spell:cast(caster, target)
	local spellThread = nil

	-- Construct a virtual event
	local event = {}

	if self.words then
		-- Instant spell

		event.class = SPEAK_SAY
		if type(target) == "string" then
			event.text = target

			local param = string.strip_whitespace(string.sub(event.text, self.words:len()+1) or "")
			if self.needTarget then
				event.targetCreature = getPlayerByName(param)
			elseif self.maybeTarget then
				event.targetCreature = caster:getTarget()
			end
		else
			event.text = ""
			if typeof(target, "Creature") then
				event.targetCreature = target
			elseif typeof(target, "Position") then
				event.targetPosition = target
			end
		end

		event.spell = self
		event.caster = caster
		event.creature = caster

		if self.onSay then
			spellThread = coroutine.create(self.onSay)
		else
			spellThread = coroutine.create(otstd.onSpell)
		end
	elseif self.rune ~= 0 and self.rune then
		-- Rune spell
		return false
	end

	-- Invoke the coroutine
	while true do
		state, ret = coroutine.resume(spellThread, event)
		if not state then
			error(ret)
			break
		end

		if ret == "WAIT" then
			-- Pass waits on
			wait(param)
		else
			break
		end
	end
end

-- Returns the area affected by this spell casted on position centerPos by the caster
-- Second parameter should be a creature OR the direction, if the spell does not require direction, it may be nil
function Spell:getAffectedArea(centerPos, caster)
	local hitTiles = {}
	if self.area then
		-- Collect the positions that the spell hit on
		local areaWidth = #self.area[1]
		local areaHeight = table.getn(self.area)

		local centerX = (areaWidth - 1) / 2
		local centerY = (areaHeight - 1) / 2

		local dir = nil
		if typeof(caster, "Creature") then
			local casterPos = caster:getPosition()

			local dx = centerPos.x - casterPos.x
			local dy = centerPos.y - casterPos.y
			-- Decide the direction (should be moved to another function?)
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
			else
				dir = caster:getDirection()
			end
		else
			dir = caster
		end

		-- Go through the area array, and assemble all tiles that match the direction
		for rowIndex, rows in pairs(self.area) do
			for colIndex, value in ipairs(rows) do
				if		(value:find("a") or value:find("%[a%]") ) or
						(dir == NORTHWEST and ( value:find("%[nw%]") or value:find("%[wn%]")) ) or
						(dir == NORTHEAST and ( value:find("%[ne%]") or value:find("%[en%]")) ) or
						(dir == SOUTHWEST and ( value:find("%[sw%]") or value:find("%[ws%]")) ) or
						(dir == SOUTHEAST and ( value:find("%[se%]") or value:find("%[es%]")) ) or
						(dir == NORTH     and ( value == "n" or string.find(value, "%[n%]")) ) or
						(dir == SOUTH     and ( value == "s" or string.find(value, "%[s%]")) ) or
						(dir == WEST      and ( value == "w" or string.find(value, "%[w%]")) ) or
						(dir == EAST      and ( value == "e" or value:find("%[e%]")) )
						then
					local posx = centerPos.x + (centerX - (areaWidth - 1)) + colIndex - 1
					local posy = centerPos.y + (centerY - (areaHeight - 1)) + rowIndex - 1

					local pos = {x = posx, y = posy, z = centerPos.z}

					if map:rayCast(centerPos, pos, true) then
						local tile = map:getTile(pos)
						hitTiles[pos] = tile and tile:getCreatures()
					end
				end
			end
		end
	else
		local tile = map(centerPos)
		hitTiles[centerPos] = tile and tile:getCreatures()
	end

	return hitTiles
end

-- Register / Unregister the spell
function Spell:register()
	self:unregister()

	if otstd.spells[self.name] then
		error("Duplicate spell \"" .. self.name .. "\"")
	end

	if self.words then
		assert(self.words:len() > 0, "Words for spells must be atleast one letter.")
		-- Instant spell
		if table.findf(otstd.spells, function(s) return s.words == self.words end) ~= nil then
			error("Duplicate spell \"" .. self.words .. "\": two instant spells can't have the same words.")
		end

		self.internalFinishCast = otstd.onFinishCastInstantSpell

		--  Figure out spell type
		if self.product.id ~= 0 then
			-- Conjuration spell
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
		else
			-- Other instant spell (attack)
		end

		-- Lamba callback to include what spell is being cast
		local function spellSayHandler(event)
			event:propagate()

			local text = string.explode(event.text, "\"")
			local words = string.strip_whitespace(text[1] or "")
			local param = string.strip_whitespace(text[2] or "")
			if self.words:lower() == words:lower() then
				-- this is the right one
				event:skip()

				event.spell = self
				event.caster = event.creature
				if self.needTarget then
					event.targetCreature = getPlayerByName(param)
				elseif self.maybeTarget then
					event.targetCreature = event.creature:getTarget()
				end

				if self.onSay then
					self:onSay(event)
				else
					otstd.onSpell(event)
				end
			end
		end

		-- Register the onSay event
		self.onSayHandler = registerOnSay("beginning", false, self.words, spellSayHandler)

	elseif self.rune ~= 0 and self.rune then

		if table.findf(otstd.spells, function(s) return s.rune == self.rune end) ~= nil then
			error("Duplicate spell \"" .. self.words .. "\": two rune spells can't use the same ID.")
		end

		self.internalBeginCast  = otstd.onBeginCastRuneSpell
		self.internalFinishCast = otstd.onFinishCastRuneSpell

		-- Lamba callback to include what spell is being used
		local function spellUseRuneHandler(event)
			event:skip()

			event.caster = event.player
			event.spell = self
			if self.needTarget and not event.targetCreature then
				local tile = map:getTile(event.targetPosition)
				event.targetCreature = tile and tile:getTopCreature(event.player)
			end

			event.caster:sendNote("Casting rune spell " .. self.name)
			otstd.onSpell(event)
		end

		self.onUseRuneHandler = registerOnUseItem("itemid", self.rune, spellUseRuneHandler)
	else
		error("Unknown spell type, spell must be either instant (words) or rune type")
	end

	otstd.spells[self.name] = self
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

	otstd.spells[self.name] = nil
end

-- Placeholder
function Player:hasLearnedSpell(spellname)
	return true
end
