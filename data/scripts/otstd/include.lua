
function include(file_name)
	local include_path = _G['include_path'] or {}
	_G['include_path'] = include_path
	
	local path = string.match(file_name, '([^/]*/)')
	
	local final_path = ''
	for k, n in ipairs(include_path) do
		final_path = final_path .. n
	end
	final_path = final_path .. file_name
	if path ~= nil then
		table.insert(include_path, path)
		require(final_path)
		table.remove(include_path)
	else
		require(final_path)
	end
end

function include_directory(directory)
	local include_path = _G['include_path'] or {}
	_G['include_path'] = include_path
	
	local final_path = ''
	for k, n in ipairs(include_path) do
		final_path = final_path .. n
	end
	final_path = final_path .. directory
	require_directory(final_path)
end
