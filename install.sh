#!/bin/sh

if [ $# -lt 2 ] ; then
    echo "Usage: $0 target_dir executable_file"
    exit 1
fi

prefix=$1
exe=$2

config=dadder.config
badip=badip.txt
blacklist=blacklist.txt

if [ ! -d "$prefix" ] ; then
    echo "$prefix  does not exist, try to make it... "
    if  ! mkdir $prefix ; then 
        echo "Unable to mkdir of $prefix."
        exit 2
    fi
fi

echo "Now copy executable files and configuration files..."
cp $exe $prefix

if [ ! -e "$prefix/$config" ] ; then
	cp ./config/$config $prefix
fi
if [ ! -e "$prefix/blacklist.txt" ] ; then
	cp ./config/$blacklist $prefix
fi

if [ ! -e "$prefix/$badip" ] ; then
	cp ./config/$badip $prefix
fi

    
echo "Done."
echo "Now, eddit configuration file: $prefix/$config , and then start the process by:"
echo "$prefix/$exe -c $prefix/$config"
    
