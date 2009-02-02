//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////
#include "otpch.h"

#include "chat.h"
#include "player.h"
#include "game.h"

// Avoid unnecessary includes!
extern void g_gameUnscript(void* v);
extern void g_gameOnLeaveChannel(Player* player, ChatChannel* channel);

PrivateChatChannel::PrivateChatChannel(uint16_t channelId, std::string channelName) :
ChatChannel(channelId, channelName)
{
	m_owner = 0;
}

bool PrivateChatChannel::isInvited(const Player* player)
{
	if(!player)
		return false;

	if(player->getGUID() == getOwner())
		return true;

	InvitedMap::iterator it = m_invites.find(player->getGUID());
	if(it != m_invites.end())
		return true;

	return false;
}

bool PrivateChatChannel::addInvited(Player* player)
{
	InvitedMap::iterator it = m_invites.find(player->getGUID());
	if(it != m_invites.end())
		return false;

	m_invites[player->getGUID()] = player;

	return true;
}

bool PrivateChatChannel::removeInvited(Player* player)
{
	InvitedMap::iterator it = m_invites.find(player->getGUID());
	if(it == m_invites.end())
		return false;

	m_invites.erase(it);
	return true;
}

void PrivateChatChannel::invitePlayer(Player* player, Player* invitePlayer)
{
	if(player != invitePlayer && addInvited(invitePlayer)){
		std::string msg;
		msg = player->getName();
		msg += " invites you to ";
		msg += (player->getSex() == PLAYERSEX_FEMALE ? "her" : "his");
		msg += " private chat channel.";
		invitePlayer->sendTextMessage(MSG_INFO_DESCR, msg.c_str());

		msg = invitePlayer->getName();
		msg += " has been invited.";
		player->sendTextMessage(MSG_INFO_DESCR, msg.c_str());
	}
}

void PrivateChatChannel::excludePlayer(Player* player, Player* excludePlayer)
{
	if(player != excludePlayer && removeInvited(excludePlayer)){

		if(player) {
			std::string msg;
			msg = excludePlayer->getName();
			msg += " has been excluded.";
			player->sendTextMessage(MSG_INFO_DESCR, msg.c_str());
		}

		removeUser(excludePlayer, true);
	}
}

void PrivateChatChannel::closeChannel()
{
	UsersMap::iterator cit;
	for(cit = m_users.begin(); cit != m_users.end(); ++cit){
		cit->second->sendClosePrivate(getId());
	}
}

ChatChannel::ChatChannel(uint16_t channelId, std::string channelName)
{
	m_id = channelId;
	m_name = channelName;
}

ChatChannel::~ChatChannel()
{
	g_gameUnscript(this);
}

bool ChatChannel::addUser(Player* player)
{
	UsersMap::iterator it = m_users.find(player->getID());
	if(it != m_users.end())
		return false;

	if(getId() == 0x03 && !player->hasFlag(PlayerFlag_CanAnswerRuleViolations)){ //Rule Violations channel
		return false;
	}

	m_users[player->getID()] = player;

	return true;
}

bool ChatChannel::removeUser(Player* player, bool sendCloseChannel /*= false*/)
{
	UsersMap::iterator it = m_users.find(player->getID());
	if(it == m_users.end())
		return false;

	m_users.erase(it);

	if(sendCloseChannel){
		player->sendClosePrivate(getId());
	}

	return true;
}

bool ChatChannel::talk(Player* fromPlayer, SpeakClass type, const std::string& text, uint32_t time /*= 0*/)
{
	bool success = false;
	UsersMap::iterator it;

	// Can't speak to a channel you're not connected to
	UsersMap::const_iterator iter = m_users.find(fromPlayer->getID());
	if(iter == m_users.end())
		return false;

	for(it = m_users.begin(); it != m_users.end(); ++it){
		it->second->sendToChannel(fromPlayer, type, text, getId(), time);
		success = true;
	}
	return success;
}

bool ChatChannel::sendInfo(SpeakClasses type, const std::string& text, uint32_t time)
{
	bool success = false;
	UsersMap::iterator it;

	for(it = m_users.begin(); it != m_users.end(); ++it){
		it->second->sendToChannel(NULL, type, text, getId(), time);
		success = true;
	}
	return success;
}

