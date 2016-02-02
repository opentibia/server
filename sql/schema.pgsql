-- WORLD
CREATE TABLE "worlds" (
	"id" SERIAL PRIMARY KEY,
	"name" VARCHAR(32) NOT NULL,
	"ip" BIGINT NOT NULL DEFAULT 0,
	"port" INT NOT NULL
);
CREATE INDEX "worlds_name_index" ON "worlds" USING HASH ("name");

CREATE TABLE "global_storage" (
	"id" TEXT NOT NULL,
	"value" TEXT NOT NULL
);
CREATE INDEX "global_storage_id" ON "global_storage" USING HASH ("id");

CREATE TABLE "schema_info" (
	"name" VARCHAR(255) PRIMARY KEY,
	"value" VARCHAR(255) NOT NULL
);

INSERT INTO "schema_info" ("name", "value") VALUES ('version', 25);

-- Player-related
CREATE TABLE "groups" (
	"id" SERIAL PRIMARY KEY,
	"name" VARCHAR(255) NOT NULL,
	"flags" BIGINT NOT NULL DEFAULT 0,
	"access" INT NOT NULL DEFAULT 0,
	"violation" INT NOT NULL DEFAULT 0,
	"maxdepotitems" INT NOT NULL,
	"maxviplist" INT NOT NULL
);

CREATE TABLE "accounts" (
	"id" SERIAL PRIMARY KEY,
	"name" VARCHAR(32) NOT NULL UNIQUE,
	"password" VARCHAR(255) NOT NULL,
	"email" VARCHAR(255) NOT NULL DEFAULT '',
	"premend" BIGINT NOT NULL DEFAULT 0,
	"blocked" SMALLINT NOT NULL DEFAULT 0,
	"warnings" INT NOT NULL DEFAULT 0
);

CREATE TABLE "players" (
	"id" SERIAL PRIMARY KEY,
	"name" VARCHAR(255) NOT NULL UNIQUE,
	"account_id" INT NOT NULL REFERENCES "accounts" ON DELETE CASCADE,
	"group_id" INT NOT NULL REFERENCES "groups",
	"world_id" INT NOT NULL REFERENCES "worlds" ON DELETE CASCADE,
	"town_id" INT NOT NULL,
	"sex" SMALLINT NOT NULL DEFAULT 0,
	"vocation" SMALLINT NOT NULL DEFAULT 0,

	"experience" BIGINT NOT NULL DEFAULT 0,
	"level" BIGINT NOT NULL DEFAULT 1,
	"maglevel" BIGINT NOT NULL DEFAULT 0,
	"health" BIGINT NOT NULL DEFAULT 100,
	"healthmax" BIGINT NOT NULL DEFAULT 100,
	"mana" BIGINT NOT NULL DEFAULT 100,
	"manamax" BIGINT NOT NULL DEFAULT 100,
	"manaspent" BIGINT NOT NULL DEFAULT 0,
	"soul" BIGINT NOT NULL DEFAULT 0,
	"cap" INT NOT NULL DEFAULT 0,
	"stamina" INT NOT NULL DEFAULT 151200000,
	"lookbody" BIGINT NOT NULL DEFAULT 10,
	"lookfeet" BIGINT NOT NULL DEFAULT 10,
	"lookhead" BIGINT NOT NULL DEFAULT 10,
	"looklegs" BIGINT NOT NULL DEFAULT 10,
	"looktype" BIGINT NOT NULL DEFAULT 136,
	"lookaddons" BIGINT NOT NULL DEFAULT 0,

	"posx" INT NOT NULL DEFAULT 0,
	"posy" INT NOT NULL DEFAULT 0,
	"posz" INT NOT NULL DEFAULT 0,
	"direction" SMALLINT NOT NULL DEFAULT 0,
	"lastlogin" BIGINT NOT NULL DEFAULT 0,
	"lastlogout" BIGINT NOT NULL DEFAULT 0,
	"lastip" BIGINT NOT NULL DEFAULT 0,
	"save" SMALLINT NOT NULL DEFAULT 1,
	"conditions" BYTEA NOT NULL,
	"skull_type" SMALLINT NOT NULL DEFAULT 0,
	"skull_time" BIGINT NOT NULL DEFAULT 0,
	"loss_experience" INT NOT NULL DEFAULT 100,
	"loss_mana" INT NOT NULL DEFAULT 100,
	"loss_skills" INT NOT NULL DEFAULT 100,
	"loss_items" INT NOT NULL DEFAULT 10,
	"loss_containers" INT NOT NULL DEFAULT 100,

	"online" SMALLINT NOT NULL DEFAULT 0
);
CREATE INDEX "player_online_index" ON "players" USING HASH ("online");

