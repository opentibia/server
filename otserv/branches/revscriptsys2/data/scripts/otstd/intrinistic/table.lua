

function table.find(t, val, comparator)
	for k, v in pairs(t) do
		if comparator then
			if comparator(val, v) then
				return k
			end
		else
			if v == val then
				return k
			end
		end
	end
	return nil
end

function table.findf(t, comparator)
	for k, v in pairs(t) do
		if comparator(val) then
			return k
		end
	end
	return nil
end

function table.contains(t, val)
	return table.find(t, val) ~= nil
end

function table.append(t, v)
	table.insert(t, v)
end

function table.empty(t)
	return table.next() == nil
end

function table.serialize(x, recur)
	local t = type(x)
	if getmetatable(x) then
		error("Can not serialize a table that has a metatable associated with it.")
	elseif t == "table" then
		if table.find(recur, x) then
			error("Can not serialize recursive tables.")
		end
		table.append(recur, x)
		
		local s = "{"
		for k, v in pairs(x) do
			s = s .. "[" .. table.serialize(k) .. "]"
			s = s .. " = " .. table.serialize(v) .. ";"
		end
		s = s .. "}"
		return s
	elseif t == "string" then
		return string.format("%q", x)
	elseif t == "number" then
		return tostring(x)
	elseif t == "boolean" then
		return t and "true" or false
	elseif t == "nil" then
		return "nil"
	else
		error("Can not serialize value of type '" .. t .. "'.")
	end
end

function table.unserialize(str)
	return loadstring("return " .. str)()
end
