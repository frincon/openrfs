#!/bin/bash

if [ ! -z $1 ]; then
	TOP_SRCDIR=`readlink -f $1`
else
	TOP_SRCDIR=.
fi

echo 1..1

if [ ! -c /dev/fuse ] || [ ! -w /dev/fuse ]; then
	echo not ok 1 # SKIP Fuse is not available
fi

NEW_UUID=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 32 | head -n 1)

CURRENT_DIR=`dirname $(readlink -f $0)`

mkdir -p $TOP_SRCDIR/testsuite.dir/mount_$NEW_UUID
mkdir -p $TOP_SRCDIR/testsuite.dir/test_$NEW_UUID

PATH_TEST=`readlink -f $TOP_SRCDIR/testsuite.dir/test_$NEW_UUID`

$TOP_SRCDIR/src/openrfs $TOP_SRCDIR/testsuite.dir/mount_$NEW_UUID -o path=$PATH_TEST,attr_timeout=0
if [ $? -ne 0 ]; then
	echo not ok 1 Openrfs cannot be mounted
else
	echo ok 1
fi
sleep 1
fusermount -u $TOP_SRCDIR/testsuite.dir/mount_$NEW_UUID
