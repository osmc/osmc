#!/usr/bin/python

# Compare and display Staging and Release OSMC package versions - (c) 2015 DBMandrake.
# Accepts optional command line parameter '--changes' to only display packages whose Staging and Release versions differ.

import urllib
import sys

print_changes = False
if len(sys.argv) > 1 and sys.argv[1] == '--changes':
	print_changes = True

results = {}
archlist = [ 'binary-i386', 'binary-amd64', 'binary-armhf' ]

for arch in archlist:

	staging = urllib.urlopen('http://staging.apt.osmc.tv/dists/jessie-devel/main/' + arch + '/Packages').read()
	release = urllib.urlopen('http://staging.apt.osmc.tv/dists/jessie/main/' + arch + '/Packages').read()

	staging_packages = staging.split('\n\n')
	release_packages = release.split('\n\n')

	for package in staging_packages:
		package_name = None
		package_version = None
		package_depends = None
		package_architecture = None

		for line in package.splitlines():
			if line.startswith('Package:'):
				package_name = str(line.split('Package:',1)[1].lstrip())

			if line.startswith('Version:'):
				package_version = str(line.split('Version:',1)[1].lstrip())

			if line.startswith('Depends:'):
				package_depends = str(line.split('Depends:',1)[1].lstrip())

			if line.startswith('Architecture:'):
				package_architecture = str(line.split('Architecture:',1)[1].lstrip())

		if package_version and package_architecture:
			package_version = package_architecture + ' ' + package_version

		if package_name and package_version:
			try:
				list = results[package_name]
				list[0] = package_version

			except KeyError:
				list = [package_version, 'MISSING']

			results.update({package_name:list})

			if package_name.endswith('-kernel-osmc') and package_depends:
				kernel_prefix = package_name.split('-kernel-osmc')[0]
				kernel_name = '(Active ' + kernel_prefix + ' Kernel)'
				kernel_version = package_depends.split(kernel_prefix + '-image-')[1]

				try:
					list = results[kernel_name]
					list[0] = kernel_version

				except KeyError:
					list = [kernel_version, 'MISSING' ]

				results.update({kernel_name:list})


	for package in release_packages:
		package_name = None
		package_version = None
		package_depends = None
		package_architecture = None

		for line in package.splitlines():
			if line.startswith('Package:'):
				package_name = str(line.split('Package:',1)[1].lstrip())

			if line.startswith('Version:'):
				package_version = str(line.split('Version:',1)[1].lstrip())

			if line.startswith('Depends:'):
				package_depends = str(line.split('Depends:',1)[1].lstrip())

			if line.startswith('Architecture:'):
				package_architecture = str(line.split('Architecture:',1)[1].lstrip())

		if package_version and package_architecture:
			package_version = package_architecture + ' ' + package_version

		if package_name and package_version:
			try:
				list = results[package_name]
				list[1] = package_version

			except KeyError:
				list = ['MISSING', package_version]

			results.update({package_name:list})

			if package_name.endswith('-kernel-osmc') and package_depends:
				kernel_prefix = package_name.split('-kernel-osmc')[0]
				kernel_name = '(Active ' + kernel_prefix + ' Kernel)'
				kernel_version = package_depends.split(kernel_prefix + '-image-')[1]

				try:
					list = results[kernel_name]
					list[1] = kernel_version

				except KeyError:
					list = [ 'MISSING', kernel_version ]

				results.update({kernel_name:list})


package_length = len(max(results, key=len))
staging_length = max([len(x) for x, y in results.values()])
release_length = max([len(y) for x, y in results.values()])

print 'Package name' + (package_length - 7) * ' ' + 'Staging' + (staging_length - 2) * ' ' + 'Release' + (release_length - 1) * ' ' + 'Changes'
print (package_length + staging_length + release_length + 23) * '-' + '\n'

for pkg in sorted(results):
	staging_version = results[pkg][0]
	release_version = results[pkg][1]

	if staging_version == release_version and print_changes is True:
		continue

	print pkg + (package_length + 5 - len(pkg)) * ' ' + staging_version + (staging_length + 5 - len(staging_version)) * ' ' + release_version + (release_length + 5 - len(release_version)) * ' ',

	if staging_version != release_version:
		print '<------'
	else:
		print
