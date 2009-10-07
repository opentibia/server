
functional = {}

--

function fmap(f, list)
    local rlist = {}
    for i, v in ipairs(list) do
        rlist[i] = f(v)
    end
    return rlist
end

--

function famap(f, list)
    local rlist = {}
    for i, v in pairs(list) do
        rlist[i] = f(v)
    end
    return rlist
end

--

function foldl(f, list)
    local r = nil
    if #list == 1 then
        return r
    end
    for i, v in ipairs(list) do
        if r then
            r = f(r, v)
        else
            r = v
        end
    end
    return r
end

function filter(f, list)
	local rlist = {}
	for k, v in ipairs(list) do
		if f(v) then
			table.append(rlist, v)
		end
	end
	return rlist
end

--

function with(obj, f, ...)
	if obj then
		return obj:f(...)
	end
	return nil
end
