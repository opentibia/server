CREATE DATABASE `otserv2` DEFAULT CHARACTER SET latin1 COLLATE latin1_bin;

USE `otserv2`;

DROP TABLE IF EXISTS `item`;

CREATE TABLE `item` (
	`itemid` int(11) UNSIGNED NOT NULL AUTO_INCREMENT,
	`itemname` varchar(255) NOT NULL DEFAULT '',
	`tibiaid` int(11) NOT NULL DEFAULT '0',
	`weight`  float(2),
	UNIQUE KEY `tibiaid` (`tibiaid`),
	PRIMARY KEY (itemid)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;


DROP TABLE IF EXISTS `accountgroup`;

CREATE TABLE `accountgroup` (
	`accountgroupid` int(11) UNSIGNED NOT NULL AUTO_INCREMENT,
	`accountgroupname` varchar(255) NOT NULL DEFAULT '',
	`accountgrouptype` enum('player', 'bigplayer', 'tutor', 'high_tutor', 'consellor', 'gamemaster', 'senator','god','administrator_non_god','administrator_god') NOT NULL,
	`maxplayers` int(11) NOT NULL DEFAULT '15',
  `save` int(11) UNSIGNED NOT NULL DEFAULT '1',
	PRIMARY KEY (accountgroupid),
	UNIQUE KEY `accountgroupname` (`accountgroupname`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

DROP TABLE IF EXISTS `account`;

CREATE TABLE `account` (
  `accountid` int(11) UNSIGNED NOT NULL AUTO_INCREMENT,
  `accountno` int(11) UNSIGNED NOT NULL DEFAULT '0',
  `accountgroupid` int(11) UNSIGNED NOT NULL REFERENCES accountgroup(accountgroupid),
  `password` varchar(255) NOT NULL DEFAULT '',
  `premDays` int(11) NOT NULL DEFAULT '0',
	PRIMARY KEY (accountid),
	UNIQUE KEY `accountno` (`accountno`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

DROP TABLE IF EXISTS `playergroup`;

CREATE TABLE `playergroup` (
	`playergroupid` int(11) UNSIGNED NOT NULL AUTO_INCREMENT,
	`playergroupname` varchar(255) NOT NULL DEFAULT '',
	`maxdepotitems` int(11) NOT NULL default '1000',
	PRIMARY KEY (playergroupid),
	UNIQUE KEY `playergroupname` (`playergroupname`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

DROP TABLE IF EXISTS `player`;

CREATE TABLE `player` (
  `playerid` int(11) UNSIGNED NOT NULL AUTO_INCREMENT,
  `accountid` int(11) UNSIGNED NOT NULL REFERENCES account(accountid),
  `playername` varchar(255) NOT NULL DEFAULT '',
  `playergroupid` int(11) UNSIGNED NOT NULL REFERENCES playergroup(playergroupid),
  `sex` enum('male', 'female') NOT NULL,
  `vocation` enum('knight', 'palladin', 'soccerer', 'druid','no vocation') NOT NULL,
  `playertype` enum('normal', 'special') NOT NULL,
  `looktype` int(11) NOT NULL DEFAULT '0',
  `lookhead` int(11) NOT NULL DEFAULT '0',
  `lookbody` int(11) NOT NULL DEFAULT '0',
  `looklegs` int(11) NOT NULL DEFAULT '0',
  `lookfeet` int(11) NOT NULL DEFAULT '0',
  `masterposx` int(11) NOT NULL DEFAULT '0',
  `masterposy` int(11) NOT NULL DEFAULT '0',
  `masterposz` int(11) NOT NULL DEFAULT '0',
  `masterposdir` int(11) NOT NULL DEFAULT '0', 
  `currentposx` int(11) NOT NULL DEFAULT '0',
  `currentposy` int(11) NOT NULL DEFAULT '0',
  `currentposz` int(11) NOT NULL DEFAULT '0',
  `currentposdir` int(11) NOT NULL DEFAULT '0',   
  `experience` int(11) NOT NULL DEFAULT '0',
  `level` int(11) NOT NULL DEFAULT '0',
  `health` int(11) NOT NULL DEFAULT '0',
  `healthmax` int(11) NOT NULL DEFAULT '0',
  `speed` int(11) NOT NULL DEFAULT '0',
  `cap` int(11) NOT NULL DEFAULT '0',
  `manaspent` int(11) NOT NULL DEFAULT '0',
  `maglevel` int(11) NOT NULL DEFAULT '0',
  `mana` int(11) NOT NULL DEFAULT '0',
  `manamax` int(11) NOT NULL DEFAULT '0',
  `food` int(11) NOT NULL DEFAULT '0',
  `lastlogin` int(11) UNSIGNED NOT NULL DEFAULT '0',
	PRIMARY KEY (playerid),
	UNIQUE KEY `playername` (`playername`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
