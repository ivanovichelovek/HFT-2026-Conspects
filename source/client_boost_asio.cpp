#include <iostream>
#include <chrono>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>

#include <boost/asio.hpp>

void write_data(boost::asio::ip::tcp::socket& socket, const std::string& data)
{
	boost::asio::write(socket, boost::asio::buffer(data));
}

uint64_t get_current_time_mcs() {
	auto now = std::chrono::high_resolution_clock::now();
	auto mcs = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
	return static_cast<uint64_t>(mcs);
}

std::string read_data_until(boost::asio::ip::tcp::socket& socket, char delimiter = '!') {
	boost::asio::streambuf buffer;
	boost::asio::read_until(socket, buffer, delimiter);

	std::string message;
	std::istream is(&buffer);
	std::getline(is, message, delimiter);
	return message;
}

int main(int argc, char** argv)
{
	std::string raw_ip_address = "192.168.3.2";
	auto port = 3333;

	if (argc >= 3) {
		raw_ip_address = argv[2];
	}

	if (argc >= 4) {
		port = std::atoi(argv[3]);
	}

	const char* output_path = "recv_duo_boost.txt";
	const int message_count = 10000;

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
			const uint64_t client_send_time = get_current_time_mcs();
			write_data(socket, std::to_string(client_send_time) + "!");

			const std::string message = read_data_until(socket, '!');
			const uint64_t receive_time = get_current_time_mcs();
			const uint64_t server_send_time = std::stoull(message);

			deltas.push_back(receive_time - server_send_time);
		}
	}
	catch (boost::system::system_error& e) {
		std::cout << "Error occured! Error code = " << e.code()
		          << ". Message: " << e.what() << std::endl;
		exit_code = e.code().value();
	}

	std::ofstream fout(output_path);

	for (const uint64_t delta : deltas) {
		fout << delta << '\n';
	}

	fout.close();

	return exit_code;
}