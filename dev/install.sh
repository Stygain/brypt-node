#!/bin/bash

if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 
   exit 1
fi

# Before rebooting for AP configuration
before_reboot(){
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
}

# After rebooting for AP configuration
after_reboot(){
   if [ -f /root/start_ap ]; then
      ### Copy and configure static IP address
      ## cp /home/pi/brypt-node/dev/config/AP/dhcpcd.conf.on /etc/dhcpcd.conf
      ## service dhcpcd restart

      # Replace old rc.local, so reboot doesn't occur again
      cp /home/pi/brypt-node/dev/config/AP/rc.local.base /etc/rc.local

      echo "================================================================="
      echo "======================== Setting Up AP =========================="
      echo "================================================================="
      
      # Copy the dnsmasq configuration
      mv /etc/dnsmasq /etc/dnsmasq.conf.orig
      cp /home/pi/brypt-node/dev/config/AP/dnsmasq.conf.on /etc/dnsmasq.conf
      
      # Set the SSID name
      RAND=`echo $RANDOM | md5sum | cut -b 1-5`
      "SSID=brypt-net-$RAND" >> /home/pi/brypt-node/dev/config/AP/hostapd.conf.on
      cp /home/pi/brypt-node/dev/config/AP/hostapd.conf.on /etc/hostapd/hostapd.conf
      
      # Tell hostapd where to find the config file
      cp /home/pi/brypt-node/dev/config/AP/default-hostapd.conf.on /etc/default/hostapd.conf

      # Ensure hostapd stays live
      cp /home/pi/brypt-node/dev/config/AP/hostapd.service /etc/systemd/system/hostapd.service
      
      # Restart the services
      systemctl start hostapd
      systemctl start dnsmasq
      
      # Add routing and masquerade
      cp /home/pi/brypt-node/dev/config/AP/sysctl.conf.on /etc/sysctl.conf
      iptables -t nat -A  POSTROUTING -o eth0 -j MASQUERADE
      sh -c "iptables-save > /etc/iptables.ipv4.nat"
      cp /home/pi/brypt-node/dev/config/AP/rc.local.on /etc/rc.local
      
      # Clean up remaining file
      rm /root/start_ap
   fi
}

# Persist through reboot and do stuff
if [ -f /root/rebooted ]; then
   after_reboot
   rm /root/rebooted
   reboot
else
   before_reboot
   touch /root/rebooted
   cp /etc/rc.local /home/pi/brypt-node/dev/config/AP/rc.local.base
   cp /home/pi/brypt-node/dev/config/AP/rc.local.reboot /etc/rc.local
   reboot
fi
