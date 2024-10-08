#pragma once
#ifndef XSL_SYNC
#  define XSL_SYNC
#  include "xsl/coro.h"
#  include "xsl/def.h"
#  include "xsl/sync/mutex.h"
#  include "xsl/sync/poller.h"
#  include "xsl/sync/spsc.h"

#  include <array>
XSL_NB
using sync::IOM_EVENTS;
using sync::LockGuard;
using sync::poll_add_shared;
using sync::poll_add_unique;
using sync::Poller;
using sync::PollHandleHint;
using sync::PollHandleHintTag;
using sync::PollHandler;
using sync::ShardGuard;
using sync::ShardRes;
using sync::SPSC;

namespace sync {
  struct PollTraits {
    static PollHandleHintTag poll_check(IOM_EVENTS events) {
      return ((!events) || !!(events & IOM_EVENTS::HUP)) ? PollHandleHintTag::DELETE
                                                         : PollHandleHintTag::NONE;
    }
  };

  template <class Traits, IOM_EVENTS... Events>
  class PollCallback {
  public:
    PollCallback(std::array<std::shared_ptr<coro::CountingSemaphore<1>>, sizeof...(Events)> sems)
        : sems(std::move(sems)) {}

    template <class... Sems>
    PollCallback(Sems &&...sems) : sems{std::forward<Sems>(sems)...} {}
    sync::PollHandleHint operator()(int, sync::IOM_EVENTS events) {
      if (Traits::poll_check(events) == sync::PollHandleHintTag::DELETE) {
        for (auto &sem : sems) {
          sem->release(false);
        }
        return sync::PollHandleHintTag::DELETE;
      } else {
        return handle_event(std::make_index_sequence<sizeof...(Events)>{}, events)
                   ? sync::PollHandleHintTag::DELETE
                   : sync::PollHandleHintTag::NONE;
      }
    }

  private:
    std::array<std::shared_ptr<coro::CountingSemaphore<1>>, sizeof...(Events)> sems;

    template <std::size_t... I>
    bool handle_event(std::index_sequence<I...>, sync::IOM_EVENTS events) {
      return (handle_event<Events, I>(events) && ...);
    }

    template <IOM_EVENTS E, std::size_t I>
    bool handle_event(sync::IOM_EVENTS events) {
      if (sems[I].use_count() > 1) {
        if (!!(events & E)) {
          sems[I]->release();
        }
        return false;
      }
      return true;
    }
  };
}  // namespace sync
XSL_NE
#endif
