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

#include "../../definitions.h"
#include "../../configmanager.h"
#include "../../database.h"

#include <iostream>

ConfigManager g_config;

// Crude global config, no need to complicate it. :)
bool wait_for_input = true;
bool always_update = false;

struct SimpleUpdateQuery{
	int version;
	const char* pg_query[32];
	const char* my_query[32];
	const char* lite_query[32];
};

SimpleUpdateQuery updateQueries[] = {
	/*
	//Example update query
	{ 	2,  //schema version
		{ // pgsql
			//Queries here
			"SELECT * FROM `players`;",
			"SELECT * FROM `schema_info`;",
			NULL
		},
		{ // mysql
			//Queries here
			"SELECT * FROM `players`;",
			"SELECT * FROM `schema_info`;",
			NULL
		},
		{ // sqlite
			//Queries here
			"SELECT * FROM `players`;",
			"SELECT * FROM `schema_info`;",
			NULL
		}
	}
	*/
	{ 2,
		{ // PgSql
			"CREATE TABLE `map_store` ( "
				"`house_id` INT NOT NULL,"
				"`data` BYTEA NOT NULL,"
				"KEY(`house_id`)"
			");",
			NULL
		},
		{ // MySql
			"CREATE TABLE `map_store` ( "
				"`house_id` INT UNSIGNED NOT NULL,"
				"`data` BLOB NOT NULL,"
				"KEY(`house_id`)"
			");",
			NULL
		},
		{ // Sqlite
			"CREATE TABLE `map_store` ( "
				"`house_id` INTEGER NOT NULL,"
				"`data` BLOB NOT NULL,"
				"UNIQUE(`house_id`)"
			");",
			NULL
		}
	},
    { 3,
        { // PgSql
            "DROP TABLE `schema_info`;",
            "CREATE TABLE `schema_info` ("
                "`name` VARCHAR(255) NOT NULL,"
                "`value` VARCHAR(255) NOT NULL,"
                "PRIMARY KEY (`name`)"
            ");",
            "INSERT INTO `schema_info` (`name`, `value`) VALUES ('version', 'ERROR');",
            NULL
        },
        { // MySql
            "DROP TABLE `schema_info`;",
            "CREATE TABLE `schema_info` ("
                "`name` VARCHAR(255) NOT NULL,"
                "`value` VARCHAR(255) NOT NULL,"
                "PRIMARY KEY (`name`)"
            ");",
            "INSERT INTO `schema_info` (`name`, `value`) VALUES ('version', 'ERROR');",
            NULL
        },
        { // Sqlite
            "DROP TABLE `schema_info`;",
            "CREATE TABLE `schema_info` ("
                "`name` VARCHAR(255) NOT NULL,"
                "`value` VARCHAR(255) NOT NULL,"
                "UNIQUE (`name`)"
            ");",
            "INSERT INTO `schema_info` (`name`, `value`) VALUES ('version', 'ERROR');",
            NULL
        }
    },
    { 4,
        { // PgSql
            "ALTER TABLE `players` ADD `lastlogout` BIGINT NOT NULL DEFAULT 0;",
            "ALTER TABLE `players` ADD `stamina` INT NOT NULL DEFAULT 201660000;",
            NULL
        },
        { // MySql
            "ALTER TABLE `players` ADD `lastlogout` INT UNSIGNED NOT NULL DEFAULT 0;",
            "ALTER TABLE `players` ADD `stamina` INT NOT NULL DEFAULT 201660000 COMMENT 'player stamina in milliseconds';",
            NULL
        },
        { // Sqlite
            "ALTER TABLE `players` ADD `lastlogout` INTEGER NOT NULL DEFAULT 0;",
            "ALTER TABLE `players` ADD `stamina` INTEGER NOT NULL DEFAULT 201660000;",
            NULL
        }
    },
	{ 5,
        { // PgSql
            "ALTER TABLE `players` CHANGE `stamina` `stamina` INT NOT NULL DEFAULT 151200000;",
            "ALTER TABLE `players` CHANGE `loss_mana` `loss_mana` INT NOT NULL DEFAULT 100;",
			"UPDATE `players` SET `loss_mana`=`loss_mana`*10;",
            "ALTER TABLE `players` CHANGE `loss_skills` `loss_skills` INT NOT NULL DEFAULT 100;",
			"UPDATE `players` SET `loss_skills`=`loss_skills`*10;",
            "ALTER TABLE `players` CHANGE `loss_experience` `loss_experience` INT NOT NULL DEFAULT 100;",
			"UPDATE `players` SET `loss_experience`=`loss_experience`*10;",
            NULL
        },
        { // MySql
            "ALTER TABLE `players` CHANGE `stamina` `stamina` INT NOT NULL DEFAULT 151200000 COMMENT 'player stamina in milliseconds';",
            "ALTER TABLE `players` CHANGE `loss_mana` `loss_mana` INT NOT NULL DEFAULT 100;",
			"UPDATE `players` SET `loss_mana`=`loss_mana`*10;",
            "ALTER TABLE `players` CHANGE `loss_skills` `loss_skills` INT NOT NULL DEFAULT 100;",
			"UPDATE `players` SET `loss_skills`=`loss_skills`*10;",
            "ALTER TABLE `players` CHANGE `loss_experience` `loss_experience` INT NOT NULL DEFAULT 100;",
			"UPDATE `players` SET `loss_experience`=`loss_experience`*10;",
            NULL
        },
        { // Sqlite
            "ALTER TABLE `players` CHANGE `stamina` `stamina` INTEGER NOT NULL DEFAULT 151200000;",
            "ALTER TABLE `players` CHANGE `loss_mana` `loss_mana` INTEGER NOT NULL DEFAULT 100;",
			"UPDATE `players` SET `loss_mana`=`loss_mana`*10;",
            "ALTER TABLE `players` CHANGE `loss_skills` `loss_skills` INTEGER NOT NULL DEFAULT 100;",
			"UPDATE `players` SET `loss_skills`=`loss_skills`*10;",
            "ALTER TABLE `players` CHANGE `loss_experience` `loss_experience` INTEGER NOT NULL DEFAULT 100;",
			"UPDATE `players` SET `loss_experience`=`loss_experience`*10;",
            NULL
        }
	},
    { 6,
        { // PgSql
            "ALTER TABLE `players` ADD `loss_containers` INT NOT NULL DEFAULT 100;",
            NULL
        },
        { // MySql
            "ALTER TABLE `players` ADD `loss_containers` INT NOT NULL DEFAULT 100;",
            NULL
        },
        { // Sqlite
            "ALTER TABLE `players` ADD `loss_containers` INTEGER NOT NULL DEFAULT 100;",
            NULL
        }
    }
    { 7,
        { // PgSql
           "ALTER TABLE `players` CHANGE `loss_items` `loss_items` INT NOT NULL DEFAULT 100;", 	 
                      "UPDATE `players` SET `loss_items`=`loss_items`*10;",
           NULL
        },
        { // MySql
           "ALTER TABLE `players` CHANGE `loss_items` `loss_items` INT NOT NULL DEFAULT 100;", 	 
                      "UPDATE `players` SET `loss_items`=`loss_items`*10;",
            NULL
        },
        { // Sqlite
           "ALTER TABLE `players` CHANGE `loss_items` `loss_items` INTEGER NOT NULL DEFAULT 100;", 	 
                      "UPDATE `players` SET `loss_items`=`loss_items`*10;",
            NULL
        }
    }
};

