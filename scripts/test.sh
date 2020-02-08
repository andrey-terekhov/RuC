#!/bin/bash

init()
{
	full_out=$1

	output_time=0.1
	wait_for=1

	pass=0
	fail=0
	timeout=0
}

build()
{
	cd `dirname $0`/..
	mkdir -p build && cd build && cmake ..
	if ! cmake --build . --config Release ; then
		exit 1
	fi

	ruc_compiler=./ruc
	test_dir=../tests
}

test()
{
	for code in ${test_dir}/*.c ${test_dir}/*/*.c ${test_dir}/*/*/*.c
	do
		out=`timeout $wait_for ${ruc_compiler} $code >/dev/null 2>/dev/null`

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
				fi
				let fail++
				;;
		esac
	done


	if ! [[ -z $full_out ]] ; then
		echo
	fi

	echo -e "\x1B[1;39m pass = $pass, fail = $fail, timeout = $timeout"
}

main()
{
	init $@

	build
	test

	if ! [ $pass -eq 0 ] ; then
		exit 0
	fi

	if ! [ $fail -ge 0 ] ; then
		if ! [ $timeout -ge 0 ] ; then
			exit 0
		fi
	fi

	exit 1
}

main $@
