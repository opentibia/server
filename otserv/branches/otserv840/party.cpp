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

#include "party.h"
#include "player.h"
#include "game.h"
#include "chat.h"
#include "configmanager.h"

#include <sstream>

extern Game g_game;
extern Chat g_chat;
extern ConfigManager g_config;

Party::Party(Player* _leader)
{
	sharedExpActive = false;
	sharedExpEnabled = false;

	if(_leader){
		setLeader(_leader);
		_leader->setParty(this);
		_leader->sendPlayerPartyIcons(_leader);
	}
}

Party::~Party()
{
}

void Party::disband()
{
	g_chat.deleteChannel(this);

	getLeader()->sendTextMessage(MSG_INFO_DESCR, "Your party has been disbanded.");
	getLeader()->setParty(NULL);
	getLeader()->sendPlayerPartyIcons(getLeader());

	for(uint32_t i = 0; i < inviteList.size(); ++i){
		inviteList[i]->removePartyInvitation(this);
		inviteList[i]->sendPlayerPartyIcons(getLeader());
		inviteList[i]->sendPlayerPartyIcons(inviteList[i]);
		getLeader()->sendPlayerPartyIcons(inviteList[i]);
	}
	inviteList.clear();

	for(uint32_t i = 0; i < memberList.size(); ++i){
		memberList[i]->setParty(NULL);
		memberList[i]->sendPlayerPartyIcons(getLeader());
		memberList[i]->sendPlayerPartyIcons(memberList[i]);
		memberList[i]->sendTextMessage(MSG_INFO_DESCR, "Your party has been disbanded.");
		getLeader()->sendPlayerPartyIcons(memberList[i]);
	}
	memberList.clear();

	setLeader(NULL);
	delete this;
}

bool Party::invitePlayer(Player* player)
{
	if(!player || player->isRemoved()){
		return false;
	}

	if(std::find(inviteList.begin(), inviteList.end(), player) != inviteList.end()){
		//already on the invitation list
		return false;
	}

	inviteList.push_back(player);
	getLeader()->sendPlayerPartyIcons(player);
	player->sendPlayerPartyIcons(getLeader());
	player->addPartyInvitation(this);

	std::stringstream ss;
	
	if(inviteList.empty() && memberList.empty()) {
		ss << player->getName() << " has been invited.";
	} else {
		ss << player->getName() << " has been invited. Open the party channel to communicate with other members.";
	}
	getLeader()->sendTextMessage(MSG_INFO_DESCR, ss.str());

	ss.str("");
	ss << getLeader()->getName() << " has invited you to " <<
		(getLeader()->getSex() == PLAYERSEX_FEMALE ? "her" : "his") << " party.";
	player->sendTextMessage(MSG_INFO_DESCR, ss.str());
	return true;
}

bool Party::joinParty(Player* player)
{
	if(!player || player->isRemoved()){
		return false;
	}

	std::stringstream ss;
	ss << player->getName() << " has joined the party.";
	broadcastPartyMessage(MSG_INFO_DESCR, ss.str());

	memberList.push_back(player);
	player->setParty(this);

	PlayerVector::iterator it = std::find(inviteList.begin(), inviteList.end(), player);
	if(it != inviteList.end()){
		inviteList.erase(it);
	}
	player->removePartyInvitation(this);

	updateSharedExperience();
	updatePartyIcons(player);

	ss.str("");
	ss << "You have joined " << getLeader()->getName() << "'s party.";
	player->sendTextMessage(MSG_INFO_DESCR, ss.str());
	return true;
}

bool Party::removeInvite(Player* player)
{
	if(!player || player->isRemoved() || !isPlayerInvited(player)){
		return false;
	}

	PlayerVector::iterator it = std::find(inviteList.begin(), inviteList.end(), player);
	if(it != inviteList.end()){
		inviteList.erase(it);
	}
	player->removePartyInvitation(this);

	getLeader()->sendPlayerPartyIcons(player);
	player->sendPlayerPartyIcons(getLeader());

	if(disbandParty()){
		disband();
	}

	return true;
}

bool Party::revokeInvitation(Player* player)
{
	std::stringstream ss;
	ss << getLeader()->getName() << " has revoked " <<
		(getLeader()->getSex() == PLAYERSEX_FEMALE ? "her" : "his") << " invitation.";
	player->sendTextMessage(MSG_INFO_DESCR, ss.str());

	ss.str("");
	ss << "Invitation for " << player->getName() << " has been revoked.";
	getLeader()->sendTextMessage(MSG_INFO_DESCR, ss.str());
	removeInvite(player);

	return true;
}

