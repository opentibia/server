local combat = createCombatObject()
setCombatParam(combat, COMBAT_PARAM_TYPE, COMBAT_PHYSICALDAMAGE)
setCombatParam(combat, COMBAT_PARAM_BLOCKARMOR, TRUE)
setCombatParam(combat, COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_ETHEREALSPEAR)

function getSpellDamage(cid, weaponSkill, weaponAttack, attackStrength)
	local attack = 25 --Spear's attack
	local skill = getPlayerSkill(cid, CONST_SKILL_DISTANCE)

	local maxWeaponDamage = (skill * attack) / 20 + attack
	local damage = -((math.random(0, maxWeaponDamage) * attackStrength) / 100) * 0.8 --0.8 is the multiplier

	return damage, damage --The random part of the formula has already been made, just return the normal damage
end

setCombatCallback(combat, CALLBACK_PARAM_SKILLVALUE, "getSpellDamage")

function onCastSpell(cid, var)
	return doCombat(cid, combat, var)
end