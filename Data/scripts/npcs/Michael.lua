local Michael = NPC:new("Michael")

Michael.outfit = {
	type = 130,
	head = 20,
	body = 15,
	legs = 10,
	feet = 5
}

Michael.greeting = {"Hello $name.", "Greetings $name."}
Michael.farewell = {"Goodbye $sir $name."}
Michael.runoff = {"How rude!"}

Michael.idleTimeout = 30
Michael.listenRadius = 4
Michael.walkRadius = 3

--[[
Michael.dialog =
	{
		{ { "heal$", isBurning() }, { "You are burning, I will help you.", healBurning(), playerEffect(EFFECT_SOMETHING) } };
		{ { "heal$", isPoisoned() }, { "You are poisoned, I will help you.", healPoison(), playerEffect(EFFECT_SOMETHING) } };
		{ { "heal$", hasHealth(40) }, { "You are badly wounded, let me help you.", setHealth(40), playerEffect(EFFECT_SOMETHING) } };
		{ { "heal$", isPvpEnforced() }, "You aren't looking that bad." };
		{   "heal$", "You aren't looking that bad. Sorry, I can't help you. But if you are looking for additional protection you should go on the pilgrimage of ashes."};

		{ { "blessing", isPvpEnforced() }, "The lifeforce of this world is waning. There are no more blessings avaliable on this world."};
		{   "pilgrimage", last};
		
		{   "sacred+places", "Just ask in which of the five blessings you are interested in."};
		
		{ { "spiritual", hasStorage("quest_carpet") }, "I see you received the spiritual shielding in the whiteflower temple south of Thais."};
		{   "spiritual", "You can receive the spiritual shielding in the whiteflower temple south of Thais."};

		{   "arena", ask("Do you want to go to the arena?", 
			{
				{ isPzBlocked(), "First finish your current fight!" };
				{ {}, "Then off with you!", travel {2030, 2003, 7} };
			},
			{ "Then stay here in serenity." }
		) };

		{"job|occupation", "My job is to be awesome."};
		{"queue", function(self) return "There is " .. #self.queue .. " people in queue." end};
		{"count my gold", function(self) return "You have " .. self.focus:countMoney() .. " gold." end};
		{"monologue", {"This message is split into several parts and should be presented after one other.", "Here comes the next part.", "And the next."}};
		
		--{"ab'dendriel", travel{122, 322, 7}};
		{ "highest", ask("Do you wanna play highest number?",
			function(self)
				self:say("Awesome, let's do it. You go first.")
				local n = nil
				while not n do
					n = tonumber(self:listen())
					if not n then
						self:say("That's not a number!")
					end
				end
				self:say((n + 1) .. "!")
				wait(1000)
				self:say("Seems I won, suck it.")
			end,
			"Too bad, it's a fun game."
		) };
		
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
]]--

Michael.trade = {
	{"mace", id=2398, buy=90, sell=30};
	{"empty vial|vial", type=0, id=2006, sell=5, sell_phrase="Do you want to exchange all your $itemcount vials for $price gold?", all = true};
	{name="mana fluid", type=7, id=2006, buy=100};
	{name="cellar key", actionID=1234, id=2087, buy=800};
}

function Michael:onHear(message, class)
	self:say("I don't understand that.")
end
