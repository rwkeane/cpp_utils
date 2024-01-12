#include <sstream>

#include "gtest/gtest.h"
#include "threading/parallel_circular_buffer.hpp"

namespace util {

class CircularBufferTest : public ::testing::Test {
 public:
  CircularBufferTest() = default;
  ~CircularBufferTest() override = default;

 protected:
  std::string IntToString(size_t int_value) {
    std::stringstream ss;
    ss << int_value;
    return ss.str();
  }

  static constexpr size_t kElementCount = 1024;
  ParallelCircularBuffer<std::string, kElementCount> buffer_;
};

TEST_F(CircularBufferTest, TestSequentialWrite) {
  for (size_t i = 0; i < kElementCount; i++) {
    std::string val = IntToString(i);
    const auto result = buffer_.TryEnqueue(val);
    EXPECT_TRUE(result);
    EXPECT_FALSE(buffer_.is_empty());
  }
  std::string val = IntToString(10);
  EXPECT_FALSE(buffer_.TryEnqueue(val));
  EXPECT_FALSE(buffer_.is_empty());

  for (size_t i = 0; i < kElementCount; i++) {
    EXPECT_FALSE(buffer_.is_empty());
    
    const auto result = buffer_.Dequeue();
    ASSERT_TRUE(!!result);
    EXPECT_STREQ(result->c_str(), IntToString(i).c_str());
  }

  EXPECT_FALSE(!!buffer_.Dequeue());
  EXPECT_TRUE(buffer_.is_empty());
}

TEST_F(CircularBufferTest, TestAlternatingReadWrite) {
  for (size_t i = 0; i < 3 * kElementCount; i++) {
    std::string val = IntToString(i);
    const auto enqueue_result = buffer_.TryEnqueue(val);
    ASSERT_TRUE(enqueue_result);
    EXPECT_FALSE(buffer_.is_empty());

    const auto dequeue_result = buffer_.Dequeue();
    ASSERT_TRUE(!!dequeue_result);
    EXPECT_STREQ(dequeue_result->c_str(), val.c_str());

    EXPECT_TRUE(buffer_.is_empty());
  }
}

}  // namespace util