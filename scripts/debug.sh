#!/bin/bash

install()
{
	which gdb
	if [[ $? != 0 ]] ; then
		if [[ $OSTYPE == "linux-gnu" ]] ; then
			sudo apt-get -y install gdb
		else
			brew install gdb
			echo "set startup-with-shell off" >> ~/.gdbinit
		fi
	fi
}

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
	install
	build
	gdbinit

	sudo gdb -x $init --args $ruc $@ &>$log
	while [[ $? != 139 ]]
	do
		sudo gdb -x $init --args $ruc $@ &>$log
	done

	cat $log

	rm $init
	rm $log

	exit 139
}

main $@