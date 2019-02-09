#!/bin/bash
# Install ZMQ
clear

read -r -p "This script will install many packages that may cause you to reimage your SD card if you wish to uninstall. Do you wish to continue? [y/N] " cont

if [[ $cont =~ ^([yY][eE][sS]|[yY])$ ]]; then
   echo "================================================================="
   echo "================ Installing Necessary Packages =================="
   echo "================================================================="
   # Start with necessary packages
   apt-get install libtool pkg-config build-essential autoconf automake git

   # Download ZMQ package
   echo "================================================================="
   echo "====================== Installing ZeroMQ ========================"
   echo "================================================================="
   wget http://download.zeromq.org/zeromq-4.1.4.tar.gz
   tar -zxvf zeromq-4.1.4.tar.gz
   cd zeromq-4.1.4/
   ./configure --without-libsodium
   make
   make install
   ldconfig

   # Get ZMQ CPP files
   cd /home/pi
   git clone https://github.com/zeromq/cppzmq.git
   cp cppzmq/zmq.hpp /usr/local/include/

   # Get and compile brypt-node
   echo "================================================================="
   echo "============= Downloading and Compiling Node Code ==============="
   echo "================================================================="
   cd /home/pi
   git clone https://github.com/Stygain/brypt-node.git
   cd /home/pi/brypt-node
   git fetch origin rthowerton
   git checkout -t origin/rthowerton
   cd /home/pi/brypt-node/dev
   g++ node.cpp "test.cpp" -o device -O2 -Wall -lzmq

   # Run DNSMasq and HostAPD
   read -r -p "The script will now install packages necessary for instantiating a Wi-Fi access point/hotspot. This will cause your device to restart. Do you wish to continue? [y/N]" cont
   if [[ $cont =~ ^([yY][eE][sS]|[yY])$ ]]; then
      apt-get install dnsmasq hostapd
      systemctl stop dnsmasq
      systemctl stop hostapd
   fi

   read -r -p "Do you wish to set up a wireless access point after reboot? [y/N]" cont
   if [[ $cont =~ ^([yY][eE][sS]|[yY])$ ]]; then
      touch /root/start_ap
   fi
fi

# Prepare the system for reboot and starting the access point
cp /etc/rc.local /home/pi/brypt-node/dev/config/AP/rc.local.base
cp /home/pi/brypt-node/dev/config/AP/rc.local.reboot /etc/rc.local
chmod +x /etc/rc.local

echo "The system will now restart in 10 seconds. Press Ctrl-C to stop reboot."
sleep 10
reboot
