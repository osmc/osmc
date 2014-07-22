#!/bin/bash
PATCHES=$(find ../patches/ -name "${1}-*.patch" -printf '%P\n')
for PATCH in $PATCHES
do
	cp ../patches/$PATCH .
	echo Applying patch $PATCH
	patch -p1 < $PATCH
	rm $PATCH
done
