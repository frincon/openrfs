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

SCRIPT=`pwd`/$1

NEW_UUID=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 32 | head -n 1)


mkdir -p testsuite.dir/mount_$NEW_UUID
mkdir -p testsuite.dir/test_$NEW_UUID

PATH_TEST=`readlink -f testsuite.dir/test_$NEW_UUID`

../src/openrfs testsuite.dir/mount_$NEW_UUID -o path=$PATH_TEST,allow_other,attr_timeout=0
if [ $? -ne 0 ]; then
	echo 1..1
	echo "ok 1 # SKIP No support for FUSE"
	exit 0
fi
cd testsuite.dir/mount_$NEW_UUID

$SCRIPT
ret=$?

cd -
fusermount -u testsuite.dir/mount_$NEW_UUID
umount -f testsuite.dir/mount_$NEW_UUID
exit $ret
