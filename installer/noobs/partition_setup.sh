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
# We really don't want automated fscking
tune2fs -c 0 $part2
# Temporary mounting directory
mkdir -p /tmp/mount
# Fix the cmdline.txt
mount $part1 /tmp/mount
echo "root=$part2 osmcdev=$dev rootfstype=ext4 rootwait quiet" > /tmp/mount/cmdline.txt
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

