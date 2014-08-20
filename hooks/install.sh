#!/bin/sh

HOOKS="pre-commit"

for hook in $HOOKS
do
echo Installing $hook hook
cp -ar $hook ../.git/hooks/
chmod +x ../.git/hooks/$hook
done