CREATE TABLE "player_viplist" (
	"player_id" INT NOT NULL REFERENCES "players" ON DELETE CASCADE,
	"vip_id" INT NOT NULL REFERENCES "players" ON DELETE CASCADE
);

CREATE TABLE "player_storage" (
	"player_id" INT NOT NULL REFERENCES "players" ON DELETE CASCADE,
	"id" TEXT NOT NULL,
	"value" TEXT NOT NULL
);
CREATE INDEX "player_storage_ids" ON "player_storage" ("player_id", "id");

CREATE TABLE "player_skills" (
	"player_id" INT NOT NULL REFERENCES "players" ON DELETE CASCADE,
	"skill_id" BIGINT NOT NULL,
	"value" BIGINT NOT NULL DEFAULT 0,
	"count" BIGINT NOT NULL DEFAULT 0
);

CREATE TABLE "player_deaths" (
	"id" SERIAL PRIMARY KEY,
	"player_id" INT NOT NULL REFERENCES "players" ON DELETE CASCADE,
	"date" INT NOT NULL,
	"level" INT NOT NULL
);
CREATE INDEX "player_deaths_date_index" ON "player_deaths" USING HASH ("date");

CREATE TABLE "killers" (
	"id" SERIAL PRIMARY KEY,
	"death_id" INT NOT NULL REFERENCES "player_deaths" ON DELETE CASCADE,
	"final_hit" SMALLINT NOT NULL DEFAULT 1
);

CREATE TABLE "environment_killers" (
	"kill_id" INT NOT NULL REFERENCES "killers" ON DELETE CASCADE,
	"name" VARCHAR(255) NOT NULL,
	PRIMARY KEY ("kill_id", "name")
);

CREATE TABLE "player_killers" (
	"kill_id" INT NOT NULL REFERENCES "killers" ON DELETE CASCADE,
	"player_id" INT NOT NULL REFERENCES "players" ON DELETE CASCADE,
	"unjustified" SMALLINT NOT NULL DEFAULT 0,
	PRIMARY KEY ("kill_id", "player_id")
);

-- GUILDS
CREATE TABLE "guilds" (
	"id" SERIAL PRIMARY KEY,
	"name" VARCHAR(255) NOT NULL,
	"owner_id" INT NOT NULL REFERENCES "players" ON DELETE CASCADE,
	"creation_time" INT NOT NULL
);
CREATE INDEX "guilds_name_index" ON "guilds" USING HASH ("name");

CREATE TABLE "guild_ranks" (
	"id" SERIAL PRIMARY KEY,
	"guild_id" INT NOT NULL REFERENCES "guilds" ON DELETE CASCADE,
	"name" VARCHAR(255) NOT NULL,
	"level" INT NOT NULL
);
CREATE INDEX "guilds_ranks_level_index" ON "guild_ranks" USING HASH ("level");

CREATE TABLE "guild_members" (
	"player_id" INT NOT NULL REFERENCES "players" ON DELETE CASCADE,
	"rank_id" INT NOT NULL REFERENCES "guild_ranks" ON DELETE CASCADE,
	"nick" TEXT
);

