#!/bin/sh
find RuC -iname *.c -o -iname *.h | xargs clang-format -i -style=file
find RuCVM -iname *.c -o -iname *.h | xargs clang-format -i -style=file
find UI -iname *.c -o -iname *.h | xargs clang-format -i -style=file
find util -iname *.c -o -iname *.h | xargs clang-format -i -style=file
