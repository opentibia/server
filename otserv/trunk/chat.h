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
	ChatChannel(unsigned short channelId, std::string channelName);
	~ChatChannel(){}
	
	bool addUser(Player *player);
	bool removeUser(Player *player);
	
	bool talk(Player *fromPlayer, SpeakClasses type, std::string &text, unsigned short channelId);
	
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
	Chat();
	~Chat(){}
	ChatChannel *createChannel(unsigned short channelId, std::string channelName);
	bool deleteChannel(unsigned short channelId, std::string &channelName);
	
	bool addUserToChannel(Player *player, unsigned short channelId);
	bool removeUserFromChannel(Player *player, unsigned short channelId);
	void removeUserFromAllChannels(Player *player);
	
	bool talkToChannel(Player *player, SpeakClasses type, std::string &text, unsigned short channelId);	
	
	std::string getChannelName(Player *player, unsigned short channelId);	
	ChannelList getChannelList(Player *player);
	
private:
	ChatChannel *getChannel(unsigned short channelId, std::string channelName);
	ChatChannel *getChannel(unsigned short channelId, Player *player);
	
	typedef std::map<unsigned short, ChatChannel*> NormalChannelMap;
	typedef std::map<std::string, ChatChannel*> GuildChannelMap;
	NormalChannelMap m_normalChannels;
	GuildChannelMap m_guildChannels;
};

#endif
