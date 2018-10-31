#!/bin/bash

echo -e "Enter tag version, i.e. 2018.10-1"
read tag

git tag -a $tag -m "OSMC ${tag}"
echo -e "Tag $tag created successfully"
git push origin $tag
