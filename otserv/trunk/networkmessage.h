

#ifndef __NETWORK_MESSAGE_H__
#define __NETWORK_MESSAGE_H__


#include "definitions.h"
#include "otsystem.h"

#define NETWORKMESSAGE_MAXSIZE 16384

#define NM_ME_DRAW_BLOOD          0
#define NM_ME_LOOSE_ENERGY        1
#define NM_ME_PUFF                2
#define NM_ME_BLOCKHIT            3
#define NM_ME_EXPLOSION_AREA      4
#define NM_ME_EXPLOSION_DAMAGE    5
#define NM_ME_FIRE_AREA           6
#define NM_ME_YELLOW_RINGS        7
#define NM_ME_POISEN_RINGS        8
#define NM_ME_HIT_AREA            9
#define NM_ME_ENERGY_AREA        10
#define NM_ME_ENERGY_DAMAGE      11

#define NM_ME_MAGIC_ENERGIE      12
#define NM_ME_MAGIC_BLOOD        13
#define NM_ME_MAGIC_POISEN       14

#define NM_ME_HITBY_FIRE         15
#define NM_ME_POISEN             16
#define NM_ME_MORT_AREA          17
#define NM_ME_SOUND              18

#define NM_ANI_BOLT              1
#define NM_ANI_ARROW             2
#define NM_ANI_FIRE              3
#define NM_ANI_ENERGY            4
#define NM_ANI_POISONARROW       5
#define NM_ANI_BURSTARROW        6
#define NM_ANI_THROWINGSTAR      7
#define NM_ANI_THROWINGKNIFE     8
#define NM_ANI_SMALLSTONE        9
#define NM_ANI_SUDDENDEATH       10
#define NM_ANI_LARGEROCK         11
#define NM_ANI_SNOWBALL          12
#define NM_ANI_POWERBOLT         13

enum SpeakClasses {
	SPEAK_MONSTER = 0x11
};

enum MessageClasses {
      MSG_SMALLINFO = 0x17,
		  MSG_INFO      = 0x16,
		  MSG_EVENT     = 0x14,
		  MSG_ADVANCE   = 0x13,
};

enum Icons {
 ICON_POISON = 1,
 ICON_BURN = 2, 
 ICON_ENERGY =  4, 
 ICON_DRUNK = 8, 
 ICON_MANASHIELD = 16, 
 ICON_PARALYZE = 32, 
 ICON_HASTE = 64, 
 ICON_SWORDS = 128
 };

class Creature;
class Player;
class Item;
class Position;


class NetworkMessage
{
public:
  // constructor/destructor
  NetworkMessage();
  virtual ~NetworkMessage();


  // resets the internal buffer to an empty message
  void Reset();


  // socket functions
  bool ReadFromSocket(SOCKET socket);
  bool WriteToSocket(SOCKET socket);


  // simply read functions for incoming message
  unsigned char  GetByte();
  unsigned short GetU16();
  unsigned int   GetU32();
  std::string    GetString();
  std::string	 GetRaw();
  Position       GetPosition();


  // skips count unknown/unused bytes in an incoming message
  void SkipBytes(int count);


  // simply write functions for outgoing message
  void AddByte(unsigned char  value);
  void AddU16 (unsigned short value);
  void AddU32 (unsigned int   value);

  void AddString(const std::string &value);
  void AddString(const char* value);


  // write functions for complex types
  void AddPosition(const Position &pos);
  void AddItem(const Item *item);
  void AddCreature(const Creature *creature, bool known, unsigned int remove);


  // write functions for complete message blocks
  void AddTextMessage(MessageClasses mclass, const char* message);
  void AddAnimatedText(const Position &pos, unsigned char color, std::string text);

  void AddMagicEffect(const Position &pos, unsigned char type);
  void AddDistanceShoot(const Position &from, const Position &to, unsigned char type);
  
  void AddCreatureSpeak(const Creature *creature, unsigned char type, std::string text, unsigned short channelId);
  void AddCreatureHealth(const Creature *creature);
  
  void AddPlayerInventoryItem(const Player *player, int item);
  void AddPlayerSkills(const Player *player);
  void AddPlayerStats(const Player *player);

  int getMessageLength(){
      return m_MsgSize;
      }

protected:
  int m_MsgSize;
  int m_ReadPos;

  unsigned char m_MsgBuf[NETWORKMESSAGE_MAXSIZE];
};


#endif // #ifndef __NETWORK_MESSAGE_H__
