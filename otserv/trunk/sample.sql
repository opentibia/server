-- phpMyAdmin SQL Dump
-- version 2.6.0-pl3
-- http://www.phpmyadmin.net
-- 
-- Host: localhost
-- Generation Time: Mar 31, 2005 at 08:47 PM
-- Server version: 4.1.8
-- PHP Version: 5.0.3
-- 
-- Database: `otserv`
-- 

-- --------------------------------------------------------

-- 
-- Table structure for table `accounts`
-- 

CREATE TABLE `accounts` (
  `id` int(11) NOT NULL auto_increment,
  `accno` varchar(255) NOT NULL default '',
  `password` varchar(255) NOT NULL default '',
  `type` int(11) NOT NULL default '0',
  `premDays` int(11) NOT NULL default '0',
  UNIQUE KEY `id` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 AUTO_INCREMENT=2 ;

-- 
-- Dumping data for table `accounts`
-- 

INSERT INTO `accounts` (`id`, `accno`, `password`, `type`, `premDays`) VALUES (1, '1', 'test', 0, 0);

-- --------------------------------------------------------

-- 
-- Table structure for table `items`
-- 

CREATE TABLE `items` (
  `id` int(11) NOT NULL auto_increment,
  `player` int(11) NOT NULL default '0',
  `slot` tinyint(4) NOT NULL default '0',
  `sid` int(11) NOT NULL default '0',
  `pid` int(11) NOT NULL default '0',
  `type` int(11) NOT NULL default '0',
  `number` tinyint(4) NOT NULL default '0',
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 AUTO_INCREMENT=11 ;

-- 
-- Dumping data for table `items`
-- 

INSERT INTO `items` (`id`, `player`, `slot`, `sid`, `pid`, `type`, `number`) VALUES (9, 1, 3, 1, 0, 1988, 0);
INSERT INTO `items` (`id`, `player`, `slot`, `sid`, `pid`, `type`, `number`) VALUES (10, 1, 0, 2, 1, 2195, 0);

-- --------------------------------------------------------

-- 
-- Table structure for table `players`
-- 

CREATE TABLE `players` (
  `id` int(11) NOT NULL auto_increment,
  `name` varchar(255) collate armscii8_bin NOT NULL default '',
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
  `masterpos` varchar(255) collate armscii8_bin NOT NULL default '',
  `pos` varchar(255) collate armscii8_bin NOT NULL default '',
  `speed` int(11) NOT NULL default '0',
  `cap` int(11) NOT NULL default '0',
  `food` int(11) NOT NULL default '0',
  `sex` int(11) NOT NULL default '0',
  UNIQUE KEY `id` (`id`),
  KEY `name` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=armscii8 COLLATE=armscii8_bin AUTO_INCREMENT=2 ;

-- 
-- Dumping data for table `players`
-- 

INSERT INTO `players` (`id`, `name`, `access`, `account`, `level`, `vocation`, `cid`, `health`, `healthmax`, `direction`, `experience`, `lookbody`, `lookfeet`, `lookhead`, `looklegs`, `looktype`, `maglevel`, `mana`, `manamax`, `manaspent`, `masterpos`, `pos`, `speed`, `cap`, `food`, `sex`) VALUES (1, 0x4875727a, 1, 1, 18, 1, 675, 840, 840, 2, 76789, 20, 30, 40, 50, 128, 10, 640, 640, 4000, 0x313b313b36, 0x33313b33313b37, 900, 300, 129, 1);

-- --------------------------------------------------------

-- 
-- Table structure for table `skills`
-- 

CREATE TABLE `skills` (
  `player` int(11) NOT NULL default '0',
  `id` tinyint(4) NOT NULL default '0',
  `skill` int(11) NOT NULL default '0',
  `tries` int(11) NOT NULL default '0',
  KEY `player` (`player`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- 
-- Dumping data for table `skills`
-- 

INSERT INTO `skills` (`player`, `id`, `skill`, `tries`) VALUES (5, 6, 7, 8);
INSERT INTO `skills` (`player`, `id`, `skill`, `tries`) VALUES (1, 0, 0, 0);
INSERT INTO `skills` (`player`, `id`, `skill`, `tries`) VALUES (1, 1, 0, 0);
INSERT INTO `skills` (`player`, `id`, `skill`, `tries`) VALUES (1, 2, 0, 0);
INSERT INTO `skills` (`player`, `id`, `skill`, `tries`) VALUES (1, 3, 0, 0);
INSERT INTO `skills` (`player`, `id`, `skill`, `tries`) VALUES (1, 4, 0, 0);
INSERT INTO `skills` (`player`, `id`, `skill`, `tries`) VALUES (1, 5, 0, 0);
INSERT INTO `skills` (`player`, `id`, `skill`, `tries`) VALUES (1, 6, 0, 0);
        