//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// the map of OpenTibia
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


#ifndef __OTSERV_MAP_H
#define __OTSERV_MAP_H


#include <queue>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "position.h"
#include "item.h"
#include "creature.h"

#include "otsystem.h"

#include "scheduler.h"
#include "networkmessage.h"


class Creature;   // see creature.h
class Player;



#define MAP_WIDTH    512
#define MAP_HEIGHT   512
#define MAP_LAYER     16


class Tile;

//struct effectInfo {
class EffectInfo
{
public:
	//Position GetViewState(Player *spectator);
	//createMagicEffect(Player *spectator
	//CanSee(Player *spectator);
//private:
	Position centerpos;
	unsigned char areaEffect;
	unsigned char damageEffect;
	unsigned char animationEffect;
	unsigned char area[14][18];
	unsigned char direction;
	unsigned char animationcolor;
	int manaCost;
	int minDamage;
	int maxDamage;
	int minManaDrain;
	int maxManaDrain;
	bool offensive;
	bool needtarget;
};

struct AStarNode{
	int x,y;
	AStarNode* parent;
	float f, g, h;
	bool operator<(const AStarNode &node){return this->h < node.h;}
};

template<class T> class lessPointer : public std::binary_function<T*, T*, bool> {
		  public:
		  bool operator()(T*& t1, T*& t2) {
				return *t1 < *t2;
		  }
};

class Map {
  public:
    Map();
    ~Map();
		
    bool LoadMap(std::string filename);

    Tile* getTile(unsigned short _x, unsigned short _y, unsigned char _z);

    std::map<long, Creature*> playersOnline;

    bool placeCreature(Creature* c);
    bool removeCreature(Creature* c);

    void thingMove(Creature *player, Thing *thing,
        unsigned short to_x, unsigned short to_y, unsigned char to_z);

    void thingMove(Creature *player,
        unsigned short from_x, unsigned short from_y, unsigned char from_z,
        unsigned char stackPos,
        unsigned short to_x, unsigned short to_y, unsigned char to_z);

    void creatureTurn(Creature *creature, Direction dir);

    void creatureSay(Creature *creature, unsigned char type, const std::string &text);
    void creatureWhisper(Creature *creature, const std::string &text);
    void creatureYell(Creature *creature, std::string &text);
    void creatureSpeakTo(Creature *creature, const std::string &receiver, const std::string &text);
    void creatureBroadcastMessage(Creature *creature, const std::string &text);
	  void creatureChangeOutfit(Creature *creature);

	  //void playerCastSpell(Creature *creature, const std::string &text);
		//void makeCastSpell(Creature *creature, int mana, int mindamage, int maxdamage, unsigned char area[14][18], unsigned char ch, unsigned char typeArea, unsigned char typeDamage);
	  void creatureThrowRune(Creature *creature, const EffectInfo &ei);
  	void creatureCastSpell(Creature *creature, const EffectInfo &ei);
		bool creatureSaySpell(Creature *creature, const std::string &text);
     void changeOutfitAfter(unsigned long id, int looktype, long time);
    //void addEvent(long ticks, int type, void *data);
	  void addEvent(SchedulerTask*);
   
   
    Creature* getCreatureByID(unsigned long id);
		Creature* getCreatureByName(const char* s);

	std::list<Position> getPathTo(Position start, Position to, bool creaturesBlock=true);

    OTSYS_THREAD_LOCKVAR mapLock;
  protected:
    // use this internal function to move things around to avoid the need of
    // recursive locks
    void thingMoveInternal(Creature *player,
        unsigned short from_x, unsigned short from_y, unsigned char from_z,
        unsigned char stackPos,
        unsigned short to_x, unsigned short to_y, unsigned char to_z);

        void changeOutfit(unsigned long id, int looktype);
		bool creatureOnPrepareAttack(Creature *creature, Position pos);
		void creatureMakeDamage(Creature *creature, Creature *attackedCreature, fight_t damagetype);
		void creatureMakeMagic(Creature *creature, const EffectInfo &ei);
		void creatureMakeAreaEffect(Creature *spectator, Creature *attacker, const EffectInfo &ei, NetworkMessage& msg);
	  void CreateDamageUpdate(Creature* player, Creature* attackCreature, int damage, NetworkMessage& msg);
	  //void getSpectators(const Position& pos, std::vector<Player*>& list);

    OTSYS_THREAD_LOCKVAR eventLock;
	  OTSYS_THREAD_SIGNALVAR eventSignal;

    static OTSYS_THREAD_RETURN eventThread(void *p);

    struct MapEvent
    {
      __int64  tick;
      int      type;
      void*    data;
    };

    void checkPlayerAttacking(unsigned long id);
    void checkPlayer(unsigned long id);
    void decayItem(Item* item);
    void decaySplash(Item* item);

	 std::priority_queue<SchedulerTask*, std::vector<SchedulerTask*>, lessSchedTask > eventList;
    //list<MapEvent> *eventLists[12000];

    int loadMapXml(const char *filename);


    typedef std::map<unsigned long, Tile*> TileMap;
    TileMap tileMaps[64][64][MAP_LAYER];

    void Map::setTile(unsigned short _x, unsigned short _y, unsigned char _z, unsigned short groundId);

	 uint32_t max_players;
};

#endif
