#!/bin/bash

echo 1..1

if [ ! -c /dev/fuse ] || [ ! -w /dev/fuse ]; then
	echo "ok 1 # SKIP Fuse is not available"
	exit 0
fi

NEW_UUID=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 32 | head -n 1)

CURRENT_DIR=`dirname $(readlink -f $0)`

mkdir -p testsuite.dir/mount_$NEW_UUID
mkdir -p testsuite.dir/test_$NEW_UUID

PATH_TEST=`readlink -f testsuite.dir/test_$NEW_UUID`

../src/openrfs testsuite.dir/mount_$NEW_UUID -o path=$PATH_TEST,attr_timeout=0
if [ $? -ne 0 ]; then
	echo not ok 1 Openrfs cannot be mounted
else
	echo ok 1
fi
sleep 1
fusermount -u testsuite.dir/mount_$NEW_UUID
