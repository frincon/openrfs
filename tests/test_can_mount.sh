#!/bin/bash

TOP_SRCDIR=`readlink -f $1`
if [ "$TOP_SRCDIR" == "" ]; then
	TOP_SRCDIR=.
fi

NEW_UUID=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 32 | head -n 1)

CURRENT_DIR=`dirname $(readlink -f $0)`

mkdir -p $TOP_SRCDIR/testsuite.dir/mount_$NEW_UUID
mkdir -p $TOP_SRCDIR/testsuite.dir/test_$NEW_UUID

PATH_TEST=`readlink -f $TOP_SRCDIR/testsuite.dir/test_$NEW_UUID`

$TOP_SRCDIR/src/openrfs $TOP_SRCDIR/testsuite.dir/mount_$NEW_UUID -o path=$PATH_TEST,attr_timeout=0
if [ $? -ne 0 ]; then
	echo 1..1
	echo not ok 1 Openrfs cannot be mounted
else
	echo 1..1
	echo ok 1
fi
