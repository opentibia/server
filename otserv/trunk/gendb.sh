#!/bin/sh

#based on 
# http://otfans.net/wiki/index.php/After_compiling:_Creating_first_accounts

rm db.s3db 
sqlite3 db.s3db < schema.sqlite

sqlite3 db.s3db << _EOF
DELETE FROM \`players\`;
DELETE FROM \`accounts\`;
DELETE FROM \`groups\`;
INSERT INTO \`groups\` (\`id\`, \`name\`, \`flags\`, \`access\`, \`maxdepotitems\`, \`maxviplist\`) 
VALUES ('1', 'Player', 0, 0, 1000, 50);
INSERT INTO \`groups\` (\`id\`, \`name\`, \`flags\`, \`access\`, \`maxdepotitems\`, \`maxviplist\`) 
VALUES ('2', 'Premium Player', 0, 0, 2000, 100);
INSERT INTO \`groups\` (\`id\`, \`name\`, \`flags\`, \`access\`, \`maxdepotitems\`, \`maxviplist\`) 
VALUES ('3', 'Tutor', 16777216, 0, 1000, 50);
INSERT INTO \`groups\` (\`id\`, \`name\`, \`flags\`, \`access\`, \`maxdepotitems\`, \`maxviplist\`) 
VALUES ('4', 'Premium Tutor', 16777216, 0, 2000, 100);
INSERT INTO \`groups\` (\`id\`, \`name\`, \`flags\`, \`access\`, \`maxdepotitems\`, \`maxviplist\`) 
VALUES ('5', 'Gamemaster', 217768239050, 1, 2000, 100);
INSERT INTO \`groups\` (\`id\`, \`name\`, \`flags\`, \`access\`, \`maxdepotitems\`, \`maxviplist\`) 
VALUES ('6', 'Senior Gamemaster', 269307846602, 2, 2000, 100);
INSERT INTO \`groups\` (\`id\`, \`name\`, \`flags\`, \`access\`, \`maxdepotitems\`, \`maxviplist\`) 
VALUES ('7', 'Community Manager', 272227082232, 3, 2000, 100);
INSERT INTO \`accounts\` (\`id\`, \`name\`, \`password\`) 
VALUES ('1', '1', '1');
INSERT INTO \`accounts\` (\`id\`, \`name\`, \`password\`) 
VALUES ('111111', '111111', 'tibia');
INSERT INTO \`players\` (\`id\`, \`name\`, \`account_id\`, \`group_id\`, \`sex\`, \`rank_id\`, \`town_id\`, \`conditions\`) 
VALUES ('3', 'Player', '1', '1', '0', '0', '1', '0');
INSERT INTO \`players\` (\`id\`, \`name\`, \`account_id\`, \`group_id\`, \`sex\`, \`rank_id\`, \`town_id\`, \`conditions\`) 
VALUES ('0', 'Hurz', '111111', '1', '0', '0', '1', '0');
INSERT INTO \`players\` (\`id\`, \`name\`, \`account_id\`, \`group_id\`, \`sex\`, \`rank_id\`, \`town_id\`, \`conditions\`) 
VALUES ('1', 'Hurzine','111111', '1', '1', '0', '1', '0');
INSERT INTO \`players\` (\`id\`, \`name\`, \`account_id\`, \`group_id\`, \`sex\`, \`rank_id\`, \`town_id\`, \`conditions\`) 
VALUES ('2', 'Hurzel','111111', '1', '0', '0', '1', '0');
INSERT INTO \`players\` (\`id\`, \`name\`, \`account_id\`, \`group_id\`, \`sex\`, \`rank_id\`, \`town_id\`, \`conditions\`) 
VALUES ('4', 'Senior Gamemaster', '1', '5', '0', '1', '0', '0');
INSERT INTO \`players\` (\`id\`, \`name\`, \`account_id\`, \`group_id\`, \`sex\`, \`rank_id\`, \`town_id\`, \`conditions\`) 
VALUES ('5', 'GM Hurz', '111111', '5', '0', '0', '1', '0');
INSERT INTO \`player_items\` (\`player_id\`, \`sid\`, \`pid\`, \`itemtype\`, \`count\`, \`attributes\`)
VALUES ('5', '101', '4', '2638', '2', '0');
_EOF


