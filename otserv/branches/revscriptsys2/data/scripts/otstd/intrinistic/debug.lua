
function debug.dump(x, tabs, recur)
	local t = type(x)
	recur = recur or {}
	tabs = tabs or 1
	local s = ""
	
	if t == nil then
		return "nil"
	elseif t == "string" then
		return string.format("%q", x)
	elseif t == "number" then
		return tostring(x)
	elseif t == "boolean" then
		return t and "true" or "false"
	elseif t == "table" then
		if table.find(recur, x) then
			return "recursive reference"
		end
		table.append(recur, x)
		
		s = "{\n"
		if getmetatable(x) then
			s = s .. string.rep("\t", tabs) .. "metatable = " .. tostring(getmetatable(x)) .. ";\n"
		end
		for k, v in pairs(x) do
			s = s .. string.rep("\t", tabs)
			s = s .. debug.dump(k, tabs + 1, recur)
			s = s .. " = " .. debug.dump(v, tabs + 1, recur) .. ";\n"
		end
		s = s .. string.rep("\t", tabs) .. "}"
	else
		s = tostring(x)
	end
	return s
end

var_dump = debug.dump
