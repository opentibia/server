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
		removeUser(excludePlayer);

		std::string msg;
		msg = excludePlayer->getName();
		msg += " has been excluded.";
		player->sendTextMessage(MSG_INFO_DESCR, msg.c_str());
		
		excludePlayer->sendClosePrivate(getId());
	}
}

void PrivateChatChannel::closeChannel()
{
	ChatChannel::UsersMap::iterator cit;
	for(cit = m_users.begin(); cit != m_users.end(); ++cit){
		Player* toPlayer = cit->second->getPlayer();
		if(toPlayer){
			toPlayer->sendClosePrivate(getId());
		}
	}	
}

ChatChannel::ChatChannel(uint16_t channelId, std::string channelName)
{
	m_id = channelId;
	m_name = channelName;
}
	
bool ChatChannel::addUser(Player* player)
{
	UsersMap::iterator it = m_users.find(player->getID());
	if(it != m_users.end())
		return false;
	
	m_users[player->getID()] = player;
	return true;
}

bool ChatChannel::removeUser(Player* player)
{
	UsersMap::iterator it = m_users.find(player->getID());
	if(it == m_users.end())
		return false;
		
	m_users.erase(it);
	return true;
}

bool ChatChannel::talk(Player* fromPlayer, SpeakClasses type, const std::string& text, uint16_t channelId)
{
	bool success = false;
	UsersMap::iterator it;
	
	for(it = m_users.begin(); it != m_users.end(); ++it){
		Player* toPlayer = it->second->getPlayer();
		if(toPlayer){
			toPlayer->sendToChannel(fromPlayer, type, text, channelId);
			success = true;
		}
	}
	return success;
}

Chat::Chat()
{
	// Create the default channels
	ChatChannel *newChannel;
		
	newChannel = new ChatChannel(0x04, "Game-Chat");
	if(newChannel)
		m_normalChannels[0x04] = newChannel;
		
	newChannel = new ChatChannel(0x05, "Trade");
	if(newChannel)
		m_normalChannels[0x05] = newChannel;
		
	newChannel = new ChatChannel(0x06, "RL-Chat");
	if(newChannel)
		m_normalChannels[0x06] = newChannel;
				
	newChannel = new ChatChannel(0x08, "Help");
	if(newChannel)
		m_normalChannels[0x08] = newChannel;
		
	newChannel = new PrivateChatChannel(0xFFFF, "Private Chat Channel");
	if(newChannel)
		dummyPrivate = newChannel;
	
}

ChatChannel* Chat::createChannel(Player* player, uint16_t channelId)
{
	if(getChannel(player, channelId))
		return NULL;
		
	if(channelId == 0x00){	
		ChatChannel *newChannel = new ChatChannel(channelId, player->getGuildName());
		if(!newChannel)
			return NULL;
		
		m_guildChannels[player->getGuildId()] = newChannel;
		return newChannel;
	}
	else if(channelId == 0xFFFF){
		//only 1 private channel for each player
		if(getPrivateChannel(player)){
			return NULL;
		}
		
		//find a free private channel slot
		for(uint16_t i = 100; i < 10000; ++i){
			if(m_privateChannels.find(i) == m_privateChannels.end()){
				PrivateChatChannel* newChannel = new PrivateChatChannel(i, player->getName() + "'s Channel");
				if(!newChannel)
					return NULL;
				
				newChannel->setOwner(player->getGUID());

				m_privateChannels[i] = newChannel;
				return newChannel;
			}
		}
	}
		
	return NULL;
}

bool Chat::deleteChannel(Player* player, uint16_t channelId)
{
	if(channelId == 0x00){
		GuildChannelMap::iterator it = m_guildChannels.find(player->getGuildId());
		if(it == m_guildChannels.end())
			return false;
			
		delete it->second;
		m_guildChannels.erase(it);
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
		
		if(channel->getOwner() == player->getGUID())
			deleteChannel(player, channel->getId());
	}
}

bool Chat::talkToChannel(Player* player, SpeakClasses type, const std::string& text, uint16_t channelId)
{
	ChatChannel *channel = getChannel(player, channelId);
	if(!channel)
		return false;
	
	//0x08 is the channelId of Help channel
	if(channelId == 0x08 && type == SPEAK_CHANNEL_Y && player->hasFlag(PlayerFlag_TalkOrangeHelpChannel)){
		type = SPEAK_CHANNEL_O;
	}
	
	if(channel->talk(player, type, text, channelId))
		return true;
	else
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
		ChatChannel *channel = getChannel(player, 0x00);
		if(channel)
			list.push_back(channel);
		else if((channel = createChannel(player, 0x00)))
			list.push_back(channel);
	}
				
	for(itn = m_normalChannels.begin(); itn != m_normalChannels.end(); ++itn){
		//TODO: Permisions for channels and checks
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

ChatChannel* Chat::getChannel(Player* player, uint16_t channelId)
{
	if(channelId == 0x00){
		GuildChannelMap::iterator it = m_guildChannels.find(player->getGuildId());
		if(it != m_guildChannels.end()){
			return it->second;
		}
		else{	
			return NULL;
		}
	}
	else{
		NormalChannelMap::iterator it = m_normalChannels.find(channelId);
		if(it != m_normalChannels.end()){
			return it->second;
		}
		else{
			PrivateChannelMap::iterator it = m_privateChannels.find(channelId);
			if(it != m_privateChannels.end()){
				return it->second;		
			}
			else{
				return NULL;	
			}
		}
	}
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
