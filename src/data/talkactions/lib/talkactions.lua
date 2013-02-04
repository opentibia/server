function serializeParam( param )
	local values = {}
	for value in string.gmatch( param, "%s*([%s%w]+)%s*" ) do
		local _, _, isString = string.find( value, "(%a+)" )
		if not( isString )then
			value = tonumber( value )
		end
		table.insert( values, value )
	end
	return values
end