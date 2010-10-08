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

#ifndef __OTSERV_DBUPDATE_QUERIES_H__
#define __OTSERV_DBUPDATE_QUERIES_H__

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
			"CREATE TABLE `map_store` ( \n\
				`house_id` INT NOT NULL,\n\
				`data` BYTEA NOT NULL,\n\
				KEY(`house_id`)\n\
			);",
			NULL
		},
		{ // MySql
			"CREATE TABLE `map_store` ( \n\
				`house_id` INT UNSIGNED NOT NULL,\n\
				`data` BLOB NOT NULL,\n\
				KEY(`house_id`)\n\
			);",
			NULL
		},
		{ // Sqlite
			"CREATE TABLE `map_store` ( \n\
				`house_id` INTEGER NOT NULL,\n\
				`data` BLOB NOT NULL,\n\
				UNIQUE(`house_id`)\n\
			);",
			NULL
		}
	},
	{ 3,
		{ // PgSql
			"DROP TABLE `schema_info`;",
			"CREATE TABLE `schema_info` (\n\
				`name` VARCHAR(255) NOT NULL,\n\
				`value` VARCHAR(255) NOT NULL,\n\
				PRIMARY KEY (`name`)\n\
			);",
			"INSERT INTO `schema_info` (`name`, `value`) VALUES ('version', 'ERROR');",
			NULL
		},
		{ // MySql
			"DROP TABLE `schema_info`;",
			"CREATE TABLE `schema_info` (\n\
				`name` VARCHAR(255) NOT NULL,\n\
				`value` VARCHAR(255) NOT NULL,\n\
				PRIMARY KEY (`name`)\n\
			);",
			"INSERT INTO `schema_info` (`name`, `value`) VALUES ('version', 'ERROR');",
			NULL
		},
		{ // Sqlite
			"DROP TABLE `schema_info`;",
			"CREATE TABLE `schema_info` (\n\
				`name` VARCHAR(255) NOT NULL,\n\
				`value` VARCHAR(255) NOT NULL,\n\
				UNIQUE (`name`)\n\
			);",
			"INSERT INTO `schema_info` (`name`, `value`) VALUES ('version', 'ERROR');",
			NULL
		}
	},
	{ 4,
		{ // PgSql
			"ALTER TABLE `players` ADD `lastlogout` BIGINT;",
			"ALTER TABLE `players` ALTER COLUMN `lastlogout` SET NOT NULL;",
			"ALTER TABLE `players` ALTER COLUMN `lastlogout` SET DEFAULT 0;",

			"ALTER TABLE `players` ADD `stamina` INT;",
			"ALTER TABLE `players` ALTER COLUMN `stamina` SET NOT NULL;",
			"ALTER TABLE `players` ALTER COLUMN `stamina` SET DEFAULT 201660000;",
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
			"ALTER TABLE `players` ADD COLUMN `stamina_tmp` INT;",
			"UPDATE      `players` SET `stamina_tmp` = `stamina`;",
			"ALTER TABLE `players` DROP COLUMN `stamina`;",
			"ALTER TABLE `players` ALTER COLUMN `stamina_tmp` SET NOT NULL;",
			"ALTER TABLE `players` ALTER COLUMN `stamina_tmp` SET DEFAULT 151200000;",
			"ALTER TABLE `players` RENAME COLUMN `stamina_tmp` TO `stamina`;",

			"ALTER TABLE `players` ADD COLUMN `loss_mana_tmp` INT;",
			"UPDATE      `players` SET `loss_mana_tmp` = `loss_mana` * 10;",
			"ALTER TABLE `players` DROP COLUMN `loss_mana`;",
			"ALTER TABLE `players` ALTER COLUMN `loss_mana_tmp` SET NOT NULL;",
			"ALTER TABLE `players` ALTER COLUMN `loss_mana_tmp` SET DEFAULT 100;",
			"ALTER TABLE `players` RENAME COLUMN `loss_mana_tmp` TO `loss_mana`;",

			"ALTER TABLE `players` ADD COLUMN `loss_skills_tmp` INT;",
			"UPDATE      `players` SET `loss_skills_tmp` = `loss_skills` * 10;",
			"ALTER TABLE `players` DROP COLUMN `loss_skills`;",
			"ALTER TABLE `players` ALTER COLUMN `loss_skills_tmp` SET NOT NULL;",
			"ALTER TABLE `players` ALTER COLUMN `loss_skills_tmp` SET DEFAULT 100;",
			"ALTER TABLE `players` RENAME COLUMN `loss_skills_tmp` TO `loss_skills`;",

			"ALTER TABLE `players` ADD COLUMN `loss_experience_tmp` INT;",
			"UPDATE      `players` SET `loss_experience_tmp` = `loss_experience` * 10;",
			"ALTER TABLE `players` DROP COLUMN `loss_experience`;",
			"ALTER TABLE `players` ALTER COLUMN `loss_experience_tmp` SET NOT NULL;",
			"ALTER TABLE `players` ALTER COLUMN `loss_experience_tmp` SET DEFAULT 100;",
			"ALTER TABLE `players` RENAME COLUMN `loss_experience_tmp` TO `loss_experience`;",
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
			// No support
			NULL
		}
	},
	{ 6,
		{ // PgSql
			"ALTER TABLE `players` ADD `loss_containers` INT;",
			"ALTER TABLE `players` ALTER COLUMN `loss_containers` SET NOT NULL;",
			"ALTER TABLE `players` ALTER COLUMN `loss_containers` SET DEFAULT 100;",
			NULL
		},
		{ // MySql
			"ALTER TABLE `players` ADD `loss_containers` INT NOT NULL DEFAULT 100;",
			NULL
		},
		{ // Sqlite
			// No support
			NULL
		}
	},
	{ 7,
		{ // PgSql
			"ALTER TABLE `players` ADD COLUMN `loss_items_tmp` INT;",
			"UPDATE      `players` SET `loss_items_tmp` = `loss_items` * 10;",
			"ALTER TABLE `players` DROP COLUMN `loss_items`;",
			"ALTER TABLE `players` ALTER COLUMN `loss_items_tmp` SET NOT NULL;",
			"ALTER TABLE `players` ALTER COLUMN `loss_items_tmp` SET DEFAULT 100;",
			"ALTER TABLE `players` RENAME COLUMN `loss_items_tmp` TO `loss_items`;",
			NULL
		},
		{ // MySql
			"ALTER TABLE `players` CHANGE `loss_items` `loss_items` INT NOT NULL DEFAULT 100;",
			"UPDATE `players` SET `loss_items`=`loss_items`*10;",
			NULL
		},
		{ // Sqlite
			// No support
			NULL
		}
	},
	{ 8,
		{ // PgSql
			"ALTER TABLE `players` ADD COLUMN `loss_items_tmp` INT;",
			"UPDATE      `players` SET `loss_items_tmp` = `loss_items` / 10;",
			"ALTER TABLE `players` DROP COLUMN `loss_items`;",
			"ALTER TABLE `players` ALTER COLUMN `loss_items_tmp` SET NOT NULL;",
			"ALTER TABLE `players` ALTER COLUMN `loss_items_tmp` SET DEFAULT 10;",
			"ALTER TABLE `players` RENAME COLUMN `loss_items_tmp` TO `loss_items`;",
			NULL
		},
		{ // MySql
			"ALTER TABLE `players` CHANGE `loss_items` `loss_items` INT NOT NULL DEFAULT 10;",
			"UPDATE `players` SET `loss_items`=`loss_items`/10;",
			NULL
		},
		{ // Sqlite
			// No support
			NULL
		}
	},
	{ 9,
		{ // PgSql
			// Groups table
			// pgsql does not support ALTER ADD with NULL / DEFAULT values
			"ALTER TABLE `groups` ADD COLUMN `access_tmp` INT;",
			"UPDATE      `groups` SET `access_tmp` = CAST(`access` AS INT);",
			"ALTER TABLE `groups` DROP COLUMN `access`;",
			"ALTER TABLE `groups` ALTER COLUMN `access_tmp` SET NOT NULL;",
			"ALTER TABLE `groups` ALTER COLUMN `access_tmp` SET DEFAULT 0;",
			"ALTER TABLE `groups` RENAME COLUMN `access_tmp` TO `access`;",

			"ALTER TABLE `groups` ADD `violation` INT;",
			"ALTER TABLE `groups` ALTER COLUMN `violation` SET NOT NULL;",
			"ALTER TABLE `groups` ALTER COLUMN `violation` SET DEFAULT 0;",

			// Accounts table
			"ALTER TABLE `accounts` DROP `deleted`;",
			"ALTER TABLE `accounts` DROP `warned`;",

			"ALTER TABLE `accounts` ADD `warnings` INT;",
			"ALTER TABLE `accounts` ALTER COLUMN `warnings` SET NOT NULL;",
			"ALTER TABLE `accounts` ALTER COLUMN `warnings` SET DEFAULT 0;",

			// Bans table
			"ALTER TABLE `bans` ADD COLUMN `comment_tmp` VARCHAR(1024);",
			"UPDATE      `bans` SET `comment_tmp` = `comment`;",
			"ALTER TABLE `bans` DROP COLUMN `comment`;",
			"ALTER TABLE `bans` ALTER COLUMN `comment_tmp` SET NOT NULL;",
			"ALTER TABLE `bans` ALTER COLUMN `comment_tmp` SET DEFAULT '';",
			"ALTER TABLE `bans` RENAME COLUMN `comment_tmp` TO `comment`;",

			"ALTER TABLE `bans` ADD `action` INT UNSIGNED;",
			"ALTER TABLE `bans` ALTER COLUMN `action` SET NOT NULL;",
			"ALTER TABLE `bans` ALTER COLUMN `action` SET DEFAULT 0;",

			"ALTER TABLE `bans` ADD `statement` VARCHAR(255);",
			"ALTER TABLE `bans` ALTER COLUMN `statement` SET NOT NULL;",
			"ALTER TABLE `bans` ALTER COLUMN `statement` SET DEFAULT '';",
			NULL
		},
		{ // MySql
			// Groups table
			"ALTER TABLE `groups` CHANGE `access` `access` INT NOT NULL DEFAULT 0;",
			"ALTER TABLE `groups` ADD `violation` INT NOT NULL DEFAULT 0;",

			// Accounts table
			"ALTER TABLE `accounts` DROP `deleted`;",
			"ALTER TABLE `accounts` DROP `warned`;",
			"ALTER TABLE `accounts` ADD `warnings` INT NOT NULL DEFAULT 0;",

			// Bans table
			"ALTER TABLE `bans` CHANGE `comment` `comment` VARCHAR(1024) NOT NULL DEFAULT '';",
			"ALTER TABLE `bans` ADD `action` INT UNSIGNED NOT NULL DEFAULT 0;",
			"ALTER TABLE `bans` ADD `statement` VARCHAR(255) NOT NULL DEFAULT '';",
			NULL
		},
		{ // Sqlite
			// No support
			NULL
		}
	},
	{ 10,
		{ // PgSql
			"CREATE TABLE `player_deaths` (\n\
				`id` SERIAL,\n\
				`player_id` INT NOT NULL,\n\
				`date` INT NOT NULL,\n\
				`level` INT NOT NULL,\n\
				PRIMARY KEY (`id`),\n\
				FOREIGN KEY (`player_id`) REFERENCES `players` (`id`) ON DELETE CASCADE\n\
			);",

			"CREATE TABLE `killers` (\n\
				`id` SERIAL,\n\
				`death_id` INT NOT NULL,\n\
				`final_hit` SMALLINT NOT NULL DEFAULT 1,\n\
				PRIMARY KEY(`id`),\n\
				FOREIGN KEY (`death_id`) REFERENCES `player_deaths` (`id`) ON DELETE CASCADE\n\
			);",

			"CREATE TABLE `environment_killers` (\n\
				`kill_id` INT NOT NULL,\n\
				`name` VARCHAR(255) NOT NULL,\n\
				FOREIGN KEY (`kill_id`) REFERENCES `killers` (`id`) ON DELETE CASCADE\n\
			);",

			"CREATE TABLE `player_killers` (\n\
				`kill_id` INT NOT NULL,\n\
				`player_id` INT NOT NULL,\n\
				FOREIGN KEY (`kill_id`) REFERENCES `killers` (`id`) ON DELETE CASCADE,\n\
				FOREIGN KEY (`player_id`) REFERENCES `players` (`id`) ON DELETE CASCADE\n\
			);",
			NULL
		},
		{ // MySql
			"CREATE TABLE `player_deaths` (\n\
				`id` INT UNSIGNED NOT NULL AUTO_INCREMENT,\n\
				`player_id` INT UNSIGNED NOT NULL,\n\
				`date` INT UNSIGNED NOT NULL,\n\
				`level` INT NOT NULL,\n\
				FOREIGN KEY (`player_id`) REFERENCES `players` (`id`) ON DELETE CASCADE,\n\
				PRIMARY KEY(`id`),\n\
				INDEX(`date`)\n\
			) ENGINE = InnoDB;",

			"CREATE TABLE `killers` (\n\
				`id` INT UNSIGNED NOT NULL AUTO_INCREMENT,\n\
				`death_id` INT UNSIGNED NOT NULL,\n\
				`final_hit` TINYINT(1) NOT NULL DEFAULT 1,\n\
				PRIMARY KEY(`id`),\n\
				FOREIGN KEY (`death_id`) REFERENCES `player_deaths` (`id`) ON DELETE CASCADE\n\
			) ENGINE = InnoDB;",

			"CREATE TABLE `environment_killers` (\n\
				`kill_id` INT UNSIGNED NOT NULL,\n\
				`name` VARCHAR(255) NOT NULL,\n\
				FOREIGN KEY (`kill_id`) REFERENCES `killers` (`id`) ON DELETE CASCADE\n\
			) ENGINE = InnoDB;",

			"CREATE TABLE `player_killers` (\n\
				`kill_id` INT UNSIGNED NOT NULL,\n\
				`player_id` INT UNSIGNED NOT NULL,\n\
				FOREIGN KEY (`kill_id`) REFERENCES `killers` (`id`) ON DELETE CASCADE,\n\
				FOREIGN KEY (`player_id`) REFERENCES `players` (`id`) ON DELETE CASCADE\n\
			) ENGINE = InnoDB;",
			NULL
		},
		{ // Sqlite
			// No support
			"CREATE TABLE `player_deaths` (\n\
				`id` INTEGER NOT NULL,\n\
				`player_id` INTEGER NOT NULL,\n\
				`date` INTEGER NOT NULL,\n\
				`level` INTEGER NOT NULL,\n\
				PRIMARY KEY (`id`),\n\
				FOREIGN KEY (`player_id`) REFERENCES `players` (`id`)\n\
			);",

			"CREATE TABLE `killers` (\n\
				`id` INTEGER NOT NULL,\n\
				`death_id` INTEGER NOT NULL,\n\
				`final_hit` SMALLINT NOT NULL DEFAULT 1,\n\
				PRIMARY KEY(`id`),\n\
				FOREIGN KEY (`death_id`) REFERENCES `player_deaths` (`id`)\n\
			);",

			"CREATE TABLE `environment_killers` (\n\
				`kill_id` INTEGER NOT NULL,\n\
				`name` VARCHAR(255) NOT NULL,\n\
				FOREIGN KEY (`kill_id`) REFERENCES `killers` (`id`)\n\
			);",

			"CREATE TABLE `player_killers` (\n\
				`kill_id` INTEGER NOT NULL,\n\
				`player_id` INTEGER NOT NULL,\n\
				FOREIGN KEY (`kill_id`) REFERENCES `killers` (`id`),\n\
				FOREIGN KEY (`player_id`) REFERENCES `players` (`id`)\n\
			);",

			"CREATE TRIGGER `oninsert_player_deaths`\n\
			BEFORE INSERT\n\
			ON `player_deaths`\n\
			FOR EACH ROW\n\
			BEGIN\n\
				SELECT RAISE(ROLLBACK, 'INSERT on table `player_deaths` violates foreign: `player_id`')\n\
				WHERE NEW.`player_id` IS NULL\n\
					OR (SELECT `id` FROM `players` WHERE `id` = NEW.`player_id`) IS NULL;\n\
			END;",

			"CREATE TRIGGER `onupdate_player_deaths`\n\
			BEFORE UPDATE\n\
			ON `player_deaths`\n\
			FOR EACH ROW\n\
			BEGIN\n\
				SELECT RAISE(ROLLBACK, 'UPDATE on table `player_deaths` violates foreign: `player_id`')\n\
				WHERE NEW.`player_id` IS NULL\n\
					OR (SELECT `id` FROM `players` WHERE `id` = NEW.`player_id`) IS NULL;\n\
			END;",

			"CREATE TRIGGER `oninsert_killers`\n\
			BEFORE INSERT\n\
			ON `killers`\n\
			FOR EACH ROW\n\
			BEGIN\n\
				SELECT RAISE(ROLLBACK, 'INSERT on table `killers` violates foreign: `death_id`')\n\
				WHERE NEW.`death_id` IS NULL\n\
					OR (SELECT `id` FROM `player_deaths` WHERE `id` = NEW.`death_id`) IS NULL;\n\
			END;",

			"CREATE TRIGGER `onupdate_killers`\n\
			BEFORE UPDATE\n\
			ON `killers`\n\
			FOR EACH ROW\n\
			BEGIN\n\
				SELECT RAISE(ROLLBACK, 'UPDATE on table `killers` violates foreign: `death_id`')\n\
				WHERE NEW.`death_id` IS NULL\n\
					OR (SELECT `id` FROM `player_deaths` WHERE `id` = NEW.`death_id`) IS NULL;\n\
			END;",

			"CREATE TRIGGER `oninsert_environment_killers`\n\
			BEFORE INSERT\n\
			ON `environment_killers`\n\
			FOR EACH ROW\n\
			BEGIN\n\
				SELECT RAISE(ROLLBACK, 'INSERT on table `enviroment_killers` violates foreign: `kill_id`')\n\
				WHERE NEW.`kill_id` IS NULL\n\
					OR (SELECT `id` FROM `killers` WHERE `id` = NEW.`kill_id`) IS NULL;\n\
			END;",

			"CREATE TRIGGER `onupdate_environment_killers`\n\
			BEFORE UPDATE\n\
			ON `environment_killers`\n\
			FOR EACH ROW\n\
			BEGIN\n\
				SELECT RAISE(ROLLBACK, 'INSERT on table `enviroment_killers` violates foreign: `kill_id`')\n\
				WHERE NEW.`kill_id` IS NULL\n\
					OR (SELECT `id` FROM `killers` WHERE `id` = NEW.`kill_id`) IS NULL;\n\
			END;",

			"CREATE TRIGGER `oninsert_player_killers`\n\
			BEFORE INSERT\n\
			ON `player_killers`\n\
			FOR EACH ROW\n\
			BEGIN\n\
				SELECT RAISE(ROLLBACK, 'INSERT on table `player_killers` violates foreign: `player_id`')\n\
				WHERE NEW.`player_id` IS NULL\n\
					OR (SELECT `id` FROM `players` WHERE `id` = NEW.`player_id`) IS NULL;\n\
				\n\
				SELECT RAISE(ROLLBACK, 'INSERT on table `player_killers` violates foreign: `kill_id`')\n\
				WHERE NEW.`kill_id` IS NULL\n\
					OR (SELECT `id` FROM `killers` WHERE `id` = NEW.`kill_id`) IS NULL;\n\
			END;",

			"CREATE TRIGGER `onupdate_player_killers`\n\
			BEFORE UPDATE\n\
			ON `player_killers`\n\
			FOR EACH ROW\n\
			BEGIN\n\
				SELECT RAISE(ROLLBACK, 'UPDATE on table `player_killers` violates foreign: `player_id`')\n\
				WHERE NEW.`player_id` IS NULL\n\
					OR (SELECT `id` FROM `players` WHERE `id` = NEW.`player_id`) IS NULL;\n\
					\n\
				SELECT RAISE(ROLLBACK, 'UPDATE on table `killers` violates foreign: `kill_id`')\n\
				WHERE NEW.`kill_id` IS NULL\n\
					OR (SELECT `id` FROM `killers` WHERE `id` = NEW.`kill_id`) IS NULL;\n\
			END;",
			NULL
		}
	},
	{ 11,
		{ // PgSql
			"ALTER TABLE `houses` ADD `doors` INT;",
			"ALTER TABLE `houses` ALTER COLUMN `doors` SET NOT NULL;",
			"ALTER TABLE `houses` ALTER COLUMN `doors` SET DEFAULT 0;",

			"ALTER TABLE `houses` ADD `beds` INT;",
			"ALTER TABLE `houses` ALTER COLUMN `beds` SET NOT NULL;",
			"ALTER TABLE `houses` ALTER COLUMN `beds` SET DEFAULT 0;",
			NULL
		},
		{ // MySql
			"ALTER TABLE `houses` ADD `doors` INT NOT NULL DEFAULT 0;",
			"ALTER TABLE `houses` ADD `beds` INT NOT NULL DEFAULT 0;",
			NULL
		},
		{ // Sqlite
			"ALTER TABLE `houses` ADD `doors` INTEGER NOT NULL DEFAULT 0;",
			"ALTER TABLE `houses` ADD `beds` INTEGER NOT NULL DEFAULT 0;",
			NULL
		}
	},
	{ 12,
		{ // PgSql
			"",
			NULL
		},
		{ // MySql
			"ALTER TABLE `map_store` CHANGE `data` `data` LONGBLOB NOT NULL",
			NULL
		},
		{ // Sqlite
			"",
			NULL
		}
	},
	{ 13,
		{ // PgSql
			"ALTER TABLE `players` ADD `online` SMALLINT;",
			"ALTER TABLE `players` ALTER COLUMN `online` SET NOT NULL;",
			"ALTER TABLE `players` ALTER COLUMN `online` SET DEFAULT 0;",
			"ALTER TABLE `players` ADD KEY `online`(`online`);",
			NULL
		},
		{ // MySql
			"ALTER TABLE `players` ADD `online` TINYINT(1) NOT NULL DEFAULT 0;",
			"ALTER TABLE `players` ADD INDEX `online`(`online`);",
			NULL
		},
		{ // Sqlite
			"ALTER TABLE `players` ADD `online` BOOLEAN NOT NULL DEFAULT 0;",
			NULL
		}
	},
	{ 14,
		{ // PgSql
			"ALTER TABLE `player_depotitems` DROP COLUMN `depot_id`;",
			NULL
		},
		{ // MySql
			"ALTER TABLE `player_depotitems` DROP COLUMN `depot_id`;",
			NULL
		},
		{ // Sqlite
			"",
			NULL
		}
	},
	{ 15,
		{ // PgSql
			"ALTER TABLE `houses` ADD `name` VARCHAR(100);",
			"ALTER TABLE `houses` ALTER COLUMN `name` SET NOT NULL;",

			"ALTER TABLE `houses` ADD `townid` INT;",
			"ALTER TABLE `houses` ALTER COLUMN `townid` SET NOT NULL;",
			"ALTER TABLE `houses` ALTER COLUMN `townid` SET DEFAULT 0;",

			"ALTER TABLE `houses` ADD `rent` INT;",
			"ALTER TABLE `houses` ALTER COLUMN `rent` SET NOT NULL;",
			"ALTER TABLE `houses` ALTER COLUMN `rent` SET DEFAULT 0;",

			"ALTER TABLE `houses` ADD `guildhall` SMALLINT;",
			"ALTER TABLE `houses` ALTER COLUMN `guildhall` SET NOT NULL;",
			"ALTER TABLE `houses` ALTER COLUMN `guildhall` SET DEFAULT 0;",

			"ALTER TABLE `houses` ADD `clear` SMALLINT;",
			"ALTER TABLE `houses` ALTER COLUMN `clear` SET NOT NULL;",
			"ALTER TABLE `houses` ALTER COLUMN `clear` SET DEFAULT 0;",

			"ALTER TABLE `houses` ALTER COLUMN `owner` SET DEFAULT 0;",

			"ALTER TABLE `tiles` ADD `house_id` INT;",
			"ALTER TABLE `tiles` ALTER COLUMN `house_id` SET NOT NULL;",
			"ALTER TABLE `tiles` ALTER COLUMN `house_id` SET DEFAULT 0;",
			NULL
		},
		{ // MySql
			"ALTER TABLE `houses` ADD COLUMN `name` VARCHAR(100) NOT NULL;",
			"ALTER TABLE `houses` ADD COLUMN `townid` INTEGER UNSIGNED NOT NULL DEFAULT 0;",
			"ALTER TABLE `houses` ADD COLUMN `rent` INTEGER UNSIGNED NOT NULL DEFAULT 0;",
			"ALTER TABLE `houses` ADD COLUMN `guildhall` TINYINT(1) NOT NULL DEFAULT 0;",
			"ALTER TABLE `houses` ADD COLUMN `clear` TINYINT(1) NOT NULL DEFAULT 0;",
			"ALTER TABLE `houses` MODIFY COLUMN `owner` INTEGER NOT NULL DEFAULT 0;",

			"ALTER TABLE `tiles` ADD COLUMN `house_id` INTEGER UNSIGNED NOT NULL DEFAULT 0;",
			NULL
		},
		{ // Sqlite
			NULL
		}
	},
	{ 16,
		{ // PgSql
			"ALTER TABLE `houses` ADD `price` INT;",
			"ALTER TABLE `houses` ALTER COLUMN `price` SET NOT NULL;",
			"ALTER TABLE `houses` ALTER COLUMN `price` SET DEFAULT 0;",
			NULL
		},
		{ // MySql
			"ALTER TABLE `houses` ADD COLUMN `price` INTEGER UNSIGNED NOT NULL DEFAULT 0;",
			NULL
		},
		{ // Sqlite
			NULL
		}
	},
	{ 17,
		{ // PgSql
			"ALTER TABLE `houses` DROP COLUMN `price`;",

			"ALTER TABLE `houses` ADD `tiles` INT;",
			"ALTER TABLE `houses` ALTER COLUMN `tiles` SET NOT NULL;",
			"ALTER TABLE `houses` ALTER COLUMN `tiles` SET DEFAULT 0;",

			"CREATE TABLE `house_auctions` (\n\
				`house_id` INT NOT NULL,\n\
				`player_id` INT NOT NULL,\n\
				`bid` INT NOT NULL DEFAULT 0,\n\
				`limit` INT NOT NULL DEFAULT 0,\n\
				`endtime` BIGINT NOT NULL DEFAULT 0,\n\
				FOREIGN KEY (`house_id`) REFERENCES `houses` (`id`) ON DELETE CASCADE\n\
			); ENGINE = InnoDB",
			NULL
		},
		{ // MySql
			"ALTER TABLE `houses` DROP `price`;",

			"ALTER TABLE `houses` ADD `tiles` INT NOT NULL DEFAULT 0;",

			"CREATE TABLE `house_auctions` (\n\
				`house_id` INT UNSIGNED NOT NULL,\n\
				`player_id` INT UNSIGNED NOT NULL,\n\
				`bid` INT UNSIGNED NOT NULL DEFAULT 0,\n\
				`limit` INT UNSIGNED NOT NULL DEFAULT 0,\n\
				`endtime` INT UNSIGNED NOT NULL DEFAULT 0,\n\
				FOREIGN KEY (`house_id`) REFERENCES `houses` (`id`) ON DELETE CASCADE\n\
			) ENGINE = InnoDB;",
			NULL
		},
		{ // Sqlite
			NULL
		}
	},
	{ 18,
		{ // PgSql
			"ALTER TABLE `players` DROP COLUMN `redskulltime`;",

			"ALTER TABLE `players` ADD COLUMN `skull_type_tmp` SMALLINT;",
			"UPDATE      `players` SET `skull_type_tmp` = 4 WHERE `redskull` = 1;",
			"ALTER TABLE `players` DROP COLUMN `redskull`;",
			"ALTER TABLE `players` ALTER COLUMN `skull_type_tmp` SET NOT NULL;",
			"ALTER TABLE `players` ALTER COLUMN `skull_type_tmp` SET DEFAULT 0;",
			"ALTER TABLE `players` RENAME COLUMN `skull_type_tmp` TO `skull_type`;",

			"ALTER TABLE `players` ADD `skull_time` BIGINT;",
			"ALTER TABLE `players` ALTER COLUMN `skull_time` SET NOT NULL;",
			"ALTER TABLE `players` ALTER COLUMN `skull_time` SET DEFAULT 0;",

			"ALTER TABLE `player_killers` ADD `unjustified` SMALLINT;",
			"ALTER TABLE `player_killers` ALTER COLUMN `unjustified` SET NOT NULL;",
			"ALTER TABLE `player_killers` ALTER COLUMN `unjustified` SET DEFAULT 0;",
			NULL
		},
		{ // MySql
			"ALTER TABLE `players` DROP COLUMN `redskulltime`;",
			"ALTER TABLE `players` CHANGE `redskull` `skull_type` INT NOT NULL DEFAULT 0;",
			"UPDATE      `players` SET `skull_type` = 4 WHERE `skull_type` = 1;",
			"ALTER TABLE `players` ADD `skull_time` INT UNSIGNED NOT NULL DEFAULT 0;",

			"ALTER TABLE `player_killers` ADD COLUMN `unjustified` TINYINT(1) NOT NULL DEFAULT 0;",

			NULL
		},
		{ // Sqlite
			NULL
		}
	},
	{ 19,
		{ // PgSql
			"",
			NULL
		},
		{ // MySql
			"ALTER TABLE `bans` CHANGE `expires` `expires` INT NOT NULL;",
			NULL
		},
		{ // Sqlite
			NULL
		}
	},
	{ 20,
		{ // PgSql
			"",
			NULL
		},
		{ // MySql
			"ALTER TABLE `tiles` ADD FOREIGN KEY (`house_id`) REFERENCES `houses`(`id`) ON DELETE NO ACTION;",
			"ALTER TABLE `bans` CHANGE COLUMN `admin_id` `admin_id` INT UNSIGNED",
			"UPDATE `bans` SET `admin_id` = NULL WHERE NOT EXISTS (SELECT 1 FROM `players` WHERE `id` = `bans`.`admin_id`);",
			"ALTER TABLE `bans` ADD FOREIGN KEY (`admin_id`) REFERENCES `players`(`id`) ON DELETE SET NULL;",

			"ALTER TABLE `guilds` CHANGE COLUMN `ownerid` `owner_id` INT UNSIGNED NOT NULL;",
			"DELETE FROM `guilds` WHERE NOT EXISTS (SELECT 1 FROM `players` WHERE `id` = `guilds`.`owner_id`);",
			"ALTER TABLE `guilds` ADD FOREIGN KEY (`owner_id`) REFERENCES `players`(`id`) ON DELETE CASCADE;",

			"DROP TRIGGER `ondelete_guilds`;",

			"CREATE TABLE `guild_members` (\n\
				`player_id` INT UNSIGNED NOT NULL,\n\
				`rank_id` INT UNSIGNED NOT NULL COMMENT 'a rank which belongs to certain guild',\n\
				`nick` VARCHAR(255) NOT NULL DEFAULT '',\n\
				UNIQUE (`player_id`),\n\
				FOREIGN KEY (`player_id`) REFERENCES `players` (`id`) ON DELETE CASCADE,\n\
				FOREIGN KEY (`rank_id`) REFERENCES `guild_ranks` (`id`) ON DELETE CASCADE\n\
			) ENGINE = InnoDB;",

			"INSERT INTO `guild_members`(`player_id`, `rank_id`, `nick`) SELECT `id`, `rank_id`, `guildnick` FROM `players` WHERE EXISTS "
				"(SELECT 1 FROM `guild_ranks` WHERE `guild_ranks`.`id` = `players`.`rank_id`);",

			"ALTER TABLE `players` DROP COLUMN `rank_id`;",
			"ALTER TABLE `players` DROP COLUMN `guildnick`;",

			"CREATE TABLE `guild_wars` (\n\
				`id` INT UNSIGNED NOT NULL AUTO_INCREMENT,\n\
				`guild_id` INT UNSIGNED NOT NULL COMMENT 'the guild which declared the war',\n\
				`opponent_id` INT UNSIGNED NOT NULL COMMENT 'the enemy guild at war',\n\
				`frag_limit` INT UNSIGNED NOT NULL DEFAULT 10 COMMENT 'kills needed to win the war',\n\
				`declaration_date` INT UNSIGNED NOT NULL,\n\
				`end_date` INT UNSIGNED NOT NULL,\n\
				`guild_fee` INT UNSIGNED NOT NULL DEFAULT 1000 COMMENT 'amount of money the guild has to pay if loses the war',\n\
				`opponent_fee` INT UNSIGNED NOT NULL DEFAULT 1000 COMMENT 'amount of money the enemy guild has to pay if loses the war',\n\
				`guild_frags` INT UNSIGNED NOT NULL DEFAULT 0,\n\
				`opponent_frags` INT UNSIGNED NOT NULL DEFAULT 0,\n\
				`comment` VARCHAR(255) NOT NULL DEFAULT '' COMMENT 'the guild leader can leave a message for the other guild',\n\
				`status` INT NOT NULL DEFAULT 0 COMMENT '-1 -> will be ignored (finished or unaccepted) 0 -> to be started 1 -> started/not finished',\n\
				PRIMARY KEY (`id`),\n\
				FOREIGN KEY (`guild_id`) REFERENCES `guilds` (`id`) ON DELETE CASCADE,\n\
				FOREIGN KEY (`opponent_id`) REFERENCES `guilds` (`id`) ON DELETE CASCADE\n\
			) ENGINE = InnoDB;",

			"ALTER TABLE `houses` ADD COLUMN `id` INT UNSIGNED NOT NULL AUTO_INCREMENT;",
			"ALTER TABLE `houses` ADD PRIMARY KEY(`id`);",

			NULL
		},
		{ // Sqlite
			NULL
		}
	},
	{21,
		{// PgSQL
			"",
			NULL
		},
		{// MySQL
			"ALTER TABLE `players` MODIFY COLUMN `health` INT(10) NOT NULL DEFAULT 100;",
			"ALTER TABLE `players` MODIFY COLUMN `healthmax` INT(10) NOT NULL DEFAULT 100;",
			"ALTER TABLE `players` MODIFY COLUMN `mana` INT(10) NOT NULL DEFAULT 100;",
			"ALTER TABLE `players` MODIFY COLUMN `manamax` INT(10) NOT NULL DEFAULT 100;",

			NULL
		},
		{// SQLite
			NULL
		}
	},
	// Empty, since somebody changed default value at schemas
	// to 22, so new users would have problems if next changes
	// were posted here, as nothing would have effect
	{22,
		{// PgSQL
			NULL
		},
		{// MySQL
			NULL
		},
		{// SQLite
			NULL
		}
	},
	{23,
		{// PgSQL
			NULL
		},
		{// MySQL
			"ALTER TABLE `players` ADD `guildnick` varchar(255) NOT NULL;", // for compatibility with __OLD_GUILD_SYSTEM__
			"ALTER TABLE `players` ADD `rank_id` INT(11) NOT NULL;",	// for compatibility with __OLD_GUILD_SYSTEM__
			NULL
		},
		{// SQLite
			NULL
		}
	},
	{24,
		{// PgSQL
			NULL
		},
		{// MySQL
		"CREATE TABLE `guild_invites` (\n\
			`player_id` INT UNSIGNED NOT NULL,\n\
			`guild_id` INT UNSIGNED NOT NULL COMMENT 'guild',\n\
			UNIQUE (`player_id`),\n\
			FOREIGN KEY (`player_id`) REFERENCES `players` (`id`) ON DELETE CASCADE,\n\
			FOREIGN KEY (`guild_id`) REFERENCES `guilds` (`id`) ON DELETE CASCADE\n\
		) ENGINE = InnoDB;"
			NULL
		},
		{// SQLite
			NULL
		}
	},
};

#endif
