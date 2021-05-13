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


init()
{
	case $1 in
		-h|--help)
			echo -e "Usage: ./${0##*/} [KEY / COMPILER] ..."
			echo -e "Description:"
			echo -e "\tThis script build and execute RuC code on virtual machine."
			echo -e "Keys:"
			echo -e "\t-h, --help\tTo output help info."
			echo -e "\t-t, --tag\tGet script tag."
			echo -e "\t-s, --stage\tGet stage name by number."
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
}

main()
{
	init $@

	exit 0
}

main $@
