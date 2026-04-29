#include <array>
#include <atomic>
#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include <arpa/inet.h>
#include <signal.h>
#include <sys/ioctl.h>

extern "C"
{
#include <ff_api.h>
}

namespace
{

constexpr std::size_t kPayloadSize = 1024;
constexpr std::size_t kExpectedEchoes = 1'000;

struct Client_context
{
	int socket = -1;

	bool is_connected = false;

	std::array < char, kPayloadSize > input_frame = {};

	std::size_t input_size = 0;

	std::string output;

	std::size_t output_offset = 0;

	std::size_t pending_echoes = 0;

	std::size_t echoes_sent = 0;

	int error_code = EXIT_SUCCESS;

	sockaddr_in server_address = {};
};

std::atomic < bool > g_stop_requested = false;

int get_error_code()
{
	return errno == 0 ? EXIT_FAILURE : errno;
}

void put_errno(Client_context & context, std::string const & operation)
{
	context.error_code = get_error_code();

	std::cerr
		<< operation
		<< " failed. Error code = "
		<< context.error_code
		<< ". Message: "
		<< std::strerror(context.error_code)
		<< std::endl;
}

void terminate(int)
{
	g_stop_requested = true;
}

void queue_echo(Client_context & context, char const * data, std::size_t size)
{
	std::size_t offset = 0;

	while (offset < size)
	{
		auto available = kPayloadSize - context.input_size;

		auto chunk = size - offset;

		if (chunk > available)
		{
			chunk = available;
		}

		std::memcpy
		(
			context.input_frame.data() + context.input_size,
			data + offset,
			chunk
		);

		context.input_size += chunk;

		offset += chunk;

		if (context.input_size == kPayloadSize)
		{
			context.output.append(context.input_frame.data(), context.input_frame.size());

			context.input_size = 0;

			++context.pending_echoes;
		}
	}
}

[[nodiscard]] bool flush_echo(Client_context & context)
{
	if (context.output.empty())
	{
		return true;
	}

	while (context.output_offset < std::size(context.output))
	{
		auto data = context.output.data() + context.output_offset;

		auto size = std::size(context.output) - context.output_offset;

		auto sent_bytes = ff_send(context.socket, data, size, 0);

		if (sent_bytes > 0)
		{
			context.output_offset += static_cast < std::size_t > (sent_bytes);

			continue;
		}

		if (sent_bytes == 0 || errno == EAGAIN || errno == EWOULDBLOCK)
		{
			return true;
		}

		put_errno(context, "ff_send");

		ff_stop_run();

		return false;
	}

	context.echoes_sent += context.pending_echoes;

	context.pending_echoes = 0;

	context.output.clear();

	context.output_offset = 0;

	if (context.echoes_sent >= kExpectedEchoes)
	{
		ff_stop_run();
	}

	return true;
}

int loop(void * arg)
{
	auto & context = *static_cast < Client_context * > (arg);

	if (g_stop_requested)
	{
		ff_stop_run();

		return 0;
	}

	if (!context.is_connected)
	{
		auto result = ff_connect
		(
			context.socket,
			reinterpret_cast < linux_sockaddr * > (&context.server_address),
			sizeof(context.server_address)
		);

		if (result == 0 || errno == EISCONN)
		{
			context.is_connected = true;

			return 0;
		}

		if (errno != EINPROGRESS && errno != EALREADY)
		{
			put_errno(context, "ff_connect");

			ff_stop_run();

			return -1;
		}

		return 0;
	}

	if (!flush_echo(context))
	{
		return -1;
	}

	if (context.echoes_sent >= kExpectedEchoes)
	{
		return 0;
	}

	std::array < char, kPayloadSize > buffer = {};

	auto received_bytes = ff_recv(context.socket, buffer.data(), std::size(buffer), 0);

	if (received_bytes > 0)
	{
		queue_echo(context, buffer.data(), static_cast < std::size_t > (received_bytes));

		if (!flush_echo(context))
		{
			return -1;
		}
	}
	else if (received_bytes == 0)
	{
		ff_stop_run();
	}
	else if (errno != EAGAIN && errno != EWOULDBLOCK)
	{
		put_errno(context, "ff_recv");

		ff_stop_run();

		return -1;
	}

	return 0;
}

}

int main(int argc, char ** argv)
{
	struct sigaction action;

	std::memset(&action, 0, sizeof(struct sigaction));

	action.sa_handler = terminate;

	sigaction(SIGINT, &action, nullptr);

	sigaction(SIGTERM, &action, nullptr);

	Client_context context;

	std::string raw_ip_address = "192.168.1.104";

	const auto port = 3333;

	if (auto result = ff_init(argc, argv); result < 0)
	{
		context.error_code = get_error_code();

		std::cerr
			<< "ff_init failed. Result = "
			<< result
			<< ". Error code = "
			<< context.error_code
			<< std::endl;

		return context.error_code;
	}

	context.socket = ff_socket(AF_INET, SOCK_STREAM, 0);

	if (context.socket < 0)
	{
		put_errno(context, "ff_socket");

		return context.error_code;
	}

	int on = 1;

	if (auto result = ff_ioctl(context.socket, FIONBIO, &on); result < 0)
	{
		put_errno(context, "ff_ioctl");

		ff_close(context.socket);

		return context.error_code;
	}

	context.server_address.sin_family = AF_INET;
	context.server_address.sin_port = htons(port);

	if (inet_pton(AF_INET, raw_ip_address.c_str(), &context.server_address.sin_addr) != 1)
	{
		context.error_code = EXIT_FAILURE;

		std::cerr << "Invalid IPv4 address: " << raw_ip_address << std::endl;

		ff_close(context.socket);

		return context.error_code;
	}

	ff_run(loop, &context);

	if (context.socket >= 0)
	{
		ff_close(context.socket);
	}

	return context.error_code;
}
