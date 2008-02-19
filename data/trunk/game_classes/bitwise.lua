--[[
Bitwise Class 0.1
Jovial
------------------------------------------------------------------------------------------------------------

OBS:
This is an example class of bitwise operators implementation, since it is already implemented, it is an alternative and a way to make it and understand it.
This is only for educational purposes, if you really want to use bitwise operators, you can use the built-in functions already in this project.
------------------------------------------------------------------------------------------------------------
This should NOT be added to global.lua as a class
--]]

Bit = {
	numberToBinary = function(number)
		if (number < 3) then
			if (number == 1) then
				return number
			elseif (number == 2) then
				return 10
			else
				return FALSE
			end
		else
			local ret = ""
			while number > 1 do
				local tmp = math.fmod(number, 2)
				ret = tmp .. ret
				number = math.floor(number / 2)
			end
			ret = number .. ret
			return tonumber(ret)
		end
	end,
	
    stringToBinary = function(str) 
	    local ret = ""
	    for w in string.gmatch(str, ".") do
	        ret = ret .. Bit.numberToBinary(string.byte(w)) .. ' '
	    end
	    return ret
	end,
	
	compare = function(bit1, bit2)
		local str_bit1 = tostring(bit1)
		local str_bit2 = tostring(bit2)
		local diff = math.abs(string.len(str_bit1) - string.len(str_bit2))
		local add = ""
		for _ = 1, diff do
			add = add .. "0"
		end
		if (string.len(str_bit1) < string.len(str_bit2)) then
			str_bit1 = add .. str_bit1
		else
			str_bit2 = add .. str_bit2
		end
		return str_bit1, str_bit2
	end,
	
	band = function(bit1, bit2)
		bit1, bit2 = Bit.compare(bit1, bit2)
		local i
		local ret = ""
		for i = 1, string.len(bit1) do
			if ((string.sub(bit1, i, i) == "1") and (string.sub(bit2, i, i) == "1")) then
				ret = ret .. "1"
			else
				ret = ret .. "0"
			end
		end
		return tonumber(ret)
	end,
	
	bnot = function(bit)
		bit = tostring(bit)
		local i
		local ret = ""
		for i = 1, string.len(bit) do
			if (string.sub(bit, i, i) == "1") then
				ret = ret .. "0"
			else
				ret = ret .. "1"
			end
		end
		return tonumber(ret)
	end,
	
	bor = function(bit1, bit2)
		bit1, bit2 = Bit.compare(bit1, bit2)
		local i
		local ret = ""
		for i = 1, string.len(bit1) do
			if ((string.sub(bit1, i, i) == "1") or (string.sub(bit2, i, i) == "1")) then
				ret = ret .. "1"
			else
				ret = ret .. "0"
			end
		end
		return tonumber(ret)
	end,
	
	bxor = function(bit1, bit2)
		bit1, bit2 = Bit.compare(bit1, bit2)
		local i
		local ret = ""
		for i = 1, string.len(bit1) do
			if (((string.sub(bit1, i, i) == "1") and (string.sub(bit2, i, i) == "0")) or ((string.sub(bit1, i, i) == "0") and (string.sub(bit2, i, i) == "1"))) then
				ret = ret .. "1"
			else
				ret = ret .. "0"
			end
		end
		return tonumber(ret)
	end
	
	-- TODO: Bit shifts
}
