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

#ifndef __OTSERV_CHAT_H__
#define __OTSERV_CHAT_H__

#include <map>
#include <list>
#include <string>

#include "const.h"
#include "definitions.h"

class Player;

typedef std::map<uint32_t, Player*> UsersMap;

class ChatChannel
{
public:
	ChatChannel(uint16_t channelId, std::string channelName);
	virtual ~ChatChannel(){}

	bool addUser(Player* player);
	bool removeUser(Player* player);

	bool talk(Player* fromPlayer, SpeakClasses type, const std::string& text);

	const std::string& getName(){ return m_name; }
	const uint16_t getId(){ return m_id; }
	const UsersMap getUsers(){ return m_users; }

	virtual const uint32_t getOwner(){ return 0; }

protected:
	UsersMap m_users;
	std::string m_name;
	uint16_t m_id;
};

class PrivateChatChannel : public ChatChannel
{
public:
	PrivateChatChannel(uint16_t channelId, std::string channelName);
	virtual ~PrivateChatChannel(){};

	virtual const uint32_t getOwner(){return m_owner;}
	void setOwner(uint32_t id){m_owner = id;}

	bool isInvited(const Player* player);

	void invitePlayer(Player* player, Player* invitePlayer);
	void excludePlayer(Player* player, Player* excludePlayer);

	bool addInvited(Player* player);
	bool removeInvited(Player* player);

	void closeChannel();

protected:
	typedef std::map<uint32_t, Player*> InvitedMap;

	InvitedMap m_invites;
	uint32_t m_owner;
};

typedef std::list<ChatChannel*> ChannelList;

class Chat
{
public:
	Chat();
	~Chat();
	ChatChannel* createChannel(Player* player, uint16_t channelId);
	bool deleteChannel(Player* player, uint16_t channelId);

	bool addUserToChannel(Player* player, uint16_t channelId);
	bool removeUserFromChannel(Player* player, uint16_t channelId);
	void removeUserFromAllChannels(Player* player);

	bool talkToChannel(Player* player, SpeakClasses type, const std::string& text, unsigned short channelId);

	std::string getChannelName(Player* player, uint16_t channelId);
	ChannelList getChannelList(Player* player);

	ChatChannel* getChannel(Player* player, uint16_t channelId);
	ChatChannel* getChannelById(uint16_t channelId);
	PrivateChatChannel* getPrivateChannel(Player* player);

private:

	typedef std::map<uint16_t, ChatChannel*> NormalChannelMap;
	typedef std::map<uint32_t, ChatChannel*> GuildChannelMap;
	NormalChannelMap m_normalChannels;
	GuildChannelMap m_guildChannels;

	typedef std::map<uint16_t, PrivateChatChannel*> PrivateChannelMap;
	PrivateChannelMap m_privateChannels;

	ChatChannel* dummyPrivate;
};

#endif
