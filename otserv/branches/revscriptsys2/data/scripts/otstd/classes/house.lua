
function House:type()
	return "House"
end

function House:addInvite(pattern)
	local l = self:getInvitedList()
	if not l then
		return false 
	end
	
	if table.find(l, pattern) then
		return true
	end
	
	table.append(l, pattern)
	self:setInviteList(l)
	return true
end

function House:addSubowner(pattern)
	local l = self:getSubownerList()
	if not l then
		return false 
	end
	
	if table.find(l, pattern) then
		return true
	end
	
	table.append(l, pattern)
	self:setSubownerList(l)
	return true
end

__internal_getInvitedList = House.getInvitedList
function House:getInvitedList()
	local l = __internal_getInvitedList(self)
	if not l then
		return nil
	end
	
	return string.explode(l, "\n")
end

__internal_getSubownerList = House.getSubownerList
function House:getSubownerList()
	local l = __internal_getSubownerList(self)
	if not l then
		return nil
	end
	
	return string.explode(l, "\n")
end
