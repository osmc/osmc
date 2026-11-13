user=$(whoami)
if [ "$user" != "osmc" ] && [ -z "$LC_ALL" ] && [ -z "$LANG" ]; then export LANG=C LC_ALL=C && return 0; fi

if [ -n "$LANG" ]
then
    # An SSH client sends its own LANG (sshd_config: AcceptEnv LANG LC_*). If
    # /etc/locale.gen offers that locale but has not enabled it, have
    # osmc-locale-gen@.service enable and generate it. The unit validates the
    # name: nothing here hands $LANG to a privileged command. Only osmc and
    # root may start the unit, so other users are not prompted at login.
    case "$user" in
    osmc|root)
        if LOCALE="$LANG" awk 'NF == 3 && $1 == "#" && $2 == ENVIRON["LOCALE"] { f = 1 } END { exit !f }' /etc/locale.gen 2>/dev/null
        then
            echo "Generating locale $LANG. Please be patient."
            systemctl --quiet --no-ask-password start "osmc-locale-gen@$(systemd-escape -- "$LANG").service"
        fi
        ;;
    esac
else
    export LANG=C LC_ALL=C
fi
