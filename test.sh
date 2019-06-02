#!/bin/bash
test_dir=$(pwd)/tests
full_out=$1

output_time=0.1
wait_for=1

pass=0
fail=0
timeout=0

mkdir -p build
cd build
cmake ..
if ! make >/dev/null 2>/dev/null ; then
    make
    exit 1
fi

cd bin
RUC=./ruc

function internal_timeout() { perl -e 'alarm shift; exec @ARGV' "$@"; }

for code in ${test_dir}/*.c ${test_dir}/*/*.c ${test_dir}/*/*/*.c
do
    rm -f out.txt
    out=`internal_timeout $wait_for ${RUC} $code >out.txt 2>out.txt`

    case $? in
        0)
            if ! [[ -z $full_out ]] ; then
                sleep $output_time
                echo -e "\x1B[1;32m build passing \x1B[1;39m: $code"
            fi
            let pass++
            ;;
        124)
            if ! [[ -z $full_out ]] ; then
                sleep $output_time
                echo -e "\x1B[1;34m build timeout \x1B[1;39m: $code"
            fi
            let timeout++
            ;;
        *)
            if ! [[ -z $full_out ]] ; then
                sleep $output_time
                echo -e "\x1B[1;31m build failing \x1B[1;39m: $code"
                if [ "${DEBUG_TEST_SCRIPT:-0}" == "1" ] ; then
                    echo "Output:"
                    cat out.txt
                fi
            fi
            let fail++
            ;;
    esac
done

rm -f out.txt

if ! [[ -z $full_out ]] ; then
    echo
fi

echo -e "\x1B[1;39m pass = $pass, fail = $fail, timeout = $timeout"

if ! [ $pass -eq 0 ] ; then
    exit 0
fi

if ! [ $fail -ge 0 ] ; then
    if ! [ $timeout -ge 0 ] ; then
        exit 0
    fi
fi

exit 1
