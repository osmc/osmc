user=$(whoami)
if [ "$user" != "osmc" ]; then exit 0; fi
if systemctl is-active ssh -q && sudo grep -q "osmc:\$1\$P.ZH6EFu\$L08/1ZYI6FdHu3aw0us.u0:17569:0:99999:7:::" /etc/shadow && [ ! -f /home/osmc/.nosshwarn ]
then
    echo "Warning: you are using SSH with the default OSMC credentials. This is a security risk."
    echo "To change your password, type passwd"
    echo "To disable this warning, type touch /home/osmc/.nosshwarn"
fi
