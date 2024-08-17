#include "xsl/logctl.h"
#include "xsl/sys/def.h"
#include "xsl/sys/pipe.h"

#include <fcntl.h>
#include <unistd.h>

#include <utility>
XSL_SYS_NB
// std::pair<io::Device<feature::In<byte>>, io::Device<feature::Out<byte>>> pipe() {
//   int fds[2];
//   if (pipe2(fds, O_NONBLOCK) == -1) {
//     LOG2("Failed to create pipe, err: {}", strerror(errno));
//     return {io::Device<feature::In<byte>>(-1), io::Device<feature::Out<byte>>(-1)};
//   }
//   return {io::Device<feature::In<byte>>(fds[0]), io::Device<feature::Out<byte>>(fds[1])};
// }

// std::pair<io::AsyncDevice<feature::In<byte>>, io::AsyncDevice<feature::Out<byte>>> async_pipe(
//     std::shared_ptr<sync::Poller>& poller) {
//   int fds[2];
//   if (pipe2(fds, O_NONBLOCK | O_CLOEXEC) == -1) {
//     LOG2("Failed to create pipe, err: {}", strerror(errno));
//     return {io::AsyncDevice<feature::In<byte>>(nullptr, -1),
//             io::AsyncDevice<feature::Out<byte>>(nullptr, -1)};
//   }
//   auto read_sem = std::make_shared<coro::CountingSemaphore<1>>();
//   auto write_sem = std::make_shared<coro::CountingSemaphore<1>>();
//   poller->add(fds[0], sync::IOM_EVENTS::IN | sync::IOM_EVENTS::ET,
//               sync::PollCallback<sync::PollTraits, sync::IOM_EVENTS::IN>{read_sem});
//   poller->add(fds[1], sync::IOM_EVENTS::OUT | sync::IOM_EVENTS::ET,
//               sync::PollCallback<sync::PollTraits, sync::IOM_EVENTS::OUT>{write_sem});
//   return {io::AsyncDevice<feature::In<byte>>(read_sem, fds[0]),
//           io::AsyncDevice<feature::Out<byte>>(write_sem, fds[1])};
// }

XSL_SYS_NE
