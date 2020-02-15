#!/bin/sh
find ../libs -iname *.c -o -iname *.h | xargs clang-format -i -style=file
find ../src -iname *.c -o -iname *.h | xargs clang-format -i -style=file
