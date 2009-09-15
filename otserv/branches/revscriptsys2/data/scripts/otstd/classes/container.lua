
function Container:type()
	return "Container"
end

function Container:empty()
	return self:size() == 0
end
