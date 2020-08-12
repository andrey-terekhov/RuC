#!/bin/bash

build()
{
	cd `dirname $0`/..
	mkdir -p build && cd build && cmake ..
	if ! cmake --build . --config Debug ; then
		exit 1
	fi

	ruc=./ruc
	log=tmp
}

gdbinit()
{
	init=gdbinit

	echo "set \$_exitcode = -1" >$init
	echo "run" >>$init
	echo "if \$_exitcode != -1" >>$init
	echo "	quit \$_exitcode" >>$init
	echo "end" >>$init
	echo "" >>$init
}

main()
{
	build
	gdbinit

	sudo gdb -x $init --args $ruc $1 #&>$log
	#while [[ $? != 139 ]]
	#do
	#	gdb -x $init --args $ruc $1 &>$log
	#	echo "[Inferior 1 exited with code $?]"
	#done

	#cat $log

	rm $init
	#rm $log

	exit 139
}

main $@