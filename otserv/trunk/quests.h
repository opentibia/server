//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// Quests
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

#ifndef _QUESTS_H_
#define _QUESTS_H_

#include "definitions.h"
#include "player.h"
#include "networkmessage.h"
#include <list>
#include <string>

class MissionState;
class Mission;
class Quest;

typedef std::map<uint32_t, MissionState*> StateList;
typedef std::list<Mission*> MissionsList;
typedef std::list<Quest*> QuestsList;

class MissionState
{
public:
	MissionState(const std::string& _description, const uint32_t& _missionID);
	
	const uint32_t& getMissionID() const;
	const std::string& getMissionDescription() const;

private:
	const std::string description;
	const uint32_t missionID;
};

class Mission
{
public:
	Mission(const std::string& _missionName, const uint32_t& _storageID,
		const uint32_t& _startValue, const int32_t& _endValue);
	~Mission();
		
	bool isCompleted(Player* player) const;
	bool isStarted(Player* player) const;
	std::string getName(Player* player);
	std::string getDescription(Player* player);

	MissionState* mainState;
	StateList state;

private:
	std::string missionName;
	uint32_t storageID;
	uint32_t startValue;
	uint32_t endValue;
};

class Quest
{
public:
	Quest(const std::string& _name, const uint16_t& _id,
		const uint32_t& _startStorageID, const uint32_t& _startStorageValue);
	~Quest();

	bool isCompleted(Player* player);
	bool isStarted(Player* player) const;
	const uint16_t& getID() const;
	const std::string& getName() const;
	uint16_t getMissionsCount(Player* player) const;

	void addMission(Mission* mission);

	MissionsList::const_iterator getFirstMission() const;
	MissionsList::const_iterator getEndMission() const;

private:
	std::string name;
	uint16_t id;
	uint32_t startStorageID;
	uint32_t startStorageValue;
	MissionsList missions;
};

class Quests
{
	Quests();
public:
	~Quests();

	static Quests* getInstance();

	QuestsList::const_iterator getFirstQuest() const;
	QuestsList::const_iterator getEndQuest() const;

	bool loadFromXml(const std::string& _filename);
	Quest* getQuestByID(const uint16_t& id);
	uint16_t getQuestsCount(Player* player);
	bool reload();

private:
	std::string filename;
	QuestsList quests;
};

#endif
