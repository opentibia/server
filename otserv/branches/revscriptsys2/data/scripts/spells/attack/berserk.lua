local berserk = Spell:new("Berserk")

berserk.words 		= "exori"
berserk.vocation 	= {"Knight", "Elite Knight"}
berserk.damageType 	= COMBAT_PHYSICALDAMAGE
berserk.level		= 35
berserk.mana		= 115
berserk.aggressive 	= true
berserk.premium		= false--true
berserk.areaEffect 	= MAGIC_EFFECT_BLACK_SPARK

berserk.area 		=
	{
		{"a", "a", "a"},
		{"a", " ", "a"},
		{"a", "a", "a"}
	}

berserk.formula = function(player)
	local level = player:getLevel()
	local weapon = player:getWeapon()
	local weaponAttack = weapon and weapon:getAttack() or 7 --config["fist_strenght"]
	local weaponSkill = player:getWeaponSkill(weapon)

	return math.random(
			((weaponSkill+weaponAttack)*0.5+(level/5)),
			((weaponSkill+weaponAttack)*1.5+(level/5)))
end

berserk:register()
