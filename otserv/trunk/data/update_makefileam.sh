#!/bin/sh
echo "#please update this file ONLY by using update_makefileam.sh" > Makefile.am
echo "#afterwards, if you're an official otserv developer, please" >> Makefile.am
echo "#commit your updated Makefile.am into the repo" >> Makefile.am
echo "pkgdata_DATA = \\" >> Makefile.am

# if it's .svn don't descend into it
# otherwise print all non-directories
find .  -path \*\/.svn\/\* -prune -o \! -type d -printf '\t%P \\\n' | sed 's/ [^\\n]/\\&/g' >> Makefile.am

echo "	#and that's it folks" >> Makefile.am

sed 's/Makefile\.in//g' Makefile.am | \
	sed 's/Makefile\.am//g' | \
	sed 's/Makefile//g' > Makefile.am.new
mv Makefile.am.new Makefile.am

echo "EXTRA_DIST = Makefile.am Makefile.in " '$(pkgdata_DATA)' >> Makefile.am


