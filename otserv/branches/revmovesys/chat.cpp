//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// base class for every creature
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

#include "chat.h"
#include "player.h"

ChatChannel::ChatChannel(unsigned short channelId, std::string channelName)
{
	m_id = channelId;
	m_name = channelName;
}
	
bool ChatChannel::addUser(Player *player)
{
	UsersMap::iterator it = m_users.find(player->getID());
	if(it != m_users.end())
		return false;
	
	m_users[player->getID()] = player;
	return true;
}
	
bool ChatChannel::removeUser(Player *player)
{
	UsersMap::iterator it = m_users.find(player->getID());
	if(it == m_users.end())
		return false;
		
	m_users.erase(it);
	return true;
}
	
bool ChatChannel::talk(Player *fromPlayer, SpeakClasses type, std::string &text, unsigned short channelId)
{
	bool success = false;
	UsersMap::iterator it;
	
	for(it = m_users.begin(); it != m_users.end(); ++it){
		Player *toPlayer = dynamic_cast<Player*>(it->second);
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
				
	newChannel = new ChatChannel(0x07, "Help");
	if(newChannel)
		m_normalChannels[0x07] = newChannel;
}

ChatChannel *Chat::createChannel(Player *player, unsigned short channelId)
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
		
	return NULL;
}

bool Chat::deleteChannel(Player *player, unsigned short channelId)
{
	if(channelId == 0x00){
		GuildChannelMap::iterator it = m_guildChannels.find(player->getGuildId());
		if(it == m_guildChannels.end())
			return false;
			
		delete it->second;
		m_guildChannels.erase(it);
		return true;
	}
		
	return false;
}
	
bool Chat::addUserToChannel(Player *player, unsigned short channelId)
{
	ChatChannel *channel = getChannel(player, channelId);
	if(!channel)
		return false;
		
	if(channel->addUser(player))
		return true;
	else
		return false;
}

bool Chat::removeUserFromChannel(Player *player, unsigned short channelId)
{
	ChatChannel *channel = getChannel(player, channelId);
	if(!channel)
		return false;
		
	if(channel->removeUser(player))
		return true;
	else
		return false;	
}
	
void Chat::removeUserFromAllChannels(Player *player)
{
	ChannelList list = getChannelList(player);
	while(list.size()){
		ChatChannel *channel = list.front();
		list.pop_front();
			
		channel->removeUser(player);
	}
}
	
bool Chat::talkToChannel(Player *player, SpeakClasses type, std::string &text, unsigned short channelId)
{
	ChatChannel *channel = getChannel(player, channelId);
	if(!channel)
		return false;
		
	if(channel->talk(player, type, text, channelId))
		return true;
	else
		return false;
}
	
std::string Chat::getChannelName(Player *player, unsigned short channelId)
{	
	ChatChannel *channel = getChannel(player, channelId);
	if(channel)
		return channel->getName();
	else
		return "";
}
	
ChannelList Chat::getChannelList(Player *player)
{
	ChannelList list;
	NormalChannelMap::iterator itn;
		
	// If has guild
	if(player->getGuildId() && player->getGuildName().length()){
		ChatChannel *channel = getChannel(player, 0x00);
		if(channel)
			list.push_back(channel);
		else if(channel = createChannel(player, 0x00))
			list.push_back(channel);
	}
		
	for(itn = m_normalChannels.begin(); itn != m_normalChannels.end(); ++itn){
		//TODO: Permisions for channels and checks
		ChatChannel *channel = itn->second;
		list.push_back(channel);
	}
	return list;
}

ChatChannel *Chat::getChannel(Player *player, unsigned short channelId)
{
	if(channelId == 0x00){	
		GuildChannelMap::iterator it = m_guildChannels.find(player->getGuildId());
		if(it == m_guildChannels.end())
			return NULL;
			
		return it->second;
	}
	else{
		NormalChannelMap::iterator it = m_normalChannels.find(channelId);
		if(it == m_normalChannels.end())
			return NULL;
				
		return it->second;
	}
}
