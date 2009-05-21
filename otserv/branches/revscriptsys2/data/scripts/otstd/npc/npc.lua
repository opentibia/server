NPC = {}
NPCs = {}

require("otstd/npc/constants")

-- Create a new NPC AI
function NPC:new(name)
	npc = {}
	NPCs[name:lower()] = npc
	npc.name = name
	
	-- All NPCs created will be instanced with this AI
	registerOnSpawn(name, function(event)
			NPC:make(event.actor)
		end)
	
	return npc
end

-- Event handlers

function NPC:runSuspendedState(state, event)
	local thread = state.thread
	local success, yieldtype, param = coroutine.resume(state.thread, self, event)
	
	if not success then
		local err = yieldtype
		print (debug.traceback(state.thread, "Error in NPC '" .. self:getName() .. "' : " .. err))

		-- Clear up state
		self.focus = nil
		self.state = nil
	elseif coroutine.status(state.thread) == "dead" then
		-- Thread finished, kill it
		state.thread = nil
	else
		local focus = self.focus
	
		if yieldtype == "LISTEN" then
			assert(self.focus ~= nil, "Must have focus when calling self:listen")
			-- As we are state.thread, we don't have to do anything, it's handled in onHearHandler
		elseif yieldtype == "WAIT" then
			-- Clear up state
			self.focus = nil
			self.state = nil
			state.thread = nil
			
			-- Wait the alloted time
			wait(param)
			
			-- Resume state
			self.focus = focus
			self.state = state
			state.thread = thread
			
			return runSuspendedState(state, event)
		end
	end
end

