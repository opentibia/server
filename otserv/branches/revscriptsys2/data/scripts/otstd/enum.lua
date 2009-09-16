
function Enum:type()
	return self.__name or "Enum"
end

function Enum:name(idx)
	return self.__strValues[idx or 1]
end

function Enum:value()
	return self.__intValue
end

