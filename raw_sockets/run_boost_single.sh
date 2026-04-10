echo "building server.cpp ..."
g++ server.cpp -o server.exe -std=c++23 -O3 -pthread -lboost_system -Wall -Wextra -pedantic

echo "building client_boost_asio.cpp ..."
g++ client_boost_asio.cpp -o client_boost_asio.exe -std=c++23 -O3 -pthread -lboost_system -Wall -Wextra -pedantic

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

echo "successfully built server and boost client"