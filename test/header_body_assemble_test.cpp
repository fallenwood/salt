#include "gtest/gtest.h"

#include <vector>

#include "salt/packet_assemble/header_body_assemble.h"
#include "salt/util/byte_order.h"

class message_header32 {
public:
  uint32_t magic_;
  uint32_t len_;
};

#pragma pack(1)
class message_header16 {
public:
  uint16_t magic_;
  uint16_t len_;
  uint64_t some_padding_;
};
#pragma pack()

std::string encode_with_string(message_header32 &header, const std::string &s,
                               salt::body_length_calc_mode calc_mode,
                               uint32_t reserve = 0) {
  std::string result;
  result.reserve(sizeof(message_header32));
  header.magic_ = salt::byte_order::to_network(header.magic_);
  std::copy(reinterpret_cast<char *>(&header.magic_),
            reinterpret_cast<char *>(&header.magic_) +
                sizeof(decltype(header.magic_)),
            std::back_inserter(result));

  header.len_ = s.size();
  if (calc_mode == salt::body_length_calc_mode::with_header) {
    header.len_ += sizeof(header);
  } else if (calc_mode == salt::body_length_calc_mode::with_length_field) {
    header.len_ += sizeof(decltype(header.len_));
  } else if (calc_mode == salt::body_length_calc_mode::custom_length) {
    header.len_ += reserve;
  }
  header.len_ = salt::byte_order::to_network(header.len_);

  std::copy(reinterpret_cast<char *>(&header.len_),
            reinterpret_cast<char *>(&header.len_) +
                sizeof(decltype(header.len_)),
            std::back_inserter(result));

  result += s;
  return result;
}

std::string encode_with_string(message_header16 &header, const std::string &s,
                               salt::body_length_calc_mode calc_mode,
                               uint32_t reserve = 0) {
  std::string result;
  result.reserve(sizeof(message_header16));
  header.magic_ = salt::byte_order::to_network(header.magic_);
  std::copy(reinterpret_cast<char *>(&header.magic_),
            reinterpret_cast<char *>(&header.magic_) +
                sizeof(decltype(header.magic_)),
            std::back_inserter(result));

  header.len_ = s.size();
  if (calc_mode == salt::body_length_calc_mode::with_header) {
    header.len_ += sizeof(header);
  } else if (calc_mode == salt::body_length_calc_mode::with_length_field) {
    header.len_ += sizeof(decltype(header.len_));
  } else if (calc_mode == salt::body_length_calc_mode::custom_length) {
    header.len_ += sizeof(reserve);
  }

  header.len_ = salt::byte_order::to_network(header.len_);

  std::copy(reinterpret_cast<char *>(&header.len_),
            reinterpret_cast<char *>(&header.len_) +
                sizeof(decltype(header.len_)),
            std::back_inserter(result));

  std::copy(reinterpret_cast<char *>(&header.some_padding_),
            reinterpret_cast<char *>(&header.some_padding_) +
                sizeof(decltype(header.some_padding_)),
            std::back_inserter(result));

  result += s;
  return result;
}

template <typename header_type>
class test_notify : public salt::header_body_assemble_notify<header_type> {
public:
  test_notify(std::vector<std::string> &token) : token_(token) {}

  salt::data_read_result
  packet_reserved(std::shared_ptr<salt::connection_handle> connection,
                  std::string raw_header_data, std::string body) override {
    token_.emplace_back(std::move(body));
    return salt::data_read_result::success;
  }

  void packet_read_error(const std::error_code &error_code,
                         const std::string &_) override {
    ASSERT_FALSE(true);
  }

private:
  std::vector<std::string> &token_;
};

TEST(header_body_assemble_test, body_only) {
  message_header32 h;
  h.magic_ = 12345;
  auto s =
      encode_with_string(h, "body", salt::body_length_calc_mode::body_only);
  s += encode_with_string(h, "only", salt::body_length_calc_mode::body_only);
  for (int step = 1; step < s.size() + 1; ++step) {
    auto packet_assemble =
        salt::header_body_assemble<message_header32, &message_header32::len_>();
    packet_assemble.body_length_calc_mode_ =
        salt::body_length_calc_mode::body_only;
    std::vector<std::string> token;
    auto notify = std::make_unique<test_notify<message_header32>>(token);
    packet_assemble.set_notify(std::move(notify));
    uint32_t current_offset = 0;
    while (current_offset < s.size()) {
      auto result =
          (&packet_assemble)
              ->data_received(nullptr, s.substr(current_offset, step));
      ASSERT_EQ(result, salt::data_read_result::success);
      if (result != salt::data_read_result::success) {
        return;
      }
      current_offset += step;
    }
    ASSERT_EQ(token.size(), 2);
    ASSERT_STREQ(token[0].c_str(), "body");
    ASSERT_STREQ(token[1].c_str(), "only");
  }

  ASSERT_TRUE(true);
}

