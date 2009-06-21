local SOUNDS = {
	"Ho ho ho",
	"Jingle bells, jingle bells...",
	"Have you been naughty?",
	"Have you been nice?",
	"Merry Christmas!",
	"Can you stop squeezing me now... I'm starting to feel a little sick."
}

function onUse(cid, item, frompos, item2, topos)
	local random = math.random(1,6)
	local playerPos = getPlayerPosition(cid)

	doSendMagicEffect(playerPos, CONST_ME_SOUND_PURPLE)
	doPlayerSay(cid, SOUNDS[random], TALKTYPE_ORANGE_1)

	return TRUE
end
