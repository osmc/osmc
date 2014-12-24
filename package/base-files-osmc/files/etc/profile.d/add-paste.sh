paste() { a=$(cat); curl -X POST -s -d "$a" http://paste.osmc.io/documents | awk -F '"' '{print "http://paste.osmc.io/"$4}'; }
