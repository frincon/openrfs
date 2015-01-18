#!/bin/bash

if [ "$UID" != "0" ]; then
	echo 1..1
	echo "ok 1 # SKIP Must be run as root"
	exit
fi
if [ ! -c /dev/fuse ] || [ ! -w /dev/fuse ]; then
	echo 1..1
	echo "ok 1 # SKIP Fuse is not available"
	exit 0
fi



TOP_SRCDIR=`readlink -f $1`
SCRIPT=$2

NEW_UUID=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 32 | head -n 1)


CURRENT_DIR=`dirname $(readlink -f $0)`

mkdir -p $TOP_SRCDIR/testsuite.dir/mount_$NEW_UUID
mkdir -p $TOP_SRCDIR/testsuite.dir/test_$NEW_UUID

PATH_TEST=`readlink -f $TOP_SRCDIR/testsuite.dir/test_$NEW_UUID`

$TOP_SRCDIR/src/openrfs $TOP_SRCDIR/testsuite.dir/mount_$NEW_UUID -o path=$PATH_TEST,allow_other,attr_timeout=0
if [ $? -ne 0 ]; then
	echo 1..1
	echo "ok 1 # SKIP No support for FUSE"
	exit 0
fi
cd $TOP_SRCDIR/testsuite.dir/mount_$NEW_UUID

$TOP_SRCDIR/$SCRIPT
ret=$?

cd ..
fusermount -u $TOP_SRCDIR/testsuite.dir/mount_$NEW_UUID
umount -f $TOP_SRCDIR/testsuite.dir/mount_$NEW_UUID
exit $ret
