#!/bin/sh
find RuC -iname *.c -o -iname *.h | xargs clang-format -i -style=file
