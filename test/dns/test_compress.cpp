/**
 * @file test_compress.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-08-20
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "xsl/logctl.h"
#include "xsl/net.h"

#include <gtest/gtest.h>

#include <string_view>
using namespace xsl::dns;
TEST(dns, compress) {
  std::string_view src_dns[] = {"www.google.com", "mail.google.com", "com", "."};
  unsigned char dst_dns[256];
  memset(dst_dns, 0, 256);
  int total = 0;
  DnCompressor dc(dst_dns);
  auto n = dc.prepare(src_dns[0]);
  ASSERT_GT(n.size(), 0);
  dc.compress({dst_dns + total, n.size()});
  total += n.size();
  ASSERT_EQ(dst_dns[0], 3);
  ASSERT_EQ(dst_dns[1], 'w');
  ASSERT_EQ(dst_dns[2], 'w');
  ASSERT_EQ(dst_dns[3], 'w');
  ASSERT_EQ(dst_dns[4], 6);
  ASSERT_EQ(dst_dns[5], 'g');
  ASSERT_EQ(dst_dns[6], 'o');
  ASSERT_EQ(dst_dns[7], 'o');
  ASSERT_EQ(dst_dns[8], 'g');
  ASSERT_EQ(dst_dns[9], 'l');
  ASSERT_EQ(dst_dns[10], 'e');
  ASSERT_EQ(dst_dns[11], 3);
  ASSERT_EQ(dst_dns[12], 'c');
  ASSERT_EQ(dst_dns[13], 'o');
  ASSERT_EQ(dst_dns[14], 'm');
  ASSERT_EQ(dst_dns[15], 0);
  n = dc.prepare(src_dns[1]);
  ASSERT_GT(n.size(), 0);
  dc.compress({dst_dns + total, n.size()});
  total += n.size();
  ASSERT_EQ(dst_dns[16], 4);
  ASSERT_EQ(dst_dns[17], 'm');
  ASSERT_EQ(dst_dns[18], 'a');
  ASSERT_EQ(dst_dns[19], 'i');
  ASSERT_EQ(dst_dns[20], 'l');
  ASSERT_EQ(dst_dns[21], 0xc0);
  ASSERT_EQ(dst_dns[22], 0x04);
  n = dc.prepare(src_dns[2]);
  ASSERT_GT(n.size(), 0);
  dc.compress({dst_dns + total, n.size()});
  total += n.size();
  ASSERT_EQ(dst_dns[23], 0xc0);
  ASSERT_EQ(dst_dns[24], 0x0b);
  ASSERT_EQ(dst_dns[25], 0);
  n = dc.prepare(src_dns[3]);
  ASSERT_GT(n.size(), 0);
  dc.compress({dst_dns + total, n.size()});
  total += n.size();
  ASSERT_EQ(total, 26);
};

int main(int argc, char **argv) {
  xsl::no_log();
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}