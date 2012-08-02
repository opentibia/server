--[[
The MIT License

Copyright (c) 2010 Ignacio Burgueño

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
--]]

-- tables
local _G = _G
local string, io, os, table, math, package, debug, coroutine = string, io, os, table, math, package, debug, coroutine

-- functions
local xpcall, tostring, print, unpack, require, getfenv, setmetatable = xpcall, tostring, print, unpack, require, getfenv, setmetatable
local next, assert, tonumber, rawequal, collectgarbage, getmetatable = next, assert, tonumber, rawequal, collectgarbage, getmetatable
local module, rawset, pcall, newproxy, type, select, gcinfo, pairs = module, rawset, pcall, newproxy, type, select, gcinfo, pairs
local rawget, loadstring, ipairs, dofile, setfenv, load, error, loadfile = rawget, loadstring, ipairs, dofile, setfenv, load, error, loadfile

assert(debug, "debug table must be available at this point")

local io_open = io.open
local string_gmatch = string.gmatch
local string_sub = string.sub
local table_concat = table.concat
local debug_getlocal = debug.getlocal
local debug_getinfo = debug.getinfo

local _M = {}

-- this tables should be weak so the elements in them won't become uncollectable
local m_known_tables = {
	[_G] = "_G (global table)",
	[string] = "string module",
	[io] = "io module",
	[os] = "os module",
	[table] = "table module",
	[math] = "math module",
	[package] = "package table",
	[debug] = "debug table",
	[coroutine] = "coroutine table"
}
local m_user_known_tables = {}

local m_known_functions = {
	[xpcall] = "xpcall",
	[tostring] = "tostring",
	[print] = "print",
	[unpack] = "unpack",
	[require] = "require",
	[getfenv] = "getfenv",
	[setmetatable] = "setmetatable",
	[next] = "next",
	[assert] = "assert",
	[tonumber] = "tonumber",
	[rawequal] = "rawequal",
	[collectgarbage] = "collectgarbage",
	[getmetatable] = "getmetatable",
	[module] = "module",
	[rawset] = "rawset",
	[pcall] = "pcall",
	[newproxy] = "newproxy",
	[type] = "type",
	[select] = "select",
	[gcinfo] = "gcinfo",
	[pairs] = "pairs",
	[rawget] = "rawget",
	[loadstring] = "loadstring",
	[ipairs] = "ipairs",
	[dofile] = "dofile",
	[setfenv] = "setfenv",
	[load] = "load",
	[error] = "error",
	[loadfile] = "loadfile",
	-- TODO: add table.* etc functions
}

local m_user_known_functions = {}

--local m_found_files = {}	-- aca armaria un cache de archivos encontrados, si vale la pena

-- Private:
-- Parses a line, looking for possible function definitions (in a very naïve way) 
-- Returns '(anonymous)' if no function name was found in the line
local function ParseLine(line)
	assert(type(line) == "string")
	--print(line)
	local match = line:match("^%s*function%s+(%w+)")
	if match then
		--print("+++++++++++++function", match)
		return match
	end
	match = line:match("^%s*local%s+function%s+(%w+)")
	if match then
		--print("++++++++++++local", match)
		return match
	end
	match = line:match("^%s*local%s+(%w+)%s+=%s+function")
	if match then
		--print("++++++++++++local func", match)
		return match
	end
	match = line:match("%s*function%s*%(")	-- this is an anonymous function
	if match then
		--print("+++++++++++++function2", match)
		return "(anonymous)"
	end
	return "(anonymous)"
end

-- Private:
-- Tries to guess a function's name when the debug info structure does not have it.
-- It parses either the file or the string where the function is defined.
-- Returns '?' if the line where the function is defined is not found
local function GuessFunctionName(info)
	--print("guessing function name")
	if type(info.source) == "string" and info.source:sub(1,1) == "@" then
		local file, err = io_open(info.source:sub(2), "r")
		if not file then
			print("file not found: "..tostring(err))	-- whoops!
			return "?"
		end
		local line
		for i = 1, info.linedefined do
			line = file:read("*l")
		end
		if not line then
			print("line not found")	-- whoops!
			return "?"
		end
		return ParseLine(line)
	else
		local line
		local lineNumber = 0
		for l in string_gmatch(info.source, "([^\n]+)\n-") do
			lineNumber = lineNumber + 1
			if lineNumber == info.linedefined then
				line = l
				break
			end
		end
		if not line then
			print("line not found")	-- whoops!
			return "?"
		end
		return ParseLine(line)
	end
end

