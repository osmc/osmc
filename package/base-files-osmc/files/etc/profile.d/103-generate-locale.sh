user=$(whoami)
if [ "$user" != "osmc" ] && [ -z "$LC_ALL" ] && [ -z "$LANG" ]; then export LANG=C && export LC_ALL=C && return 0; fi

if [ ! -z $LANG ]
then
    if grep -q "# $LANG" /etc/locale.gen
    then
        sudo sed -e "s/# $LANG/$LANG/" -i /etc/locale.gen
        sudo /usr/sbin/locale-gen ${LANG}
    fi
else
    export LANG=C
    export LC_ALL=C
fi
