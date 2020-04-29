#!/bin/sh
binary="clang-format-9"
which "${binary}" > /dev/null
if [ "$?" != "0" ] ; then
	binary="clang-format"
fi

find libs -iname *.c -o -iname *.h | xargs "${binary}" -i -style=file
find src -iname *.c -o -iname *.h | xargs "${binary}" -i -style=file