function NPC:onHearHandler(event)
	local speaker = event.talking_creature
	local talking_to = (self.focus_list[#speaker] ~= nil)
	
	if talking_to then
		-- We're talking to this player, alhough he may be one of many
		self.focus = speaker
		self.state = self.focus_list[#speaker]
		
		if not self.state.thread then
			self.state.thread = coroutine.create(self.onHearFocusInternal)
		end
		self:runSuspendedState(self.state, event)
		
		-- If focus is nil now, we stopped talking to this player
		if self.focus == nil then
			self.focus_list[#speaker] = nil
		else
			-- Else we did react, set idle time
			self.state.lastSpoken = os.time()
		end
		
		self.focus = nil
		self.state = nil
	else
		-- We're currently not talking to this player
		
		self.focus = nil
		self.state = {} -- New clean state
		
		-- Call handler
		self:onHearStrangerInternal(event)
		
		-- If focus is no longer nil, we started talking to this player
		if self.focus ~= nil then
			-- Acquire focus
			self.focus_list[#speaker] = self.state
			
			-- Set idle time
			self.state.lastSpoken = os.time();
			
			-- Free up var, else next script might be confused
			self.focus = nil
		end
		self.state = nil
	end
end

function NPC:onThinkHandler(event)
	for creatureID, state in pairs(self.focus_list) do
		local creature = getThingByID(creatureID)
		if creature then
			-- Check if player has walked away
			if not areInRange(self:getPosition(), creature:getPosition(), self.listenRadius, self.listenRadius, 0) then
				self.focus = creature
				self.state = state
				
				self:onRunoffInternal()
				
				if self.focus == nil then
					self.focus_list[creatureID] = nil
				end
				self.focus = nil
				self.state = nil
			end
			
			-- Check if player haven't said anything for awhile
			--self:say("Thinking about you, it's been " .. (os.time() - (state.lastSpoken or 0)) .. " seconds since I last spoke")
			if (os.time() - (state.lastSpoken or 0)) > self.idleTimeout then
				self.focus = creature
				self.state = state
				
				self:onIdleInternal()
				
				if self.focus == nil then
					self.focus_list[creatureID] = nil
				end
				self.focus = nil
				self.state = nil
			end
		else -- Focus logged out (or something)
			self.state = state
			
			self:onRunoffInternal()
			
			self.focus_list[creatureID] = nil
			self.state = nil
		end
	end
end

-- Internal handlers for messages

function NPC:onHearFocusInternal(event)
	local text = event.text
	
	-- We have focus already
	if containsFarewell(text) then
		self:onFarewellInternal(event.talking_creature, text, event.class)
	else
		-- Not a farewell, parse the message
		self:onHearInternal(text, event.class)
	end
end

-- onHearInternal is responsible for the default keyword/trade handlers
-- 
function NPC:onHearInternal(text, class)
	if self.dialog then
		for keyword, reply in pairs(self.dialog) do
			--print (text .. " contains " .. keyword .. "?")
			if containsMessage(text, keyword) then
				self:say(reply)
				return
			end
		end
	end
	
	self:onHear(text, class)
end

function NPC:onHearStrangerInternal(event)
	local text = event.text
	
	-- We don't have focus
	if containsGreeting(text) then
		self:onGreetInternal(event.talking_creature, text, event.class)
	else
		-- Not a greeting, perhaps the NPC is interested anyways?
		self:onHearStranger(event.talking_creature, text, event.class)
	end
end

function NPC:onGreetInternal(creature, text, class)
	self.focus = creature
	self:onGreet(creature, text, class)
end

function NPC:onFarewellInternal(creature, text, class)
	self:onFarewell(creature, text, class)
	-- Drop focus
	self.focus = nil
end

function NPC:onIdleInternal()
	self:onIdle()
	-- Drop focus
	self.focus = nil
end

function NPC:onRunoffInternal()
	self:onRunoff()
	-- Drop focus
	self.focus = nil
end

-- Default handlers for messages

function NPC:onHearStranger(creature, text, class)
end

function NPC:onHear(text, class)
end

function NPC:onGreet(creature, text, class)
	self:say(self.greeting or self.standardReplies.greeting)
end

function NPC:onFarewell(creature, text, class)
	self:say(self.farewell or self.standardReplies.farewell)
end

function NPC:onIdle()
	self:say(self.bored or self.standardReplies.bored)
end

function NPC:onRunoff()
	-- You MUST check if focus is valid in this function, as the player might have disappeared entirely
	self:say(self.runoff or self.standardReplies.runoff)
end

-- Makes the NPC say some stuff

-- listen() returns the next thing a player says, it suspends execution until the player actually says anything
function NPC:listen()
	local npc, event = coroutine.yield("LISTEN")
	return event.text, event.class
end

function NPC:say(message, extra_params)
	if not message then
		error("Must supply a message string to NPC:say")
	end
	
	if type(message) == "table" then
		local total_chance = #message
		-- Count total chance
		for _, m in ipairs(message) do
			if type(m) == "table" then
				if m[1] then
					-- Discount the time it was already counted
					total_chance = total_chance + m[1] - 1
				end
			end
		end
		
		-- Pick a rondom sub-reply
		local rand_chance = math.random(1, total_chance)
		local counted_chance = 0
		for _, m in ipairs(message) do
			counted_chance = counted_chance + (m[1] or 1)
			if counted_chance >= rand_chance then
				-- Pick this one
				if type(m) == "table" then
					message = m[2] or m[1]
				else
					message = m
				end
				break
			end
		end
	end
	
	if type(message) == "function" then
		message = message(self)
		
		if message == nil then -- If message is nil, then it's a custom callback, and not simply a return msg function
			return
		end
	end

	function replace(str)
		local lstr = str:lower()
		
		local f = self.replacementStrings[lstr]
		if not f and extra_params then
			f = extra_params[lstr]
		end
		
		if f then
			return f(self, self.focus)
		end
		return nil
	end
	
	message = string.gsub(message, "$(%a+)", replace)
	
	if self.private_chat then
		-- Not implemented
	else
		self.actor:say(message)
	end
end

-- Make a instance of a NPC
function NPC:make(name_or_actor, where)
	local name = ""
	local actor = nil
	
	if where == nil then
		actor = name_or_actor
		name = actor:getName()
	else
		name = name_or_actor
	end
	
	name = name:strip_whitespace()
	
	local npc_type = NPCs[name:lower()]
	if not npc_type then
		return nil
	end
	
	-- Create the actor (if necessary)
	actor = actor or createActor(name, where)
	
	-- Create NPC
	local npc = {}
	npc.actor = actor
	
	-- Setup meta table for the NPC, so calls function correctly
	local meta = {
		-- We have quite alot of tables to check
		__index = function(o, idx) return npc_type[idx] or NPC[idx] or Actor[idx] end;
		-- We overload the ID function so it works correctly
		__len = function(o) return #o.actor end;
		-- When passed as an argument to a function, alias with the actor object
		__object = actor;
	}
	setmetatable(npc, meta)
	
	-- State tables
	npc.events = {}
	npc.focus_list = {}
	
	-- Setup apperance etc.
	actor:setName(npc.name) -- Case may differ
	actor:setNameDescription(npc.name) -- Case may differ
	actor:setAlwaysThink(true)
	actor:setShouldReload(true)
	
	npc.actor:setOutfit(npc.outfit or NPC.defaultOutfit)
	
	-- All NPCs need to hear
	npc.events.onHear = registerOnHear(actor, 
		function(event)
			npc:onHearHandler(event)
		end
	)
	
	npc.events.onThink = registerOnCreatureThink(actor, 
		function(event)
			npc:onThinkHandler(event)
		end
	)
	
	return npc
end
