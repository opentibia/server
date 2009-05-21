local Michael = NPC:new("Michael")

Michael.outfit = {
	type = 130,
	head = 20,
	body = 15,
	legs = 10,
	feet = 5
}

Michael.greeting = {"Hello $name.", "Greetings $name."}
Michael.farewell = "Goodbye $sir $name."

Michael.listen_radius = 5
Michael.walk_radius = 3

Michael.dialog = {
	["job|occupation"] = "My job is to be awesome.";
	["count my gold"] = function(self) return "You have " .. self.focus:countMoney() .. " gold." end;
	-- This is not working yet. :(
	["dialog"] = function(self)
		self:say("Do you want to continue this conversation?")
		local reply = self:listen()
		if containsAgreement(reply) then
			local n = 1
			function loop(self)
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
	end;
}

-- Trade is not working either
Michael.trade = {
	{"mace", id=2398, buy=90, sell=30};
}

function Michael:onHear(message, class)
	self:say("Awesome, you said '" .. message .. "'.")
end
