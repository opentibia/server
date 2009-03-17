#!/bin/sh
echo "#please update this file ONLY by using update_makefileam.sh" > Makefile.am
echo "#afterwards, if you're an official otserv developer, please" >> Makefile.am
echo "#commit your updated makefile.am into the repo" >> Makefile.am
echo "ots_data = \\" >> Makefile.am

# if it's .svn don't descend into it
# otherwise print all non-directories
find .  -path \*\/.svn\/\* -prune -o \! -type d -printf '\t%P \\\n' | sed 's/ [^\\n]/\\&/g' >> Makefile.am

echo "	#and that's it folks" >> Makefile.am

sed 's/Makefile\.in//g' Makefile.am | \
	sed 's/Makefile\.am//g' | \
	sed 's/Makefile//g' > Makefile.am.new
mv Makefile.am.new Makefile.am

echo "EXTRA_DIST = Makefile.am Makefile.in " '$(ots_data)' >> Makefile.am

# example grabbed from autobook chapter 14.3
# this should properly preserve directory structure of files
cat >> Makefile.am << _EOF

install-data-local:
	@echo Installing data...
	@for f in \$(ots_data); do \\
## Compute the install directory at runtime.
	  d=\`echo \$\$f | sed -e s,/[^/]*\$\$,,\`; \\
## Make the install directory.
	  if [ \\! \$\$d == \$\$f ]; then \\
	    \$(mkinstalldirs) \$(DESTDIR)\$(pkgdatadir)/\$\$d; \\
	  fi; \\
## Find the header file -- in our case it might be in srcdir or
## it might be in the build directory.  "p" is the variable that
## names the actual file we will install.
	  if test -f \$(srcdir)/\$\$f; then p=\$(srcdir)/\$\$f; else p=\$\$f; fi; \\
##p=\$\$f ;
## Actually install the file.
	  \$(INSTALL_DATA) \$\$p \$(DESTDIR)\$(pkgdatadir)/\$\$f; \\
	done
	@echo Data installed


uninstall-local:
	@echo Uninstalling data...
	@for f in \$(ots_data); do \\
	  rm -f \$(DESTDIR)\$(pkgdatadir)/\$\$f; \\
	done
	@echo Data uninstalled

_EOF

