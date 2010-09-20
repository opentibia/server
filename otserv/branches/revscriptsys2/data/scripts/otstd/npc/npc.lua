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

function NPC:runSuspendedState(event)
	local thread = self.state.thread
	local success, yieldtype, param = coroutine.resume(thread, self, event)

	if not success then
		local err = yieldtype
		print (debug.traceback(thread))-- "Error in NPC '" .. self:getName() .. "' : " .. err))

		-- Clear up state
		self.focus = nil
		self.state = nil
	elseif coroutine.status(thread) == "dead" then
		-- Thread finished, kill it
		self.state.thread = nil
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

	if not typeof(speaker, "Player") then
		return nil
	end

	local talking_to = (self.focusList[#speaker] ~= nil)

	if talking_to then
		-- We're talking to this player, alhough he may be one of many
		self.focus = speaker
		self.state = self.focusList[#speaker]

		if not self.state.thread then
			self.state.thread = coroutine.create(self.onHearFocusInternal)
		end
		self:runSuspendedState(event)

		-- If focus is nil now, we stopped talking to this player
		if self.focus == nil then
			self.focusList[#speaker] = nil
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
			self.focusList[#speaker] = self.state

			-- Set idle time
			self.state.lastSpoken = os.time();

			-- Free up var, else next script might be confused
			self.focus = nil
		end
		self.state = nil
	end
end

function NPC:onThinkHandler(event)
	for creatureID, state in pairs(self.focusList) do
		local creature = getThingByID(creatureID)
		if creature then
			-- Check if player has walked away
			if not areInRange(self:getPosition(), creature:getPosition(), self.listenRadius, self.listenRadius, 0) then
				self.focus = creature
				self.state = state

				self:onRunoffInternal()

				if self.focus == nil then
					self.focusList[creatureID] = nil
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
					self.focusList[creatureID] = nil
				end
				self.focus = nil
				self.state = nil
			end
		else -- Focus logged out (or something)
			self.state = state

			self:onRunoffInternal()

			self.focusList[creatureID] = nil
			self.state = nil
		end
	end

	local checked = {}
	while #checked ~= #self.queue do
		for i, creature in ipairs(self.queue) do
			if not #creature or not areInRange(self:getPosition(), creature:getPosition(), self.listenRadius, self.listenRadius, 0) then
				table.remove(self.queue, i)
				break
			elseif not table.find(checked, creature) then
				table.insert(checked, creature)

				-- Do we have a focus
				if #self.focusList == 0 then
					-- We don't have any focus! Accept this player
					table.remove(self.queue, i)

					self.focus = nil
					self.state = {} -- New clean state

					self:onGreetInternal(creature, "", TALKTYPE_SAY)

					if self.focus ~= nil then
						-- Acquire focus
						self.focusList[#speaker] = self.state

						-- Set idle time
						self.state.lastSpoken = os.time();

						-- Free up var, else next script might be confused
						self.focus = nil
					end

					-- No more handling after this
					return
				end
			end
		end
	end

	-- walk randomly
	if #self.focusList == 0 and
		self.walkRadius > 0 and
		os.time() - self.lastWalk > self.walkInterval then

		local doneWalking = false
		local tries = 0

		local toPos = nil
		local toDir = NORTH
		while not doneWalking and tries < 3 do
			-- increase tries
			tries = tries + 1

			-- get random direction
			toDir = math.random(0, 3)

			-- get new position
			toPos = self:getPosition()
			if toDir == NORTH:value() then
				toPos.y = toPos.y - 1
			elseif toDir == EAST:value() then
				toPos.x = toPos.x + 1
			elseif toDir == WEST:value() then
				toPos.x = toPos.x - 1
			elseif toDir ==  SOUTH:value() then
				toPos.y = toPos.y + 1
			end

			if math.abs(toPos.x - self.masterPos.x) > self.walkRadius or
				math.abs(toPos.y - self.masterPos.y) > self.walkRadius then
				-- somebody has change our position?
				self.masterPos = self:getPosition()
			else
				doneWalking = self:walk(Direction(toDir))
			end
		end

		self.lastWalk = os.time()
	end
end

-- Internal handlers for messages

function NPC:onHearFocusInternal(event)
	local text = event.text
	self.state.message = text

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
function NPC:onHearInternal(message, class)
	-- Trade logic
	if self.trade then
		if containsMessage(message, "sell") then
			for _, exchange in ipairs(self.trade) do
				if exchange.sell then
					if containsMessage(message, exchange[1] or exchange.name) then
						-- Ooo, the item we want to sell!
						local count = string.match(message, "%d+") or 1
						local params = {
							itemname = getItemNameByID(exchange.id, count);
							itemcount = count;
							price = exchange.sell * count;
							}

						self:say(self.exchangeSell or self.standardReplies.exchangeSell, params)
						local reply = self:listen()
						if containsAgreement(reply) then
							if self.focus:removeItem(exchange.id, exchange.type or -1, count) then
								self.focus:addMoney(exchange.sell * count)
								self:say(self.exchangeSellComplete or self.standardReplies.exchangeSellComplete, params)
							else
								self:say(self.exchangeSellNoItem or self.standardReplies.exchangeSellNoItem, params)
							end
							return
						end
						self:say(self.exchangeSellAborted or self.standardReplies.exchangeSellAborted, params)
						return
					end
				end
			end
		else
			for _, exchange in ipairs(self.trade) do
				if exchange.buy then
					if containsMessage(message, exchange[1] or exchange.name) then
						-- Ooo, the item we want to buy
						local count = string.match(message, "%d+") or 1
						local params = {
							itemname = getItemNameByID(exchange.id, count, exchange.name);
							itemcount = count;
							price = exchange.buy*count;
							}

						self:say(self.exchangeBuy or self.standardReplies.exchangeBuy, params)
						local reply = self:listen()
						if containsAgreement(reply) then
							-- Check if we have the money first
							if self.focus:hasMoney(exchange.buy * count) then

								-- Setup custom modfunc for extra attributes
								local modfunc = exchange.modFunction
								if exchange.actionID then
									-- If more attributes are added, they must be add inside this function, as else multiple attributes won't work together
									modfunc = function(item)
										item:setActionID(exchange.actionID)
										if exchange.modFunction then
											exchange.modFunction(item)
										end
									end
								end

								-- Add the item and remove the cash
								self.focus:addItemOfType(exchange.id, exchange.type, count, modfunc)
								self.focus:removeMoney(exchange.buy * count)

								-- Say that we completed the transaction
								self:say(self.exchangeBuyComplete or self.standardReplies.exchangeBuyComplete, params)
							else
								self:say(self.exchangeBuyNoCash or self.standardReplies.exchangeBuyNoCash, params)
							end
							return
						end
						self:say(self.exchangeBuyAborted or self.standardReplies.exchangeBuyAborted, params)
					end
				end
			end
		end
	end

	-- Wasn't trade, then it must be dialog!
	if self.dialog then
		for _, v in pairs(self.dialog) do
			local keyword = v[1] or v.keyword
			local reply = v[2] or v.reply
			if containsMessage(message, keyword) then
				self:say(reply)
				return
			end
		end
	end

	-- Not dialog either? Well, pass the message to the event handler
	self:onHear(message, class)
end

function NPC:onHearStrangerInternal(event)
	local text = event.text
	local has_focus = #self.focusList > 0

	if not areInRange(self:getPosition(), event.talking_creature:getPosition(), self.listenRadius, self.listenRadius, 0) then
		return
	end

	-- We don't have focus
	if containsGreeting(text) then
		if has_focus then
			self.focus = event.talking_creature
			self:onBusyInternal(event.talking_creature, text, event.class)
			self.focus = nil
		else
			self:onGreetInternal(event.talking_creature, text, event.class)
		end
	else
		-- Not a greeting, perhaps the NPC is interested anyways?
		self:onHearStranger(event.talking_creature, text, event.class)
	end
end

function NPC:onGreetInternal(creature, message, class)
	self.focus = creature
	self:onGreet(creature, message, class)
end

function NPC:onBusyInternal(creature, message, class)
	local in_queue = table.find(self.queue, creature) ~= nil
	if not in_queue then
		table.insert(self.queue, creature)
		self:onBusy(creature, message, class)
	end
end

function NPC:onFarewellInternal(creature, message, class)
	self:onFarewell(creature, message, class)
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

function NPC:onHearStranger(creature, message, class)
end

function NPC:onHear(message, class)
end

function NPC:onGreet(creature, message, class)
	self:say(self.greeting or self.standardReplies.greeting)
end

function NPC:onBusy(creature, message, class)
	self:say(self.busy or self.standardReplies.busy, {["customers"] = #self.queue})
end

function NPC:onFarewell(creature, message, class)
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
	if self.state.thread == nil then
		error "Npc.listen: This function can only be called inside an onHear event"
	end

	local npc, event = coroutine.yield("LISTEN")
	self.state.message = event.text
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
		message = message(self, extra_params)

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
			if type(f) == "function" then
				return f(self, self.focus)
			else
				return f
			end
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

	if not actor then
		return nil
	end

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
	npc.focusList = {}
	npc.queue = {}

	-- Listening
	npc.listenRadius = 4
	npc.idleTimeout = 30

	-- Walking
	npc.walkInterval = 2
	npc.walkRadius = 2
	npc.lastWalk = 0

	if not where then
		where = actor:getPosition()
	end
	npc.masterPos = where

	-- Setup apperance etc.
	actor:setName(npc.name) -- Case may differ
	actor:setNameDescription(npc.name) -- Case may differ
	actor:setAlwaysThink(true) -- we won't stop thinking even without any spectator
	actor:setOnlyThink(true) -- won't do anything else thinking (will give us full control)
	actor:setCanTarget(false) -- can't target anyone (this way we won't chase anyone around)
	actor:setShouldReload(true) -- npc will be reloaded

	npc.actor:setOutfit(npc.outfit or NPC.defaultOutfit)

	-- All NPCs need to hear
	npc.events.onHear = registerOnHear(actor,
		function(event)
			npc:onHearHandler(event)
		end
	)

	npc.events.onThink = registerOnCreatureThink(actor,
		function(event)
			--[[
			print(">> " .. collectgarbage("count") .. " \tKB")
			collectgarbage("collect")
			return
			]]
			npc:onThinkHandler(event)
		end
	)

	return npc
end
