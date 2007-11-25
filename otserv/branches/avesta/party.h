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

#ifndef __OTSERV_PARTY_H__
#define __OTSERV_PARTY_H__

#include "definitions.h"
#include "const80.h"
#include "player.h"

#include <list>
typedef std::list<Player*> PlayerList;

class Party
{
public:
	Party(Player* _leader);
	~Party();

	Player* getLeader() const {return leader;}
	void setLeader(Player* _leader) {leader = _leader;}

	void disband();
	void invitePlayer(Player* player);
	void joinParty(Player* player);
	void revokeInvitation(Player* player);
	void passPartyLeadership(Player* player, bool isLogout = false);
	void leaveParty(Player* player, bool isLogout = false);

	bool isPlayerMember(const Player* player) const;
	void updatePartyIcons(Player* player, PartyShields_t shield);
	void updateInvitationIcons(Player* player, PartyShields_t shield);
	void broadcastPartyMessage(MessageClasses msgClass, const std::string& msg, bool sendToInvitations = false);
	void checkInvitationsAndMembers() { if(memberList.empty() && invitations.empty()) delete this; }

	PlayerList invitations;

protected:
	Player* leader;
	PlayerList memberList;
};

#endif
