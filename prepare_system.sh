#!/bin/bash 

CUR_DIR=$(pwd)

display_usage() { 
echo -e "\nUsage: [install_path]\n" 
} 

check_if_succeeded() {
	if [ $? -eq 0 ]; then
	    echo OK
	else
	    echo FAIL
	    exit 1
	fi
}


if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 
   exit 1
fi

prepare_system() {
	echo "Disable THP..."
	echo never > /sys/kernel/mm/transparent_hugepage/enabled && echo never > /sys/kernel/mm/transparent_hugepage/defrag

	echo "Drop OS caches..."
	sync; echo 3 > /proc/sys/vm/drop_caches

	echo -n "Chekicng if dump PT module loaded..."
	if [ -z "$(cat /proc/modules | grep page_table_dump)" ];
	then
		echo "not found"
		echo -n "Install dump PT module..."
		SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"/mitosis-page-table-dump/
		cd $SOURCE_DIR/kernel-module
		make > install.log 2>&1 && insmod page-table-dump.ko
		check_if_succeeded
	fi && echo "found. OK"
}

prepare_system

