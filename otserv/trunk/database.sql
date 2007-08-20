DROP TRIGGER IF EXISTS `oncreate_players`;
DROP TRIGGER IF EXISTS `oncreate_guilds`;
DROP TRIGGER IF EXISTS `ondelete_players`;
DROP TRIGGER IF EXISTS `ondelete_guilds`;
DROP TRIGGER IF EXISTS `ondelete_accounts`;

DROP TABLE IF EXISTS `player_depotitems`;
DROP TABLE IF EXISTS `tile_items`;
DROP TABLE IF EXISTS `tiles`;
DROP TABLE IF EXISTS `bans`;
DROP TABLE IF EXISTS `house_lists`;
DROP TABLE IF EXISTS `houses`;
DROP TABLE IF EXISTS `player_items`;
DROP TABLE IF EXISTS `player_skills`;
DROP TABLE IF EXISTS `player_storage`;
DROP TABLE IF EXISTS `player_viplist`;
DROP TABLE IF EXISTS `player_spells`;
DROP TABLE IF EXISTS `guild_ranks`;
DROP TABLE IF EXISTS `guilds`;
DROP TABLE IF EXISTS `players`;
DROP TABLE IF EXISTS `accounts`;
DROP TABLE IF EXISTS `groups`;

CREATE TABLE `groups` (
    `id` INT NOT NULL AUTO_INCREMENT,
    `name` VARCHAR(255) NOT NULL COMMENT 'group name',
    `flags` BIGINT UNSIGNED NOT NULL DEFAULT 0,
    `access` INT NOT NULL,
    `maxdepotitems` INT NOT NULL,
    `maxviplist` INT NOT NULL,
    PRIMARY KEY (`id`)
) ENGINE = InnoDB;

CREATE TABLE `accounts` (
    `id` INT NOT NULL AUTO_INCREMENT,
    `password` VARCHAR(255) NOT NULL/* VARCHAR(32) NOT NULL for MD5*/,
    `email` VARCHAR(255) NOT NULL DEFAULT '',
    `blocked` TINYINT(1) NOT NULL DEFAULT FALSE,
    `premdays` INT NOT NULL DEFAULT 0,
    PRIMARY KEY (`id`)
) ENGINE = InnoDB;

CREATE TABLE `players` (
    `id` INT NOT NULL AUTO_INCREMENT,
    `name` VARCHAR(255) NOT NULL,
    `account_id` INT NOT NULL,
    `group_id` INT NOT NULL COMMENT 'users group',
    `sex` INT UNSIGNED NOT NULL DEFAULT 0,
    `vocation` INT UNSIGNED NOT NULL DEFAULT 0,
    `experience` INT UNSIGNED NOT NULL DEFAULT 0,
    `level` INT UNSIGNED NOT NULL DEFAULT 1,
    `maglevel` INT UNSIGNED NOT NULL DEFAULT 0,
    `health` INT UNSIGNED NOT NULL DEFAULT 100,
    `healthmax` INT UNSIGNED NOT NULL DEFAULT 100,
    `mana` INT UNSIGNED NOT NULL DEFAULT 100,
    `manamax` INT UNSIGNED NOT NULL DEFAULT 100,
    `manaspent` INT UNSIGNED NOT NULL DEFAULT 0,
    `soul` INT UNSIGNED NOT NULL DEFAULT 0,
    `direction` INT UNSIGNED NOT NULL DEFAULT 0,
    `lookbody` INT UNSIGNED NOT NULL DEFAULT 10,
    `lookfeet` INT UNSIGNED NOT NULL DEFAULT 10,
    `lookhead` INT UNSIGNED NOT NULL DEFAULT 10,
    `looklegs` INT UNSIGNED NOT NULL DEFAULT 10,
    `looktype` INT UNSIGNED NOT NULL DEFAULT 136,
    `lookaddons` INT UNSIGNED NOT NULL DEFAULT 0,
    `posx` INT NOT NULL DEFAULT 0,
    `posy` INT NOT NULL DEFAULT 0,
    `posz` INT NOT NULL DEFAULT 0,
    `cap` INT NOT NULL DEFAULT 0,
    `lastlogin` INT UNSIGNED NOT NULL DEFAULT 0,
    `lastip` INT UNSIGNED NOT NULL DEFAULT 0,
    `save` TINYINT(1) NOT NULL DEFAULT TRUE,
    `conditions` BLOB NOT NULL COMMENT 'drunk, poisoned etc (maybe also food and redskull)',
    `redskulltime` INT UNSIGNED NOT NULL DEFAULT 0,
    `redskull` TINYINT(1) NOT NULL DEFAULT FALSE,
    `guildnick` VARCHAR(255) NOT NULL DEFAULT '' COMMENT 'additional nick in guild - mostly for web interfaces i think',
    `rank_id` INT NOT NULL COMMENT 'by this field everything with guilds is done - player has a rank which belongs to certain guild',
    `town_id` INT NOT NULL COMMENT 'old masterpos, temple spawn point position',
    `loss_experience` INT NOT NULL DEFAULT 10,
    `loss_mana` INT NOT NULL DEFAULT 10,
    `loss_skills` INT NOT NULL DEFAULT 10,
    PRIMARY KEY (`id`),
    KEY (`name`),
    FOREIGN KEY (`account_id`) REFERENCES `accounts`(`id`) ON DELETE CASCADE,
    FOREIGN KEY (`group_id`) REFERENCES `groups`(`id`)
) ENGINE = InnoDB;

