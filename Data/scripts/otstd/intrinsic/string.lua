

function string.explode(str, delimiter)
	if str == nil then
		return {}
	end
	if delimiter == nil then
		delimiter = "\n"
	end
	t = {}
	for v in string.gmatch(str, "([^".. delimiter .."]*)" .. delimiter .. "?") do
		table.insert(t, v)
	end
	table.remove(t) -- Removes last element (Always "")
	return t
end

function string.strip_whitespace(str)
	return str:match("^%s*(.-)%s*$")
end