-- HOUSES
CREATE TABLE "houses" (
	"id" SERIAL PRIMARY KEY,
	"world_id" INT NOT NULL REFERENCES "worlds" ON DELETE CASCADE,
	"map_id" INT NOT NULL,
	"town_id" INT NOT NULL DEFAULT 0,
	"owner_id" INT DEFAULT NULL REFERENCES "players" ON DELETE SET NULL,
	"name" VARCHAR(100) NOT NULL,
	"rent" INT NOT NULL DEFAULT 0,
	"guildhall" SMALLINT NOT NULL DEFAULT 0,
	"tiles" INT NOT NULL DEFAULT 0,
	"doors" INT NOT NULL DEFAULT 0,
	"beds" INT NOT NULL DEFAULT 0,
	"paid" BIGINT NOT NULL DEFAULT 0,
	"clear" SMALLINT NOT NULL DEFAULT 0,
	"warnings" INT NOT NULL DEFAULT 0,
	"lastwarning" BIGINT NOT NULL DEFAULT 0
);
CREATE INDEX "houses_map_id_index" ON "houses" USING HASH ("map_id");
CREATE INDEX "houses_town_id_index" ON "houses" USING HASH ("town_id");

CREATE TABLE "house_auctions" (
	"house_id" INT NOT NULL REFERENCES "houses" ON DELETE CASCADE,
	"player_id" INT NOT NULL REFERENCES "players" ON DELETE CASCADE,
	"bid" INT NOT NULL DEFAULT 0,
	"limit" INT NOT NULL DEFAULT 0,
	"endtime" BIGINT NOT NULL DEFAULT 0
);

CREATE TABLE "house_lists" (
	"house_id" INT NOT NULL REFERENCES "houses" ON DELETE CASCADE,
	"listid" INT NOT NULL,
	"list" TEXT NOT NULL
);

-- BANS
CREATE TABLE "bans" (
	"id" SERIAL PRIMARY KEY,
	"expires" BIGINT NOT NULL,
	"added" BIGINT NOT NULL,
	"active" SMALLINT DEFAULT 0,
	"admin_id" INT DEFAULT NULL REFERENCES "accounts" ON DELETE SET NULL,
	"comment" TEXT,
	"reason" INT DEFAULT 0,
	"action" INT DEFAULT 0,
	"statement" VARCHAR(255) DEFAULT ''
);

CREATE TABLE "account_bans" (
	"ban_id" INT NOT NULL REFERENCES "bans" ON DELETE CASCADE,
	"account_id" INT NOT NULL REFERENCES "accounts" ON DELETE CASCADE
);

CREATE TABLE "ip_bans" (
	"ban_id" INT NOT NULL REFERENCES "bans" ON DELETE CASCADE,
	"ip" INT NOT NULL,
	"mask" INT NOT NULL
);

CREATE TABLE "player_bans" (
	"ban_id" INT NOT NULL REFERENCES "bans" ON DELETE CASCADE,
	"player_id" INT NOT NULL REFERENCES "players" ON DELETE CASCADE
);

-- MAPS
CREATE TABLE "item_containers" (
	"id" SERIAL PRIMARY KEY
);

CREATE TABLE "items" (
	"container_id" INT NOT NULL REFERENCES "item_containers" ON DELETE CASCADE,
	"id" INT NOT NULL,
	"parent_id" INT DEFAULT NULL,
	"count" SMALLINT NOT NULL,
	"attributes" BYTEA
);
CREATE INDEX "items_id_index" ON "items" USING HASH ("id");

CREATE TABLE "tiles" (
	"world_id" INT NOT NULL REFERENCES "worlds" ON DELETE CASCADE,
	"house_id" INT NOT NULL DEFAULT NULL REFERENCES "houses" ON DELETE SET NULL,
	"container_id" INT NOT NULL REFERENCES "item_containers" ON DELETE CASCADE,
	"x" INT NOT NULL,
	"y" INT NOT NULL,
	"z" INT NOT NULL
);
CREATE INDEX "tiles_coordinates_index" ON "tiles" ("x", "y", "z");

CREATE TABLE "player_items" (
	"player_id" INT NOT NULL REFERENCES "players" ON DELETE CASCADE,
	"container_id" INT NOT NULL REFERENCES "item_containers" ON DELETE CASCADE
);

