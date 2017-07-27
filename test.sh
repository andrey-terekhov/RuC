#!/bin/bash

pass=0
fail=0
timeout=0


if ! make >/dev/null 2>/dev/null ; then
    make
    exit 1
fi


for i in `find ./tests -name *.c`
do
    out=`timeout 1 ./ruc $i >/dev/null 2>/dev/null`
    
    case $? in
        0)
            let pass++
            ;;
        124)
            let timeout++
            ;;
        *)
            let fail++
            ;;
    esac
done


rm tree.txt codes.txt export.txt >/dev/null 2>/dev/null
rm ruc >/dev/null 2>/dev/null


echo " pass = $pass, fail = $fail, timeout = $timeout"

exit 0
