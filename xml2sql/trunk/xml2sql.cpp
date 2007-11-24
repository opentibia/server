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

#include <ctime>
#include <iostream>
#include <sstream>
#include <map>
#include <boost/filesystem.hpp>
#include <libxml/parser.h>

#ifdef XML_GCC_FREE
#define xmlFreeOTSERV(s) free(s)
#else
#define xmlFreeOTSERV(s) xmlFree(s)
#endif

#if defined __WINDOWS__ || defined WIN32
char DIR_SEPARATOR = '\\';
#ifdef __GNUC__
#define ATOI64 atoll
#else
#define ATOI64 _atoi64
#endif
#else
char DIR_SEPARATOR = '/';
#define ATOI64 atoll
#endif

bool readXMLInteger(xmlNodePtr node, const char* tag, int& value)
{
	char* nodeValue = (char*)xmlGetProp(node, (xmlChar*)tag);
	if(nodeValue){
		value = atoi(nodeValue);
		xmlFreeOTSERV(nodeValue);
		return true;
	}

	return false;
}

bool readXMLInteger64(xmlNodePtr node, const char* tag, uint64_t& value)
{
	char* nodeValue = (char*)xmlGetProp(node, (xmlChar*)tag);
	if(nodeValue){
		value = ATOI64(nodeValue);
		xmlFreeOTSERV(nodeValue);
		return true;
	}

	return false;
}

bool readXMLString(xmlNodePtr node, const char* tag, std::string& value)
{
	char* nodeValue = (char*)xmlGetProp(node, (xmlChar*)tag);
	if(nodeValue){
		value = nodeValue;
		xmlFreeOTSERV(nodeValue);
		return true;
	}

	return false;
}

std::string sqlQuote(std::string& s)
{
	std::string buf = "'";
	uint32_t length = s.length();

	for(int32_t i = 0; i < length; i++){
		switch(s[i]){
			case '\'':
				buf += "\'\'";
				break;

			case '\0':
				buf += "\\0";
				break;

			case '\\':
				buf += "\\\\";
				break;

			case '\r':
				buf += "\\r";
				break;

			case '\n':
				buf += "\\n";
				break;

			default:
				buf += s[i];
		}
	}

	buf += "'";
	return buf;
}

void putSlot(xmlNodePtr items, int slotId, int pid = 0)
{
	static int sid = 10;
	int count, id, itemType;
	xmlNodePtr node;

	if(slotId == -1){
		sid = 10;
		return;
	}

	while(items){
		if( !xmlStrcmp(items->name, (const xmlChar*)"item") ){
			readXMLInteger(items, "id", itemType);

			if( !readXMLInteger(items, "count", count) ){
				count = 0;
			}

			if(!pid){
				pid = slotId;
			}

			std::cout << "INSERT INTO `player_items` (`player_id`, `sid`, `pid`, `itemtype`, `count`, `attributes`) VALUES ( ( SELECT `value` FROM `xml2sql` WHERE `name` = '@PLAYER_ID' ), " << ++sid << ", " << pid << ", " << itemType << ", " << count << ", '');" << std::endl;

			node = items->children;
			id = sid;

			if(node && !xmlStrcmp(node->name, (const xmlChar*)"inside") ){
				putSlot(node->children, slotId, id);
			}
		}

		items = items->next;
	}
}

void putDepot(xmlNodePtr items, int depotId, int pid = 0)
{
	static int sid = 100;
	int count, id, itemType;
	xmlNodePtr node;

	if(depotId == -1){
		sid = 100;
		return;
	}

	while(items){
		if( !xmlStrcmp(items->name, (const xmlChar*)"item") ){
			readXMLInteger(items, "id", itemType);

			if( !readXMLInteger(items, "count", count) ){
				count = 0;
			}

			if(!pid){
				pid = depotId;
			}

			std::cout << "INSERT INTO `player_depotitems` (`player_id`, `depot_id`, `sid`, `pid`, `itemtype`, `count`, `attributes`) VALUES ( ( SELECT `value` FROM `xml2sql` WHERE `name` = '@PLAYER_ID' ), " << depotId << ", " << ++sid << ", " << pid << ", " << itemType << ", " << count << ", '');" << std::endl;

			node = items->children;
			id = sid;

			if(node && !xmlStrcmp(node->name, (const xmlChar*)"inside") ){
				putDepot(node->children, depotId, id);
			}
		}

		items = items->next;
	}
}

