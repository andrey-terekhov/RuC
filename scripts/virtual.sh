#!/bin/bash

#	*****************
#	*	compiling	*	—	required compiling stage
#	*****************
#	*				* \
#	*****************	—	optional custom stages
#	*				* /
#	*****************
#	*	execution	*	—	required exec stage, if no need set to not supported
#	*****************
#
#	—————————————————————————————————
#	| 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |	—	binary representation of exit code
#	—————————————————————————————————
#	  \   /   \   /   \   /   \   /
#		|		|		|		|
#		 e		 s		 f		 c
#		  x		  n		  s		  o
#		   e	   d	   t	   m
#			c						p
#
#	0b00 == 0	—	success code
#	0b01 == 1	—	failure code
#	0b10 == 2	—	timeout code
#	0b11 == 3	—	not supported


init()
{
	exit_code=1
	vm_exec=export.txt

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
				echo -e "\t-v, --virtual\tSet RuC virtual machine release."
				echo -e "\t-w, --wait\tSet waiting time for timeout result (default = 2)."
				exit 0
				;;
			-v|--virtual)
				vm_release=$2
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

main()
{
	init $@

	exit 0
}

main $@
