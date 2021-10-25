#!/bin/bash

init()
{
	exit_code=1
	llvm_exec=export.ll
	clang_output=output

	vm_release=master
	output_time=0.0
	wait_for=2

	dir_install=./install
	dir_test=../tests
	dir_error=../tests/errors
	dir_exec=../tests/executable

	subdir_error=errors
	subdir_warning=warnings
	subdir_include=include

	while ! [[ -z $1 ]]
	do
		case $1 in
			-h|--help)
				echo -e "Usage: ./${0##*/} [KEY] ..."
				echo -e "Description:"
				echo -e "\tThis script tests all files from \"$dir_test\" directory."
				echo -e "\tFolder \"$dir_error\" should contain tests with expected error."
				echo -e "\tExecutable tests should be in \"$dir_exec\" directory."
				echo -e "\tTo ignore invalid tests output, use \"*/$subdir_warning/*\" subdirectory."
				echo -e "\tFor tests with expected runtime error, use \"*/$subdir_error/*\" subdirectory."
				echo -e "\tFor multi-file tests, use \"*/$subdir_include/*\" subdirectory."
				echo -e "\tFailed tests for debug build only will be marked with \"(Debug)\"."
				echo -e "Keys:"
				echo -e "\t-h, --help\tTo output help info."
				echo -e "\t-s, --silence\tFor silence testing."
				echo -e "\t-f, --fast\tFast testing, Release builds only."
				echo -e "\t-i, --ignore\tIgnore errors & executing stages."
				echo -e "\t-r, --remove\tRemove build folder before testing."
				echo -e "\t-d, --debug\tSwitch on debug tracing."
				echo -e "\t-v, --virtual\tSet RuC virtual machine release."
				echo -e "\t-o, --output\tSet output printing time (default = 0.0)."
				echo -e "\t-w, --wait\tSet waiting time for timeout result (default = 2)."
				exit 0
				;;
			-s|--silence)
				silence=$1
				;;
			-f|--fast)
				fast=$1
				;;
			-i|--ignore)
				ignore=$1
				;;
			-r|--remove)
				remove=$1
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

	if [[ $OSTYPE == "darwin"* ]] ; then
		runner="gtimeout $wait_for"
	else
		runner="timeout $wait_for"
	fi

	log=tmp
	buf=buf
}

build_folder()
{
	if ! [[ -z $remove ]] ; then
		rm -rf build
	fi
	mkdir -p build && cd build


	if [[ -z $fast ]] ; then
		if [[ $OSTYPE != "msys" ]] ; then
			CMAKE_BUILD_TYPE=-DCMAKE_BUILD_TYPE=Debug
		fi

		cmake .. $CMAKE_BUILD_TYPE -DTESTING_EXIT_CODE=$exit_code
		if ! cmake --build . --config Debug ; then
			exit 1
		fi

		if [[ $OSTYPE != "msys" ]] ; then
			cmake --install . --prefix $dir_install --config Debug
			rm -rf Debug
			mv $dir_install/$1 Debug
		fi
	fi

	if [[ $OSTYPE != "msys" ]] ; then
		CMAKE_BUILD_TYPE=-DCMAKE_BUILD_TYPE=Release
	fi

	cmake .. $CMAKE_BUILD_TYPE -DTESTING_EXIT_CODE=$exit_code
	if ! cmake --build . --config Release ; then
		exit 1
	fi

	if [[ $OSTYPE != "msys" ]] ; then
		cmake --install . --prefix $dir_install --config Release
		rm -rf Release
		mv $dir_install/$1 Release
		rm -rf $dir_install
	fi
}

build()
{
	cd `dirname $0`/..
	build_folder ruc

	compiler=./Release/ruc
	if [[ -z $fast ]] ; then
		compiler_debug=./Debug/ruc
	else
		compiler_debug=$compiler
	fi
}

run()
{
	exec=$1
	exec_debug=$2
	shift
	shift

	$runner $exec $@ &>$log
	ret=$?

	build_type=""
	if [[ $exec != $exec_debug ]] ; then
		if [[ $ret == 0 ]] ; then
			mv $llvm_exec $buf

			$runner $exec_debug $@ &>$log
			ret=$?

			if [[ $ret == 0 ]] ; then
				mv $buf $llvm_exec
			else
				build_type="(Debug)"
			fi
		elif [[ $ret == $exit_code ]] ; then
			mv $log $buf

			$runner $exec_debug $@ &>$log
			ret=$?

			if [[ $ret == $exit_code ]] ; then
				mv $buf $log
			else
				build_type="(Debug)"
			fi
		fi
	fi

	return $ret
}

