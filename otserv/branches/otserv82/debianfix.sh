#!/bin/sh

#
# Debian Lenny fix for OtServ 1.0
# Written by Ivan Vucica
# This file is released into public domain.

# Only in areas where above statement is not legally applicable:
# (c) 2007-2008 Ivan Vucica
# This is free software: you are free to change and redistribute it.
# There is NO WARRANTY, to the extent permitted by law. 

# This file avoids the need to compile and install libraries that 
# still do not ship in Debian Lenny as of March 19 2008.
# Do not apply unless using Debian Lenny.

echo "Debian fix for OtServ"
echo "---------------------"

echo "Replacing boost::system..."
find \( -name '*.cpp' -or -name '*.h' \) -exec sed -i 's/boost::system::/asio::/g' {} \; -print
echo "Replacing boost::asio..."
find \( -name '*.cpp' -or -name '*.h' \) -exec sed -i 's/boost::asio::/asio::/g' {} \; -print
echo "Replacing boost/asio.hpp..."
find \( -name '*.cpp' -or -name '*.h' \) -exec sed -i 's/boost\/asio.hpp/asio.hpp/g' {} \; -print

echo "Replacing autoconf stuff..."
sed -i 's/AC_MSG_ERROR("Linking against boost::system library failed.")/AC_MSG_WARN("Linking against boost::system library failed.")/g' configure.ac
echo "Replacing autoconf boost/asio.hpp..."
sed -i 's/boost\/asio.hpp/asio.hpp/g' configure.ac

