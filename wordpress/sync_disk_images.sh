#!/bin/bash
VERSIONS="RBP
RBP2"

for VERSION in $VERSIONS
do
wget http://download.osmc.tv/installers/versions_$VERSION -O /var/www/osmc-blog/htdocs/parse/versions_$VERSION
done
