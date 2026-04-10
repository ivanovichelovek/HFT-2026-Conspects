#include <iostream>
#include <chrono>
#include <fstream>

#include <boost/asio.hpp>

void write_data(boost::asio::ip::tcp::socket& socket, const std::string& data)
{

	boost::asio::write(socket, boost::asio::buffer(data));
}

auto get_current_time_mcs() {
	auto now = std::chrono::high_resolution_clock::now();
	auto mcs = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
	return mcs;
}

std::string read_data_until(boost::asio::ip::tcp::socket& socket, char delimiter='!') {
  boost::asio::streambuf buffer;
  boost::asio::read_until(socket, buffer, delimiter);

  std::string message;
  std::istream is(&buffer);
  std::getline(is, message, delimiter);
  return message;
}

int main(int argc, char ** argv)
{
	std::string raw_ip_address = "10.200.1.2";

	auto port = 3333;
	const char* output_path = "recv_single_boost.txt";
	const int message_count = 1000;

	std::vector<uint64_t> deltas;
	deltas.reserve(message_count);
	int exit_code = EXIT_SUCCESS;

	try 
	{
		boost::asio::ip::tcp::endpoint endpoint(
			boost::asio::ip::address::from_string(raw_ip_address), port);

		boost::asio::io_service io_service;

		boost::asio::ip::tcp::socket socket(io_service, endpoint.protocol());

		socket.connect(endpoint);

    std::cout << "Successfully connected to server\n";

		for (int i = 0; i < message_count; ++i) {
			const uint64_t send_time = get_current_time_mcs();
			const std::string message = std::to_string(send_time) + '!';
			write_data(socket, message);

			const std::string echoed_message = read_data_until(socket, '!');
			const uint64_t receive_time = get_current_time_mcs();
			const uint64_t echoed_send_time = std::stoull(echoed_message);
			deltas.push_back(receive_time - echoed_send_time);
		}
		}
		catch (boost::system::system_error & e) 
		{
			std::cout << "Error occured! Error code = " << e.code() << ". Message: " << e.what() << std::endl;
			exit_code = e.code().value();
		}
	std::ofstream fout(output_path);

	for (const uint64_t delta : deltas) {
		fout << delta << '\n';
	}

	fout.close();

	return exit_code;
}
/*
First terminal

echo "building server.cpp ..."
g++ server.cpp -o server.exe -std=c++23 -O3 -pthread -lboost_system -Wall -Wextra -pedantic

echo "building client_boost_asio.cpp ..."
g++ client_boost_asio.cpp -o client_boost_asio.exe -std=c++23 -O3 -pthread -lboost_system -Wall -Wextra -pedantic

sudo ip netns del ns_c 2>/dev/null || true
sudo ip netns del ns_s 2>/dev/null || true
sudo ip link del veth_c 2>/dev/null || true

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

echo "network namespaces are ready"
echo "run server:"
echo "  sudo ip netns exec ns_s ./server.exe"
echo "run client:"
echo "  sudo ip netns exec ns_c ./client_boost_asio.exe"
*/