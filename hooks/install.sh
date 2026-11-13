#!/bin/sh
#
# Point git at this directory, so hooks arrive with a clone and update with a
# pull. The old behaviour copied each hook into .git/hooks, which meant a hook
# improved later stayed old in everyone's checkout until they reinstalled.
#
# core.hooksPath cannot be committed into the repository: it lives in
# .git/config, which is not tracked, because git will not let a repository
# configure itself to run its own scripts on clone. So this is one command per
# clone -- but only one, ever.
#
# Run with --copy to get the old copying behaviour instead, for a git older
# than 2.9.

cd "$(dirname "$0")" || exit 1

HOOKS="pre-commit commit-msg pre-push"

for hook in $HOOKS
do
	chmod +x "$hook"
done

if [ "$1" = "--copy" ]
then
	for hook in $HOOKS
	do
		echo "Installing $hook hook"
		cp -a "$hook" ../.git/hooks/ && chmod +x "../.git/hooks/$hook"
	done
	echo
	echo "Copied. Re-run this after pulling a change to any hook."
	exit 0
fi

git config core.hooksPath hooks || exit 1

echo "core.hooksPath set to hooks/"
echo
echo "Active hooks:"
for hook in $HOOKS
do
	echo "  $hook"
done
echo
echo "They now update with a pull. Nothing to reinstall."