Chat::Chat()
{
	// Create the default channels
	m_normalChannels[0x03] = new ChatChannel(0x03, "Rule Violations");
	m_normalChannels[0x04] = new ChatChannel(0x04, "Game-Chat");
	m_normalChannels[0x05] = new ChatChannel(0x05, "Trade");
	m_normalChannels[0x06] = new ChatChannel(0x06, "RL-Chat");
	m_normalChannels[0x08] = new ChatChannel(0x08, "Help");

	dummyPrivate = new PrivateChatChannel(0xFFFF, "Private Chat Channel");
}

Chat::~Chat()
{
	delete dummyPrivate;

	for(NormalChannelMap::iterator it = m_normalChannels.begin(); it != m_normalChannels.end(); ++it){
		delete it->second;
	}
	m_normalChannels.clear();

	for(GuildChannelMap::iterator it = m_guildChannels.begin(); it != m_guildChannels.end(); ++it){
		delete it->second;
	}
	m_guildChannels.clear();

	for(PartyChannelMap::iterator it = m_partyChannels.begin(); it != m_partyChannels.end(); ++it){
		delete it->second;
	}
	m_partyChannels.clear();

	for(PrivateChannelMap::iterator it = m_privateChannels.begin(); it != m_privateChannels.end(); ++it){
		delete it->second;
	}
	m_privateChannels.clear();
}

uint16_t Chat::getFreePrivateChannelId() {
	for(uint16_t i = 100; i < 10000; ++i){
		if(m_privateChannels.find(i) == m_privateChannels.end()){
			return i;
		}
	}
	return 0;
}

ChatChannel* Chat::createChannel(Player* player, uint16_t channelId)
{
	if(getChannel(player, channelId))
		return NULL;

	if(channelId == CHANNEL_GUILD){
		ChatChannel *newChannel = new ChatChannel(channelId, player->getGuildName());
		m_guildChannels[player->getGuildId()] = newChannel;
		return newChannel;
	}
	else if(channelId == CHANNEL_PARTY){
		if(player->getParty() == NULL)
			return NULL;

		PrivateChatChannel *newChannel = new PrivateChatChannel(channelId, "Party");
		m_partyChannels[player->getParty()] = newChannel;
		return newChannel;
	}
	else if(channelId == CHANNEL_PRIVATE){
		// Private chat channel

		//only 1 private channel for each player
		if(getPrivateChannel(player)){
			return NULL;
		}

		//find a free private channel slot
		uint32_t i = getFreePrivateChannelId();
		if(i != 0) {
			PrivateChatChannel* newChannel = new PrivateChatChannel(i, player->getName() + "'s Channel");

			newChannel->setOwner(player->getGUID());

			m_privateChannels[i] = newChannel;
			return newChannel;
		}
	}

	return NULL;
}

bool Chat::deleteChannel(Player* player, uint16_t channelId)
{
	if(channelId == CHANNEL_GUILD){
		GuildChannelMap::iterator it = m_guildChannels.find(player->getGuildId());
		if(it == m_guildChannels.end())
			return false;

		delete it->second;
		m_guildChannels.erase(it);
		return true;
	}
	else if(channelId == CHANNEL_PARTY){
		PartyChannelMap::iterator it = m_partyChannels.find(player->getParty());
		if(it == m_partyChannels.end())
			return false;
		it->second->closeChannel();
		delete it->second;
		m_partyChannels.erase(it);
		return true;
	}
	else{
		PrivateChannelMap::iterator it = m_privateChannels.find(channelId);
		if(it == m_privateChannels.end())
			return false;

		it->second->closeChannel();

		delete it->second;
		m_privateChannels.erase(it);
		return true;
	}

	return false;
}

bool Chat::deleteChannel(Party* party)
{
	PartyChannelMap::iterator it = m_partyChannels.find(party);
	if(it == m_partyChannels.end())
		return false;

	PrivateChatChannel* cc = it->second;
	cc->closeChannel();
	m_partyChannels.erase(it);
	delete cc;
	return true;
}

bool Chat::addUserToChannel(Player* player, uint16_t channelId)
{
	ChatChannel *channel = getChannel(player, channelId);
	if(!channel)
		return false;

	if(channel->addUser(player))
		return true;
	else
		return false;
}

bool Chat::removeUserFromChannel(Player* player, uint16_t channelId)
{
	ChatChannel *channel = getChannel(player, channelId);
	if(!channel)
		return false;

	if(channel->removeUser(player)){
		if(channel->getOwner() == player->getGUID())
			deleteChannel(player, channelId);

		return true;
	}
	else
		return false;
}

