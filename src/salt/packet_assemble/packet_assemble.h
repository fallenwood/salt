#pragma once

#include <memory>
#include <string>

#include "salt/core/connection_handle.h"

#include "asio.hpp"

namespace salt {

enum class data_read_result {
  success = 0,
  error = 1,
  disconnect = 2,
};

class base_packet_assemble {
public:
  virtual data_read_result
  data_received(std::shared_ptr<connection_handle> connection,
                std::string s) = 0;
  virtual ~base_packet_assemble() = default;
};


class base_co_packet_assemble {
public:
  virtual asio::awaitable<data_read_result>
  co_data_received(std::shared_ptr<connection_handle> connection,
                std::string s) = 0;
  virtual ~base_co_packet_assemble() = default;
};

} // namespace salt