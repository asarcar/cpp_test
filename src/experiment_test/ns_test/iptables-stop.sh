echo "Stopping firewall, cleaning all rules, and allowing everyone..."
#
# Invoke the script within a namespace. Assuming network NS red: 
#> sudo ip netns exec red sh iptables-stop.sh

# -F|--flush : delete all rules in chain or all chains
iptables -F
# -X|--delete-chain: delete a user defined chain or all chains
iptables -X
# -t|--table: for table - Default table is filter
iptables -t nat -F
iptables -t nat -X
iptables -t mangle -F
iptables -t mangle -X
# -P|--policy: change policy on chain to target
iptables -P INPUT ACCEPT
iptables -P FORWARD ACCEPT
iptables -P OUTPUT ACCEPT