void Chat::removeUserFromAllChannels(Player* player)
{
	ChannelList list = getChannelList(player);
	while(list.size()){
		ChatChannel *channel = list.front();
		list.pop_front();

		channel->removeUser(player);

		g_gameOnLeaveChannel(player, channel);

		if(channel->getOwner() == player->getGUID())
			deleteChannel(player, channel->getId());
	}
}

bool Chat::talkToChannel(Player* player, SpeakClass type, const std::string& text, uint16_t channelId)
{
	ChatChannel *channel = getChannel(player, channelId);
	if(!channel) {
		return false;
	}

	switch(channelId){
		case CHANNEL_HELP:
		{
			// Help channel
			if(type == SPEAK_CHANNEL_Y && player->hasFlag(PlayerFlag_TalkOrangeHelpChannel)){
				type = SPEAK_CHANNEL_O;
			}
			break;
		}
		default:
		{
			break;
		}
	}

	if(channel->talk(player, type, text)){
		return true;
	}

	return false;
}

std::string Chat::getChannelName(Player* player, uint16_t channelId)
{
	ChatChannel *channel = getChannel(player, channelId);
	if(channel)
		return channel->getName();
	else
		return "";
}

ChannelList Chat::getChannelList(Player* player)
{
	ChannelList list;
	NormalChannelMap::iterator itn;
	PrivateChannelMap::iterator it;
	bool gotPrivate = false;

	// If has guild
	if(player->getGuildId() && player->getGuildName().length()){
		ChatChannel *channel = getChannel(player, CHANNEL_GUILD);

		if(channel)
			list.push_back(channel);
		else if((channel = createChannel(player, CHANNEL_GUILD)))
			list.push_back(channel);
	}

	if(player->getParty()){
		ChatChannel *channel = getChannel(player, CHANNEL_PARTY);

		if(channel)
			list.push_back(channel);
		else if((channel = createChannel(player, CHANNEL_PARTY)))
			list.push_back(channel);
	}

	for(itn = m_normalChannels.begin(); itn != m_normalChannels.end(); ++itn){
		if(itn->first == CHANNEL_RULE_REP && !player->hasFlag(PlayerFlag_CanAnswerRuleViolations)){ //Rule violations channel
			continue;
		}

		ChatChannel *channel = itn->second;
		list.push_back(channel);
	}

	for(it = m_privateChannels.begin(); it != m_privateChannels.end(); ++it){
		PrivateChatChannel* channel = it->second;

		if(channel){
			if(channel->isInvited(player))
				list.push_back(channel);

			if(channel->getOwner() == player->getGUID())
				gotPrivate = true;
		}
	}

	if(!gotPrivate)
		list.push_front(dummyPrivate);

	return list;
}

ChatChannel* Chat::getChannel(Party* party)
{
	PartyChannelMap::iterator git = m_partyChannels.find(party);
	if(git != m_partyChannels.end()){
		return git->second;
	}
	return NULL;
}

ChatChannel* Chat::getChannel(Player* player, uint16_t channelId)
{
	if(channelId == CHANNEL_GUILD){
		GuildChannelMap::iterator git = m_guildChannels.find(player->getGuildId());
		if(git != m_guildChannels.end()){
			return git->second;
		}
		return NULL;
	}
	else if(channelId == CHANNEL_PARTY){
		if(player->getParty() == NULL) {
			return NULL;
		}

		PartyChannelMap::iterator git = m_partyChannels.find(player->getParty());
		if(git != m_partyChannels.end()){
			return git->second;
		}
		return NULL;
	}

	NormalChannelMap::iterator nit = m_normalChannels.find(channelId);
	if(nit != m_normalChannels.end()){
		if(channelId == CHANNEL_RULE_REP && !player->hasFlag(PlayerFlag_CanAnswerRuleViolations)){ //Rule violations channel
			return NULL;
		}

		return nit->second;
	}

	PrivateChannelMap::iterator pit = m_privateChannels.find(channelId);
	if(pit != m_privateChannels.end()){
		return pit->second;
	}

	return NULL;
}

ChatChannel* Chat::getChannelById(uint16_t channelId)
{
	NormalChannelMap::iterator it = m_normalChannels.find(channelId);
	if(it != m_normalChannels.end()){
		return it->second;
	}

	return NULL;
}

PrivateChatChannel* Chat::getPrivateChannel(Player* player)
{
	for(PrivateChannelMap::iterator it = m_privateChannels.begin(); it != m_privateChannels.end(); ++it){
		if(PrivateChatChannel* channel = it->second){
			if(channel->getOwner() == player->getGUID()){
				return channel;
			}
		}
	}

	return NULL;
}
