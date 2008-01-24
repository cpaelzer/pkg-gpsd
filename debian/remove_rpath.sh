#! /bin/sh

while [ "x$1" != "x" ]
do
	chrpath --list $1 > /dev/null 2>&1 && chrpath --delete $1
	shift
done
