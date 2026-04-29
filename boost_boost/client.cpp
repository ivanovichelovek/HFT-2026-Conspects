#include <array>
#include <cstdlib>
#include <iostream>
#include <string>

#include <boost/asio.hpp>

namespace
{

constexpr std::size_t kPayloadSize = 1024;
constexpr std::size_t kIterations = 1'000;

void echo_loop(boost::asio::ip::tcp::socket & socket)
{
	std::array < char, kPayloadSize > buffer = {};

	for (std::size_t i = 0; i < kIterations; ++i)
	{
		boost::asio::read(socket, boost::asio::buffer(buffer));

		boost::asio::write(socket, boost::asio::buffer(buffer));
	}
}

}

int main(int argc, char ** argv)
{
	std::string raw_ip_address = "192.168.1.104";

	if (argc >= 2)
	{
		raw_ip_address = argv[1];
	}

	const auto port = 3333;

	try
	{
		boost::asio::ip::tcp::endpoint endpoint
		(
			boost::asio::ip::address::from_string(raw_ip_address),
			port
		);

		boost::asio::io_service io_service;

		boost::asio::ip::tcp::socket socket(io_service);

		socket.connect(endpoint);

		echo_loop(socket);
	}
	catch (boost::system::system_error & e)
	{
		std::cerr
			<< "Error occured! Error code = "
			<< e.code()
			<< ". Message: "
			<< e.what()
			<< std::endl;

		return e.code().value();
	}

	return EXIT_SUCCESS;
}
