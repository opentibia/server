-- MySQL dump 10.9
--
-- Host: localhost    Database: otserv
-- ------------------------------------------------------
-- Server version	4.1.12a-nt

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;


CREATE DATABASE /*!32312 IF NOT EXISTS*/ `otserv` /*!40100 DEFAULT CHARACTER SET latin1 */;

USE `otserv`;

--
-- Table structure for table `accounts`
--

DROP TABLE IF EXISTS `accounts`;
CREATE TABLE `accounts` (
  `id` int(11) NOT NULL auto_increment,
  `accno` int(11) unsigned NOT NULL default '0',
  `password` varchar(32) NOT NULL default '',
  `type` int(11) NOT NULL default '0',
  `premDays` int(11) NOT NULL default '0',
  `email` varchar(50) NOT NULL default '',
  `blocked` tinyint(4) NOT NULL default '0',
  UNIQUE KEY `id` (`id`),
  KEY `accno` (`accno`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Dumping data for table `accounts`
--


/*!40000 ALTER TABLE `accounts` DISABLE KEYS */;
LOCK TABLES `accounts` WRITE;
INSERT INTO `accounts` VALUES (1,1,'test',0,0,'',0);
UNLOCK TABLES;
/*!40000 ALTER TABLE `accounts` ENABLE KEYS */;

--
-- Table structure for table `items`
--

DROP TABLE IF EXISTS `items`;
CREATE TABLE `items` (
  `player` int(11) NOT NULL default '0',
  `slot` tinyint(4) NOT NULL default '0',
  `sid` int(11) NOT NULL default '0',
  `pid` int(11) NOT NULL default '0',
  `type` int(11) NOT NULL default '0',
  `number` tinyint(4) NOT NULL default '0',
  `actionid` int(5) NOT NULL default '0',
  `text` text NOT NULL default '',
  `specialdesc` text NOT NULL default '',
  KEY `player` (`player`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Dumping data for table `items`
--


/*!40000 ALTER TABLE `items` DISABLE KEYS */;
LOCK TABLES `items` WRITE;
INSERT INTO `items` VALUES (1,0,15,14,1968,0,0,'MySQL test',''),(1,0,14,13,2594,0,0,'',''),(1,101,13,0,2590,0,0,'',''),(1,10,12,0,2544,100,0,'',''),(1,9,11,0,2169,0,0,'',''),(1,8,10,0,2195,0,0,'',''),(1,7,9,0,2477,0,0,'',''),(1,6,8,0,2542,0,0,'',''),(1,5,7,0,2419,0,0,'',''),(1,4,6,0,2653,0,0,'',''),(1,0,5,3,2456,0,0,'',''),(1,0,4,3,2544,100,0,'',''),(1,3,3,0,2002,0,0,'',''),(1,2,2,0,2199,0,0,'',''),(1,1,1,0,2496,0,0,'','');
UNLOCK TABLES;
/*!40000 ALTER TABLE `items` ENABLE KEYS */;

--
-- Table structure for table `players`
--

DROP TABLE IF EXISTS `players`;
CREATE TABLE `players` (
  `id` int(11) unsigned NOT NULL auto_increment,
  `name` varchar(32) NOT NULL default '',
  `access` int(11) NOT NULL default '0',
  `account` int(11) NOT NULL default '0',
  `level` int(11) NOT NULL default '0',
  `vocation` int(11) NOT NULL default '0',
  `cid` int(11) NOT NULL default '0',
  `health` int(11) NOT NULL default '0',
  `healthmax` int(11) NOT NULL default '0',
  `direction` int(11) NOT NULL default '0',
  `experience` int(11) NOT NULL default '0',
  `lookbody` int(11) NOT NULL default '0',
  `lookfeet` int(11) NOT NULL default '0',
  `lookhead` int(11) NOT NULL default '0',
  `looklegs` int(11) NOT NULL default '0',
  `looktype` int(11) NOT NULL default '0',
  `maglevel` int(11) NOT NULL default '0',
  `mana` int(11) NOT NULL default '0',
  `manamax` int(11) NOT NULL default '0',
  `manaspent` int(11) NOT NULL default '0',
  `masterpos` varchar(16) NOT NULL default '',
  `pos` varchar(16) NOT NULL default '',
  `speed` int(11) NOT NULL default '0',
  `cap` int(11) NOT NULL default '0',
  `maxdepotitems` int(11) NOT NULL default '1000',
  `food` int(11) NOT NULL default '0',
  `sex` int(11) NOT NULL default '0',
  `guildname` varchar(32) NOT NULL default '',
  `guildrank` varchar(32) NOT NULL default '',
  `lastlogin` int(11) unsigned NOT NULL default '0',
  `lastip` int(11) unsigned NOT NULL default '0',
  `save` int(11) unsigned NOT NULL default '1',
  UNIQUE KEY `id` (`id`),
  KEY `name` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Dumping data for table `players`
--


/*!40000 ALTER TABLE `players` DISABLE KEYS */;
LOCK TABLES `players` WRITE;
INSERT INTO `players` VALUES (1,'Hurz',1,1,18,1,675,840,840,2,76000,20,30,40,50,128,20,640,640,21700,'20;20;7','29;30;7',900,300,1000,129,1,0,'',1);
INSERT INTO `players` VALUES (2,'Player',0,1,18,1,675,840,840,2,76000,20,30,40,50,128,20,640,640,21700,'27;23;7','27;23;7',900,300,1000,129,1,0,'',1);
UNLOCK TABLES;
/*!40000 ALTER TABLE `players` ENABLE KEYS */;

--
-- Table structure for table `playerstorage`
--

DROP TABLE IF EXISTS `playerstorage`;
CREATE TABLE `playerstorage` (
  `player` int(11) NOT NULL default '0',
  `key` int(11) unsigned NOT NULL default '0',
  `value` int(11) NOT NULL default '0',
  KEY `player` (`player`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Dumping data for table `playerstorage`
--


/*!40000 ALTER TABLE `playerstorage` DISABLE KEYS */;
LOCK TABLES `playerstorage` WRITE;
INSERT INTO `playerstorage` VALUES (1,1000,4);
UNLOCK TABLES;
/*!40000 ALTER TABLE `playerstorage` ENABLE KEYS */;

--
-- Table structure for table `skills`
--

DROP TABLE IF EXISTS `skills`;
CREATE TABLE `skills` (
  `player` int(11) NOT NULL default '0',
  `id` tinyint(4) NOT NULL default '0',
  `skill` int(11) unsigned NOT NULL default '0',
  `tries` int(11) unsigned NOT NULL default '0',
  KEY `player` (`player`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Dumping data for table `skills`
--


/*!40000 ALTER TABLE `skills` DISABLE KEYS */;
LOCK TABLES `skills` WRITE;
INSERT INTO `skills` VALUES (1,6,10,0),(1,5,10,0),(1,4,10,0),(1,3,10,0),(1,2,10,0),(1,1,10,0),(1,0,10,0);
UNLOCK TABLES;
/*!40000 ALTER TABLE `skills` ENABLE KEYS */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

