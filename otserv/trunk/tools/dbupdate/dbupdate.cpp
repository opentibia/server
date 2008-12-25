

#include "../../configmanager.h"
#include "../../database.h"
#include "../../definitions.h"

#include <iostream>

ConfigManager g_config;

struct SimpleUpdateQuery{
	int version;
	const char* query[32];
};

SimpleUpdateQuery updateQueries[] = {
	/*
	//Example update query
	{ 	2,  //schema version
		{
			//Queries here
			"SELECT * FROM `players`;",
			"SELECT * FROM `schema_info`;",
			NULL
		}
	}
	*/
};

bool applyUpdateQuery(const SimpleUpdateQuery& updateQuery)
{
	//Execute queris first
	Database* db = Database::instance();
	int i = 0;
	while(updateQuery.query[i]){
		std::cout << "Executing query : " << updateQuery.query[i] << std::endl;
		if(!db->executeQuery(updateQuery.query[i])){
			return false;
		}
		++i;
	}
	//update schema version
	DBQuery query;

	if(!db->executeQuery("DELETE FROM `schema_info`;")){
		return false;
	}
	query << "INSERT INTO `schema_info` (`version`) VALUES (" <<  updateQuery.version << ");";
	if(!db->executeQuery(query.str().c_str())){
		return false;
	}
	std::cout << "Schema update to version " << updateQuery.version << std::endl;
	return true;
}

void ErrorMessage(const char* message) {
	std::cout << std::endl << std::endl << "Error: " << message;

	std::string s;
	std::cin >> s;
}

int main(){
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


	std::cout << ":: Checking Schema version... ";
	DBQuery query;
	DBResult* result;

	query << "SELECT * FROM `schema_info`;";
	if(!(result = db->storeQuery(query.str()))){
		ErrorMessage("Can't get schema version! Does `schema_info` exist?");
		return -1;
	}
	int schema_version = result->getDataInt("version");
	if(schema_version == 0 || schema_version > CURRENT_SCHEMA_VERSION){
		ErrorMessage("Not valid schema version!");
		return -1;
	}
	std::cout << "Version = " << schema_version << " ";
	std::cout << "[done]" << std::endl;

	if(schema_version == CURRENT_SCHEMA_VERSION){
		std::cout << ":: Your database schema is updated." << std::endl;
		return 0;
	}

	std::string yesno;
	std::cout << "Your database is not updated. Do you want to update it? (y/n)";
	std::cin >> yesno;
	if(yesno != "y" && yesno != "yes"){
		return 0;
	}

	int n_updates = sizeof(updateQueries)/sizeof(SimpleUpdateQuery);
	for(int i = 0; i < n_updates; ++i){
		if(updateQueries[i].version > schema_version){
			if(!applyUpdateQuery(updateQueries[i])){
				char errorMessage[64];
				sprintf(errorMessage, "Error while updating to schema version %d!", updateQueries[i].version);
				ErrorMessage(errorMessage);
				return -1;
				break;
			}
		}
	}

	return 0;
}
