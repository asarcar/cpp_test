#!/bin/sh

echo ">> execute 'echo password | sudo -S ls' so that subsequent sudo commands do not require password"
echo ">> close the terminal afterwards since we now have unrestricted sudo access"

#
# Network Namespaces Basics
#
# Add Network Namespace red
sudo ip netns add red

# Move to or Execute cmd in 'red' NS bash session 
sudo ip netns exec red bash -c "echo 'Execute cmd in NS Red!'"

sudo ip netns exec red ip link list

# Bring up the loopback interface of red NS
sudo ip netns exec red ip link set dev lo up
sudo ip netns exec red ping -c1 127.0.0.1

# Create a "paired" virtual interface
sudo ip link add veth0 type veth peer name veth1
sudo ip link set veth0 netns red
sudo ip link set veth1 netns red

# Assign IP addresses to each pair
sudo ip netns exec red ifconfig veth0 10.1.1.50/24 up
sudo ip netns exec red ifconfig veth1 10.1.1.51/24 up
sudo ip netns exec red ip link
sudo ip netns exec red ifconfig

sudo ip netns exec red ping -c1 10.1.1.51
sudo ip netns exec red ping -c1 10.1.1.50

sudo ip netns exec red route
sudo ip netns exec red iptables -L

# move veth1 from red NS to base using init PID=1
sudo ip netns exec red ip link set veth1 netns 1

# VLAN interface: parent interface may be in another namespace
sudo ip link add link veth1 name veth1.100 type vlan id 100
sudo ip link set veth1.100 netns red

#
# Use bridge commands
# 
sudo ip netns exec red brctl addbr br-red
sudo ip netns exec red brctl addif br-red veth0
sudo ip netns exec red brctl show
sudo ip netns exec red brctl showmacs br-red

#
# Filters: /proc/sys/net/bridge/bridge-nd-*: filtering enabled if set to 1
# ebtables: netfilter
#

# Cleanup all state created via NS
sudo ip netns del red 