int main(int argc, char** argv)
{
	const char* path = "data";
	std::stringstream dir_name;
	boost::filesystem::directory_iterator dir_end;
	std::string leaf;
	xmlDocPtr xml;
	xmlNodePtr root, node;
	char* buf;
	time_t now = time(NULL);
	std::map<uint32_t, int> premium;
	int i;
	const char* insertId = "LAST_INSERT_ID";
	const char* timeStamp = "UNIX_TIMESTAMP( NOW() )";

	for(i = 0; i < argc; i++){
		if( !strcmp(argv[i], "sqlite") ){
			insertId = "LAST_INSERT_ROWID";
			timeStamp = "STRFTIME('%s','NOW')";
			argc--;
			break;
		}
	}

	if(argc > 1){
		path = argv[1];
	}

	if( !boost::filesystem::exists(path) ){
		std::cerr << path << " XML source directory does not exists." << std::endl;
		return 1;
	}

	std::cerr << "Using " << path << " as XML source directory." << std::endl;

	std::cout << "CREATE TABLE `xml2sql` (`name` VARCHAR(255), `value` INT);" << std::endl;
	std::cout << "CREATE TABLE `xml2sql2` (`name` VARCHAR(255), `value` INT);" << std::endl;
	std::cout << "INSERT INTO `xml2sql` (`name`, `value`) VALUES ('@GROUP_ID', 0);" << std::endl;
	std::cout << "INSERT INTO `xml2sql` (`name`, `value`) VALUES ('@PLAYER_ID', 0);" << std::endl;
	std::cout << "INSERT INTO `xml2sql` (`name`, `value`) VALUES ('@GUILD_ID', 0);" << std::endl;
	std::cout << "INSERT INTO `xml2sql` (`name`, `value`) VALUES ('@RANK_ID', 0);" << std::endl;
	std::cout << "INSERT INTO `xml2sql2` (`name`, `value`) VALUES ('@RANK_ID', 0);" << std::endl;

	dir_name.str("");
	dir_name << path << DIR_SEPARATOR << "accounts" << DIR_SEPARATOR;
	if( boost::filesystem::exists( dir_name.str() ) ){
		std::string password;
		int premDays;
		uint32_t id;

		boost::filesystem::path accounts_path( boost::filesystem::initial_path<boost::filesystem::path>() );
		accounts_path = boost::filesystem::system_complete( boost::filesystem::path( dir_name.str(), boost::filesystem::native) );

		for( boost::filesystem::directory_iterator accounts_dir(accounts_path); accounts_dir != dir_end; accounts_dir++){
			leaf = accounts_dir->path().leaf();

			if( boost::filesystem::is_regular( accounts_dir->status() ) && leaf.substr( leaf.length() - 4) == ".xml"){
				std::cerr << "Reading " << leaf << " account file...";

				xml = xmlParseFile( ( dir_name.str() + leaf).c_str() );

				if(xml){
					root = xmlDocGetRootElement(xml);

					if( xmlStrcmp(root->name, (const xmlChar*)"account") ){
						xmlFreeDoc(xml);
						std::cerr << " failed." << std::endl;
						continue;
					}

					id = atoi( leaf.substr(0, leaf.length() - 4).c_str() );

					if( !readXMLString(root, "pass", password) ){
						password = "";
					}

					if( !readXMLInteger(root, "premDays", premDays) ){
						premium[id] = 0;
					}
					else{
						premium[id] = now + premDays * 86400;
					}

					std::cout << "INSERT INTO `accounts` (`id`, `password`) VALUES (" << id << ", " << sqlQuote(password) << ");" << std::endl;

					std::cerr << " done." << std::endl;
					xmlFreeDoc(xml);
				}
				else{
					std::cerr << " failed." << std::endl;
				}
			}
		}
	}

	dir_name.str("");
	dir_name << path << DIR_SEPARATOR << "players" << DIR_SEPARATOR;
	if( boost::filesystem::exists( dir_name.str() ) ){
		std::string playerName, groupName, spellName, guildName, guildNick, guildRank;
		int playerAccount, playerSex, playerVocation, playerExperience, playerLevel, playerMagLevel, vipId;
		int groupAccess, groupMaxDepotItems;
		uint64_t groupFlags;
		int playerCap, playerSoul, playerDirection, playerLastLogin, playerLossExperience, playerLossMana, playerLossSkills;
		int playerHealth, playerHealthMax, playerMana, playerManaMax, playerManaSpent, playerPosX, playerPosY, playerPosZ;
		int playerLookHead, playerLookBody, playerLookLegs, playerLookFeet, playerLookType, playerLookAddons;
		int skillId, skillCount, skillValue, storageKey, storageValue, slotId, depotId;

		xmlNodePtr xmlMana, xmlLook, xmlHealth, xmlInventory, xmlVipList, xmlConditions, xmlSpawn, xmlGuild, xmlSkills, xmlDepots, xmlStorage, xmlKownSpells;

		boost::filesystem::path players_path( boost::filesystem::initial_path<boost::filesystem::path>() );
		players_path = boost::filesystem::system_complete( boost::filesystem::path( dir_name.str(), boost::filesystem::native) );

		for( boost::filesystem::directory_iterator players_dir(players_path); players_dir != dir_end; players_dir++){
			leaf = players_dir->path().leaf();

			if( boost::filesystem::is_regular( players_dir->status() ) && leaf.substr( leaf.length() - 4) == ".xml" && leaf != "players.xml"){
				std::cerr << "Reading " << leaf << " player file...";

				xml = xmlParseFile( ( dir_name.str() + leaf).c_str() );

				if(xml){
					xmlMana = NULL;
					xmlHealth = NULL;
					xmlLook = NULL;
					xmlSpawn = NULL;
					xmlGuild = NULL;
					xmlSkills = NULL;
					xmlInventory = NULL;
					xmlDepots = NULL;
					xmlStorage = NULL;
					xmlVipList = NULL;
					xmlKownSpells = NULL;
					xmlConditions = NULL;

					root = xmlDocGetRootElement(xml);

					if( xmlStrcmp(root->name, (const xmlChar*)"player") ){
						xmlFreeDoc(xml);
						std::cerr << " failed." << std::endl;
						continue;
					}

					xmlMana = xmlLook = xmlHealth = xmlInventory = xmlVipList = xmlConditions = xmlSpawn = xmlGuild = xmlSkills = xmlDepots = xmlStorage = xmlKownSpells = NULL;

					readXMLInteger(root, "access", groupAccess);
					readXMLInteger(root, "maxdepotitems", groupMaxDepotItems);
					readXMLInteger64(root, "groupflags", groupFlags);
					readXMLString(root, "name", playerName);

					groupName = playerName + "'s group";

					std::cout << "INSERT INTO `groups` (`name`, `flags`, `access`, `maxdepotitems`, `maxviplist`) VALUES (" << sqlQuote(groupName) << ", " << groupFlags << ", " << groupAccess << ", " << groupMaxDepotItems << ", 50);" << std::endl;
					std::cout << "UPDATE `xml2sql` SET `value` = " << insertId << "() WHERE `name` = '@GROUP_ID';" << std::endl;

					readXMLInteger(root, "account", playerAccount);
					readXMLInteger(root, "sex", playerSex);
					readXMLInteger(root, "voc", playerVocation);
					readXMLInteger(root, "exp", playerExperience);
					readXMLInteger(root, "level", playerLevel);
					readXMLInteger(root, "maglevel", playerMagLevel);
					readXMLInteger(root, "cap", playerCap);
					readXMLInteger(root, "soul", playerSoul);
					readXMLInteger(root, "lookdir", playerDirection);
					readXMLInteger(root, "lastlogin", playerLastLogin);

					if( !readXMLInteger(root, "loss_experience", playerLossExperience) )
					{
						palyerLossExperience = 10;
					}

					if( !readXMLInteger(root, "loss_mana", playerLossMana) )
					{
						playerLossMana = 10;
					}

					if( !readXMLInteger(root, "loss_skills", playerLossSkills) )
					{
						playerLossSkills = 10;
					}

					node = root->children;

					while(node){
						if( !xmlStrcmp(node->name, (const xmlChar*)"mana") ){
							xmlMana = node;
						}
						else if( !xmlStrcmp(node->name, (const xmlChar*)"health") ){
							xmlHealth = node;
						}
						else if( !xmlStrcmp(node->name, (const xmlChar*)"look") ){
							xmlLook = node;
						}
						else if( !xmlStrcmp(node->name, (const xmlChar*)"spawn") ){
							xmlSpawn = node;
						}
						else if( !xmlStrcmp(node->name, (const xmlChar*)"guild") ){
							xmlGuild = node;
						}
						else if( !xmlStrcmp(node->name, (const xmlChar*)"skills") ){
							xmlSkills = node;
						}
						else if( !xmlStrcmp(node->name, (const xmlChar*)"inventory") ){
							xmlInventory = node;
						}
						else if( !xmlStrcmp(node->name, (const xmlChar*)"depots") ){
							xmlDepots = node;
						}
						else if( !xmlStrcmp(node->name, (const xmlChar*)"storage") ){
							xmlStorage = node;
						}
						else if( !xmlStrcmp(node->name, (const xmlChar*)"viplist") ){
							xmlVipList = node;
						}
						else if( !xmlStrcmp(node->name, (const xmlChar*)"knownspells") ){
							xmlKownSpells = node;
						}
						else if( !xmlStrcmp(node->name, (const xmlChar*)"conditions") ){
							xmlConditions = node;
						}

						node = node->next;
					}

					if(xmlHealth){
						readXMLInteger(xmlHealth, "now", playerHealth);
						readXMLInteger(xmlHealth, "max", playerHealthMax);
					}
					else{
						playerHealth = 100;
						playerHealthMax = 100;
					}

					if(xmlMana){
						readXMLInteger(xmlMana, "now", playerMana);
						readXMLInteger(xmlMana, "max", playerManaMax);
						readXMLInteger(xmlMana, "spent", playerManaSpent);
					}
					else{
						playerMana = 100;
						playerManaMax = 100;
						playerManaSpent = 0;
					}

					if(xmlLook){
						readXMLInteger(xmlLook, "head", playerLookHead);
						readXMLInteger(xmlLook, "body", playerLookBody);
						readXMLInteger(xmlLook, "legs", playerLookLegs);
						readXMLInteger(xmlLook, "feet", playerLookFeet);
						readXMLInteger(xmlLook, "type", playerLookType);
						readXMLInteger(xmlLook, "addons", playerLookAddons);
					}
					else{
						playerLookHead = 10;
						playerLookBody = 10;
						playerLookLegs = 10;
						playerLookFeet = 10;
						playerLookType = 136;
						playerLookAddons = 0;
					}

					if(xmlSpawn){
						readXMLInteger(xmlSpawn, "x", playerPosX);
						readXMLInteger(xmlSpawn, "y", playerPosY);
						readXMLInteger(xmlSpawn, "z", playerPosZ);
					}
					else{
						playerPosX = 0;
						playerPosY = 0;
						playerPosZ = 0;
					}

					std::cout << "INSERT INTO `players` (`name`, `account_id`, `group_id`, `premend`, `sex`, `vocation`, `town_id`, `experience`, `level`, `maglevel`, `cap`, `soul`, `direction`, `lastlogin`, `loss_experience`, `loss_mana`, `loss_skills`, `health`, `healthmax`, `mana`, `manamax`, `manaspent`, `lookbody`, `lookfeet`, `lookhead`, `looklegs`, `looktype`, `lookaddons`, `posx`, `posy`, `posz`, `conditions`, `rank_id`) VALUES (" << sqlQuote(playerName) << ", " << playerAccount << ", ( SELECT `value` FROM `xml2sql` WHERE `name` = '@GROUP_ID' ), " << premium[playerAccount] << ", " << playerSex << ", " << playerVocation << ", 1, " << playerExperience << ", " << playerLevel << ", " << playerMagLevel << ", " << playerCap << ", " << playerSoul << ", " << playerDirection << ", " << playerLastLogin << ", " << playerLossExperience << ", " << playerLossMana << ", " << playerLossSkills << ", " << playerHealth << ", " << playerHealthMax << ", " << playerMana << ", " << playerManaMax << ", " << playerManaSpent << ", " << playerLookBody << ", " << playerLookFeet << ", " << playerLookHead << ", " << playerLookLegs << ", " << playerLookType << ", " << playerLookAddons << ", " << playerPosX << ", " << playerPosY << ", " << playerPosZ << ", '', 0);" << std::endl;
					std::cout << "UPDATE `xml2sql` SET `value` = " << insertId << "() WHERE `name` = '@PLAYER_ID';" << std::endl;

					if(xmlSkills){
						node = xmlSkills->children;

						while(node){
							if( !xmlStrcmp(node->name, (const xmlChar*)"skill") ){
								readXMLInteger(node, "skillid", skillId);
								readXMLInteger(node, "level", skillValue);
								readXMLInteger(node, "tries", skillCount);

								std::cout << "UPDATE `player_skills` SET `value` = " << skillValue << ", `count` = " << skillCount << " WHERE `player_id` = ( SELECT `value` FROM `xml2sql` WHERE `name` = '@PLAYER_ID' ) AND `skillid` = " << skillId << ";" << std::endl;
							}

							node = node->next;
						}
					}

					if(xmlStorage){
						node = xmlStorage->children;

						while(node){
							if( !xmlStrcmp(node->name, (const xmlChar*)"data") ){
								readXMLInteger(node, "key", storageKey);
								readXMLInteger(node, "value", storageValue);

								std::cout << "INSERT INTO `player_storage` (`player_id`, `key`, `value`) VALUES ( ( SELECT `value` FROM `xml2sql` WHERE `name` = '@PLAYER_ID' ), " << storageKey << ", " << storageValue << ");" << std::endl;
							}

							node = node->next;
						}
					}

					if(xmlKownSpells){
						node = xmlKownSpells->children;

						while(node){
							if( !xmlStrcmp(node->name, (const xmlChar*)"spell") ){
								readXMLString(node, "name", spellName);

								std::cout << "INSERT INTO `player_spells` (`player_id`, `name`) VALUES ( ( SELECT `value` FROM `xml2sql` WHERE `name` = '@PLAYER_ID' ), " << sqlQuote(spellName) << ");" << std::endl;
							}

							node = node->next;
						}
					}

					if(xmlVipList){
						node = xmlVipList->children;

						while(node){
							if( !xmlStrcmp(node->name, (const xmlChar*)"vip") ){
								readXMLInteger(node, "playerguid", vipId);

								std::cout << "INSERT INTO `player_viplist` (`player_id`, `vip_id`) VALUES ( ( SELECT `value` FROM `xml2sql` WHERE `name` = '@PLAYER_ID' ), " << vipId << ");" << std::endl;
							}

							node = node->next;
						}
					}

					if(xmlInventory){
						node = xmlInventory->children;
						putSlot(NULL, -1);

						while(node){
							if( !xmlStrcmp(node->name, (const xmlChar*)"slot") ){
								readXMLInteger(node, "slotid", slotId);

								putSlot(node->children, slotId);
							}

							node = node->next;
						}
					}

					if(xmlDepots){
						node = xmlDepots->children;
						putDepot(NULL, -1);

						while(node){
							if( !xmlStrcmp(node->name, (const xmlChar*)"depot") ){
								readXMLInteger(node, "depotid", depotId);

								putDepot(node->children, depotId);
							}

							node = node->next;
						}
					}

					if(xmlGuild){
						readXMLString(xmlGuild, "name", guildName);
						readXMLString(xmlGuild, "rank", guildRank);
						readXMLString(xmlGuild, "nick", guildNick);

						std::cout << "INSERT INTO `guilds` (`name`, `ownerid`, `creationdata`) SELECT " << sqlQuote(guildName) << ", ( SELECT `value` FROM `xml2sql` WHERE `name` = '@PLAYER_ID' ), " << timeStamp << " FROM `players` WHERE ( SELECT COUNT(`id`) FROM `guilds` WHERE `name` = " << sqlQuote(guildName) << ") = 0 LIMIT 1;" << std::endl;
						std::cout << "UPDATE `xml2sql` SET `value` = ( SELECT `id` FROM `guilds` WHERE `name` = " << sqlQuote(guildName) << " ) WHERE `name` = '@GUILD_ID';" << std::endl;
						std::cout << "INSERT INTO `guild_ranks` (`guild_id`, `name`, `level`) SELECT ( SELECT `value` FROM `xml2sql` WHERE `name` = '@GUILD_ID' ), " << sqlQuote(guildRank) << ", 1 FROM `players` WHERE ( SELECT COUNT(`id`) FROM `guild_ranks` WHERE `name` = " << sqlQuote(guildRank) << " AND `guild_id` = ( SELECT `value` FROM `xml2sql` WHERE `name` = '@GUILD_ID' ) ) = 0 LIMIT 1;" << std::endl;
						std::cout << "UPDATE `xml2sql2` SET `value` = ( SELECT `id` FROM `guild_ranks` WHERE `name` = " << sqlQuote(guildRank) << " AND `guild_id` = ( SELECT `value` FROM `xml2sql` WHERE `name` = '@GUILD_ID' ) );" << std::endl;
						std::cout << "UPDATE `xml2sql` SET `value` = ( SELECT `value` FROM `xml2sql2` ) WHERE `name` = '@RANK_ID';" << std::endl;
						std::cout << "UPDATE `players` SET `guildnick` = " << sqlQuote(guildNick) << ", `rank_id` = ( SELECT `value` FROM `xml2sql` WHERE `name` = '@RANK_ID' ) WHERE `id` = ( SELECT `value` FROM `xml2sql` WHERE `name` = '@PLAYER_ID' );" << std::endl;
					}

					std::cerr << " done." << std::endl;
					xmlFreeDoc(xml);
				}
				else{
					std::cerr << " failed." << std::endl;
				}
			}
		}
	}

	dir_name.str("");
	dir_name << path << DIR_SEPARATOR << "bans.xml";
	if( boost::filesystem::exists( dir_name.str() ) ){
		int banTime;
		int banType;
		int banTarget;
		int banMask;

		std::cerr << "Reading bans.xml file...";

		xml = xmlParseFile( dir_name.str().c_str() );

		if(xml){
			root = xmlDocGetRootElement(xml);

			if( xmlStrcmp(root->name, (const xmlChar*)"bans") ){
				xmlFreeDoc(xml);
				std::cerr << " failed." << std::endl;
			}
			else{
				node = root->children;
				while(node){
					if( !xmlStrcmp(node->name, (const xmlChar*)"ban") ){
						if( readXMLInteger(node, "type", banType) ){
							readXMLInteger(node, "time", banTime);

							switch(banType){
								case 1:
									readXMLInteger(node, "ip", banTarget);
									readXMLInteger(node, "mask", banMask);
									std::cout << "INSERT INTO `bans` (`type`, `ip`, `mask`, `time`) VALUES (1, " << banTarget << ", " << banMask << ", " << banTime << ");" << std::endl;
									break;

								case 2:
									readXMLInteger(node, "player", banTarget);
									std::cout << "INSERT INTO `bans` (`type`, `player`, `time`) VALUES (2, " << banTarget << ", " << banTime << ");" << std::endl;
									break;

								case 3:
									readXMLInteger(node, "account", banTarget);
									std::cout << "INSERT INTO `bans` (`type`, `account`, `time`) VALUES (3, " << banTarget << ", " << banTime << ");" << std::endl;
									break;
							}
						}
					}

					node = node->next;
				}

				std::cerr << " done." << std::endl;
			}
		}
		else{
			std::cerr << " failed." << std::endl;
		}
	}
	else{
		std::cerr << "No bans.xml file." << std::endl;
	}

	std::cout << "DROP TABLE `xml2sql2`;" << std::endl;
	std::cout << "DROP TABLE `xml2sql`;" << std::endl;

	return 0;
}
