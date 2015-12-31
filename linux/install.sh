#!/usr/bin/env bash
#
# Installer script
#
# We expect that Radian has already been built successfully.
# We will copy the executable and support files into the standard locations.

cp radian /usr/bin/radian
if [[ -e /usr/lib/radian ]]
then
	rm -rf /usr/lib/radian
fi
cp -R ../library /usr/lib/radian
cp libstdradian.a /usr/lib/radian/
which radian

