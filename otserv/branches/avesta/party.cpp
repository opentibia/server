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

#include <sstream>

Party::Party(Player* _leader)
{
	if(_leader){
		setLeader(_leader);
		_leader->setParty(this);
		_leader->sendPlayerPartyIcons(_leader, SHIELD_YELLOW);
	}
}

Party::~Party()
{
	disband();
}

void Party::disband()
{
	//This function is only called when the party is deleted
	//And the party is deleted only when there are no members, nor invited players in it
	//So all we need is to remove the leader
	getLeader()->sendTextMessage(MSG_INFO_DESCR, "The party has been disbanded.");
	getLeader()->setParty(NULL);
	getLeader()->sendPlayerPartyIcons(getLeader(), SHIELD_NONE);
	setLeader(NULL);
}

void Party::invitePlayer(Player* player)
{
	if(!player || player->isRemoved()){
		checkInvitationsAndMembers();
		return;
	}

	std::stringstream ss;
	if(player->getParty()){
		ss << player->getName() << " is already in a party.";
		getLeader()->sendTextMessage(MSG_INFO_DESCR, ss.str());
		checkInvitationsAndMembers();
		return;
	}

	player->addPartyInvitation(this);

	ss << player->getName() << " has been invited.";
	getLeader()->sendTextMessage(MSG_INFO_DESCR, ss.str());

	ss.str("");
	ss << getLeader()->getName() << " has invited you to " <<
		(getLeader()->getSex() == PLAYERSEX_FEMALE ? "her" : "his") << " party.";
	player->sendTextMessage(MSG_INFO_DESCR, ss.str());
}

void Party::joinParty(Player* player)
{
	if(!player || player->isRemoved()){
		checkInvitationsAndMembers();
		return;
	}

	std::stringstream ss;
	ss << player->getName() << " has joined the party.";
	broadcastPartyMessage(MSG_INFO_DESCR, ss.str());

	player->clearPartyInvitations();
	player->setParty(this);
	memberList.push_back(player);
	updatePartyIcons(player, SHIELD_BLUE);

	ss.str("");
	ss << "You have joined " << getLeader()->getName() << "'s party.";
	player->sendTextMessage(MSG_INFO_DESCR, ss.str());
}

void Party::revokeInvitation(Player* player)
{
	if(!player || player->isRemoved()){
		checkInvitationsAndMembers();
		return;
	}

	player->removePartyInvitation(this);

	std::stringstream ss;
	ss << getLeader()->getName() << " has revoked " <<
		(getLeader()->getSex() == PLAYERSEX_FEMALE ? "her" : "his") << " invitation.";
	player->sendTextMessage(MSG_INFO_DESCR, ss.str());

	ss.str("");
	ss << "Invitation for " << player->getName() << " has been revoked.";
	getLeader()->sendTextMessage(MSG_INFO_DESCR, ss.str());

	checkInvitationsAndMembers();
}

void Party::passPartyLeadership(Player* player, bool isLogout /*= false*/)
{
	if(!player || getLeader() == player || !isPlayerMember(player)){
		checkInvitationsAndMembers();
		return;
	}

	memberList.remove(player); //Remove it before to broadcast the message correctly

	std::stringstream ss;
	ss << player->getName() << " is now the leader of the party.";
	broadcastPartyMessage(MSG_INFO_DESCR, ss.str(), true);

	Player* oldLeader = getLeader();
	setLeader(player);
	if(!isLogout){
		memberList.push_front(oldLeader);
		updateInvitationIcons(oldLeader, SHIELD_NONE);
		updatePartyIcons(oldLeader, SHIELD_BLUE);
	}
	updateInvitationIcons(oldLeader, SHIELD_YELLOW);
	updatePartyIcons(player, SHIELD_YELLOW);

	player->sendTextMessage(MSG_INFO_DESCR, "You are now the leader of the party.");
}

void Party::leaveParty(Player* player, bool isLogout /*= false*/)
{
	if(!player || !isPlayerMember(player) && getLeader() != player){
		checkInvitationsAndMembers();
		return;
	}

	if(getLeader() == player){
		Player* newLeader = memberList.front();
		passPartyLeadership(newLeader, isLogout);
	}

	//Since we already passed the leadership, we remove the player from the list
	memberList.remove(player);
	player->sendTextMessage(MSG_INFO_DESCR, "You have left the party.");
	player->setParty(NULL);
	updatePartyIcons(player, SHIELD_NONE);

	std::stringstream ss;
	ss << player->getName() << " has left the party.";
	broadcastPartyMessage(MSG_INFO_DESCR, ss.str());

	checkInvitationsAndMembers();
}

bool Party::isPlayerMember(const Player* player) const
{
	PlayerList::const_iterator it = std::find(memberList.begin(), memberList.end(), player);
	if(it != memberList.end()){
		return true;
	}

	return false;
}

void Party::updatePartyIcons(Player* player, PartyShields_t shield)
{
	if(!memberList.empty()){
		for(PlayerList::iterator it = memberList.begin(); it != memberList.end(); ++it){
			(*it)->sendPlayerPartyIcons(player, shield);
			if(shield != SHIELD_NONE){
				player->sendPlayerPartyIcons((*it), SHIELD_BLUE); //Members are blue
			}
			else{
				player->sendPlayerPartyIcons((*it), SHIELD_NONE);
			}
		}
	}
	getLeader()->sendPlayerPartyIcons(player, shield);

	if(shield != SHIELD_NONE){
		player->sendPlayerPartyIcons(getLeader(), SHIELD_YELLOW);
	}
	else{
		player->sendPlayerPartyIcons(getLeader(), SHIELD_NONE);
	}
	player->sendPlayerPartyIcons(player, shield);
}

void Party::updateInvitationIcons(Player* player, PartyShields_t shield)
{
	if(!invitations.empty()){
		for(PlayerList::iterator it = invitations.begin(); it != invitations.end(); ++it){
			(*it)->sendPlayerPartyIcons(player, shield);
			if(shield != SHIELD_NONE){
				player->sendPlayerPartyIcons((*it), SHIELD_WHITEBLUE);
			}
			else{
				player->sendPlayerPartyIcons((*it), SHIELD_NONE);
			}
		}
	}
}

void Party::broadcastPartyMessage(MessageClasses msgClass, const std::string& msg, bool sendToInvitations /*= false*/)
{
	PlayerList::iterator it;
	if(!memberList.empty()){
		for(it = memberList.begin(); it != memberList.end(); ++it){
			(*it)->sendTextMessage(msgClass, msg);
		}
	}
	getLeader()->sendTextMessage(msgClass, msg);

	if(sendToInvitations && !invitations.empty()){
		for(it = invitations.begin(); it != invitations.end(); ++it){
			(*it)->sendTextMessage(msgClass, msg);
		}
	}
}
