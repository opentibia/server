
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

--
function local_scope(env, level, filter)
	level = level or 1; level = level + 1
	
	-- Replace environment
	local env = {}
	setmetatable(env, {
		__index = function(env, k)
			local v = namespace[k]; if v == nil then v = old_env[k] end
			return v
		end
	})
	setfenv(level, env)

	-- Return restore function
	return function(...)
		setfenv(2, old_env)       -- Restore
		if filter then return filter(...) end
		return ...
	end
end