TEST(header_body_assemble_test, with_length_field) {
  message_header16 h;
  h.magic_ = 12345;
  auto s = encode_with_string(h, "with",
                              salt::body_length_calc_mode::with_length_field);
  s += encode_with_string(h, "length",
                          salt::body_length_calc_mode::with_length_field);
  s += encode_with_string(h, "field",
                          salt::body_length_calc_mode::with_length_field);
  for (int step = 1; step < s.size() + 1; ++step) {
    auto packet_assemble =
        salt::header_body_assemble<message_header16, &message_header16::len_>();
    packet_assemble.body_length_calc_mode_ =
        salt::body_length_calc_mode::with_length_field;
    std::vector<std::string> token;
    auto notify = std::make_unique<test_notify<message_header16>>(token);
    packet_assemble.set_notify(std::move(notify));
    uint32_t current_offset = 0;
    while (current_offset < s.size()) {
      auto result =
          (&packet_assemble)
              ->data_received(nullptr, s.substr(current_offset, step));
      ASSERT_EQ(result, salt::data_read_result::success);
      if (result != salt::data_read_result::success) {
        return;
      }
      current_offset += step;
    }
    ASSERT_EQ(token.size(), 3);
    ASSERT_STREQ(token[0].c_str(), "with");
    ASSERT_STREQ(token[1].c_str(), "length");
    ASSERT_STREQ(token[2].c_str(), "field");
  }

  ASSERT_TRUE(true);
}

TEST(header_body_assemble_test, with_header) {
  message_header16 h;
  h.magic_ = 12345;
  auto s =
      encode_with_string(h, "with", salt::body_length_calc_mode::with_header);
  s +=
      encode_with_string(h, "header", salt::body_length_calc_mode::with_header);
  for (int step = 1; step < s.size() + 1; ++step) {
    auto packet_assemble =
        salt::header_body_assemble<message_header16, &message_header16::len_>();
    packet_assemble.body_length_calc_mode_ =
        salt::body_length_calc_mode::with_header;
    std::vector<std::string> token;
    auto notify = std::make_unique<test_notify<message_header16>>(token);
    packet_assemble.set_notify(std::move(notify));
    uint32_t current_offset = 0;
    while (current_offset < s.size()) {
      auto result =
          (&packet_assemble)
              ->data_received(nullptr, s.substr(current_offset, step));
      ASSERT_EQ(result, salt::data_read_result::success);
      if (result != salt::data_read_result::success) {
        return;
      }
      current_offset += step;
    }
    ASSERT_EQ(token.size(), 2);
    ASSERT_STREQ(token[0].c_str(), "with");
    ASSERT_STREQ(token[1].c_str(), "header");
  }

  ASSERT_TRUE(true);
}

TEST(header_body_assemble_test, custom_length) {
  message_header32 h;
  h.magic_ = 12345;
  constexpr uint32_t custom_length = 0xbaadf00d;
  auto s = encode_with_string(
      h, "custom", salt::body_length_calc_mode::custom_length, custom_length);
  s += encode_with_string(
      h, "length", salt::body_length_calc_mode::custom_length, custom_length);
  for (int step = 1; step < s.size() + 1; ++step) {
    auto packet_assemble =
        salt::header_body_assemble<message_header32, &message_header32::len_>();
    packet_assemble.body_length_calc_mode_ =
        salt::body_length_calc_mode::custom_length;
    packet_assemble.reserve_body_size_ = custom_length;
    std::vector<std::string> token;
    auto notify = std::make_unique<test_notify<message_header32>>(token);
    packet_assemble.set_notify(std::move(notify));
    uint32_t current_offset = 0;
    while (current_offset < s.size()) {
      auto result =
          (&packet_assemble)
              ->data_received(nullptr, s.substr(current_offset, step));
      ASSERT_EQ(result, salt::data_read_result::success);
      if (result != salt::data_read_result::success) {
        return;
      }
      current_offset += step;
    }
    ASSERT_EQ(token.size(), 2);
    ASSERT_STREQ(token[0].c_str(), "custom");
    ASSERT_STREQ(token[1].c_str(), "length");
  }

  ASSERT_TRUE(true);
}

