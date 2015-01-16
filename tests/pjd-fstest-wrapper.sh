#!/bin/bash

if [ "$UID" != "0" ]; then
	echo 1..1
	echo "ok 1 # SKIP Must be run as root"
	exit
fi
TOP_SRCDIR=`readlink -f $1`
SCRIPT=$2


CURRENT_DIR=`dirname $(readlink -f $0)`

mkdir -p $TOP_SRCDIR/testsuite.dir/mount
mkdir -p $TOP_SRCDIR/testsuite.dir/test

PATH_TEST=`readlink -f $TOP_SRCDIR/testsuite.dir/test`

$TOP_SRCDIR/src/openrfs $TOP_SRCDIR/testsuite.dir/mount -o path=$PATH_TEST,allow_other,attr_timeout=0
cd $TOP_SRCDIR/testsuite.dir/mount

$TOP_SRCDIR/$SCRIPT

cd ..
fusermount -u $TOP_SRCDIR/testsuite.dir/mount
