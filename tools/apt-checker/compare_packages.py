#!/usr/bin/python3

# Compare and display Staging and Release OSMC package versions - (c) 2015 DBMandrake.
# Accepts optional command line parameter '--changes' to only display packages whose Staging and Release versions differ.

import urllib.request
import sys
import os

print_changes = False
if len(sys.argv) > 1 and sys.argv[1] == '--changes':
	print_changes = True

repo = os.getenv('OSMCREPO', "bullseye")

results = {}
archlist = [ 'binary-amd64', 'binary-armhf', 'binary-arm64' ]

for arch in archlist:

	staging = urllib.request.urlopen('http://apt.osmc.tv/dists/' + repo + '-devel/main/' + arch + '/Packages').read().decode()
	release = urllib.request.urlopen('http://apt.osmc.tv/dists/' + repo + '/main/' + arch + '/Packages').read().decode()
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
				package_depends_list = [x.strip() for x in package_depends.split(',')]

			if line.startswith('Architecture:'):
				package_architecture = str(line.split('Architecture:',1)[1].lstrip())

		if package_version and package_architecture:
			package_version = package_architecture + ' ' + package_version

		if package_name and package_version:
			try:
				tmp_list = results[package_name]
				tmp_list[0] = package_version

			except KeyError:
				tmp_list = [package_version, 'MISSING']

			results.update({package_name:tmp_list})

			if package_name.endswith('-kernel-osmc') and package_depends:
				kernel_prefix = package_name.split('-kernel-osmc')[0]
				kernel_name = '(Active ' + kernel_prefix + ' Kernel)'
				package_depends = [ s for s in package_depends_list if "-image-" in s ][0]
				kernel_version = package_depends.split(kernel_prefix + '-image-')[1]

				try:
					tmp_list = results[kernel_name]
					tmp_list[0] = kernel_version

				except KeyError:
					tmp_list = [kernel_version, 'MISSING' ]

				results.update({kernel_name:tmp_list})

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
				package_depends_list = [x.strip() for x in package_depends.split(',')]

			if line.startswith('Architecture:'):
				package_architecture = str(line.split('Architecture:',1)[1].lstrip())

		if package_version and package_architecture:
			package_version = package_architecture + ' ' + package_version

		if package_name and package_version:
			try:
				tmp_list = results[package_name]
				tmp_list[1] = package_version

			except KeyError:
				tmp_list = ['MISSING', package_version]

			results.update({package_name:tmp_list})

			if package_name.endswith('-kernel-osmc') and package_depends:
				kernel_prefix = package_name.split('-kernel-osmc')[0]
				kernel_name = '(Active ' + kernel_prefix + ' Kernel)'
				package_depends = [ s for s in package_depends_list if "-image-" in s ][0]
				kernel_version = package_depends.split(kernel_prefix + '-image-')[1]

				try:
					tmp_list = results[kernel_name]
					tmp_list[1] = kernel_version

				except KeyError:
					tmp_list = [ 'MISSING', kernel_version ]

				results.update({kernel_name:tmp_list})


package_length = len(max(results, key=len))
staging_length = max([len(x) for x, y in results.values()])
release_length = max([len(y) for x, y in results.values()])

print('Package name' + (package_length - 7) * ' ' + 'Staging' + (staging_length - 2) * ' ' + 'Release' + (release_length - 1) * ' ' + 'Changes')
print((package_length + staging_length + release_length + 23) * '-' + '\n')

for pkg in sorted(results):
	staging_version = results[pkg][0]
	release_version = results[pkg][1]

	if staging_version == release_version and print_changes is True:
		continue

	print(pkg + (package_length + 5 - len(pkg)) * ' ' + staging_version + (staging_length + 5 - len(staging_version)) * ' ' + release_version + (release_length + 5 - len(release_version)) * ' ', end="")

	if staging_version != release_version:
		print('<------')
	else:
		print()
