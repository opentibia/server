-- TODO:
-- - Distance weapons hit chance formula
-- - Wand support

otstd.weapons = {}

Weapon = {}
Weapon_mt = {__index = Weapon}

function Weapon:new(weaponID)
	local weapon = {
		-- Default weapon params
		id = weaponID,
		vocation = "any",
		level = 0,
		magicLevel = 0,
		mana = 0,
		manaPercent = 0,
		soul = 0,
		exhaustion = false,
		premium = false,

		-- Combat params
		combatType = COMBAT_PHYSICALDAMAGE,
		blockedByDefense = true,
		blockedByArmor = true,

		-- Damage formula
		damageFormula = nil,

		-- Callbacks
		onUseWeapon = nil
	}
	setmetatable(weapon, Weapon_mt)
	return weapon
end

-- Formula to get weapon max damage based on player's level, skill and attack factor
function otstd.getWeaponMaxDamage(level, attackSkill, attackValue, attackFactor)
	return math.ceil(2 * (attackValue * (attackSkill + 5.8) / 25 + level / 10 - 0.1) / attackFactor)
end

-- Default damage formula callback
function otstd.damageFormula(player, target, weapon)
	local attackValue = weapon:getAttack()
	if weapon:getWeaponType() == WEAPON_AMMO then
		local bow = player:getWeapon(true)
		if bow and bow:getAmmoType() == weapon:getAmmoType() then
			attackValue = attackValue + bow:getAttack()
		end
	end

	local maxDamage = otstd.getWeaponMaxDamage(player:getLevel(), player:getWeaponSkill(weapon), attackValue, player:getAttackFactor())
	local minDamage = 0

	if weapon:getWeaponType() == WEAPON_DIST or
		weapon:getWeaponType() == WEAPON_AMMO then
		if typeof(target, "Player") then
			minDamage = math.ceil(player:getLevel() * 0.1)
		else
			minDamage = math.ceil(player:getLevel() * 0.2)
		end
	end

	-- vocation multipliers
	local vocation = player:getVocation()
	if vocation then
		local meleeBaseDamage = vocation:getMeleeBaseDamage(weapon:getWeaponType())
		if meleeBaseDamage ~= 1.0 then
			maxDamage = maxDamage * meleeBaseDamage
			minDamage = minDamage * meleeBaseDamage
		end
	end

	return -math.random(minDamage, maxDamage)
end

-- Default fist formula callback
function otstd.fistDamageFormula(player, target)
	return -math.random(0, otstd.getWeaponMaxDamage(player:getLevel(), player:getSkill(SKILL_FIST), 7, player:getAttackFactor()))
end

function otstd.onUseWeapon(event)
	local weapon = event.weapon

	if not typeof(event.player, "Player") then
		error("onUseWeapon not triggered by a Player!")
	end

	if not weapon then
		event.target = event.attacked
		otstd.internalUseFist(event)
	else
		local internalWeapon = otstd.weapons[weapon:getItemID()]
		if not internalWeapon then
			error("onUseWeapon event triggered with unknown weapon. (ItemID: " .. weapon:getItemID() .. ")")
		end

		event.internalWeapon = internalWeapon
		event.target = event.attacked
		event.damageModifier = 100
		if otstd.onWeaponCheck(event) then
			if internalWeapon and internalWeapon.onUseWeapon then
				internalWeapon:onUseWeapon(event)
			else
				otstd.internalUseWeapon(event)
			end
		end
	end

	return true
end

function otstd.onUsedWeapon(event)
	local player = event.player
	local weapon = event.weapon
	local internalWeapon = event.internalWeapon

	-- Add skill points
	if not player:cannotGainSkill() and
		player:getAddAttackSkill() then
		local skillType = nil
		local weaponType = weapon:getWeaponType()
		if weaponType == WEAPON_SWORD then
			skillType = SKILL_SWORD
		elseif weaponType == WEAPON_CLUB then
			skillType = SKILL_CLUB
		elseif weaponType == WEAPON_AXE then
			skillType = skill_AXE
		elseif weaponType == WEAPON_DIST or
			weaponType == WEAPON_AMMO then
			skillType = SKILL_DIST
		end

		local blockType = player:getLastAttackBlockType()
		local skillPoints = 0
		if skillType == SKILL_DIST and
			blockType == BLOCK_NONE then
			skillPoints = 2
		elseif blockType == BLOCK_DEFENSE or
				blockType == BLOCK_ARMOR or
				blockType == BLOCK_NONE then
			skillPoints = 1
		end

		player:advanceSkill(skillType, skillPoints)
	end

	if internalWeapon then
		if not player:hasInfiniteMana() then
			player:spendMana(internalWeapon.mana)
		end

		if not player:hasInfiniteSoul() then
			player:removeSoul(internalWeapon.soul)
		end

		if not player:canGetExhausted() and
			internalWeapon.exhausted then
			player:addCombatExhaustion(config["fight_exhausted"])
		end
	end
	
	if not player:cannotGainInFight() then
		player:addInFight(config["in_fight_duration"])
	end

	return true
