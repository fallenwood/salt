#include <iostream>

#include "salt/packet_assemble.h"
#include "salt/tcp_server.h"
#include "salt/version.h"

static void send_call_back(uint32_t seq, const std::error_code &error_code) {
  if (error_code) {
    std::cout << "send data seq:" << seq
              << ", with error code:" << error_code.message() << std::endl;
  } else {
    std::cout << "send data seq:" << seq << " success" << std::endl;
  }
}

class echo_packet_assemble : public salt::base_co_packet_assemble {
public:
  asio::awaitable<salt::data_read_result>
  co_data_received(std::shared_ptr<salt::connection_handle> connection,
                std::string s) override {
    static uint32_t seq = 0;
    co_await connection->co_send(++seq, std::move(s));

    co_return salt::data_read_result::success;
  }
};

int main() {
  salt::tcp_server server;
  server.set_co_assemble_creator([] { return new echo_packet_assemble(); });
  server.init(2002);

  auto &io_context = server.get_io_context();

  asio::co_spawn(io_context, server.co_start(), asio::detached);

  io_context.run();
  return 0;
}