bool applyUpdateQuery(const SimpleUpdateQuery& updateQuery)
{
	std::string sqltype = g_config.getString(ConfigManager::SQL_TYPE);
	
	//Execute queries first
	Database* db = Database::instance();
	const char* const (*queries)[32];

	if(sqltype == "pgsql") queries = &updateQuery.pg_query;
	else if(sqltype == "mysql") queries = &updateQuery.my_query;
	else if(sqltype == "sqlite") queries = &updateQuery.lite_query;
	else return false;

	for(int i = 0; (*queries)[i]; ++i){
		std::cout << "Executing query : " << (*queries)[i] << std::endl;
		if(!db->executeQuery((*queries)[i])){
			return false;
		}
	}

	std::cout << "Schema update to version " << updateQuery.version << std::endl;
	return true;
}

void ErrorMessage(const char* message) {
	std::cout << std::endl << std::endl << "Error: " << message;
	if(wait_for_input)
		std::cin.get();
}

int main(int argn, const char* argv[]){

	for(int i = 1; i < argn; ++i){
		if(argv[i] == "--noinput")
			wait_for_input = false;
		else if(argv[i] == "--update")
			always_update = true;
	}

	const char* configfile = "dbupdate.lua";

	std::cout << ":: Database maintenance tool for " OTSERV_NAME " Version " OTSERV_VERSION << std::endl;
	std::cout << ":: Schema version " << CURRENT_SCHEMA_VERSION << std::endl;
	std::cout << ":: =======================================================" << std::endl;
	std::cout << "::" << std::endl;

	std::cout << ":: Loading lua script " << configfile << "... " << std::flush;
	if(!g_config.loadFile(configfile)){
		char errorMessage[32];
		sprintf(errorMessage, "Unable to load %s!", configfile);
		ErrorMessage(errorMessage);
		return -1;
	}
	std::cout << "[done]" << std::endl;

	std::cout << ":: Checking Database Connection... ";
	Database* db = Database::instance();
	if(db == NULL || !db->isConnected()){
		ErrorMessage("Database Connection Failed!");
		return -1;
	}
	std::cout << "[done]" << std::endl;

    // indicates version 1 or 2 schema
    int schema_version;

	std::cout << ":: Checking Schema version... ";
	DBQuery query;
	DBResult* result;
	query << "SELECT `value` FROM `schema_info` WHERE `name` = 'version';";
	if(!(result = db->storeQuery(query.str()))){
		// this is for old (version 1 and 2 only) schema
		query.str("");
		query << "SELECT * FROM `schema_info`;";
		if(!(result = db->storeQuery(query.str()))){
			ErrorMessage("Can't get schema version! Does `schema_info` exist in your database?");
			return -1;
		}

		schema_version = result->getDataInt("version");

		if(schema_version == 0 || schema_version > 2){
			ErrorMessage("Not valid schema version!");
			return -1;
		}
	}
	else{
		schema_version = result->getDataInt("value");

		if(schema_version == 0 || schema_version > CURRENT_SCHEMA_VERSION){
			ErrorMessage("Not valid schema version!");
			return -1;
		}
	}
	std::cout << "Version = " << schema_version << " ";
	std::cout << "[done]" << std::endl;
	db->freeResult(result);

	if(schema_version == CURRENT_SCHEMA_VERSION){
		std::cout << ":: Your database schema is updated." << std::endl;
		std::cout << "Press any key to close ...";

		if(wait_for_input)
			std::cin.get();
		return 0;
	}

	std::cout << "Your database is not updated. Do you want to update it? (y/n)";
	std::string yesno;
	if(wait_for_input){
		std::cin >> yesno;
		if((yesno != "y" && yesno != "yes")){
			return 0;
		}
	}
	else if(!always_update){
		return 0;
	}

	int n_updates = sizeof(updateQueries)/sizeof(SimpleUpdateQuery);
	for(int i = 0; i < n_updates; ++i){
		if(updateQueries[i].version > schema_version){
			if(!applyUpdateQuery(updateQueries[i])){
				char errorMessage[64];
				sprintf(errorMessage, "Error while updating to schema version %d!\n", updateQueries[i].version);
				ErrorMessage(errorMessage);
				return -1;
			}
		}
	}

	//update schema version
	// previously version number was updated after each step
	// but this could break compatibility with version 1
	// so if you think it is realy important then try to fuck around with version 1 struct ;)
	query.str("");
	query << "UPDATE `schema_info` SET `value` = '" << CURRENT_SCHEMA_VERSION << "' WHERE `name` = 'version';";
	if(!db->executeQuery(query.str().c_str())){
		ErrorMessage("Your database has been correctly updated to most recent version, but an error occured when updating the schema version.");
		return -1;
	}

	std::cout << std::endl << "Your database has been updated to the most recent version!" << std::endl;
	std::cout << "Press any key to close ...";

	if(wait_for_input)
		std::cin.get();

	return 0;
}
