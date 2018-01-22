#!/bin/bash

full_out=$1
wait_for=1

pass=0
fail=0
timeout=0


if ! make >/dev/null 2>/dev/null ; then
    make
    exit 1
fi


for code in ./tests/*.c ./tests/*/*.c ./tests/*/*/*.c
do
    out=`timeout $wait_for ./ruc $code >/dev/null 2>/dev/null`
    
    case $? in
        0)
            if ! [[ -z $full_out ]] ; then
                echo " build passing : $code"
            fi
            let pass++
            ;;
        124)
            if ! [[ -z $full_out ]] ; then
                echo " build timeout : $code"
            fi
            let timeout++
            ;;
        *)
            if ! [[ -z $full_out ]] ; then
                echo " build failing : $code"
            fi
            let fail++
            ;;
    esac
done


rm tree.txt codes.txt export.txt >/dev/null 2>/dev/null
rm -r obj >/dev/null 2>/dev/null
rm ruc >/dev/null 2>/dev/null


if ! [[ -z $full_out ]] ; then
    echo
fi

echo " pass = $pass, fail = $fail, timeout = $timeout"

exit 0
