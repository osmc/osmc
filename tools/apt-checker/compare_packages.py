#!/usr/bin/python

# Compare and display Devel and Release OSMC package versions - (c) 2015 DBMandrake.
# Accepts command line option 'changes' to only display packages whose devel and release versions differ.

import urllib
import sys

print_changes = False
if len(sys.argv) > 1 and sys.argv[1] == 'changes':
	print_changes = True

results = {}
archlist = [ 'binary-i386', 'binary-amd64', 'binary-armhf' ]

for arch in archlist:

	devel = urllib.urlopen('http://apt.osmc.tv/dists/jessie-devel/main/' + arch + '/Packages').read()
	release = urllib.urlopen('http://apt.osmc.tv/dists/jessie/main/' + arch + '/Packages').read()

	devel_packages = devel.split('\n\n')
	release_packages = release.split('\n\n')

	for package in devel_packages:
		package_name = None
		package_version = None

		for line in package.splitlines():
			if line.startswith('Package:'):
				package_name = line.split('Package:',1)[1].lstrip()

			if line.startswith('Version:'):
				package_version = line.split('Version:',1)[1].lstrip()

		if package_name and package_version:
			try:
				list = results[package_name]
				list[0] = package_version

			except KeyError:
				list = [package_version, 'MISSING']

			results.update({package_name:list})

	for package in release_packages:
		package_name = None
		package_version = None

		for line in package.splitlines():
			if line.startswith('Package:'):
				package_name = line.split('Package:',1)[1].lstrip()

			if line.startswith('Version:'):
				package_version = line.split('Version:',1)[1].lstrip()

		if package_name and package_version:
			try:
				list = results[package_name]
				list[1] = package_version

			except KeyError:
				list = ['MISSING', package_version]

			results.update({package_name:list})

package_length = len(max(results, key=len))
devel_length = max([len(x) for x, y in results.values()])
release_length = max([len(y) for x, y in results.values()])

print 'Package name' + (package_length - 7) * ' ' + 'Devel' + (devel_length) * ' ' + 'Release' + (release_length - 1) * ' ' + 'Changes'
print (package_length + devel_length + release_length + 23) * '-' + '\n'

for pkg in sorted(results):
	devel_version = results[pkg][0]
	release_version = results[pkg][1]

	if devel_version == release_version and print_changes is True:
		continue

	print pkg + (package_length + 5 - len(pkg)) * ' ' + devel_version + (devel_length + 5 - len(devel_version)) * ' ' + release_version + (release_length + 5 - len(release_version)) * ' ',

	if devel_version != release_version:
		print '<------'
	else:
		print
