#!/bin/bash

# Allows mass Jenkins jobs creation via CLI

token=""
URL="https://jenkins.osmc.tv/job/Package/buildWithParameters?token=${token}"
PKGS="" #e.g. connman-osmc
SUITES="" #e.g. armv6l aarch64 armv7 amd64
USER="" # Username
PASS="" # API token
NODE="" # Machine to build on

for pkg in $PKGS
do
        for target in $SUITES
        do
                NEWURL="${URL}&Platform=${target}&Target=${pkg}&Node=${NODE}&GIT_URL=https://github.com/osmc/osmc&GIT_BRANCH=master&PushTo=stretch-staging"
                echo "Generating job for $pkg on $target to be built by $NODE"
                wget -q --auth-no-challenge --http-user=$USER --http-password=$PASS "$NEWURL" -O /dev/null
        done
done