CREATE TABLE `guilds` (
    `id` INT NOT NULL AUTO_INCREMENT,
    `name` VARCHAR(255) NOT NULL COMMENT 'guild name - nothing else needed here',
    `ownerid` INT NOT NULL,
    `creationdata` INT NOT NULL,
    PRIMARY KEY (`id`)
) ENGINE = InnoDB;

CREATE TABLE `guild_ranks` (
    `id` INT NOT NULL AUTO_INCREMENT,
    `guild_id` INT NOT NULL COMMENT 'guild',
    `name` VARCHAR(255) NOT NULL COMMENT 'rank name',
    `level` INT NOT NULL COMMENT 'rank level - leader, vice, member, maybe something else',
    PRIMARY KEY (`id`),
    FOREIGN KEY (`guild_id`) REFERENCES `guilds`(`id`) ON DELETE CASCADE
) ENGINE = InnoDB;

CREATE TABLE `player_viplist` (
    `player_id` INT NOT NULL COMMENT 'id of player whose viplist entry it is',
    `vip_id` INT NOT NULL COMMENT 'id of target player of viplist entry',
    FOREIGN KEY (`player_id`) REFERENCES `players`(`id`) ON DELETE CASCADE,
    FOREIGN KEY (`vip_id`) REFERENCES `players`(`id`) ON DELETE CASCADE
) ENGINE = InnoDB;

CREATE TABLE `player_spells` (
    `player_id` INT NOT NULL,
    `name` VARCHAR(255) NOT NULL,
    FOREIGN KEY (`player_id`) REFERENCES `players`(`id`) ON DELETE CASCADE
) ENGINE = InnoDB;

CREATE TABLE `player_storage` (
    `player_id` INT NOT NULL,
    `key` INT NOT NULL,
    `value` INT NOT NULL,
    FOREIGN KEY (`player_id`) REFERENCES `players`(`id`) ON DELETE CASCADE
) ENGINE = InnoDB;

CREATE TABLE `player_skills` (
    `player_id` INT NOT NULL,
    `skillid` INT UNSIGNED NOT NULL,
    `value` INT UNSIGNED NOT NULL DEFAULT 0,
    `count` INT UNSIGNED NOT NULL DEFAULT 0,
    FOREIGN KEY (`player_id`) REFERENCES `players`(`id`) ON DELETE CASCADE
) ENGINE = InnoDB;

CREATE TABLE `player_items` (
    `player_id` INT NOT NULL,
    `sid` INT NOT NULL,
    `pid` INT NOT NULL DEFAULT 0,
    `itemtype` INT NOT NULL,
    `count` INT NOT NULL DEFAULT 0,
    `attributes` BLOB COMMENT 'replaces unique_id, action_id, text, special_desc',
    FOREIGN KEY (`player_id`) REFERENCES `players`(`id`) ON DELETE CASCADE
) ENGINE = InnoDB;

CREATE TABLE `houses` (
    `id` INT NOT NULL AUTO_INCREMENT,
    `owner` INT NOT NULL,
    `paid` INT UNSIGNED NOT NULL DEFAULT 0,
    `warnings` TEXT NOT NULL,
    PRIMARY KEY (`id`)
) ENGINE = InnoDB;

CREATE TABLE `house_lists` (
    `house_id` INT NOT NULL,
    `listid` INT NOT NULL,
    `list` TEXT NOT NULL,
    FOREIGN KEY (`house_id`) REFERENCES `houses`(`id`) ON DELETE CASCADE
) ENGINE = InnoDB;

