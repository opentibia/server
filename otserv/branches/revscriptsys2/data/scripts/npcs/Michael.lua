local Michael = NPC:new("Michael")
--[[
Michael.outfit = {
	type = 130,
	head = 20,
	body = 15,
	legs = 10,
	feet = 5
}

Michael.greeting = {"Hello $name.", "Greetings $name."}
Michael.farewell = "Goodbye $sir $name."

--Michael.listen_radius = 4
--Michael.walk_radius = 3

Michael.dialog = {
	{"job|occupation", "My job is to be awesome."};
	{"queue", function(self) return "There is " .. #self.queue .. " people in queue." end};
	{"count my gold", function(self) return "You have " .. self.focus:countMoney() .. " gold." end};
	{"ab'dendriel", travel{122, 322, 7}};
	{"dialog", function(self)
		self:say("Do you want to continue this conversation?")
		local reply = self:listen()
		if containsAgreement(reply) then
			local n = 1
			local function loop(self)
				self:say("This is the " .. n .. " repeat of this phrase, want more?")
				n = n + 1
				local reply = self:listen()
				if containsAgreement(reply) then
					return loop(self)
				else
					self:say("Ok. Then let's talk about something else!")
				end
			end
			loop(self)
		else
			self:say("You didn't continue, resume normal conversation flow.")
		end
	end};
}

Michael.trade = {
	{"mace", id=2398, buy=90, sell=30};
	{"empty vial|vial", type=0, id=2006, sell=5};
	{name="mana fluid", type=7, id=2006, buy=100};
	{name="cellar key", actionID=1234, id=2087, buy=800};
}

function Michael:onHear(message, class)
	self:say("Awesome, you said '" .. message .. "'.")
end
--]]