TEST(header_body_assemble_test, empty_body) {
  message_header16 h;
  h.magic_ = 12345;
  auto s =
      encode_with_string(h, "", salt::body_length_calc_mode::with_length_field);
  s += encode_with_string(h, "empty",
                          salt::body_length_calc_mode::with_length_field);
  for (int step = 1; step < s.size() + 1; ++step) {
    auto packet_assemble =
        salt::header_body_assemble<message_header16, &message_header16::len_>();
    packet_assemble.body_length_calc_mode_ =
        salt::body_length_calc_mode::with_length_field;
    std::vector<std::string> token;
    auto notify = std::make_unique<test_notify<message_header16>>(token);
    packet_assemble.set_notify(std::move(notify));
    uint32_t current_offset = 0;
    while (current_offset < s.size()) {
      auto result =
          (&packet_assemble)
              ->data_received(nullptr, s.substr(current_offset, step));
      ASSERT_EQ(result, salt::data_read_result::success);
      if (result != salt::data_read_result::success) {
        return;
      }
      current_offset += step;
    }
    ASSERT_EQ(token.size(), 2);
    ASSERT_TRUE(token[0].empty());
    ASSERT_STREQ(token[1].c_str(), "empty");
  }

  ASSERT_TRUE(true);
}

TEST(header_body_assemble_test, empty_body_2) {
  message_header16 h;
  h.magic_ = 12345;
  auto s = encode_with_string(h, "empty",
                              salt::body_length_calc_mode::with_length_field);
  s +=
      encode_with_string(h, "", salt::body_length_calc_mode::with_length_field);
  for (int step = 1; step < s.size() + 1; ++step) {
    auto packet_assemble =
        salt::header_body_assemble<message_header16, &message_header16::len_>();
    packet_assemble.body_length_calc_mode_ =
        salt::body_length_calc_mode::with_length_field;
    std::vector<std::string> token;
    auto notify = std::make_unique<test_notify<message_header16>>(token);
    packet_assemble.set_notify(std::move(notify));
    uint32_t current_offset = 0;
    while (current_offset < s.size()) {
      auto result =
          (&packet_assemble)
              ->data_received(nullptr, s.substr(current_offset, step));
      ASSERT_EQ(result, salt::data_read_result::success);
      if (result != salt::data_read_result::success) {
        return;
      }
      current_offset += step;
    }
    ASSERT_EQ(token.size(), 2);
    ASSERT_STREQ(token[0].c_str(), "empty");
    ASSERT_TRUE(token[1].empty());
  }

  ASSERT_TRUE(true);
}

TEST(header_body_assemble_test, empty_body_3) {
  message_header16 h;
  h.magic_ = 12345;
  auto s =
      encode_with_string(h, "", salt::body_length_calc_mode::with_length_field);
  s +=
      encode_with_string(h, "", salt::body_length_calc_mode::with_length_field);
  for (int step = 1; step < s.size() + 1; ++step) {
    auto packet_assemble =
        salt::header_body_assemble<message_header16, &message_header16::len_>();
    packet_assemble.body_length_calc_mode_ =
        salt::body_length_calc_mode::with_length_field;
    std::vector<std::string> token;
    auto notify = std::make_unique<test_notify<message_header16>>(token);
    packet_assemble.set_notify(std::move(notify));
    uint32_t current_offset = 0;
    while (current_offset < s.size()) {
      auto result =
          (&packet_assemble)
              ->data_received(nullptr, s.substr(current_offset, step));
      ASSERT_EQ(result, salt::data_read_result::success);
      if (result != salt::data_read_result::success) {
        return;
      }
      current_offset += step;
    }
    ASSERT_EQ(token.size(), 2);
    ASSERT_TRUE(token[0].empty());
    ASSERT_TRUE(token[1].empty());
  }

  ASSERT_TRUE(true);
}

TEST(header_body_assemble_test, empty_body_4) {
  message_header16 h;
  h.magic_ = 12345;
  auto s =
      encode_with_string(h, "", salt::body_length_calc_mode::with_length_field);
  s += encode_with_string(h, "hello",
                          salt::body_length_calc_mode::with_length_field);
  s +=
      encode_with_string(h, "", salt::body_length_calc_mode::with_length_field);
  for (int step = 1; step < s.size() + 1; ++step) {
    auto packet_assemble =
        salt::header_body_assemble<message_header16, &message_header16::len_>();
    packet_assemble.body_length_calc_mode_ =
        salt::body_length_calc_mode::with_length_field;
    std::vector<std::string> token;
    auto notify = std::make_unique<test_notify<message_header16>>(token);
    packet_assemble.set_notify(std::move(notify));
    uint32_t current_offset = 0;
    while (current_offset < s.size()) {
      auto result =
          (&packet_assemble)
              ->data_received(nullptr, s.substr(current_offset, step));
      ASSERT_EQ(result, salt::data_read_result::success);
      if (result != salt::data_read_result::success) {
        return;
      }
      current_offset += step;
    }
    ASSERT_EQ(token.size(), 3);
    ASSERT_TRUE(token[0].empty());
    ASSERT_STREQ(token[1].c_str(), "hello");
    ASSERT_TRUE(token[2].empty());
  }

  ASSERT_TRUE(true);
}

// tyzual:以后再写拆包器我是狗（）