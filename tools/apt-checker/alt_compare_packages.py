#!/usr/bin/python

# Compare and display Devel and Release OSMC package versions - (c) 2015 DBMandrake.
# Accepts command line option 'changes' to only display packages whose devel and release versions differ.

import urllib
import threading
import Queue
import sys

THREAD_COUNT = 6
PACKAGE_GAP  = len('Package name')
DEVEL_GAP    = len('Devel')
REL_GAP      = len('Release')

PRINT_CHANGES = True if len(sys.argv) > 1 and sys.argv[1] == 'changes' else False

ARCHLIST = [ 'binary-i386', 'binary-amd64', 'binary-armhf' ]

urls_queue = Queue.Queue()
res_queue  = Queue.Queue()


def tuplize(package, version):

    return (package[len('Package:'):].strip().replace('\n',''), version[len('Version:'):].strip().replace('\n',''))


def download_package_details(urls_queue, res_queue):

    while not urls_queue.empty():

        url = urls_queue.get()

        result = urllib.urlopen(url)

        res_queue.put((url, result))


def process_packages(page):

    package_text = page.read().split('\n')

    result = filter(lambda x: any([x.startswith('Package:'), x.startswith('Version:')]), package_text)

    result = dict([tuplize(p, v) for p,v in zip(result[0::2], result[1::2])])

    return result

for arch in ARCHLIST:

    devel_url = 'http://apt.osmc.tv/dists/jessie-devel/main/%s/Packages' % arch
    jessi_url = 'http://apt.osmc.tv/dists/jessie/main/%s/Packages' % arch

    urls_queue.put(devel_url)
    urls_queue.put(jessi_url)


threads = []

for x in range(THREAD_COUNT):
    t = threading.Thread(target=download_package_details, args=(urls_queue, res_queue))
    t.start()
    threads.append(t)

for t in threads:
    t.join()

data = {}

while not res_queue.empty():

    url, page = res_queue.get()

    res_queue.task_done()

    data[url] = process_packages(page)

new_data = {}

for key, devel in data.iteritems():

    if '-devel' not in key:
        continue

    rel = data[key.replace('-devel','')]

    combined = {}

    for k, v in devel.iteritems():

        PACKAGE_GAP = max([PACKAGE_GAP, len(k)])
        DEVEL_GAP   = max([DEVEL_GAP, len(v)])

        combined[k] = [v, 'None']

    for k, v in rel.iteritems():

        PACKAGE_GAP = max([PACKAGE_GAP, len(k)])
        REL_GAP     = max([REL_GAP, len(v)])

        new_v = combined.get(k, ['None'])
        if len(new_v) == 1:
            new_v.append(v)
        else:
            new_v[1] = v

    new_data[key] = combined

PACKAGE_GAP  += 3
DEVEL_GAP    += 3
REL_GAP      += 1

for k, v in new_data.iteritems():
	print '\n'
	print '(%s)' % k
	print '{0}{1}{2}'.format('Package name'.ljust(PACKAGE_GAP), 'Devel'.ljust(DEVEL_GAP), 'Release'.ljust(REL_GAP))
	print (PACKAGE_GAP + DEVEL_GAP + REL_GAP + 23) * '-' + '\n'
	packages = v.keys()
	packages.sort()
	for pack in packages:
		vs = v[pack]
		if PRINT_CHANGES and vs[0] == vs[1]:
			continue
		print '{0}{1}{2}{3}'.format(pack.ljust(PACKAGE_GAP), vs[0].ljust(DEVEL_GAP), vs[1].ljust(REL_GAP), '<------' if vs[0] != vs[1] else '')