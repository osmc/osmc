#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
#  kernel_prune.py
#
#  Copyright 2017 Sam Nazarko <email@samnazarko.co.uk>

import argparse
import os
import urllib
import sys
import re

def main(args):
    use_web = False
    repositories = ['buster', 'buster-devel']
    arches = [ 'i386', 'amd64', 'armhf', 'arm64']
    kernel_regex = re.compile('(vero364|pc|vero2|vero|rbp1|rbp2|atv)-image-(\d).\d{1,3}.\d{1,3}-\d{1,3}-osmc')
    parser = argparse.ArgumentParser(description="Kernel pruner 0.1")
    parser.add_argument("--dry-run", default=False, action="store_true" , help="Prints kernels that would be deleted only")
    args = parser.parse_args()

    if not os.path.isdir("/var/repos/apt/debian"):
        print("Couldn't find APT repository")
        args.dry_run = True
        use_web = True

    if args.dry_run == True:
        print("Running in dry mode, no kernels will be removed")

    for arch in arches:
        for repository in repositories:
            if args.dry_run == True:
                file_pkgs = urllib.urlopen('http://apt.osmc.tv/dists/' + repository + '/main/binary-' + arch + '/Packages').read()
            else:
                with open('/var/repos/apt/debian/dists/' + repository + '/main/binary-' + arch + '/Packages', 'r') as package_file:
                    file_pkgs = package_file.read()
            packages = file_pkgs.split('\n\n')
            package_name = None
            package_version = None
            package_depends = None
            package_architecture = None
            active_kernels = []
            all_kernels = []
            for package in packages:
                for line in package.splitlines():
        			if line.startswith('Package:'):
        				package_name = str(line.split('Package:',1)[1].lstrip())

        			if line.startswith('Version:'):
        				package_version = str(line.split('Version:',1)[1].lstrip())

        			if line.startswith('Depends:'):
        				package_depends = str(line.split('Depends:',1)[1].lstrip())
        				package_depends_list = [x.strip() for x in package_depends.split(',')]

        			if line.startswith('Architecture:'):
        				package_architecture = str(line.split('Architecture:',1)[1].lstrip())
                # Find the active kernel
                if package_name.endswith('-kernel-osmc') and package_depends:
                    active_kernel = package_name.split('-kernel-osmc')[0]
                    package_depends = [ s for s in package_depends_list if "-image-" in s ][0]
                    print(package_depends + " is an active kernel")
                    active_kernels.append(package_depends)
                # Find all other kernels
                if kernel_regex.match(package_name):
                    all_kernels.append(package_name)
            # Work out what to remove
            for kernel in set.difference(set(all_kernels), set(active_kernels)):
                if args.dry_run == True:
                    print ("We should remove " + kernel + " from " + arch + " in " + repository)
                    print kernel.replace("image", "headers")
                else:
                    print ("Removing " + kernel + " from " + arch + " in " + repository)
                    os.chdir("/var/repos/apt/debian")
                    os.system("/usr/bin/reprepro remove " + repository + " " + kernel)
                    os.system("/usr/bin/reprepro remove " + repository + " " + kernel.replace("image", "headers"))
                    os.system("/usr/bin/reprepro remove " + repository + " " + kernel.replace("image", "source"))
    return 0

if __name__ == '__main__':
    import sys
    sys.exit(main(sys.argv))
