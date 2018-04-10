#!/bin/ash

# Let OSIRIS see what we are doing
set -x

# Are we Pi1 or Pi2
grep -q ARMv7 /proc/cpuinfo
if [ $? -eq 0 ]
then
    dev="rbp2"
else
   dev="rbp1"
fi
# Temporary mounting directory
mkdir -p /tmp/mount
# To UUID or not to UUID
vfat_part=$part1
ext4_part=$part2
if [ -n $id1 ]; then vfat_part=$id1; fi
if [ -n $id2 ]; then ext4_part=$id2; fi
# Fix the cmdline.txt
mount $part1 /tmp/mount
echo "root=$ext4_part osmcdev=$dev rootfstype=ext4 rootwait quiet" > /tmp/mount/cmdline.txt
umount /tmp/mount
# Wait
sync
# Fix the fstab
mount $part2 /tmp/mount
echo "$vfat_part  /boot    vfat     defaults,noatime,noauto,x-systemd.automount    0   0
">/tmp/mount/etc/fstab
umount /tmp/mount
# Wait
sync

