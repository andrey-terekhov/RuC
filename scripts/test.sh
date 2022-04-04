#!/bin/bash

init()
{
	exit_code=64
	vm_exec=export.txt

	vm_release=master
	output_time=0.0
	wait_for=2

	dir_install=./install
	dir_test=../tests
	dir_lexing=../tests/lexing
	dir_preprocessor=../tests/preprocessor
	dir_semantics=../tests/semantics
	dir_syntax=../tests/syntax
	dir_multiple_errors=../tests/multiple_errors
	dir_unsorted=../tests/unsorted
	dir_exec=../tests/codegen/executable

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
				echo -e "\tFolder \"$dir_lexing\" should contain tests with lexer errors."
				echo -e "\tFolder \"$dir_preprocessor\" should contain tests with preprocessor errors."
				echo -e "\tFolder \"$dir_semantics\" should contain tests with semantics errors."
				echo -e "\tFolder \"$dir_syntax\" should contain tests with syntax errors."
				echo -e "\tFolder \"$dir_multiple_errors\" should contain tests with multiple errors."
				echo -e "\tFolder \"$dir_unsorted\" should contain tests with unsorted errors."
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

build_vm()
{
	if ! [[ -z $remove ]] ; then
		rm -rf ruc-vm
	fi

	if ! [[ -d ruc-vm ]] ; then
		git clone -b $vm_release --recursive https://github.com/andrey-terekhov/RuC-VM ruc-vm
		cd ruc-vm
	else
		cd ruc-vm
		git checkout $vm_release
	fi

	if [[ $OSTYPE != "msys" ]] ; then
		CMAKE_BUILD_TYPE=-DCMAKE_BUILD_TYPE=Release
	fi

	mkdir -p build && cd build && cmake .. $CMAKE_BUILD_TYPE -DTESTING_EXIT_CODE=$exit_code
	if ! cmake --build . --config Release ; then
		exit 1
	fi

	cd ../..
	if [[ $OSTYPE == "msys" ]] ; then
		interpreter=./ruc-vm/build/Release/ruc-vm
	else
		interpreter=./ruc-vm/build/ruc-vm
	fi

	interpreter_debug=$interpreter
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

	if [[ -z $ignore ]] ; then
		build_vm
	fi
}

run()
{
	exec=`realpath $1`
	exec_debug=`realpath $2`
	shift
	shift

	$runner $exec $@ &>$log
	ret=$?

	build_type=""
	if [[ $exec != $exec_debug ]] ; then
		if [[ $ret == 0 ]] ; then
			mv $vm_exec $buf

			$runner $exec_debug $@ &>$log
			ret=$?

			if [[ $ret == 0 ]] ; then
				mv $buf $vm_exec
			else
				build_type=" (Debug)"
			fi
		elif [[ $ret == $exit_code ]] ; then
			mv $log $buf

			$runner $exec_debug $@ &>$log
			ret=$?

			if [[ $ret == $exit_code ]] ; then
				mv $buf $log
			else
				build_type=" (Debug)"
			fi
		fi
	fi

	return $ret
}

message_success()
{
	if [[ -z $debug ]] ; then
		if [[ -z $silence ]] ; then
			echo -e "\x1B[1;32m $action success \x1B[1;39m: $path$build_type"
			sleep $output_time
		fi
	fi
}

message_warning()
{
	if [[ -z $silence ]] ; then
		echo -e "\x1B[1;33m $action warning \x1B[1;39m: $path$build_type"
		sleep $output_time
	fi
}

message_timeout()
{
	if [[ -z $silence ]] ; then
		echo -e "\x1B[1;34m $action timeout \x1B[1;39m: $path$build_type"
		sleep $output_time
	fi
}

message_failure()
{
	if [[ -z $silence ]] ; then
		echo -e "\x1B[1;31m $action failure \x1B[1;39m: $path$build_type"
		sleep $output_time
	fi
}

execution()
{
	if [[ $path == $dir_exec/* ]] ; then
		action="execution"
		run $interpreter $interpreter_debug $vm_exec

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
	# в unsorted проверяется только наличие ошибки. multiple_errors и preprocessor здесь временно
	if [[ $path == */$subdir_warning/* || $path == $dir_unsorted/* || $dir_multiple_errors/* || 
		$path == $dir_preprocessor/* ]] ; then
		message_success
		let success++
	else
		if [[ $OSTYPE == "msys" ]] ; then
			flag=`cat $log | iconv -c -f CP866 -t UTF-8 | grep -c "ошибка: "`
		else
			flag=`grep -c "ошибка: " $log`
		fi

		if [[ $flag > 1 ]] ; then
			# проверка на наличие одной ошибки
			if [[ $path == $dir_lexing/* || $path == $dir_semantics/* || $path == $dir_syntax/* ]] ; then
				message_failure
				let failure++
			fi

			# # проверка на наличие нескольких ошибки. временно убрано
			# if [[ $path == $dir_multiple_errors/* ]] ; then
			# 	message_success
			# 	let success++
			# fi

			if ! [[ -z $debug ]] ; then
				cat $log
			fi
		else
			# проверка на наличие одной ошибки
			if [[ $path == $dir_lexing/* || $path == $dir_semantics/* || $path == $dir_syntax/* ]] ; then
				message_success
				let success++
			fi

			# # проверка на наличие нескольких ошибки. временно убрано
			# if [[ $path == $dir_multiple_errors/* ]] ; then
			# 	message_failure
			# 	let failure++
			# fi
		fi
	fi
}

compiling()
{
	if [[ -z $ignore || $path != $dir_lexing/* || $path != $dir_preprocessor/* || $path != $dir_semantics/* 
		|| $path != $dir_syntax/* || $path != $dir_multiple_errors/* || $path != $dir_unsorted/* ]] ; then
		action="compiling"
		run $compiler $compiler_debug $sources -o $vm_exec -VM

		case $? in
			0)
				if [[ $path == $dir_lexing/* || $path == $dir_preprocessor/* || $path == $dir_semantics/* 
					|| $path == $dir_syntax/* || $path == $dir_multiple_errors/* || $path == $dir_unsorted/* ]] ; then
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
				if [[ $path == $dir_lexing/* || $path == $dir_preprocessor/* || $path == $dir_semantics/* 
					|| $path == $dir_syntax/* || $path == $dir_multiple_errors/* || $path == $dir_unsorted/* ]] ; then
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
