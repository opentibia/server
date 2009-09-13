-------------------------------------------------------------------
-- OpenTibia - an opensource roleplaying game
----------------------------------------------------------------------
-- This program is free software; you can redistribute it and-or
-- modify it under the terms of the GNU General Public License
-- as published by the Free Software Foundation; either version 2
-- of the License, or (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this program; if not, write to the Free Software Foundation,
-- Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
----------------------------------------------------------------------

function enum(params, ...)
	local name = ""
	local namespace = nil
	local nameprefix = ""
	local values = {...}
	
	if type(params) == "string" then
		namespace, name = params:match("(.-)::(.+)")
		if not name then
			namespace = nil
			name = params
		end
	else
		name = params["name"]
		namespace = params["namespace"]
	end
	if namespace then nameprefix = namespace .. "::" end
	
	-- Automask = values will be generated automatically as a bitmask
	local automask = params["bitmask"]
	
	-- Pre-process the values some
	-- We split them into a tree structure, with name as a list, and value as an int
	local nvalues = {}
	for _, value in ipairs(values) do
		nval = {}
		
		function split(s)
			local e, v = s:match("(.-)%s*=%s*(.*)")
			if v then
				if v:match("%d+") then
					automask = false
				end
			else
				e = s
			end
			local n = {}
			n["name"] = {e}
			n["int" ] = v
			
			return n
		end
		
		if type(value) == "string" then
			nval = split(value)
		elseif type(value) == "table" then
			nval = split(table.remove(value, 1))
			for e, v in ipairs(value) do
				table.insert(nval["name"], v)
			end
			if value["null"] then
				nval["null"] = true
			end
		else
			error("Value must be table/string")
		end
		table.insert(nvalues, nval)
	end
	values = nvalues
	
	-- Automask the values, if we should
	local mask_count = 0
	local fullmask = 0
	local allmask = {}
	if automask then
		local mask = 1
		for _, v in ipairs(values) do
			if v["null"] then
				-- Some values are NULL
				v["int"] = 0
			elseif v["all"] then
				-- All values!
				table.insert(allmask, v)
			elseif v["int"] then
				-- Leave it alone, it's a composition
			else
				v["int"] = mask
				fullmask = fullmask + mask
				mask = mask * 2
				mask_count = mask_count + 1
			end
		end
	end
	
	-- Fill the 'all' values
	for _, v in ipairs(allmask) do
		v["int"] = fullmask
	end
	
	if namespace then
		header:write("namespace " .. namespace .. " {" .. "\n")
	end
	
	-- Write the enum declaration to the header
	header:write("namespace enums {\n")
	header:write("\tenum " .. name .. " {\n")
	for _, v in ipairs(values) do
		header:write("\t\t" .. v["name"][1])
		if v["int"] then
			header:write(" = " .. v["int"])
		end
		header:write(",\n")
	end
	
	-- End enum
	header:write("\t}; // end enum\n")
	header:write("} // end namespace\n\n")
	
	-- Create the Enum type
	local enumtype = ""
	local enumbase = ""
	
	if params["bitmask"] then
		if automask then
			enumbase = "Enum<" .. nameprefix .. "enums::" .. name .. ", " .. mask_count .. ">"
		else
			enumbase = "Enum<" .. nameprefix .. "enums::" .. name .. ", -1>"
		end
		enumtype = "Bit" .. enumbase
	else
		enumbase = "Enum<" .. nameprefix .. "enums::" .. name .. ", " .. nameprefix .. "enums::" .. values[#values]["name"][1] .. " + 1>"
		enumtype = enumbase
	end
	header:write("typedef " .. enumtype .. " " .. name .. ";\n\n")
	header:write("typedef " .. enumbase .. " " .. name .. "__Base;\n\n")
	
	-- Write the enum definitions (as real types)
	header:write("//begin enum definitions\n")
	for _, v in ipairs(values) do
		header:write("\tconst " .. name .. " " .. v["name"][1] .. "(enums::" .. v["name"][1] .. ");\n")
	end
	
	header:write("//end enum definitions\n\n")
	
	if namespace then
		header:write("} // end namespace " .. namespace .. "\n")
	end
	
	
	-- Write the .implementation declarations
	implementation:write("template<> bool " .. nameprefix .. name .. "__Base::initialized = false;\n")
	implementation:write("template<> std::string " .. nameprefix .. name .. "__Base::enum_name = \"" .. name .. "\";\n")
	implementation:write("template<> " .. nameprefix .. name .. "__Base::EnumToString " .. nameprefix .. name .. "__Base::enum_to_string;\n")
	implementation:write("template<> " .. nameprefix .. name .. "__Base::StringToEnum " .. nameprefix .. name .. "__Base::string_to_enum;\n")
	implementation:write("template<> " .. nameprefix .. name .. "__Base::StringToEnum " .. nameprefix .. name .. "__Base::lstring_to_enum;\n")
	
	-- Init function
	implementation:write("template<> void " .. nameprefix .. name .. "__Base::initialize()\n")
	implementation:write("{\n")
	for _, v in ipairs(values) do
		local first = table.remove(v["name"], 1)
		implementation:write("\t" .. "initAddValue(" .. nameprefix .. "enums::" .. first .. ", \"" .. first .. "\", true);\n")
		
		for _, ename in ipairs(v["name"]) do
			implementation:write("\t" .. "initAddValue(" .. nameprefix .. "enums::" .. first .. ", \"" .. ename.. "\", false);\n")
		end
	end
	implementation:write("}\n\n")
end

function BeginEnumFile(filename)
	-- These are global
	header = {name = filename .. ".h", _ = ""}
	function header:write(s) header._ = header._ .. s end
	implementation = {name = filename .. ".cpp", _ = ""}
	function implementation:write(s) implementation._ = implementation._ .. s end
	
	-- Some warning text
	automated_text = "\n" ..
		"// This file has been automatically generated by a script.\n" ..
		"// Do not make changes to this file manually, as they will be discarded\n" ..
		"// as soon as the project is recompiled\n\n"
	header:write(automated_text)
	implementation:write(automated_text)
	
	-- Include guards
	header:write("#ifndef " .. filename:upper() .. "_H\n")
	header:write("#define " .. filename:upper() .. "_H\n")
	
	-- Includes
	header:write("#include \"enum.h\"\n\n")
	implementation:write("#include \"otpch.h\"\n")
	implementation:write("#include \"" .. header.name .. "\"\n\n")
end


function EndEnumFile()
	header:write("\n#endif\n")
	
	-- Write if any changes were made
	local hfile = io.open(header.name, "r")
	
	local full = hfile and hfile:read("*a")
	if header._ ~= full then
		if hfile then
			hfile:close()
		end
		print("Notice: " .. header.name .. " modified, generating new .h / .cpp file")
		
		hfile = io.open(header.name, "w+")
		hfile:write(header._)
		hfile:close()
		cppfile = io.open(implementation.name, "w+")
		cppfile:write(implementation._)
		cppfile:close()
	else
		print("Notice: " .. header.name .. " no file modification detected.")
	end
end


