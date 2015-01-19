#!/bin/bash

if [ "$UID" != "0" ]; then
	exit 77
fi

$1

