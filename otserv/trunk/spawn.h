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


#ifndef __SPAWN_H
#define __SPAWN_H

#include "tile.h"
#include "position.h"
#include "monster.h"
#include "templates.h"

#include <vector>
#include <map>

class Game;
class Spawn;
typedef std::list<Spawn*> spawnsList;

class Spawn /*: public Event*/ {
public:
	Spawn(Game *igame, Position pos, int _radius);
	void idle(int t);
	bool addMonster(std::string name, Direction dir, int x, int y, int spawntime);

public:
	bool startup();

	/*
	virtual void onCreatureEnter(const Creature *creature, const Position &pos);
	virtual void onCreatureLeave(const Creature *creature, const Position &pos);
	*/

private:
	Game *game;
	Position centerPos;
	int radius;

	bool isInSpawnRange(const Position &pos);
	Monster* respawn(unsigned long spawnid, Position &pos, std::string &name, Direction dir);

	struct spawninfo {
		Position pos;
		std::string name;
		Direction dir;
		int spawntime;
		uint64_t lastspawn;
	};

	//List of monsters in the spawn
	typedef std::map<unsigned long, struct spawninfo> SpawnMap;
	SpawnMap spawnmap;

	//For spawned monsters
	typedef std::multimap<unsigned long, Monster*, std::less<unsigned long> > SpawnedMap;
	typedef SpawnedMap::value_type spawned_pair;
	SpawnedMap spawnedmap;
};

class SpawnManager {
public:
	SpawnManager();
	~SpawnManager();
	
	static SpawnManager* instance();
	static bool initialize(Game *igame);
	static bool addSpawn(Spawn* spawn);
	static bool loadSpawnsXML(std::string filename);
#ifdef __USE_MYSQL__
	static bool loadSpawnsSQL(std::string identifier);
#endif
	static bool startup();

	void checkSpawns(int t);
protected:
	static SpawnManager* _instance;
	static spawnsList spawns;
	static Game *game;
};

#endif
