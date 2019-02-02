#!/bin/bash
# check number of arguments
if [ "$#" -ne 1 ]
	then
		echo "USAGE: ./setup [administrator_password]"
		exit 1
fi
# Get serial number
SERIAL=`echo $1 | sudo -S cat /proc/cpuinfo | grep Serial | cut -d ' ' -f 2`
# Get IP address
IP=`hostname -I | cut -d ' ' -f 1`
# Get random ID(?) number
RAND=`$RANDOM | md5sum | cut -b 1-5`