CREATE TABLE `bans` (
    `type` INT NOT NULL COMMENT 'this field defines if its ip, account, player, or any else ban',
    `ip` INT UNSIGNED NOT NULL DEFAULT 0,
    `mask` INT UNSIGNED NOT NULL DEFAULT 4294967295,
    `player` INT UNSIGNED NOT NULL DEFAULT 0,
    `account` INT UNSIGNED NOT NULL DEFAULT 0,
    `time` INT UNSIGNED NOT NULL DEFAULT 0
) ENGINE = InnoDB;

CREATE TABLE `tiles` (
    `id` INT NOT NULL AUTO_INCREMENT,
    `x` INT NOT NULL,
    `y` INT NOT NULL,
    `z` INT NOT NULL,
    PRIMARY KEY(`id`)
) ENGINE = InnoDB;

CREATE TABLE `tile_items` (
    `tile_id` INT NOT NULL,
    `sid` INT NOT NULL,
    `pid` INT NOT NULL DEFAULT 0,
    `itemtype` INT NOT NULL,
    `count` INT NOT NULL DEFAULT 0,
    `attributes` BLOB NOT NULL,
    FOREIGN KEY (`tile_id`) REFERENCES `tiles`(`id`) ON DELETE CASCADE
) ENGINE = InnoDB;

CREATE TABLE `player_depotitems` (
    `player_id` INT NOT NULL,
    `sid` INT NOT NULL COMMENT 'any given range eg 0-100 will be reserved for depot lockers and all > 100 will be then normal items inside depots',
    `pid` INT NOT NULL DEFAULT 0,
    `itemtype` INT NOT NULL,
    `count` INT NOT NULL DEFAULT 0,
    `attributes` BLOB NOT NULL,
    FOREIGN KEY (`player_id`) REFERENCES `players`(`id`) ON DELETE CASCADE,
    KEY (`player_id`, `depotid`)
) ENGINE = InnoDB;

DELIMITER |

CREATE TRIGGER `ondelete_accounts`
BEFORE DELETE
ON `accounts`
FOR EACH ROW
BEGIN
    DELETE FROM `bans` WHERE `type` = 3 AND `account` = OLD.`id`;
END|

CREATE TRIGGER `ondelete_guilds`
BEFORE DELETE
ON `guilds`
FOR EACH ROW
BEGIN
    UPDATE `players` SET `guildnick` = '', `rank_id` = 0 WHERE `rank_id` IN (SELECT `id` FROM `guild_ranks` WHERE `guild_id` = OLD.`id`);
END|

CREATE TRIGGER `ondelete_players`
BEFORE DELETE
ON `players`
FOR EACH ROW
BEGIN
    DELETE FROM `bans` WHERE `type` = 2 AND `player` = OLD.`id`;
    UPDATE `houses` SET `owner` = 0 WHERE `owner` = OLD.`id`;
END|

CREATE TRIGGER `oncreate_guilds`
AFTER INSERT
ON `guilds`
FOR EACH ROW
BEGIN
    INSERT INTO `guild_ranks` (`name`, `level`, `guild_id`) VALUES ('Leader', 3, NEW.`id`);
    INSERT INTO `guild_ranks` (`name`, `level`, `guild_id`) VALUES ('Vice-Leader', 2, NEW.`id`);
    INSERT INTO `guild_ranks` (`name`, `level`, `guild_id`) VALUES ('Member', 1, NEW.`id`);
END|

CREATE TRIGGER `oncreate_players`
AFTER INSERT
ON `players`
FOR EACH ROW
BEGIN
    INSERT INTO `player_skills` (`player_id`, `skillid`, `value`) VALUES (NEW.`id`, 0, 10);
    INSERT INTO `player_skills` (`player_id`, `skillid`, `value`) VALUES (NEW.`id`, 1, 10);
    INSERT INTO `player_skills` (`player_id`, `skillid`, `value`) VALUES (NEW.`id`, 2, 10);
    INSERT INTO `player_skills` (`player_id`, `skillid`, `value`) VALUES (NEW.`id`, 3, 10);
    INSERT INTO `player_skills` (`player_id`, `skillid`, `value`) VALUES (NEW.`id`, 4, 10);
    INSERT INTO `player_skills` (`player_id`, `skillid`, `value`) VALUES (NEW.`id`, 5, 10);
    INSERT INTO `player_skills` (`player_id`, `skillid`, `value`) VALUES (NEW.`id`, 6, 10);
END|

DELIMITER ;
