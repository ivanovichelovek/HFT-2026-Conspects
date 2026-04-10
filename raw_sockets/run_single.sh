echo "building server.cpp ..."

g++ server.cpp -o server.exe -std=c++23 -O3 -pthread -lboost_system -Wall -Wextra -pedantic

echo "building client.cpp ..."

g++ client.cpp -o client.exe -std=c++23 -O3 -Wall -Wextra -pedantic

sudo setcap cap_net_raw+ep ./client.exe

sudo ip netns add ns_c
sudo ip netns add ns_s
sudo ip link add veth_c type veth peer name veth_s
sudo ip link set veth_c netns ns_c
sudo ip link set veth_s netns ns_s
sudo ip netns exec ns_c ip addr add 10.200.1.1/24 dev veth_c
sudo ip netns exec ns_s ip addr add 10.200.1.2/24 dev veth_s
sudo ip netns exec ns_c ip link set lo up
sudo ip netns exec ns_s ip link set lo up
sudo ip netns exec ns_c ip link set veth_c up
sudo ip netns exec ns_s ip link set veth_s up

sudo ip netns exec ns_c iptables -A OUTPUT -p tcp --tcp-flags RST RST -j DROP

echo "successfully build server.cpp, elient.cpp"