-- Private:
-- Iterates over the local variables of a given function
-- @param level The stack level where the function is.
-- @param message The message object 
local function DumpLocals(level, message)
	local prefix = "\t "
	local i = 1
	level = level + 1
	
	local name, value = debug_getlocal(level, i)
	if not name then
		return
	end
	message:add("\tLocal variables:\r\n")
	while name do
		if type(value) == "number" then
			message:add_f("%s%s = number: %g\r\n", prefix, name, value)
		elseif type(value) == "boolean" then
			message:add_f("%s%s = boolean: %s\r\n", prefix, name, tostring(value))
		elseif type(value) == "string" then
			message:add_f("%s%s = string: %q\r\n", prefix, name, value)
		elseif type(value) == "userdata" then
			message:add_f("%s%s = %s\r\n", prefix, name, tostring(value))
		elseif type(value) == "nil" then
			message:add_f("%s%s = nil\r\n", prefix, name)
		elseif type(value) == "table" then
			if m_known_tables[value] then
				message:add_f("%s%s = %s\r\n", prefix, name, m_known_tables[value])
			elseif m_user_known_tables[value] then
				message:add_f("%s%s = %s\r\n", prefix, name, m_user_known_tables[value])
			else
				local txt = "{"
				for k,v in pairs(value) do
					txt = txt..k..":"..tostring(v)
					if #txt > 70 then
						txt = txt.." (more...)"
						break
					end
					if next(value, k) then txt = txt..", " end
				end
				message:add_f("%s%s = %s  %s\r\n", prefix, name, tostring(value), txt.."}")
			end
		elseif type(value) == "function" then
			local info = debug_getinfo(value, "nS")
			local fun_name = info.name or m_known_functions[value] or m_user_known_functions[value]
			if info.what == "C" then
				message:add_f("%s%s = C %s\r\n", prefix, name, (fun_name and ("function: " .. fun_name) or tostring(value)))
			else
				local source = info.short_src
				if source:sub(2,7) == "string" then
					source = source:sub(9)	-- uno más, por el espacio que viene (string "Baragent.Main", por ejemplo)
				end
				--for k,v in pairs(info) do print(k,v) end
				fun_name = fun_name or GuessFunctionName(info)
				message:add_f("%s%s = Lua function '%s' (defined at line %d of chunk %s)\r\n", prefix, name, fun_name, info.linedefined, source)
			end
		elseif type(value) == "thread" then
			message:add_f("%sthread %q = %s\r\n", prefix, name, tostring(value))
		end
		i = i + 1
		name, value = debug_getlocal(level, i)
	end
end

-- Public:
-- Collects a detailed stack trace, dumping locals, resolving function names when they're not available, etc
-- This functions is suitable to be used as an error handler with pcall
-- @param err An optional error string or object.
-- Returns a string with the stack trace and a string with the original error.
function _M.stacktrace(err)
	--print(err)
	local original_error
	-- a helper for collecting strings to be used when assembling the final message
	local message = {}
	function message:add(text)
		self[#self + 1] = text
	end
	function message:add_f(fmt, ...)
		self:add(fmt:format(...))
	end
	
	if type(err) == "table" then
		message:add("an error object {\r\n")
		local first = true
		for k,v in pairs(err) do
			if first then
				message:add("  ")
				first = false
			else
				message:add(",\r\n  ")
			end
			message:add(tostring(k))
			message:add(": ")
			message:add(tostring(v))
		end
		message:add("\r\n}")
		original_error = table_concat(message)
	elseif type(err) == "string" then
		message:add(err)
		original_error = err
	end
	
	message:add("\r\n")
	message:add[[
Stack Traceback
===============
]]
	--print(error_message)
	
	local start_level = 2
	local info = debug_getinfo(start_level, "nSlf")
	while info do
		if info.what == "main" then
			if string_sub(info.source, 1, 1) == "@" then
				message:add_f("(%d) main chunk of file '%s' at line %d\r\n", start_level, string_sub(info.source, 2), info.currentline)
			else
				message:add_f("(%d) main chunk of %s at line %d\r\n", start_level, info.short_src, info.currentline)
			end
		elseif info.what == "C" then
			--print(info.namewhat, info.name)
			--for k,v in pairs(info) do print(k,v, type(v)) end
			local function_name = info.name or m_known_functions[info.func] or m_user_known_functions[value] or tostring(info.func)
			message:add_f("(%d) %s C function '%s'\r\n", start_level, info.namewhat, function_name)
			--message:add_f("%s%s = C %s\r\n", prefix, name, (m_known_functions[value] and ("function: " .. m_known_functions[value]) or tostring(value)))
		elseif info.what == "tail" then
			--print("tail")
			--for k,v in pairs(info) do print(k,v, type(v)) end--print(info.namewhat, info.name)
			message:add_f("(%d) tail call\r\n", start_level)
			DumpLocals(start_level, message)
		elseif info.what == "Lua" then
			local source = info.short_src
			local function_name
			if source:sub(2, 7) == "string" then
				source = source:sub(9)
			end
			local was_guessed = false
			if not info.name then
				--for k,v in pairs(info) do print(k,v, type(v)) end
				function_name = GuessFunctionName(info)
				was_guessed = true
			else
				function_name = info.name
			end
			-- test if we have a file name
			local function_type = (info.namewhat == "") and "function" or info.namewhat
			if info.source and info.source:sub(1, 1) == "@" then
				message:add_f("(%d) Lua %s '%s' at file '%s:%d'%s\r\n", start_level, function_type, function_name, info.source:sub(2), info.currentline, was_guessed and " (best guess)" or "")
			elseif info.source and info.source:sub(1,1) == '#' then
				message:add_f("(%d) Lua %s '%s' at template '%s:%d'%s\r\n", start_level, function_type, function_name, info.source:sub(2), info.currentline, was_guessed and " (best guess)" or "")
			else
				message:add_f("(%d) Lua %s '%s' at line %d of chunk '%s'\r\n", start_level, function_type, function_name, info.currentline, source)
			end
			--stackTrace += messageLine;
			DumpLocals(start_level, message)
		else
			message:add_f("(%d) unknown frame %s\r\n", start_level, info.what)
		end
		
		start_level = start_level + 1
		info = debug_getinfo(start_level, "nSlf")
	end
	
	return table_concat(message), original_error
end

--
-- Adds a table to the list of known tables
function _M.add_known_table(tab, description)
	if m_known_tables[tab] then
		error("Cannot override an already known table")
	end
	m_user_known_tables[tab] = description
end

--
-- Adds a function to the list of known functions
function _M.add_known_function(fun, description)
	if m_known_functions[fun] then
		error("Cannot override an already known function")
	end
	m_user_known_functions[fun] = description
end

return _M
