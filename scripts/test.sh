#!/bin/bash

init()
{
	output_time=0.1
	wait_for=1

	while ! [[ -z $1 ]]
	do
		case "$1" in
			-h|--help)
				echo -e "Usage: ./${0##*/} [KEY]..."
				echo -e "Keys:"
				echo -e "\t-h, --help\tTo output help info"
				echo -e "\t-s, --silence\tFor silence testing"
				echo -e "\t-d, --debug\tSwitch on debug tracing"
				echo -e "\t-o, --output\tSet output printing time (default = 0.1)"
				echo -e "\t-w, --wait\tSet waiting time for timeout result (default = 1)"
				exit 0
				;;
			-s|--silence)
				silence=$1
				;;
			-d|--debug)
				debug=$1
				echo -e "In future..."
				exit 0
				;;
			-o|--output)
				output_time=$2
				shift
				;;
			-w|--wait)
				wait_for=$2
				shift
				;;
		esac
		shift
	done

	success=0
	failure=0
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
	error_dir=../tests/errors
}

internal_timeout()
{
	which timeout
	if [[ $? == 0 ]] ; then
		timeout $@
	else
		perl -e 'alarm shift; exec @ARGV' $@;
	fi
}

message_success()
{
	if [[ -z $silence ]] ; then
		echo -e "\x1B[1;32m $action success \x1B[1;39m: $code"
		sleep $output_time
	fi
	let success++
}

message_timeout()
{
	if [[ -z $silence ]] ; then
		echo -e "\x1B[1;34m $action timeout \x1B[1;39m: $code"
		sleep $output_time
	fi
	let timeout++
}

message_failure()
{
	if [[ -z $silence ]] ; then
		echo -e "\x1B[1;31m $action failure \x1B[1;39m: $code"
		sleep $output_time
	fi
	let failure++
}

test()
{
	# Do not use names with spaces!
	for code in `find ${test_dir} -name *.c`
	do
		action="compiling"
		out=`internal_timeout $wait_for $ruc_compiler $code >/dev/null 2>/dev/null`

		case "$?" in
			0)
				if [[ $code == $error_dir/* ]] ; then
					message_failure
				else
					message_success
				fi
				;;
			124|142)
				message_timeout
				;;
			*)
				if [[ $code == $error_dir/* ]] ; then
					message_success
				else
					message_failure
				fi
				;;
		esac
	done


	if [[ -z $silence ]] ; then
		echo
	fi

	echo -e "\x1B[1;39m success = $success, failure = $failure, timeout = $timeout"
}

main()
{
	init $@

	build
	test

	if [[ $failure != 0 || $timeout != 0 ]] ; then
		exit 1
	fi

	exit 0
}

main $@
