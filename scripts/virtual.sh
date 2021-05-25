#!/bin/bash

#	*****************
#	*	compiling	*	—	required compiling stage
#	*****************
#	*				*
#	*		...		*	—	optional custom stages
#	*				*
#	*****************
#	*	execution	*	—	required exec stage, if no need set to unsupported
#	*****************
#
#	—————————————————————————————————
#	| 0 | 0 | 0 | 0 | 0 | 0 | 1 | 1 |	—	binary representation of exit code
#	—————————————————————————————————
#	  \					  /	  \	  /
#		—————————————————		—
#		   stage number		exit code
#
#	0b00 == 0	—	success code
#	0b01 == 1	—	failure code
#	0b10 == 2	—	timeout code
#	0b11 == 3	—	unsupported code


ROOT=`dirname $0`/..
CURRENT_DIR=$PWD

EXIT_CODE=1
OUTPUT=out.ruc
FLAGS="-VM -o $OUTPUT"

RELEASE=master


init()
{
	while ! [[ -z $1 ]]
	do
		case $1 in
			-h|--help)
				echo -e "Usage: ./${0##*/} [KEY / COMPILER] ..."
				echo -e "Description:"
				echo -e "\tThis script build and execute RuC code on virtual machine."
				echo -e "Keys:"
				echo -e "\t-h, --help\tTo output help info."
				echo -e "\t-t, --tag\tGet script tag."
				echo -e "\t-s, --stage\tGet stage name by number."
				echo -e "\t-w, --wait\tSet up timeout (default = 2)."
				echo -e "\t-e, --exit\tSet up error exit code (default = 1)."
				exit 0
				;;
			-t|--tag)
				echo -e "VM"
				exit 0
				;;
			-s|--stage)
				case $2 in
					0)
						echo -e "compiling"
						;;
					1)
						echo -e "execution"
						;;
					*)
						exit 1
						;;
				esac
				exit 0
				;;
		esac
	done
}

build()
{
	if [[ $OSTYPE == "msys" ]] ; then
		interpreter=$ROOT/build/ruc-vm/build/Release/ruc-vm
	else
		interpreter=$ROOT/build/ruc-vm/build/ruc-vm
	fi

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

	interpreter_debug=$interpreter
}






main()
{
	init $@

	exit 0
}

main $@
