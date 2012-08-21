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

#ifndef __OTSERV_SPAWN_H__
#define __OTSERV_SPAWN_H__

#include <stdint.h>
#include <string>
#include <list>
#include <map>

// Forward declaration
class CreatureType;
class Spawn;
class Position;

typedef std::list<Spawn*> SpawnList;

struct spawnBlock_t{
	CreatureType* mType;
	Direction direction;
	Position pos;
	uint32_t interval;
	int64_t lastSpawn;
};

class Spawns{
public:
	Spawns();
	~Spawns();

	static Spawns* getInstance();

	bool isInZone(const Position& centerPos, int32_t radius, const Position& pos);

	bool loadFromXml(const std::string& datadir);
	void startup();
	void clear();

	bool isLoaded() const;
	bool isStarted() const;

private:
	SpawnList spawnList;

	bool loaded;
	bool started;
	std::string filename;
};

class Spawn{
public:
	Spawn(const Position& pos, int32_t radius);
	~Spawn();

	bool addMonster(const std::string& name, const Position& pos, Direction dir, uint32_t interval);
	bool addNPC(const std::string& name, const Position& pos, Direction dir);
	void removeMonster(Actor* monster);

	uint32_t getInterval() const;
	void startup();

	void startSpawnCheck();
	void stopEvent();

	bool isInSpawnZone(const Position& pos);
	void cleanup();

private:
	Position centerPos;
	int32_t radius;
	int32_t despawnRange;
	int32_t despawnRadius;

	//map of creatures in the spawn
	typedef std::map<uint32_t, spawnBlock_t> SpawnMap;
	SpawnMap spawnMap;

	//map of the spawned creatures
	typedef std::multimap<uint32_t, Actor*, std::less<uint32_t> > SpawnedMap;
	typedef SpawnedMap::value_type spawned_pair;
	SpawnedMap spawnedMap;

	uint32_t interval;
	uint32_t checkSpawnEvent;

	bool findPlayer(const Position& pos);
	bool spawnMonster(uint32_t spawnId, CreatureType* mType, const Position& pos, Direction dir, bool startup = false);
	void checkSpawn();
};

#endif
