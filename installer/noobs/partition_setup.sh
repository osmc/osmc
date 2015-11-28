#!/bin/ash

# Let OSIRIS see what we are doing
set -x

is_pi_zero()
{
    revision=$(cat /proc/cmdline | awk -v RS=" " -F= '/boardrev/ { print $2 }')
    if [ "$(( $revision >> 23 & 1 ))" -eq 1 ]
    then
        if [ "$(( $revision >> 4 & 0x7F ))" -eq 9 ]
        then
            return 1 # This is a Pi Zero
    fi
    fi
    return 0
}

# Are we Pi1 or Pi2
grep -q ARMv7 /proc/cpuinfo
if [ $? -eq 0 ]
then
    dev="rbp2"
else
   dev="rbp1"
fi
# We really don't want automated fscking
tune2fs -c 0 $part2
# Temporary mounting directory
mkdir -p /tmp/mount
# Fix the cmdline.txt
mount $part1 /tmp/mount
echo "root=$part2 osmcdev=$dev rootfstype=ext4 rootwait quiet" > /tmp/mount/cmdline.txt
# Check for Pi Zero
is_pi_zero
pi_zero=$?
if [ "$pi_zero" -eq 0 ]
then
    echo "arm_freq=850" >> /tmp/mount/config.txt
    echo "core_freq=375" >> /tmp/mount/config.txt
fi
umount /tmp/mount
# Wait
sync
# Fix the fstab
mount $part2 /tmp/mount
echo "$part1  /boot    vfat     defaults,noatime    0   0
$part2  /    ext4      defaults,noatime    0   0
">/tmp/mount/etc/fstab
umount /tmp/mount
# Wait
sync

