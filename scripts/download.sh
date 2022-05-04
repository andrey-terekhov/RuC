#!/bin/bash

function to_download_url()
{
	url=`curl -s $1`
	url=${url#*<a href=\"}
	url=${url%%\">*}
	echo ${url/tag/download}
}

function wait_release()
{
	until curl -o /dev/null -sfI $1
	do
		sleep 1
	done
}

function download()
{
	url=$1
	tag=${url##*/}
	shift
	
	name=$1
	shift

	while ! [[ -z $1 ]]
	do
		file=$url/$name-$tag-$1.zip
		wait_release $file
		wget $file -O $name-$1.zip
		shift
	done
}

function archive()
{
	while ! [[ -z $1 ]]
	do
		unzip compiler-$1.zip
		rm compiler-$1.zip

		unzip interpreter-$1.zip
		rm interpreter-$1.zip

		mv ruc temp
		mkdir ruc

		mv temp ruc/RuC
		mv ruc-vm ruc/RuC-VM

		zip -r ruc-$1.zip ruc
		rm -rf ruc

		shift
	done
}


URL_COMPILER=`to_download_url https://github.com/andrey-terekhov/RuC/releases/latest`
URL_INTERPRETER=`to_download_url https://github.com/andrey-terekhov/RuC-VM/releases/latest`

download $URL_COMPILER "compiler" linux mac win32 win64
download $URL_INTERPRETER "interpreter" linux mac win32 win64

archive linux mac win32 win64
