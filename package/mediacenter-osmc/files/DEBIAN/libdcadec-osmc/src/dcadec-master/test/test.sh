#!/bin/sh

set -e

if [ ! -f samples/README ] ; then
    echo "ERROR: Run 'git submodule update --init test/samples' first."
    exit 1
fi

[ -z $SHA1SUM ] && command -v sha1sum > /dev/null && SHA1SUM=sha1sum
[ -z $SHA1SUM ] && command -v shasum > /dev/null && SHA1SUM=shasum

if [ -z $SHA1SUM ] ; then
    echo "ERROR: Neither sha1sum nor shasum found"
    exit 1
fi

rm -rf decoded
mkdir -p decoded/dmix_0 decoded/dmix_2 decoded/dmix_6 decoded/mono
for i in samples/*.dtshd ; do
    ../dcadec -b -q $i decoded/dmix_0/$(basename $i .dtshd).wav
    ../dcadec -b -q -2 $i decoded/dmix_2/$(basename $i .dtshd).wav
    ../dcadec -b -q -6 $i decoded/dmix_6/$(basename $i .dtshd).wav
    ../dcadec -m -q $i decoded/mono/$(basename $i .dtshd)_%s.wav
done

LOSSY="\
decoded/mono/core_*.wav \
decoded/mono/x96_*.wav \
decoded/mono/xbr_*.wav \
decoded/mono/xch_*.wav \
decoded/mono/xxch_*.wav"

if [ "$1" = "--update" ] ; then
    $SHA1SUM -b decoded/dmix_0/*.wav decoded/dmix_2/*.wav decoded/dmix_6/*.wav > checksum.txt
    $SHA1SUM -b samples/reference/xll_*.wav | sed 's|samples/reference|decoded/mono|' >> checksum.txt
    for i in $LOSSY ; do
        ./stddev $i samples/reference/$(basename $i) ?
    done > stddev.txt
else
    $SHA1SUM -c checksum.txt
    for i in $LOSSY ; do
        ./stddev $i samples/reference/$(basename $i) $(grep -F $i stddev.txt | cut -d ' ' -f 2)
    done
fi
