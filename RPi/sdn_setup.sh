#!/bin/bash

# Ensure you have the required permissions
if [ "$EUID" -ne 0 ]; then
  echo "Please run as root"
  exit
fi

# Install required packages
apt update
apt install -y iproute2 iptables

# Define variables
INTERFACE="eth0"  # Change to your active network interface
URL="sync-schedule.example.com"  # Replace with the target URL
HIGHEST_PRIORITY_MARK=0
HIGH_PRIORITY_MARK=1
MID_PRIORITY_MARK=2
LOW_PRIORITY_MARK=3

echo "Setting up SDN-based traffic prioritization on $INTERFACE..."

# Clear previous tc settings
tc qdisc del dev "$INTERFACE" root 2>/dev/null

# Set up root qdisc
tc qdisc add dev "$INTERFACE" root handle 1: htb default $LOW_PRIORITY_MARK

# Define root class and child classes
tc class add dev "$INTERFACE" parent 1: classid 1:1 htb rate 100mbit burst 15k
tc class add dev "$INTERFACE" parent 1:1 classid 1:$HIGHEST_PRIORITY_MARK htb rate 80mbit ceil 100mbit burst 20k prio 0
tc class add dev "$INTERFACE" parent 1:1 classid 1:$HIGH_PRIORITY_MARK htb rate 50mbit ceil 80mbit burst 15k prio 1
tc class add dev "$INTERFACE" parent 1:1 classid 1:$MID_PRIORITY_MARK htb rate 20mbit ceil 50mbit burst 10k prio 2
tc class add dev "$INTERFACE" parent 1:1 classid 1:$LOW_PRIORITY_MARK htb rate 10mbit ceil 30mbit burst 5k prio 3

# Attach filters to prioritize traffic
tc filter add dev "$INTERFACE" protocol ip parent 1: prio 0 handle $HIGHEST_PRIORITY_MARK fw classid 1:$HIGHEST_PRIORITY_MARK
tc filter add dev "$INTERFACE" protocol ip parent 1: prio 1 handle $HIGH_PRIORITY_MARK fw classid 1:$HIGH_PRIORITY_MARK
tc filter add dev "$INTERFACE" protocol ip parent 1: prio 2 handle $MID_PRIORITY_MARK fw classid 1:$MID_PRIORITY_MARK
tc filter add dev "$INTERFACE" protocol ip parent 1: prio 3 handle $LOW_PRIORITY_MARK fw classid 1:$LOW_PRIORITY_MARK

# Use iptables to mark packets
# 1. Internal serial communication gets the highest priority
iptables -t mangle -A OUTPUT -o "$INTERFACE" -p tcp -m owner --uid-owner $(id -u serial_port_user) -j MARK --set-mark $HIGHEST_PRIORITY_MARK

# 2. Active TCP connections get high priority
iptables -t mangle -A POSTROUTING -p tcp -m conntrack --ctstate ESTABLISHED -j MARK --set-mark $HIGH_PRIORITY_MARK

# 3. Requests to the specific URL get medium priority
IP=$(dig +short $URL | head -n 1)
if [ -n "$IP" ]; then
  iptables -t mangle -A OUTPUT -d "$IP" -p tcp --dport 80 -j MARK --set-mark $MID_PRIORITY_MARK
  iptables -t mangle -A OUTPUT -d "$IP" -p tcp --dport 443 -j MARK --set-mark $MID_PRIORITY_MARK
else
  echo "Error: Unable to resolve IP for $URL"
  exit 1
fi

# 4. All other traffic gets low priority
iptables -t mangle -A POSTROUTING -j MARK --set-mark $LOW_PRIORITY_MARK

# Save iptables rules
iptables-save > /etc/iptables/rules.v4

echo "SDN-based traffic prioritization setup completed successfully."
