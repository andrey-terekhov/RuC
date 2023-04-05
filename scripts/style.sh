#!/bin/sh

# Needed clang clang-format clang-format-10 (clang-tidy)

#clang_tidy="clang-tidy"
clang_format="clang-format-10"

which $clang_format >/dev/null 2>/dev/null
if [ "$?" != "0" ] ; then
	clang_format="clang-format"
fi

# Enter to root dir
cd `dirname $0`/..


# Set checking files
headers="libs/*/*.h"
sources="src/*.c libs/*/*.c"

# TO-DO auto include directory `-Ipath_to_dir`
#directories="-Ilibs/utils -Ilibs/preprocessor -Ilibs/compiler"

#$clang_tidy -fix-errors $sources -- $directories
$clang_format --Werror --dry-run --verbose -style=file:.clang-format $sources $headers
exit $?
