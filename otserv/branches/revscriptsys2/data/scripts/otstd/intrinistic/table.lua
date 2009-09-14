

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

