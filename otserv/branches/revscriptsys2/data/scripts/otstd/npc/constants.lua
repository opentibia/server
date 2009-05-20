
-- Default configuration (all these can be overloaded)
NPC.idleTimeout = 5
NPC.listenRadius = 5
NPC.walkRadius = 3

NPC.defaultOutfit = {
	type = 130,
	head = 20,
	body = 15,
	legs = 10,
	feet = 5
}

-- Default phrases
	
NPC.greetings = {
	"hello",
	"hi",
	"greetings",
	"greeting",
	"hiho",
}
	
NPC.farewells = {
	"farewell",
	"bye",
	"goodbye",
}
	
NPC.agreements = {
	"yes",
	"agree",
}
	
NPC.disagreements = {
	"no",
	"never",
}

NPC.standardReplies = {
	["greeting"] = "Hello $name.";
	["farewell"] = "Goodbye $name.";
	["runoff"] = "What a rude one!";
	["bored"] = "I don't have time for this.";
	["busy"] = "Hello $name, there is $customers people in line before you.";
}

-- Default replacements, you can pass more as an optional argument to NPC:say
NPC.replacementStrings = {
	-- Attributes
	["name"] = function(npc, speaker) return speaker:getName() end;
	["vocation"] = function(npc, speaker) return speaker:getVocationName() end;
	
	-- Titles
	["lord"] = function(npc, speaker) return speaker:getSex() == MALE and "lord" or "lady"  end;
	["lady"] = function(npc, speaker) return speaker:getSex() == MALE and "lord" or "lady"  end;
	
	["lass"] = function(npc, speaker) return speaker:getSex() == MALE and "lad" or "lass"  end;
	["lad"]  = function(npc, speaker) return speaker:getSex() == MALE and "lad" or "lass"  end;
	
	["sir"]  = function(npc, speaker) return speaker:getSex() == MALE and "sir" or "madam"  end;
	["madam"]= function(npc, speaker) return speaker:getSex() == MALE and "sir" or "madam"  end;
}

-- Functions
function containsMessage(msg, what)
	msg = msg:lower()
	
	-- Should be replaced by a more complete parser
	if string.find(msg, "|") ~= nil then
		what = what:explode("|")
	end
	
	-- Check recursively if what is a table
	if type(what) == "table" then
		for _, v in ipairs(what) do
			if containsMessage(msg, v) then
				return true
			end
		end
		return false
	end
	
	what = string.gsub(what, "[%%%^%$%(%)%.%[%]%*%+%-%?]", function(s) return "%" .. s end)
	return string.match(msg, what) ~= nil
end

function containsGreeting(msg)
	return containsMessage(msg, NPC.greetings)
end

function containsFarewell(msg)
	return containsMessage(msg, NPC.farewells)
end

function containsAgreement(msg)
	return containsMessage(msg, NPC.agreements)
end

function containsDisagreement(msg)
	return containsMessage(msg, NPC.disagreements)
end
