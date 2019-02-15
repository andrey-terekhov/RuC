#!/bin/bash

full_out=$1

output_time=0.1
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
    out=`timeout $wait_for ./ruc-compiler $code >/dev/null 2>/dev/null`
    
    case $? in
        0)
            if ! [[ -z $full_out ]] ; then
                sleep $output_time
                echo -e "\e[1;32m build passing \e[1;39m: $code"
            fi
            let pass++
            ;;
        124)
            if ! [[ -z $full_out ]] ; then
                sleep $output_time
                echo -e "\e[1;34m build timeout \e[1;39m: $code"
            fi
            let timeout++
            ;;
        *)
            if ! [[ -z $full_out ]] ; then
                sleep $output_time
                echo -e "\e[1;31m build failing \e[1;39m: $code"
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

echo -e "\e[1;39m pass = $pass, fail = $fail, timeout = $timeout"


if ! [ $pass -eq 0 ] ; then
    exit 0
fi

if ! [ $fail -ge 0 ] ; then
    if ! [ $timeout -ge 0 ] ; then
        exit 0
    fi
fi

exit 1
