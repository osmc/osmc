if test -z "$(find /var/lib/apt/lists/ -maxdepth 1 -type f \( ! -name 'lock' \))"
then
    echo -e "Updating APT cache. Please be patient."
    # osmc-apt-update.service runs apt-get update as root; it takes no input,
    # so needs no sudo. Wait for it for at most a minute, or until Ctrl-C:
    # stopping systemctl leaves the update running in the background. The trap
    # makes Ctrl-C end only the wait, not the rest of the login scripts.
    trap : INT
    timeout --foreground 60 systemctl --quiet --no-ask-password start osmc-apt-update.service >/dev/null 2>&1
    apt_update_status=$?
    trap - INT
    case $apt_update_status in
    0)
        ;;
    124|130)
        echo "The APT cache is still updating in the background (systemctl status osmc-apt-update). If you are planning to install a package, please wait for it to finish first"
        ;;
    *)
        echo "apt-get update was unsuccessful. If you are planning to install a package, please run apt-get update first and verify it was successful"
        ;;
    esac
    unset apt_update_status
fi
