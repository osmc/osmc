#!/usr/bin/python

# Script to show how the fstab_compare module can be used
# 2016-05-10 Brian Millham bmillham@gmail.com

from fstab_compare import fstab_compare

fstab = '/etc/fstab'
fstab_backup = "fstab.backup"

fstab_diffs = fstab_compare(fstab, fstab_backup)

print "Lines in backup that are not in new fstab, skipping lines that would override a fs_file"

for l in fstab_diffs.unique_fs_files_formatted():
    print l