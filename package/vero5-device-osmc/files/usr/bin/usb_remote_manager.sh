#!/bin/bash

. /etc/osmc/prefs.d/verov_remote_preferences
if [ $OSMC_VEROV_REMOTE_AUTO_DISABLE -ne 1 ]
then
	exit
fi

DEVPATH1="/devices/platform/fde00000.dwc3/xhci-hcd.0.auto/usb1/1-1/1-1.2"
DEVPATH2="/devices/platform/fde00000.dwc3/xhci-hcd.0.auto/usb1/1-1/1-1.2/1-1.2:1.1"
DEVPATH3="/devices/platform/fde00000.dwc3/xhci-hcd.0.auto/usb1/1-1/1-1.2/1-1.2:1.0"

DEVFLAG="/var/run/verov_remote_2017:1690"
REMOVEPATH="/sys/devices/platform/fde00000.dwc3/xhci-hcd.0.auto/usb1/1-1/1-1.2/remove"

if [ "$DEVPATH" != "$DEVPATH1" ] && [ "$DEVPATH" != "$DEVPATH2" ] && [ "$DEVPATH" != "$DEVPATH3" ]
then
	touch "$DEVFLAG"
fi

if [ -f "$DEVFLAG" ] && [ -e "$REMOVEPATH" ]
then
	echo 1 > "$REMOVEPATH"
	/usr/bin/logger -t $(basename $0) "External OSMC Remote USB receiver detected - internal receiver disabled."
fi
