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

#ifndef __CHAT_H
#define __CHAT_H

#include <map>

class ChatChannel
{
public:
	ChatChannel(unsigned short channelId, std::string channelName){
		m_id = channelId;
		m_name = channelName;
	};
	
	bool addUser(Player *player){
		UsersMap::iterator it = m_users.find(player->getID());
		if(it != m_users.end())
			return false;
		
		m_users[player->getID()] = player;
		return true;
	};
	
	bool removeUser(Player *player){
		UsersMap::iterator it = m_users.find(player->getID());
		if(it == m_users.end())
			return false;
		
		m_users.erase(it);
		return true;
	};
	
	bool talk(Player *fromPlayer, SpeakClasses type, std::string &text, unsigned short channelId){
		bool success = false;
		UsersMap::iterator it;
		//DONE?: Check player rights for the text "type" (the colour of messages)
		if(fromPlayer->access == 0){
			type = SPEAK_CHANNEL_Y;
		}
		for(it = m_users.begin(); it != m_users.end(); ++it)
		{
			Player *toPlayer = dynamic_cast<Player*>(it->second);
			if(toPlayer)
			{
				toPlayer->sendToChannel(fromPlayer, type, text, channelId);
				success = true;
			}
		}
		return success;
	};
	
	const std::string& getName(){ return m_name; };
	const unsigned short getId(){ return m_id; };
private:
	typedef std::map<long, Player*> UsersMap;
	UsersMap m_users;
	std::string m_name;
	unsigned short m_id;
};

typedef std::list<ChatChannel*> ChannelList;

class Chat
{
public:
	Chat(){
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
		
	};
	ChatChannel *createChannel(unsigned short channelId, std::string channelName){
		if(getChannel(channelId, channelName))
			return NULL;
		
		if(channelId == 0x00)
		{	
			ChatChannel *newChannel = new ChatChannel(channelId, channelName);
			if(!newChannel)
				return NULL;
		
			m_guildChannels[channelName] = newChannel;
			return newChannel;
		}
		
		return NULL;
	};
	bool deleteChannel(unsigned short channelId, std::string &channelName){
		if(channelId == 0x00)
		{
			GuildChannelMap::iterator it = m_guildChannels.find(channelName);
			if(it == m_guildChannels.end())
				return false;
			
			delete it->second;
			m_guildChannels.erase(it);
			return true;
		}
		
		return false;
	};
	
	bool addUserToChannel(Player *player, unsigned short channelId){
		std::string channelName;
		if(channelId == 0x00)
			channelName = player->getGuildName();
		
		ChatChannel *channel = getChannel(channelId, channelName);
		if(!channel)
			return false;
		
		if(channel->addUser(player))
			return true;
		else
			return false;
	};
	bool removeUserFromChannel(Player *player, unsigned short channelId){
		std::string channelName;
		if(channelId == 0x00)
			channelName = player->getGuildName();
	
		ChatChannel *channel = getChannel(channelId, channelName);
		if(!channel)
			return false;
		
		if(channel->removeUser(player))
			return true;
		else
			return false;	
	};
	
	void removeUserFromAllChannels(Player *player){
		ChannelList list = getChannelList(player);
		while(list.size())
		{
			ChatChannel *channel = list.front();
			list.pop_front();
			
			channel->removeUser(player);
		}
	};
	
	bool talkToChannel(Player *player, SpeakClasses type, std::string &text, unsigned short channelId){
		ChatChannel *channel = getChannel(channelId, player);
		if(!channel)
			return false;
		
		if(channel->talk(player, type, text, channelId))
			return true;
		else
			return false;
	};
	
	std::string getChannelName(Player *player, unsigned short channelId){
		std::string channelName;
		if(channelId == 0x00)
			channelName = player->getGuildName();
		
		ChatChannel *channel = getChannel(channelId, channelName);
		if(channel)
			return channel->getName();
		else
			return "";
	};
	
	ChannelList getChannelList(Player *player){
		ChannelList list;
		NormalChannelMap::iterator itn;
		
		if(player->getGuildName().length())
		{
			ChatChannel *channel = getChannel(0x00, player->getGuildName());
			if(channel)
				list.push_back(channel);
			else if(channel = createChannel(0x00, player->getGuildName()))
				list.push_back(channel);
		}
		
		for(itn = m_normalChannels.begin(); itn != m_normalChannels.end(); ++itn)
		{
			//TODO: Permisions for channels and checks
			ChatChannel *channel = itn->second;
			list.push_back(channel);
		}
		return list;
	};
private:
	ChatChannel *getChannel(unsigned short channelId, std::string channelName){
		if(channelId == 0x00)
		{
			GuildChannelMap::iterator it = m_guildChannels.find(channelName);
			if(it == m_guildChannels.end())
				return NULL;
			
			return it->second;
		}
		else
		{
			NormalChannelMap::iterator it = m_normalChannels.find(channelId);
			if(it == m_normalChannels.end())
				return NULL;
				
			return it->second;
		}
		return NULL;
	};
	ChatChannel *getChannel(unsigned short channelId, Player *player){
		if(channelId == 0x00)
		{
			std::string channelName = player->getGuildName();
			if(!channelName.length())
				return NULL;
				
			GuildChannelMap::iterator it = m_guildChannels.find(channelName);
			if(it == m_guildChannels.end())
				return NULL;
			
			return it->second;
		}
		else
		{
			NormalChannelMap::iterator it = m_normalChannels.find(channelId);
			if(it == m_normalChannels.end())
				return NULL;
				
			return it->second;
		}
	};
	
	typedef std::map<unsigned short, ChatChannel*> NormalChannelMap;
	typedef std::map<std::string, ChatChannel*> GuildChannelMap;
	NormalChannelMap m_normalChannels;
	GuildChannelMap m_guildChannels;
};

#endif
