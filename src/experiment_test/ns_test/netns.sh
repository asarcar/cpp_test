#!/bin/sh

# 
# * Create NS red, execute script inside red, and delete NS
# > sudo ip netns add test && \
#   sudo ip netns exec test sh netns.sh && \
#   sudo ip netns del test
#

# Move to or Execute cmd in 'red' NS bash session 
ip link list

# Bring up the loopback interface of red NS
ip link set dev lo up
ping -c1 127.0.0.1

# Create a "paired" virtual interface
ip link add veth-test type veth peer name veth-base
ip link set veth-base netns 1

# Assign IP addresses to each pair
ifconfig veth-test 10.1.1.31/24 up
ping -c1 10.1.1.31

# VLAN interface: parent interface may be in another namespace
ip link add link veth-test name veth-test.100 type vlan id 100

#
# Use bridge commands
# 
brctl addbr br-test
brctl addif br-test veth-test.100
brctl show
brctl showmacs br-test

#
# Adding new Tap/Tun Interfaces
#
ip tuntap add dev ethtap mode tap
ip addr add 10.1.2.30/24 broadcast 10.1.2.255 dev ethtap
ip link set dev ethtap up
ping -c1 10.1.2.30

#
# Add multiple addresses to the same device
#
ip addr add 10.1.3.30/24 dev ethtap label ethtap:1
ping -c1 10.1.3.30

#
# SIDE NOTE:
# For interface aliases that would persist (say on br-red): 
# one could configure ip aliases the following way:
# auto br-red:0
# iface br-red:0 inet static
#   name br-red Alias
#   address 10.1.1.40
#   netmask 255.255.255.0
#   broadcast 10.1.1.255
#   network 10.1.1.0
#   gateway 10.1.1.1
#

#
# Filters: /proc/sys/net/bridge/bridge-nd-*: filtering enabled if set to 1
# ebtables: netfilter
#

##################
# VRF-lite test: #
##################
ip netns add test2
ip link add veth-t1 type veth peer name veth-t2
ip link set veth-t2 netns test2
ip link set veth-t1 up
ip netns exec test2 ip link set veth-t2 up
ip addr add 10.1.4.31/24 dev veth-t1
ip netns exec test2 ip addr add 10.1.4.32/24 dev veth-t2
ip netns exec test2 ping -c1 10.1.4.31
# Add VRF: append /etc/iproute2/rt_tables and add the VRF tables
echo "100      TEST" >> /etc/iproute2/rt_tables
ip netns exec test2 ip route add default via 10.1.4.31 dev veth-t2 proto static scope global table TEST
# Create an ip rule list so that all lookups are executed in the RED VRF
ip netns exec test2 ip rule add from default pref 100 lookup TEST
# We are all set to execute a ping test to an interface and prefix 
# outside the test2 NS
ping -c1 10.1.2.30
# Restore the rt_tables to original state
sed -i '$ d' /etc/iproute2/rt_tables
##################
ip netns del test2
