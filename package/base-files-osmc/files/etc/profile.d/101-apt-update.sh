if [ ! -f /var/cache/apt/pkgcache.bin ]
then
    echo -e "Updating APT cache. Please be patient."
    sudo apt-get update >/dev/null 2>&1
    if [ $? != 0 ]
    then
       echo "apt-get update was unsuccessful. If you are planning to install a package, please run apt-get update first and verify it was successful"
    fi
fi