CREATE TABLE "player_depot" (
	"depot_id" INT NOT NULL,
	"player_id" INT NOT NULL REFERENCES "players" ON DELETE CASCADE,
	"container_id" INT NOT NULL REFERENCES "item_containers" ON DELETE CASCADE
);
CREATE INDEX "player_depot_id_index" ON "player_depot" USING HASH ("depot_id");

CREATE TABLE "map_store" (
	"house_id" INT NOT NULL REFERENCES "houses" ON DELETE CASCADE,
	"world_id" INT NOT NULL REFERENCES "worlds" ON DELETE CASCADE,
	"data" BYTEA NOT NULL
);

INSERT INTO "groups" ("id", "name", "flags", "access", "maxdepotitems", "maxviplist") 
	VALUES ('1', 'Player', 0, 0, 1000, 50);
INSERT INTO "groups" ("id", "name", "flags", "access", "maxdepotitems", "maxviplist") 
	VALUES ('2', 'Premium Player', 0, 0, 2000, 100);
INSERT INTO "groups" ("id", "name", "flags", "access", "maxdepotitems", "maxviplist") 
	VALUES ('3', 'Tutor', 16777216, 0, 1000, 50);
INSERT INTO "groups" ("id", "name", "flags", "access", "maxdepotitems", "maxviplist") 
	VALUES ('4', 'Premium Tutor', 16777216, 0, 2000, 100);
INSERT INTO "groups" ("id", "name", "flags", "access", "maxdepotitems", "maxviplist") 
	VALUES ('5', 'Gamemaster', 217768239050, 1, 2000, 300);
INSERT INTO "groups" ("id", "name", "flags", "access", "maxdepotitems", "maxviplist") 
	VALUES ('6', 'Senior Gamemaster', 269307846602, 2, 2000, 300);
INSERT INTO "groups" ("id", "name", "flags", "access", "maxdepotitems", "maxviplist") 
	VALUES ('7', 'Community Manager', 272227082232, 3, 2000, 300);
INSERT INTO "groups" ("id", "name", "flags", "access", "maxdepotitems", "maxviplist") 
	VALUES ('8', 'Server Administrator', 821982896120, 3, 2000, 300);

CREATE FUNCTION "oncreate_guilds"()
RETURNS TRIGGER
AS $$
BEGIN
	INSERT INTO "guild_ranks" ("name", "level", "guild_id") VALUES ('Leader', 3, NEW."id");
	INSERT INTO "guild_ranks" ("name", "level", "guild_id") VALUES ('Vice-Leader', 2, NEW."id");
	INSERT INTO "guild_ranks" ("name", "level", "guild_id") VALUES ('Member', 1, NEW."id");

	RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER "oncreate_guilds"
AFTER INSERT
ON "guilds"
FOR EACH ROW
EXECUTE PROCEDURE "oncreate_guilds"();

CREATE FUNCTION "oncreate_players"()
RETURNS TRIGGER
AS $$
BEGIN
	INSERT INTO "player_skills" ("player_id", "skill_id", "value") VALUES (NEW."id", 0, 10);
	INSERT INTO "player_skills" ("player_id", "skill_id", "value") VALUES (NEW."id", 1, 10);
	INSERT INTO "player_skills" ("player_id", "skill_id", "value") VALUES (NEW."id", 2, 10);
	INSERT INTO "player_skills" ("player_id", "skill_id", "value") VALUES (NEW."id", 3, 10);
	INSERT INTO "player_skills" ("player_id", "skill_id", "value") VALUES (NEW."id", 4, 10);
	INSERT INTO "player_skills" ("player_id", "skill_id", "value") VALUES (NEW."id", 5, 10);
	INSERT INTO "player_skills" ("player_id", "skill_id", "value") VALUES (NEW."id", 6, 10);

	RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER "oncreate_players"
AFTER INSERT
ON "players"
FOR EACH ROW
EXECUTE PROCEDURE "oncreate_players"();