end

function otstd.onUsedFist(event)
	local player = event.player

	if not player:cannotGainSkill() and
		player:getAddAttackSkill() then
		player:advanceSkill(SKILL_FIST, 1)
	end
end

function otstd.onWeaponCheck(event)
	local player = event.player
	local target = event.target
	local weapon = event.weapon
	local internalWeapon = event.internalWeapon

	local shootRange = weapon:getShootRange()

	-- If is a two-handed distance weapon we may check the ammo types
	-- and get the bow range
	if weapon:getWeaponType() == WEAPON_AMMO then
		local bow = player:getWeapon(true)
		if bow:getWeaponType() ~= WEAPON_DIST or
			bow:getAmmoType() ~= weapon:getAmmoType() then
			return false
		end

		shootRange = bow:getShootRange()
	end

	if not areInRange(player:getPosition(), target:getPosition(), shootRange) then
		return false
	end

	if not internalWeapon then
		-- No script-specific restrictions
		return true
	end
	
	if not player:ignoreWeaponCheck() then
		if (internalWeapon.premium and not player:isPremium()) or
			(internalWeapon.mana > player:getMana()) or
			(internalWeapon.soul > player:getSoulPoints()) or
			(not checkVocation(player:getVocation(), internalWeapon.vocation)) then
			return false
		end

		-- check if wielded properly
		-- level
		if internalWeapon.level > player:getLevel() then
			local penalty = (internalWeapon.level - player:getLevel()) * 0.02
			if penalty > 0.5 then
				penalty = 0.5
			end
			event.damageModifier = event.damageModifier - (event.damageModifier * penalty)
		end

		-- magic level
		if internalWeapon.magicLevel > player:getMagicLevel() then
			event.damageModifier = event.damageModifier / 2
		end
	end

	return true
end

function otstd.internalUseWeapon(event)
	event:skip()

	local player = event.player
	local target = event.target
	local weapon = event.weapon
	local internalWeapon = event.internalWeapon

	local damage = 0
	if internalWeapon and internalWeapon.damageFormula then
		damage = internalWeapon:damageFormula(player, target, weapon)
	else
		damage = otstd.damageFormula(player, target, weapon)
	end
	damage = (damage * event.damageModifier) / 100

	if internalWeapon then
		internalCastSpell(
			internalWeapon.combatType, player, target, damage,
			internalWeapon.blockedByShield, internalWeapon.blockedByArmor)
	else
		internalCastSpell(
			COMBAT_PHYSICALDAMAGE, player, target, damage,
			true, true)
	end
	
	-- send effect
	local shootType = weapon:getShootType()
	if shootType ~= SHOOT_EFFECT_NONE then
		sendDistanceEffect(player:getPosition(), target:getPosition(), shootType)
	end

	-- call finish handler
	if internalWeapon and internalWeapon.onUsedWeapon then
		internalWeapon:onUsedWeapon(event)
	else
		otstd.onUsedWeapon(event)
	end
end

function otstd.internalUseFist(event)
	event:skip()

	local player = event.player
	local target = event.target


	-- the target must be in range
	if areInRange(player:getPosition(), target:getPosition(), 1) then
		local damage = otstd.fistDamageFormula(player, target)
		-- do the damage
		internalCastSpell(COMBAT_PHYSICALDAMAGE, player, target, damage, true, true)
		-- call finish handler
		otstd.onUsedFist(event)
	end
end

function Weapon:register()
	self:unregister()
	if otstd.weapons[self.id] ~= nil then
		error("Duplicate weapon id \"" .. self.id .. "\"")
	else
		self.onUseWeaponHandler = registerOnUseWeapon(self.id, "itemid", otstd.onUseWeapon)
		otstd.weapons[self.id] = self
	end
end

function Weapon:unregister()
	if self.onUseWeaponHandler ~= nil then
		stopListener(self.onUseWeaponHandler)
		self.onUseWeaponHandler = nil
	end
	otstd.weapons[self.id] = nil
end


otstd.onFistHandler = registerOnUseWeapon("fist", otstd.onUseWeapon)
otstd.onDefaultWeaponHandler = registerOnUseWeapon("all", otstd.onUseWeapon)