bool Party::passPartyLeadership(Player* player)
{
	if(!player || getLeader() == player || !isPlayerMember(player)){
		return false;
	}

	 //Remove it before to broadcast the message correctly
	PlayerVector::iterator it = std::find(memberList.begin(), memberList.end(), player);
	if(it != memberList.end()){
		memberList.erase(it);
	}

	std::stringstream ss;
	ss << player->getName() << " is now the leader of the party.";
	broadcastPartyMessage(MSG_INFO_DESCR, ss.str(), true);

	Player* oldLeader = getLeader();
	setLeader(player);

	memberList.insert(memberList.begin(), oldLeader);

	updateSharedExperience();
	updatePartyIcons(oldLeader);
	updatePartyIcons(getLeader());
	updatePartyIcons(player);

	player->sendTextMessage(MSG_INFO_DESCR, "You are now the leader of the party.");
	return true;
}

bool Party::leaveParty(Player* player)
{
	if(!player){
		return false;
	}

	if(!isPlayerMember(player) && getLeader() != player){
		return false;
	}

	// Remove from chat
	PrivateChatChannel* pcc = static_cast<PrivateChatChannel*>(g_chat.getChannel(this));
	pcc->excludePlayer(NULL, player);


	bool hasNoLeader = false;
	if(getLeader() == player){
		if(!memberList.empty()){
			if(memberList.size() == 1 && inviteList.empty()){
				//just one member left, disband instead of passing leadership
				hasNoLeader = true;
			}
			else{
				passPartyLeadership(memberList.front());
			}
		}
		else{
			hasNoLeader = true;
		}
	}

	//since we already passed the leadership, we remove the player from the list
	PlayerVector::iterator it = std::find(memberList.begin(), memberList.end(), player);
	if(it != memberList.end()){
		memberList.erase(it);
	}

	it = std::find(inviteList.begin(), inviteList.end(), player);
	if(it != inviteList.end()){
		inviteList.erase(it);
	}

	player->setParty(NULL);
	player->sendTextMessage(MSG_INFO_DESCR, "You have left the party.");
	player->sendPlayerPartyIcons(player);

	updateSharedExperience();
	updatePartyIcons(player);
	clearPlayerPoints(player);

	std::stringstream ss;
	ss << player->getName() << " has left the party.";
	broadcastPartyMessage(MSG_INFO_DESCR, ss.str());

	if(hasNoLeader || disbandParty()){
		disband();
	}

	return true;
}

bool Party::isPlayerMember(const Player* player) const
{
	PlayerVector::const_iterator it = std::find(memberList.begin(), memberList.end(), player);
	if(it != memberList.end()){
		return true;
	}

	return false;
}

bool Party::isPlayerInvited(const Player* player) const
{
	PlayerVector::const_iterator it = std::find(inviteList.begin(), inviteList.end(), player);
	if(it != inviteList.end()){
		return true;
	}

	return false;
}

void Party::updatePartyIcons(Player* player)
{
	for(PlayerVector::iterator it = memberList.begin(); it != memberList.end(); ++it){
		(*it)->sendPlayerPartyIcons(player);
		player->sendPlayerPartyIcons((*it));
	}

	for(PlayerVector::iterator it = inviteList.begin(); it != inviteList.end(); ++it){
		(*it)->sendPlayerPartyIcons(player);
		player->sendPlayerPartyIcons((*it));
	}

	getLeader()->sendPlayerPartyIcons(player);
	player->sendPlayerPartyIcons(getLeader());
}

void Party::updateAllPartyIcons()
{
	for(PlayerVector::iterator it = memberList.begin(); it != memberList.end(); ++it){
		for(PlayerVector::iterator it2 = memberList.begin(); it2 != memberList.end(); ++it2){
			(*it)->sendPlayerPartyIcons((*it2));
		}

		(*it)->sendPlayerPartyIcons(getLeader());
		getLeader()->sendPlayerPartyIcons((*it));
	}

	for(PlayerVector::iterator it = inviteList.begin(); it != inviteList.end(); ++it){
		(*it)->sendPlayerPartyIcons(getLeader());
	}

	getLeader()->sendPlayerPartyIcons(getLeader());
}

void Party::broadcastPartyMessage(MessageClasses msgClass, const std::string& msg, bool sendToInvitations /*= false*/)
{
	PlayerVector::iterator it;
	if(!memberList.empty()){
		for(it = memberList.begin(); it != memberList.end(); ++it){
			(*it)->sendTextMessage(msgClass, msg);
		}
	}

	getLeader()->sendTextMessage(msgClass, msg);

	if(sendToInvitations && !inviteList.empty()){
		for(it = inviteList.begin(); it != inviteList.end(); ++it){
			(*it)->sendTextMessage(msgClass, msg);
		}
	}
}

void Party::broadcastLoot(Creature* creature, Container* corpse)
{
	std::ostringstream os;

	os << "Loot of " << creature->getName() << ": " << corpse->getContentDescription();

	ChatChannel* channel = g_chat.getChannel(this);
	if(channel)
		channel->sendInfo(SPEAK_CHANNEL_W, os.str(), std::time(NULL));
}

