#!/bin/bash

clear

if [ -f /root/start_ap ]; then
   ### Copy and configure static IP address
   ## cp /home/pi/brypt-node/dev/config/AP/dhcpcd.conf.on /etc/dhcpcd.conf
   ## service dhcpcd restart

   # Replace old rc.local, so reboot doesn't occur again
   sudo cp /home/pi/brypt-node/dev/config/AP/rc.local.base /etc/rc.local
   
   # Copy the dnsmasq configuration
   sudo mv /etc/dnsmasq.conf /etc/dnsmasq.conf.orig
   sudo cp /home/pi/brypt-node/dev/config/AP/dnsmasq.conf.on /etc/dnsmasq.conf
   
   # Set the SSID name
   RAND=`echo $RANDOM | md5sum | cut -b 1-5`
   sudo echo "SSID=brypt-net-$RAND" >> /home/pi/brypt-node/dev/config/AP/hostapd.conf.on
   sudo cp /home/pi/brypt-node/dev/config/AP/hostapd.conf.on /etc/hostapd/hostapd.conf
   
   # Tell hostapd where to find the config file
   sudo cp /home/pi/brypt-node/dev/config/AP/default-hostapd.conf.on /etc/default/hostapd.conf

   # Ensure hostapd stays live
   sudo cp /home/pi/brypt-node/dev/config/AP/hostapd.service /etc/systemd/system/hostapd.service
   
   # Restart the services
   sudo systemctl start hostapd
   sudo systemctl start dnsmasq
   
   # Add routing and masquerade
   sudo cp /home/pi/brypt-node/dev/config/AP/sysctl.conf.on /etc/sysctl.conf
   sudo iptables -t nat -A  POSTROUTING -o eth0 -j MASQUERADE
   sudo sh -c "iptables-save > /etc/iptables.ipv4.nat"
   sudo cp /home/pi/brypt-node/dev/config/AP/rc.local.on /etc/rc.local
   
   # Clean up remaining file
   sudo rm /root/start_ap
   sudo reboot
fi
