

function Item:removeCount(count)
	if count <= 0 then
		return
	end

	if self:getCount() > count then
		self:setCount(self:getCount() - count)
	else
		self:destroy()
	end
end

function Item:setRawAttribute(key, value)
	if type(value) == "string" or type(value) == "number" then
		return self:setAttribute(value)
	end
	error("Raw Item attribute must be either string or number (was " + type(value) + ").")
end

function Item:setAttribute(key, value)
	-- Attributes are much more type safe than normal custom values
	if type(value) == "nil" then
		return self:eraseAttribute(key)
	elseif type(value) == "boolean" then
		return self:setBooleanAttribute(value)
	elseif type(value) == "string" or type(value) == "number" then
		if tonumber(value) ~= nil then
			if tostring(value):find(".", 1, true) then
				return self:setFloatAttribute(key, value)
			else
				return self:setIntegerAttribute(key, value)
			end
		else
			return self:setStringAttribute(key, value)
		end
	elseif type(value) == "table" then
		return self:setStringAttribute(key, table.serialize(value))
	end
	error("Item attribute must be either boolean, string, number or table (was " + type(value) + ").")
end

function Item:getAttribute(key)
	local value = self:getRawAttribute(key)
	if type(value) == "string" and value:len() > 0 and value:sub(1, 1) == '{' then
		return table.unserialize(value)
	end
	return value
end

Item.setCustomValue = Item.setAttribute
Item.getCustomValue = Item.getAttribute
Item.setRawCustomValue = Item.setRawAttribute
Item.getRawCustomValue = Item.getRawAttribute
Item.setStorageValue = Item.setAttribute
Item.getStorageValue = Item.getAttribute

function Item:getIntegerAttribute(key)
	local v = self:getRawAttribute(key)
	if type(v) == "number" then
		return v
	end
	return nil
end

function Item:getStringAttribute(key)
	local v = self:getRawAttribute(key)
	if type(v) == "string" then
		return v
	end
	return nil
end

function Item:getBooleanAttribute(key)
	local v = self:getRawAttribute(key)
	if type(v) == "boolean" then
		return v
	end
	return nil
end

-- Common attributes set/get

function Item:getActionID() return self:getIntegerAttribute("aid") or 0 end
function Item:setActionID(v) return self:setIntegerAttribute("aid", v) end

function Item:getUniqueID() return self:getIntegerAttribute("uid") or 0 end

function Item:getSpecialDescription() return self:getStringAttribute("desc") or "" end
function Item:setSpecialDescription(v) return self:setStringAttribute("desc", v) end

function Item:getText() return self:getStringAttribute("text") or "" end
function Item:setText(v) return self:setStringAttribute("text", v) end

function Item:getAttack(extra)
	local a = self:getIntegerAttribute("attack") or self:getDefaultAttack()
	if extra then
		a = a + self:getExtraAttack()
	end
	return a
end
function Item:getAttack() return self:getIntegerAttribute("attack") or self:getDefaultAttack() end
function Item:getDefaultAttack() return Items[self:getItemID()].attack end
function Item:setAttack(v) return self:setIntegerAttribute("attack", v) end

function Item:getDefense() return self:getIntegerAttribute("defense") or self:getDefaultDefense() end
function Item:getDefaultDefense() return Items[self:getItemID()].defense end
function Item:setDefense(v) return self:setIntegerAttribute("defense", v) end

function Item:getArmor() return self:getIntegerAttribute("armor") or self:getDefaultArmor() end
function Item:getDefaultArmor() return Items[self:getItemID()].armor end
function Item:setArmor(v) return self:setIntegerAttribute("armor", v) end

function Item:getWeaponType() return Items[self:getItemID()].weaponType end
function Item:getAmmoType() return Items[self:getItemID()].ammoType end
function Item:getShootType() return Items[self:getItemID()].shootType end

function Item:getShootRange() return Items[self:getItemID()].shootRange end
function Item:getHitChance() return Items[self:getItemID()].hitChance end
function Item:getMaxHitChance() return Items[self:getItemID()].maxHitChance end
function Item:getBreakChance() return Items[self:getItemID()].breakChance end

function Item:isCorpse() return Items[self:getItemID()].corpseType ~= RACE_NONE end