void Party::updateSharedExperience()
{
	if(sharedExpActive){
		bool result = canEnableSharedExperience();
		if(result != sharedExpEnabled){
			sharedExpEnabled = result;
			updateAllPartyIcons();
		}
	}
}

bool Party::setSharedExperience(Player* player, bool _sharedExpActive)
{
	if(!player || getLeader() != player){
		return false;
	}

	if(sharedExpActive == _sharedExpActive){
		return true;
	}

	sharedExpActive = _sharedExpActive;

	if(sharedExpActive){
		sharedExpEnabled = canEnableSharedExperience();
		if(sharedExpEnabled){
			getLeader()->sendTextMessage(MSG_INFO_DESCR, "Shared Experience is now active.");
		}
		else{
			getLeader()->sendTextMessage(MSG_INFO_DESCR, "Shared Experience has been activated," \
				" but some members of your party are inactive.");
		}
	}
	else{
		getLeader()->sendTextMessage(MSG_INFO_DESCR, "Shared Experience has been deactivated.");
	}

	updateAllPartyIcons();
	return true;
}

void Party::shareExperience(uint64_t experience)
{
	double member_factor = g_config.getNumber(ConfigManager::PARTY_MEMBER_EXP_BONUS);
	double xpgained = experience / (memberList.size() + 1) + experience * member_factor;

	if(xpgained < 0)
		return;
	uint64_t shareExp = (uint64_t)std::ceil(xpgained);
	
	for(PlayerVector::iterator it = memberList.begin(); it != memberList.end(); ++it){
		(*it)->onGainSharedExperience(shareExp);
	}

	getLeader()->onGainSharedExperience(shareExp);
}

bool Party::canUseSharedExperience(const Player* player) const
{
	uint32_t highestLevel = getLeader()->getLevel();
	for(PlayerVector::const_iterator it = memberList.begin(); it != memberList.end(); ++it){
		if( (*it)->getLevel() > highestLevel){
			highestLevel = (*it)->getLevel();
		}
	}

	uint32_t minLevel = (int32_t)std::ceil(((float)(highestLevel) * 2) / 3);
	if(player->getLevel() < minLevel){
		return false;
	}

	const Position& leaderPos = getLeader()->getPosition();
	const Position& memberPos = player->getPosition();

	if(!Position::areInRange<30,30,1>(leaderPos, memberPos)){
		return false;
	}

	if(!player->hasFlag(PlayerFlag_NotGainInFight)){
		//check if the player has healed/attacked anything recently
		CountMap::const_iterator it = pointMap.find(player->getID());
		if(it == pointMap.end()){
			return false;
		}

		uint64_t timeDiff = OTSYS_TIME() - it->second.ticks;
		if(timeDiff > g_game.getInFightTicks()){
			//player has not attacked or healed anyone for a period of infight ticks
			return false;
		}
	}

	return true;
}

bool Party::canEnableSharedExperience()
{
	if(!canUseSharedExperience(getLeader())){
		return false;
	}

	for(PlayerVector::iterator it = memberList.begin(); it != memberList.end(); ++it){
		if(!canUseSharedExperience(*it)){
			return false;
		}
	}

	return true;
}

void Party::addPlayerHealedMember(Player* player, uint32_t points)
{
	if(!player->hasFlag(PlayerFlag_NotGainInFight)){
		if(points > 0){
			CountMap::iterator it = pointMap.find(player->getID());
			if(it == pointMap.end()){
				CountBlock_t cb;
				cb.ticks = OTSYS_TIME();
				cb.totalHeal = points;
				cb.totalDamage = 0;
				pointMap[player->getID()] = cb;
			}
			else{
				it->second.totalHeal += points;
				it->second.ticks = OTSYS_TIME();
			}

			updateSharedExperience();
		}
	}
}

void Party::addPlayerDamageMonster(Player* player, uint32_t points)
{
	if(!player->hasFlag(PlayerFlag_NotGainInFight)){
		if(points > 0){
			CountMap::iterator it = pointMap.find(player->getID());
			if(it == pointMap.end()){
				CountBlock_t cb;
				cb.ticks = OTSYS_TIME();
				cb.totalDamage = points;
				cb.totalHeal = 0;
				pointMap[player->getID()] = cb;
			}
			else{
				it->second.totalDamage += points;
				it->second.ticks = OTSYS_TIME();
			}

			updateSharedExperience();
		}
	}
}

void Party::clearPlayerPoints(Player* player)
{
	CountMap::iterator it = pointMap.find(player->getID());
	if(it != pointMap.end()){
		pointMap.erase(it);
		updateSharedExperience();
	}
}

bool Party::canOpenCorpse(uint32_t ownerId)
{
	Player* player = g_game.getPlayerByID(ownerId);
	if(!player){
		return false;
	}

	if(leader->getID() == ownerId || isPlayerMember(player)){
		return true;
	}

	return false;
}
