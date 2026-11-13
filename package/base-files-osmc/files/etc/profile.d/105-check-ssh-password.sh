user=$(whoami)
if [ "$user" != "osmc" ]; then return 0; fi

# osmc-password-check.service keeps this flag current: it runs at boot, and
# through osmc-password-check.path whenever /etc/shadow changes.
if [ -e /run/osmc-password-check/default ] && systemctl is-active ssh -q && [ ! -f /home/osmc/.nosshwarn ]
then
    echo "Warning: You are using SSH with the default OSMC credentials."
    echo "This is a security risk!"
    echo "To change your password, type passwd"
    echo "To disable this warning, type touch /home/osmc/.nosshwarn"
fi
