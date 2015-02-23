#!/bin/ash

# Let OSIRIS see what we are doing
set -x
# We really don't want automated fscking
tune2fs -c 0 $part2
# Temporary mounting directory
mkdir -p /tmp/mount
# Fix the cmdline.txt
mount $part1 /tmp/mount
echo "root=$part2 osmcdev=rbp rootfstype=ext4 rootwait quiet" > /tmp/mount/cmdline.txt
# Set the right config.txt
cores=$(cat /proc/cpuinfo | grep cores | wc -l)
if [ $cores == 4 ]
then
mv /tmp/mount/config2.txt /tmp/mount/config.txt && rm /tmp/mount/config1.txt
else
mv /tmp/mount/config1.txt /tmp/mount/config.txt && rm /tmp/mount/config2.txt
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