message_success()
{
	if [[ -z $debug ]] ; then
		if [[ -z $silence ]] ; then
			echo -e "\x1B[1;32m $action success \x1B[1;39m: $path $build_type"
			sleep $output_time
		fi
	fi
}

message_warning()
{
	if [[ -z $silence ]] ; then
		echo -e "\x1B[1;33m $action warning \x1B[1;39m: $path $build_type"
		sleep $output_time
	fi
}

message_timeout()
{
	if [[ -z $silence ]] ; then
		echo -e "\x1B[1;34m $action timeout \x1B[1;39m: $path $build_type"
		sleep $output_time
	fi
}

message_failure()
{
	if [[ -z $silence ]] ; then
		echo -e "\x1B[1;31m $action failure \x1B[1;39m: $path $build_type"
		sleep $output_time
	fi
}

execution()
{
	if [[ $path == $dir_exec/* ]] ; then
		action="clang exe"
		clang $llvm_exec -o $clang_output

		case $? in
			0)
				if [[ $path == */$subdir_error/* ]] ; then
					message_failure
					let failure++
				else
					if [[ $build_type == "(Debug)" ]] ; then
						build_type=""

						message_failure
						let failure++
					else
						message_success
						let success++
					fi
				fi
				;;
			124|142)
				message_timeout
				let timeout++
				;;
			$exit_code)
				if [[ $path == */$subdir_error/* ]] ; then
					if [[ $build_type == "(Debug)" ]] ; then
						build_type=""

						message_failure
						let failure++
					else
						message_success
						let success++
					fi
				else
					message_failure
					let failure++

					if ! [[ -z $debug ]] ; then
						cat $log
					fi
				fi
				;;
			*)
				# Segmentation fault
				# Double free or corruption (!prev)
				# Etcetera

				message_failure
				let failure++

				if ! [[ -z $debug ]] ; then
					cat $log
				fi
				;;
		esac
	else
		let success++
	fi
}

check_warnings()
{
	if [[ $path == */$subdir_warning/* ]] ; then
		message_success
		let success++
	else
		if [[ $OSTYPE == "msys" ]] ; then
			flag=`cat $log | iconv -c -f CP866 -t UTF-8 | grep -c "ошибка: "`
		else
			flag=`grep -c "ошибка: " $log`
		fi

		if [[ $flag > 1 ]] ; then
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
	if [[ -z $ignore || $path != $dir_error/* ]] ; then
		action="compiling"
		run $compiler $compiler_debug $sources -LLVM -o $llvm_exec

		case $? in
			0)
				if [[ $path == $dir_error/* ]] ; then
					message_failure
					let failure++
				else
					if [[ $build_type == "(Debug)" ]] ; then
						build_type=""

						message_failure
						let failure++
					else
						message_success

						if [[ -z $ignore ]] ; then
							execution
						else
							let success++
						fi
					fi
				fi
				;;
			124|142)
				message_timeout
				let timeout++
				;;
			$exit_code)
				if [[ $path == $dir_error/* ]] ; then
					if [[ $build_type == "(Debug)" ]] ; then
						build_type=""

						message_failure
						let failure++
					else
						check_warnings
					fi
				else
					message_failure
					let failure++

					if ! [[ -z $debug ]] ; then
						cat $log
					fi
				fi
				;;
			*)
				# Segmentation fault
				# Double free or corruption (!prev)
				# Etcetera

				message_failure
				let failure++

				if ! [[ -z $debug ]] ; then
					cat $log
				fi
				;;
		esac
	fi
}

test()
{
	# Do not use names with spaces!
	for path in `find $dir_test -name *.c`
	do
		sources=$path

		if [[ $path != */$subdir_include/* ]] ; then
			compiling
		fi
	done

	for include in `find $dir_test -name $subdir_include -type d`
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
	rm -f $log
	rm -f $buf
}

main()
{
	init $@

	build
	if [[ -z $debug ]] ; then
		test 2>/dev/null
	else
		test
	fi

	if [[ $failure != 0 || $timeout != 0 ]] ; then
		exit 1
	fi

	exit 0
}

main $@
