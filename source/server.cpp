#include <boost/asio.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

std::string read_data(boost::asio::ip::tcp::socket& socket) {
  const std::size_t length = 10;
  char buffer[length];
  boost::asio::read(socket, boost::asio::buffer(buffer, length));
  return std::string(buffer, length);
}

uint64_t Stoi(const std::string& str) {
  uint64_t res = 0;
  for (char i : str) {
    res = res * 10 + (i - '0');
  }
  return res;
}

std::string read_data_until(boost::asio::ip::tcp::socket& socket) {
  boost::asio::streambuf buffer;

  boost::asio::read_until(socket, buffer, '!');

  std::string message;

  // Because buffer 'buf' may contain some other data
  // after '\n' symbol, we have to parse the buffer and
  // extract only symbols before the delimiter.
  std::istream input_stream(&buffer);
  std::getline(input_stream, message, '!');

  return message;
}

void send_data(boost::asio::ip::tcp::socket& socket,
               const std::string& message) {
  boost::asio::write(socket, boost::asio::buffer(message));
}

auto get_current_time_mcs() {
  auto now = std::chrono::high_resolution_clock::now();
  auto mcs = std::chrono::duration_cast<std::chrono::microseconds>(
                 now.time_since_epoch())
                 .count();
  return mcs;
}

int main(int argc, char** argv)

{
  const std::size_t size = 30;

  auto port = 3333;

  boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address_v4::any(),
                                          port);

  boost::asio::io_service io_service;

  try {
    boost::asio::ip::tcp::acceptor acceptor(io_service, endpoint.protocol());

    acceptor.set_option(
        boost::asio::socket_base::reuse_address(true));  // allow reuse

    acceptor.bind(endpoint);

    acceptor.listen(size);

    boost::asio::ip::tcp::socket socket(io_service);

    acceptor.accept(socket);

    std::cout << "Client connected" << std::endl;

    // ---------- duplex echo ----------

    // Старый вариант: сервер сам генерировал сообщения и отправлял их клиенту.
    // for (int i = 0; i < 1000; ++i) {
    //   uint64_t send_time = get_current_time_mcs();
    //   send_data(socket, std::to_string(send_time) + '!');
    //
    //   std::this_thread::sleep_for(std::chrono::milliseconds(10));
    // }

    // Новый вариант: сервер только возвращает клиенту его же сообщение.
    while (true) {
      std::string message = read_data_until(socket);
      send_data(socket, message + '!');
    }

    // fout.close();
  } catch (boost::system::system_error& e) {
    std::cout << "Error occured! Error code = " << e.code()
              << ". Message: " << e.what() << std::endl;

    return e.code().value();
  }

  return EXIT_SUCCESS;
}
