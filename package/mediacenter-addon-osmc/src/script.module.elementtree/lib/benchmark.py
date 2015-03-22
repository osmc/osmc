# $Id: benchmark.py 2923 2006-11-19 08:05:45Z fredrik $
# simple elementtree benchmark program

from elementtree import ElementTree, XMLTreeBuilder
try:
    import cElementTree
except ImportError:
    try:
        from xml.etree import cElementTree
    except ImportError:
        cElementTree = None
try:
    from elementtree import SimpleXMLTreeBuilder # xmllib
except ImportError:
    SimpleXMLTreeBuilder = None
try:
    from elementtree import SgmlopXMLTreeBuilder # sgmlop
except ImportError:
    SgmlopXMLTreeBuilder = None
try:
    from xml.dom import minidom # pyexpat+minidom
except ImportError:
    minidom = None

import sys, time

try:
    file = sys.argv[1]
except IndexError:
    file = "hamlet.xml"

def benchmark(file, builder_module):
    source = open(file, "rb")
    t0 = time.time()
    parser = builder_module.TreeBuilder()
    while 1:
        data = source.read(32768)
        if not data:
            break
        parser.feed(data)
    tree = parser.close()
    t1 = time.time()
    print "%s: %d nodes read in %.3f seconds" % (
        builder_module.__name__, len(tree.getiterator()), t1-t0
        )
    raw_input("press return to continue...")
    del tree

def benchmark_parse(file, driver):
    t0 = time.time()
    tree = driver.parse(file)
    t1 = time.time()
    print driver.__name__ + ".parse done in %.3f seconds" % (t1-t0)
    raw_input("press return to continue...")
    del tree

def benchmark_minidom(file):
    t0 = time.time()
    dom = minidom.parse(file)
    t1 = time.time()
    print "minidom tree read in %.3f seconds" % (t1-t0)
    raw_input("press return to continue...")
    del dom

benchmark_parse(file, ElementTree)
if cElementTree:
    benchmark_parse(file, cElementTree)
if sys.platform != "cli":
    benchmark(file, XMLTreeBuilder)
    benchmark(file, SimpleXMLTreeBuilder) # use xmllib
    try:
        benchmark(file, SgmlopXMLTreeBuilder) # use sgmlop
    except RuntimeError, v:
        print "=== SgmlopXMLTreeBuilder not available (%s)" % v

if minidom:
    benchmark_minidom(file)
else:
    print "=== minidom not available"
