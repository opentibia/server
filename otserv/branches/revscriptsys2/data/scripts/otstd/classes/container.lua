
function Container:type()
	return "Container"
end

function Container:empty()
	return self:size() == 0
end

-- "Smart" function to create items (mainly containers)

function createContainer(contents)
	if type(contents) == "table" then
		local item = createItem(contents.itemid or contents.id, contents.count or contents.fluidtype or contents.charges)
		for k, v in ipairs(contents) do
			-- Create
			item:addItem(createContainer(v))
		end
		
		for k, v in pairs(contents) do
			if tonumber(k) == nil then
				if k == "actionid" then
					k = "aid"
				end
				
				item:setAttribute(k, v)
			end
		end
		
		return item
	end
	error("Parameter 'contents' should be a table")
end
