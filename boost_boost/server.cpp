#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>

#include <boost/asio.hpp>

namespace
{

using namespace std::literals;

constexpr auto kPort = 3333;
constexpr int kBacklog = 30;
constexpr std::size_t kPayloadSize = 1024;
constexpr std::size_t kIterations = 1'000;
constexpr std::size_t kReportStep = 100;
constexpr auto kInterval = 10ms;

char const * kTimestampFile = "server_timestamps.csv";

[[nodiscard]] long long timestamp_ns()
{
	return std::chrono::duration_cast < std::chrono::nanoseconds >
	(
		std::chrono::high_resolution_clock::now().time_since_epoch()
	)
	.count();
}

[[nodiscard]] auto make_payload(std::size_t sequence) -> std::array < char, kPayloadSize >
{
	std::array < char, kPayloadSize > payload = {};

	payload.fill(static_cast < char > (sequence % 251));

	auto sequence_value = static_cast < std::uint32_t > (sequence);

	std::memcpy(payload.data(), &sequence_value, sizeof(sequence_value));

	return payload;
}

void ping_client(boost::asio::ip::tcp::socket & socket)
{
	std::ofstream log(kTimestampFile);

	if (!log.is_open())
	{
		throw std::runtime_error("cannot open server_timestamps.csv");
	}

	log << "seq,send_timestamp_ns,recv_timestamp_ns,rtt_ns" << std::endl;

	std::array < char, kPayloadSize > response = {};

	for (std::size_t i = 0; i < kIterations; ++i)
	{
		auto request = make_payload(i);

		auto send_ts = timestamp_ns();

		boost::asio::write(socket, boost::asio::buffer(request));

		boost::asio::read(socket, boost::asio::buffer(response));

		auto recv_ts = timestamp_ns();

		auto rtt = recv_ts - send_ts;

		log << i << "," << send_ts << "," << recv_ts << "," << rtt << std::endl;

		if (response != request)
		{
			std::cerr << "Unexpected echo payload at seq " << i << std::endl;
		}

		if ((i + 1) % kReportStep == 0)
		{
			std::cout << "Ping " << (i + 1) << "/" << kIterations << std::endl;
		}

		std::this_thread::sleep_for(kInterval);
	}

	std::cout << "Done. Log: " << kTimestampFile << std::endl;
}

}

int main()
{
	boost::asio::ip::tcp::endpoint endpoint
	(
		boost::asio::ip::address_v4::any(),
		kPort
	);

	boost::asio::io_service io_service;

	try
	{
		boost::asio::ip::tcp::acceptor acceptor(io_service, endpoint.protocol());

		acceptor.bind(endpoint);

		acceptor.listen(kBacklog);

		boost::asio::ip::tcp::socket socket(io_service);

		acceptor.accept(socket);

		ping_client(socket);
	}
	catch (boost::system::system_error & e)
	{
		std::cerr
			<< "Error occured! Error code = "
			<< e.code().value()
			<< ". Message: "
			<< e.what()
			<< std::endl;

		return e.code().value();
	}
	catch (std::exception & e)
	{
		std::cerr
			<< "Error occured! Error code = "
			<< EXIT_FAILURE
			<< ". Message: "
			<< e.what()
			<< std::endl;

		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
