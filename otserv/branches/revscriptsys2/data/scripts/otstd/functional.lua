
_1 = {1}
_2 = {2}
_3 = {3}
_4 = {4}
_5 = {5}
_6 = {6}
_7 = {7}
_8 = {8}
_9 = {9}
_ps = {_1, _2, _3, _4, _5, _6, _7, _8, _9}

function bind(f, ...)
	prp = {...}
	return function(...)
		local pp = {}
		local lp = {...}
		local n = 1
		for i, v in prp do
			local d = false
			for _, p in _ps do
				if v == p then
					d = true
					pp[p[1]] = lp[i] 
					n = n + 1
				end
			end
			if d then
				pp[n] = v
			end
		end
		[[--
		for i = 1, (#prp + lp) do
			if not pp[i] then
				pp[i] = lp[i]
			end
		end
		--]]
		
		f(unpack(pp))
	end
end
