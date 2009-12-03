#!/bin/sh

#based on 
# http://otfans.net/wiki/index.php/After_compiling:_Creating_first_accounts

echo ""
echo "gendb.sh"
echo ""
if [ -z `which sqlite3` ] ; then
	echo "This utility requires sqlite3 to be somewhere in your \$PATH."
	exit 1
fi
echo "WARNING"
echo "This will reinitialize your db.s3db file from the current directory,"
echo "using schema.sqlite, and populate it with basic players and accounts."
echo "This means you LOSE ALL DATA in SQLite database db.s3db."
echo ""
echo "If you are upgrading, you should try the database utility!"
echo "If you don't care about your data, type 'yes, ok' below."
echo "Pressing enter cancels."
CONFIRM=""
while [ "$CONFIRM" != "yes, ok" ] ; do
	read CONFIRM
	if [ -z "$CONFIRM" ] ; then 
		echo "Cancelled. You need to type 'yes, ok' to proceed."
		exit 
	fi

	if [ "$CONFIRM" != "yes, ok" ] ; then echo "You must either press enter, or type 'yes, ok'" ; fi
done

echo "Reinitializing"

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
INSERT INTO \`players\` (\`id\`, \`name\`, \`account_id\`, \`group_id\`, \`sex\`, \`rank_id\`, \`town_id\`, \`conditions\`, \`cap\`) 
VALUES ('3', 'Player', '1', '1', '0', '0', '1', '0', '100');
INSERT INTO \`players\` (\`id\`, \`name\`, \`account_id\`, \`group_id\`, \`sex\`, \`rank_id\`, \`town_id\`, \`conditions\`, \`cap\`) 
VALUES ('0', 'Hurz', '111111', '1', '0', '0', '1', '0', '100');
INSERT INTO \`players\` (\`id\`, \`name\`, \`account_id\`, \`group_id\`, \`sex\`, \`rank_id\`, \`town_id\`, \`conditions\`, \`cap\`) 
VALUES ('1', 'Hurzine','111111', '1', '1', '0', '1', '0', '100');
INSERT INTO \`players\` (\`id\`, \`name\`, \`account_id\`, \`group_id\`, \`sex\`, \`rank_id\`, \`town_id\`, \`conditions\`, \`cap\`) 
VALUES ('2', 'Hurzel','111111', '1', '0', '0', '1', '0', '100');
INSERT INTO \`players\` (\`id\`, \`name\`, \`account_id\`, \`group_id\`, \`sex\`, \`rank_id\`, \`town_id\`, \`conditions\`, \`cap\`) 
VALUES ('4', 'Senior Gamemaster', '1', '6', '0', '1', '0', '0', '800');
INSERT INTO \`players\` (\`id\`, \`name\`, \`account_id\`, \`group_id\`, \`sex\`, \`rank_id\`, \`town_id\`, \`conditions\`, \`cap\`) 
VALUES ('5', 'Godly One', '1', '7', '0', '0', '1', '0', '800');
INSERT INTO \`player_items\` (\`player_id\`, \`sid\`, \`pid\`, \`itemtype\`, \`count\`, \`attributes\`)
VALUES ('5', '101', '4', '2638', '2', '0');

_EOF

echo "Done!"
