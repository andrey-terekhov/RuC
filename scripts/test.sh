#!/bin/bash

init()
{
	output_time=0.0
	wait_for=2
	vm_release=master

	test_dir=../tests
	error_dir=../tests/errors
	exec_dir=../tests/executable

	error_subdir=errors
	warning_subdir=warnings
	include_subdir=include

	while ! [[ -z $1 ]]
	do
		case "$1" in
			-h|--help)
				echo -e "Usage: ./${0##*/} [KEY] ..."
				echo -e "Description:"
				echo -e "\tThis script tests all files from \"$test_dir\" directory."
				echo -e "\tFolder \"$error_dir\" should contain tests with expected error."
				echo -e "\tExecutable tests should be in \"$exec_dir\" directory."
				echo -e "\tTo ignore invalid tests output, use \"*/$warning_subdir/*\" subdirectory."
				echo -e "\tFor tests with expected runtime error, use \"*/$error_subdir/*\" subdirectory."
				echo -e "\tFor multi-file tests, use \"*/$include_subdir/*\" subdirectory."
				echo -e "Keys:"
				echo -e "\t-h, --help\tTo output help info."
				echo -e "\t-s, --silence\tFor silence testing."
				echo -e "\t-d, --debug\tSwitch on debug tracing."
				echo -e "\t-v, --virtual\tSet RuC virtual machine release."
				echo -e "\t-o, --output\tSet output printing time (default = 0.0)."
				echo -e "\t-w, --wait\tSet waiting time for timeout result (default = 2)."
				exit 0
				;;
			-s|--silence)
				silence=$1
				;;
			-d|--debug)
				debug=$1
				;;
			-v|--virtual)
				vm_release=$2
				shift
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
	warning=0
	failure=0
	timeout=0

	log=tmp
}

build_vm()
{
	rm -rf ruc-vm
	git clone -b $vm_release --recursive https://github.com/andrey-terekhov/RuC-VM ruc-vm

	cd ruc-vm
	mkdir -p build && cd build && cmake ..
	if ! cmake --build . --config Release ; then
		exit 1
	fi

	cd ../..
	if [[ $OSTYPE == "msys" ]] ; then
		ruc_interpreter=./ruc-vm/build/Release/ruc-vm
	else
		ruc_interpreter=./ruc-vm/build/ruc-vm
	fi
}

build()
{
	cd `dirname $0`/..
	mkdir -p build && cd build && cmake ..
	if ! cmake --build . --config Release ; then
		exit 1
	fi

	if [[ $OSTYPE == "msys" ]] ; then
		ruc_compiler=./Release/ruc.exe
	else
		ruc_compiler=./ruc
	fi

	build_vm
}

internal_timeout()
{
	if [[ $OSTYPE == "darwin" ]] ; then
		gtimeout $@
	else
		timeout $@
	fi
}

message_success()
{
	if [[ -z $debug ]] ; then
		if [[ -z $silence ]] ; then
			echo -e "\x1B[1;32m $action success \x1B[1;39m: $path"
			sleep $output_time
		fi
	fi
}

message_warning()
{
	if [[ -z $silence ]] ; then
		echo -e "\x1B[1;33m $action warning \x1B[1;39m: $path"
		sleep $output_time
	fi
}

message_timeout()
{
	if [[ -z $silence ]] ; then
		echo -e "\x1B[1;34m $action timeout \x1B[1;39m: $path"
		sleep $output_time
	fi
}

message_failure()
{
	if [[ -z $silence ]] ; then
		echo -e "\x1B[1;31m $action failure \x1B[1;39m: $path"
		sleep $output_time
	fi
}

execution()
{
	if [[ $path == $exec_dir/* ]] ; then
		action="execution"
		internal_timeout $wait_for $ruc_interpreter export.txt &>$log

		case "$?" in
			0)
				if [[ $path == */$error_subdir/* ]] ; then
					message_failure
					let failure++
				else
					message_success
					let success++
				fi
				;;
			124|142)
				message_timeout
				let timeout++
				;;
			139|134)
				# Segmentation fault
				# Double free or corruption (!prev)

				message_failure
				let failure++

				if ! [[ -z $debug ]] ; then
					cat $log
				fi
				;;
			*)
				if [[ $path == */$error_subdir/* ]] ; then
					message_success
					let success++
				else
					message_failure
					let failure++

					if ! [[ -z $debug ]] ; then
						cat $log
					fi
				fi
				;;
		esac
	fi
}

check_warnings()
{
	if [[ $path == */$warning_subdir/* ]] ; then
		message_success
		let success++
	else
		if [[ `grep -c "ошибка: " $log` > 1 ]] ; then
			message_warning
			let warning++

			if ! [[ -z $debug ]] ; then
				cat $log
			fi
		else
			message_success
			let success++
		fi
	fi
}

compiling()
{
	action="compiling"
	internal_timeout $wait_for $ruc_compiler $sources &>$log

	case "$?" in
		0)
			if [[ $path == $error_dir/* ]] ; then
				message_failure
				let failure++
			else
				message_success
				execution
			fi
			;;
		124|142)
			message_timeout
			let timeout++
			;;
		139|134)
			# Segmentation fault
			# Double free or corruption (!prev)

			message_failure
			let failure++

			if ! [[ -z $debug ]] ; then
				cat $log
			fi
			;;
		*)
			if [[ $path == $error_dir/* ]] ; then
				check_warnings
			else
				message_failure
				let failure++

				if ! [[ -z $debug ]] ; then
					cat $log
				fi
			fi
			;;
	esac
}

test()
{
	# Do not use names with spaces!
	for path in `find $test_dir -name *.c`
	do
		sources=$path

		if [[ $path != */$include_subdir/* ]] ; then
			compiling
		fi
	done

	for include in `find $test_dir -name $include_subdir -type d`
	do
		for path in `ls -d $include/*`
		do
			sources=`find $path -name *.c`

			for subdir in `find $path -name *.h`
			do
				temp=`dirname $subdir`
				sources="$sources -I$temp"
			done

			compiling
		done
	done

	if [[ -z $silence ]] ; then
		echo
	fi

	echo -e "\x1B[1;39m success = $success, warning = $warning, failure = $failure, timeout = $timeout"
	rm $log
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