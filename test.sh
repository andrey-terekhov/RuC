#!/bin/bash
set -ueo pipefail
test_dir=tests
full_out=${1:-}

output_time=0.1
wait_for=1

pass=0
fail=0
timeout=0

cd build/bin
RUC=./ruc

function internal_timeout() { perl -e 'alarm shift; exec @ARGV' "$@"; }
[[ -e "$RUC" ]]
for code in $(find "${test_dir}" -name "*.c")
do
    RC=0
    internal_timeout $wait_for ${RUC} "$code" &>out.txt || RC=$?
    case $RC in
        0)
            if [[ -n $full_out ]] ; then
                sleep $output_time
                echo -e "\x1B[1;32m build passing \x1B[1;39m: $code"
            fi
	    (( ++pass ))
            ;;
        124)
            if [[ -n $full_out ]] ; then
                sleep $output_time
                echo -e "\x1B[1;34m build timeout \x1B[1;39m: $code"
            fi
	    (( ++timeout ))
            ;;
        *)
            if [[ -n $full_out ]] ; then
                sleep $output_time
                echo -e "\x1B[1;31m build failing \x1B[1;39m: $code"
                if [ "${DEBUG_TEST_SCRIPT:-0}" == "1" ] ; then
                    echo "Output:"
                    cat out.txt
                fi
            fi
	    (( ++fail ))
            ;;
esac
done

rm -f out.txt

if [[ -n $full_out ]] ; then
    echo ""
fi

echo -e "\x1B[1;39m pass = $pass, fail = $fail, timeout = $timeout"

if (( pass != 0 )) ; then
    exit 0
fi

if  (( fail < 0 )) ; then
    if (( timeout < 0 )); then
        exit 0
    fi
fi

exit 1
